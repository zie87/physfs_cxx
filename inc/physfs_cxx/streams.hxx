#ifndef PHYSFS_CXX__STREAMS_HXX
#define PHYSFS_CXX__STREAMS_HXX

#include <physfs.h>

#include "file_device.hxx"

#include <fstream>
#include <iostream>
#include <vector>

namespace physfs
{

  /// Class template for stream buffer.
  template <typename CharT, typename Traits = std::char_traits<CharT>>
  class basic_fstreambuf : public std::basic_streambuf<CharT, Traits>
  {
    static const unsigned int bufsz = 32; ///< Size of fstreambuf buffers.
    static const unsigned int pbsz = 2;   ///< Number of putback characters kept.

  public:
    // Type definitions for dependent types
    typedef CharT char_type;
    typedef Traits traits_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::off_type off_type;
    typedef typename traits_type::pos_type pos_type;

    basic_fstreambuf() : wbuffer_(nullptr), rsrc_(rsrc_out) { init_rbuffers(); }
    basic_fstreambuf(const std::string& filename, access_mode mode) : basic_fstreambuf() { open(filename, mode); }
    ~basic_fstreambuf() { close(); }

    basic_fstreambuf* open(const std::string& filename, access_mode mode)
    {
      m_file_device.open(filename, mode);
      create_buffers(mode);
      return this;
    }

    basic_fstreambuf* close()
    {
      sync();
      destroy_buffers();
      m_file_device.close();

      return this;
    }

    /// Report whether the stream buffer has been initialised.
    inline bool is_open() const noexcept { return m_file_device.is_open(); }

  protected:
    int_type overflow(int_type c) override
    {
      if (!empty_buffer())
        return traits_type::eof();
      else if (!traits_type::eq_int_type(c, traits_type::eof()))
        return this->sputc(c);
      else
        return traits_type::not_eof(c);
    }

    /// Transfer characters from the pipe when the character buffer is empty.
    int_type underflow() override
    {
      if (this->gptr() < this->egptr() || fill_buffer())
        return traits_type::to_int_type(*this->gptr());
      else
        return traits_type::eof();
    }

    /// Make a character available to be returned by the next extraction.
    int_type pbackfail(int_type c = traits_type::eof()) override
    {
      if (this->gptr() != this->eback())
      {
        this->gbump(-1);
        if (!traits_type::eq_int_type(c, traits_type::eof())) *this->gptr() = traits_type::to_char_type(c);
        return traits_type::not_eof(c);
      }
      else
        return traits_type::eof();
    }

    inline int sync() { return (is_open() && empty_buffer()) ? 0 : -1; }

    std::streamsize xsputn(const char_type* s, std::streamsize n) override
    {
      std::streamsize done = 0;
      while (done < n)
      {
        if (std::streamsize nbuf = this->epptr() - this->pptr())
        {
          nbuf = std::min(nbuf, n - done);
          traits_type::copy(this->pptr(), s + done, nbuf);
          this->pbump(nbuf);
          done += nbuf;
        }
        else if (!empty_buffer())
          break;
      }
      return done;
    }

    /// Insert a sequence of characters into the pipe.
    std::streamsize write(const char_type* s, std::streamsize n)
    {
      std::streamsize nwritten = m_file_device.write(s, n * sizeof(char_type));
      return nwritten /= sizeof(char_type);
    }

    /// Extract a sequence of characters from the pipe.
    std::streamsize read(char_type* s, std::streamsize n)
    {
      std::streamsize nread = m_file_device.read(s, n * sizeof(char_type));
      return nread /= sizeof(char_type);
    }

    /// Report how many characters can be read from active input without blocking.
    std::streamsize showmanyc() override
    {
      int avail = 0;
      if (sizeof(char_type) == 1) avail = fill_buffer() ? this->egptr() - this->gptr() : -1;
      return std::streamsize(avail);
    }

