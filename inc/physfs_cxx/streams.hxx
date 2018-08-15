#ifndef PHYSFS_CXX_STREAMS_HXX
#define PHYSFS_CXX_STREAMS_HXX

#include <physfs.h>

#include "file_device.hxx"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

namespace physfs
{

  template <typename CharT, typename Traits = std::char_traits<CharT>>
  class basic_fstreambuf : public std::basic_streambuf<CharT, Traits>
  {
    static const unsigned int buffer_size = 32;
    static const unsigned int put_back_amount = 2;

  public:
    typedef CharT char_type;
    typedef Traits traits_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::off_type off_type;
    typedef typename traits_type::pos_type pos_type;

    basic_fstreambuf() noexcept : m_write_buffer(nullptr), m_read_buffer(nullptr) {}
    basic_fstreambuf(const std::string& filename, access_mode mode) : basic_fstreambuf() { open(filename, mode); }
    ~basic_fstreambuf() override { close(); }

    inline basic_fstreambuf* open(const std::string& filename, access_mode mode)
    {
      m_file_device.open(filename, mode);
      create_buffers(mode);
      return this;
    }

    inline basic_fstreambuf* close()
    {
      if (is_open())
      {
        sync();
        destroy_buffers();
        m_file_device.close();
      }
      return this;
    }

    inline bool is_open() const noexcept { return m_file_device.is_open(); }

  protected:
    inline int_type overflow(int_type c) override
    {
      if (!empty_buffer())
        return traits_type::eof();
      else if (!traits_type::eq_int_type(c, traits_type::eof()))
        return this->sputc(c);
      else
        return traits_type::not_eof(c);
    }

    inline int_type underflow() override
    {
      if (this->gptr() < this->egptr() || fill_buffer())
        return traits_type::to_int_type(*this->gptr());
      else
        return traits_type::eof();
    }

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

    inline int sync() override { return (is_open() && empty_buffer()) ? 0 : -1; }

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

    inline std::streamsize write(const char_type* s, std::streamsize n)
    {
      std::streamsize nwritten = m_file_device.write(s, n * sizeof(char_type));
      return nwritten /= sizeof(char_type);
    }

    inline std::streamsize read(char_type* s, std::streamsize n)
    {
      std::streamsize nread = m_file_device.read(s, n * sizeof(char_type));
      return nread /= sizeof(char_type);
    }