    pos_type seekoff(off_type pos, std::ios_base::seekdir dir, std::ios_base::openmode mode) override
    {
      if (dir == std::ios_base::beg)
      {
        m_file_device.seek(pos);
      }
      else if (dir == std::ios_base::cur)
      {
        auto current_pos = m_file_device.tell();
        m_file_device.seek((current_pos + pos) - (this->egptr() - this->gptr()));
      }
      else if (dir == std::ios_base::end)
      {
        auto length = m_file_device.file_length();
        m_file_device.seek(length + pos);
      }

      if (mode & std::ios_base::in)
      {
        this->setg(this->egptr(), this->egptr(), this->egptr());
      }
      if (mode & std::ios_base::out)
      {
        this->setp(wbuffer_, wbuffer_ + bufsz);
      }
      return m_file_device.tell();
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode mode) override
    {
      m_file_device.seek(pos);
      if (mode & std::ios_base::in)
      {
        this->setg(this->egptr(), this->egptr(), this->egptr());
      }
      if (mode & std::ios_base::out)
      {
        this->setp(wbuffer_, wbuffer_ + bufsz);
      }
      return m_file_device.tell();
    }

  protected:
    /// Enumerated type to indicate whether stdout or stderr is to be read.
    enum buf_read_src
    {
      rsrc_out = 0,
      rsrc_err = 1
    };

    void create_buffers(access_mode mode)
    {
      if (mode == access_mode::read)
      {
        delete[] rbuffer_[rsrc_out];
        rbuffer_[rsrc_out] = new char_type[bufsz];
        rsrc_ = rsrc_out;
        this->setg(rbuffer_[rsrc_out] + pbsz, rbuffer_[rsrc_out] + pbsz, rbuffer_[rsrc_out] + pbsz);
      }
      else
      {
        delete[] wbuffer_;
        wbuffer_ = new char_type[bufsz];
        this->setp(wbuffer_, wbuffer_ + bufsz);
      }
    }

    void destroy_buffers()
    {
      if (rbuffer_ != nullptr)
      {
        if (rsrc_ == rsrc_out) this->setg(nullptr, nullptr, nullptr);
        delete[] rbuffer_[rsrc_out];
        rbuffer_[rsrc_out] = nullptr;
      }

      if (wbuffer_ != nullptr)
      {
        this->setp(nullptr, nullptr);
        delete[] wbuffer_;
        wbuffer_ = nullptr;
      }
    }

    /// Writes buffered characters to the process' stdin pipe.
    bool empty_buffer()
    {
      const std::streamsize count = this->pptr() - this->pbase();
      if (count > 0)
      {
        const std::streamsize written = this->write(this->wbuffer_, count);
        if (written > 0)
        {
          if (const std::streamsize unwritten = count - written) traits_type::move(this->pbase(), this->pbase() + written, unwritten);
          this->pbump(-written);
          return true;
        }
      }
      return false;
    }

    bool fill_buffer()
    {
      const std::streamsize pb1 = this->gptr() - this->eback();
      const std::streamsize pb2 = pbsz;
      const std::streamsize npb = std::min(pb1, pb2);

      char_type* const rbuf = rbuffer();

      if (npb) traits_type::move(rbuf + pbsz - npb, this->gptr() - npb, npb);

      std::streamsize rc = -1;

      rc = read(rbuf + pbsz, bufsz - pbsz);

      if (rc > 0)
      {
        this->setg(rbuf + pbsz - npb, rbuf + pbsz, rbuf + pbsz + rc);
        return true;
      }
      else
      {
        this->setg(nullptr, nullptr, nullptr);
        return false;
      }
    }

    inline char_type* rbuffer() { return rbuffer_[rsrc_]; }

    buf_read_src switch_read_buffer(buf_read_src src)
    {
      if (rsrc_ != src)
      {
        char_type* tmpbufstate[] = {this->eback(), this->gptr(), this->egptr()};
        this->setg(rbufstate_[0], rbufstate_[1], rbufstate_[2]);
        for (std::size_t i = 0; i < 3; ++i)
          rbufstate_[i] = tmpbufstate[i];
        rsrc_ = src;
      }
      return rsrc_;
    }

  private:
    basic_fstreambuf(const basic_fstreambuf&);
    basic_fstreambuf& operator=(const basic_fstreambuf&);

    void init_rbuffers()
    {
      rbuffer_[rsrc_out] = rbuffer_[rsrc_err] = nullptr;
      rbufstate_[0] = rbufstate_[1] = rbufstate_[2] = nullptr;
    }

    file_device m_file_device;

    char_type* wbuffer_;
    char_type* rbuffer_[2];
    char_type* rbufstate_[3];

    buf_read_src rsrc_;
  };

  template <typename CharT, typename Traits = std::char_traits<CharT>>
  class fstream_common : virtual public std::basic_ios<CharT, Traits>
  {
  protected:
    typedef basic_fstreambuf<CharT, Traits> streambuf_type;

    fstream_common() : std::basic_ios<CharT, Traits>(nullptr), m_filename(), m_buffer() { this->std::basic_ios<CharT, Traits>::rdbuf(&m_buffer); }
    fstream_common(const std::string& filename, access_mode mode) : std::basic_ios<CharT, Traits>(nullptr), m_filename(filename), m_buffer()
    {
      this->std::basic_ios<CharT, Traits>::rdbuf(&m_buffer);
      do_open(filename, mode);
    }

    virtual ~fstream_common() = default;

    inline void do_open(const std::string& filename, access_mode mode)
    {
      m_buffer.open((m_filename = filename), mode);
      if (!m_buffer.is_open()) this->setstate(std::ios_base::failbit);
    }

  public:
    inline void close()
    {
      m_buffer.close();
      if (m_buffer.is_open()) this->setstate(std::ios_base::failbit);
    }

    inline bool is_open() const { return m_buffer.is_open(); }

    inline std::string filename() const noexcept { return m_filename; }
    inline streambuf_type* rdbuf() const { return const_cast<streambuf_type*>(&m_buffer); }

  protected:
    std::string m_filename;  ///< The command used to start the process.
    streambuf_type m_buffer; ///< The stream buffer.
  };

  /**
   * @class basic_ifstream
   * @brief Class template for Input PStreams.
   *
   * Reading from an ifstream reads the command's standard output and/or
   * standard error (depending on how the ifstream is opened)
   * and the command's standard input is the same as that of the process
   * that created the object, unless altered by the command itself.
   */

  template <typename CharT, typename Traits = std::char_traits<CharT>>
  class basic_ifstream : public std::basic_istream<CharT, Traits>, public fstream_common<CharT, Traits>
  {
    typedef std::basic_istream<CharT, Traits> istream_type;
    typedef fstream_common<CharT, Traits> stream_base_type;

    using stream_base_type::m_buffer;

  public:
    basic_ifstream() : istream_type(nullptr), stream_base_type() {}
    explicit basic_ifstream(const std::string& filename, access_mode mode = access_mode::read) : istream_type(nullptr), stream_base_type(filename, mode) {}
    ~basic_ifstream() {}

    inline void open(const std::string& filename, access_mode mode = access_mode::read) { this->do_open(filename, mode); }
  };

  template <typename CharT, typename Traits = std::char_traits<CharT>>
  class basic_ofstream : public std::basic_ostream<CharT, Traits>, public fstream_common<CharT, Traits>
  {
    typedef std::basic_ostream<CharT, Traits> ostream_type;
    typedef fstream_common<CharT, Traits> stream_base_type;

    using stream_base_type::m_buffer;

  public:
    basic_ofstream() : ostream_type(nullptr), stream_base_type() {}
    explicit basic_ofstream(const std::string& filename, access_mode mode = access_mode::write) : ostream_type(nullptr), stream_base_type(filename, mode) {}
    ~basic_ofstream() {}

    inline void open(const std::string& filename, access_mode mode = access_mode::write) { this->do_open(filename, mode); }
  };

  template <typename CharT, typename Traits = std::char_traits<CharT>>
  class basic_fstream : public std::basic_iostream<CharT, Traits>, public fstream_common<CharT, Traits>
  {
    typedef std::basic_iostream<CharT, Traits> iostream_type;
    typedef fstream_common<CharT, Traits> stream_base_type;

    using stream_base_type::m_buffer; // declare name in this scope

  public:
    basic_fstream() : iostream_type(nullptr), stream_base_type() {}
    explicit basic_fstream(const std::string& filename, access_mode mode = access_mode::read) : iostream_type(nullptr), stream_base_type(filename, mode) {}
    ~basic_fstream() {}

    inline void open(const std::string& filename, access_mode mode = access_mode::read) { this->do_open(filename, mode); }
  };

  typedef basic_fstreambuf<char> fstreambuf;
  typedef basic_ifstream<char> ifstream;
  typedef basic_ofstream<char> ofstream;
  typedef basic_fstream<char> fstream;

} // namespace physfs

#endif /*PHYSFS_CXX__STREAMS_HXX*/