    inline std::streamsize showmanyc() override
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
        this->setp(m_write_buffer, m_write_buffer + buffer_size);
      }
      return m_file_device.tell();
    }

    inline pos_type seekpos(pos_type pos, std::ios_base::openmode mode) override
    {
      m_file_device.seek(pos);
      if (mode & std::ios_base::in)
      {
        this->setg(this->egptr(), this->egptr(), this->egptr());
      }
      if (mode & std::ios_base::out)
      {
        this->setp(m_write_buffer, m_write_buffer + buffer_size);
      }
      return m_file_device.tell();
    }

  protected:
    inline void create_buffers(access_mode mode)
    {
      if (mode == access_mode::read)
      {
        delete[] m_read_buffer;
        m_read_buffer = new char_type[buffer_size];
        this->setg(m_read_buffer + put_back_amount, m_read_buffer + put_back_amount, m_read_buffer + put_back_amount);
      }
      else
      {
        delete[] m_write_buffer;
        m_write_buffer = new char_type[buffer_size];
        this->setp(m_write_buffer, m_write_buffer + buffer_size);
      }
    }

    inline void destroy_buffers()
    {
      if (m_read_buffer != nullptr)
      {
        this->setg(nullptr, nullptr, nullptr);
        delete[] m_read_buffer;
        m_read_buffer = nullptr;
      }

      if (m_write_buffer != nullptr)
      {
        this->setp(nullptr, nullptr);
        delete[] m_write_buffer;
        m_write_buffer = nullptr;
      }
    }

    /// Writes buffered characters to the process' stdin pipe.
    inline bool empty_buffer()
    {
      const std::streamsize count = this->pptr() - this->pbase();
      if (count > 0)
      {
        const std::streamsize written = this->write(this->m_write_buffer, count);
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
      const std::streamsize pb2 = put_back_amount;
      const std::streamsize npb = std::min(pb1, pb2);

      char_type* const rbuf = m_read_buffer;

      if (npb) traits_type::move(rbuf + put_back_amount - npb, this->gptr() - npb, npb);

      std::streamsize rc = -1;

      rc = read(rbuf + put_back_amount, buffer_size - put_back_amount);

      if (rc > 0)
      {
        this->setg(rbuf + put_back_amount - npb, rbuf + put_back_amount, rbuf + put_back_amount + rc);
        return true;
      }
      else
      {
        this->setg(nullptr, nullptr, nullptr);
        return false;
      }
    }

  private:
    basic_fstreambuf(const basic_fstreambuf&) = delete;
    basic_fstreambuf& operator=(const basic_fstreambuf&) = delete;

    file_device m_file_device;

    char_type* m_write_buffer;
    char_type* m_read_buffer;
  };

  template <typename CharT, typename Traits = std::char_traits<CharT>>
  class fstream_common : virtual public std::basic_ios<CharT, Traits>
  {
  protected:
    typedef basic_fstreambuf<CharT, Traits> streambuf_type;

    fstream_common() noexcept : std::basic_ios<CharT, Traits>(nullptr), m_filename(), m_buffer() { this->std::basic_ios<CharT, Traits>::rdbuf(&m_buffer); }
    fstream_common(const std::string& filename, access_mode mode) : std::basic_ios<CharT, Traits>(nullptr), m_filename(filename), m_buffer()
    {
      this->std::basic_ios<CharT, Traits>::rdbuf(&m_buffer);
      do_open(filename, mode);
    }

    ~fstream_common() override = default;

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
    std::string m_filename;
    streambuf_type m_buffer;
  };

  template <typename CharT, typename Traits = std::char_traits<CharT>>
  class basic_ifstream : public std::basic_istream<CharT, Traits>, public fstream_common<CharT, Traits>
  {
    typedef std::basic_istream<CharT, Traits> istream_type;
    typedef fstream_common<CharT, Traits> stream_base_type;

    using stream_base_type::m_buffer;

  public:
    basic_ifstream() noexcept : istream_type(nullptr), stream_base_type() {}
    explicit basic_ifstream(const std::string& filename, access_mode mode = access_mode::read) : istream_type(nullptr), stream_base_type(filename, mode) {}
    ~basic_ifstream() override = default;

    inline void open(const std::string& filename, access_mode mode = access_mode::read) { this->do_open(filename, mode); }
  };

  template <typename CharT, typename Traits = std::char_traits<CharT>>
  class basic_ofstream : public std::basic_ostream<CharT, Traits>, public fstream_common<CharT, Traits>
  {
    typedef std::basic_ostream<CharT, Traits> ostream_type;
    typedef fstream_common<CharT, Traits> stream_base_type;

    using stream_base_type::m_buffer;

  public:
    basic_ofstream() noexcept : ostream_type(nullptr), stream_base_type() {}
    explicit basic_ofstream(const std::string& filename, access_mode mode = access_mode::write) : ostream_type(nullptr), stream_base_type(filename, mode) {}
    ~basic_ofstream() override = default;

    inline void open(const std::string& filename, access_mode mode = access_mode::write) { this->do_open(filename, mode); }
  };

  template <typename CharT, typename Traits = std::char_traits<CharT>>
  class basic_fstream : public std::basic_iostream<CharT, Traits>, public fstream_common<CharT, Traits>
  {
    typedef std::basic_iostream<CharT, Traits> iostream_type;
    typedef fstream_common<CharT, Traits> stream_base_type;

    using stream_base_type::m_buffer;

  public:
    basic_fstream() : iostream_type(nullptr), stream_base_type() {}
    explicit basic_fstream(const std::string& filename, access_mode mode = access_mode::read) : iostream_type(nullptr), stream_base_type(filename, mode) {}
    ~basic_fstream() = default;

    inline void open(const std::string& filename, access_mode mode = access_mode::read) { this->do_open(filename, mode); }
  };

  typedef basic_fstreambuf<char> fstreambuf;
  typedef basic_ifstream<char> ifstream;
  typedef basic_ofstream<char> ofstream;
  typedef basic_fstream<char> fstream;

} // namespace physfs

#endif /*PHYSFS_CXX_STREAMS_HXX*/
