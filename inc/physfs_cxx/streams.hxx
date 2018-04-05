#ifndef PHYSFS_CXX_STREAMS_HXX
#define PHYSFS_CXX_STREAMS_HXX

#include <physfs.h>

#include "file_device.hxx"

#include <fstream>
#include <iostream>
#include <vector>

namespace physfs
{

  class filebuf : public std::streambuf
  {
  public:
    using char_type = typename std::streambuf::char_type;
    using int_type = typename std::streambuf::int_type;

    filebuf(const filebuf& other) = delete;
    filebuf& operator=(const filebuf& other) = delete;

    filebuf(file_device& file, std::size_t buffer_size = 2048) : m_file_device(file), m_buffer(nullptr), m_buffer_size(buffer_size)
    {

      m_buffer = new char[buffer_size];
      char* end = m_buffer + buffer_size;
      setg(end, end, end);
      setp(m_buffer, end);
    }

    ~filebuf()
    {
      sync();
      delete[] m_buffer;
    }

    int_type underflow() override
    {
      if (m_file_device.eof())
      {
        return traits_type::eof();
      }
      auto bytes_read = m_file_device.read(m_buffer, m_buffer_size);
      if (bytes_read < 1)
      {
        return traits_type::eof();
      }
      setg(m_buffer, m_buffer, m_buffer + bytes_read);
      return traits_type::to_int_type(*gptr());
    }

    int_type overflow(int_type c = traits_type::eof()) override
    {
      if (!traits_type::eq_int_type(c, traits_type::eof()))
      {
        return this->sputc(c);
      }

      return traits_type::not_eof(c);
    }

    int_type sputc(char_type ch)
    {
      if (ch != traits_type::eof())
      {
        if (m_file_device.write(&(ch), 1) < 1)
        {
          return traits_type::eof();
        }
      }

      return traits_type::to_int_type(ch);
    }

    int_type xsputn(const char* s, int_type n)
    {
      std::cout << __FUNCTION__ << "content: " << std::string(s, n) << std::endl;
      return m_file_device.write(s, n);
    }
    std::size_t xsgetn(char* s, std::size_t n) { return m_file_device.read(s, n); }

    pos_type seekoff(off_type pos, std::ios_base::seekdir dir, std::ios_base::openmode mode) override
    {
      if (dir == std::ios_base::beg)
      {
        m_file_device.seek(pos);
      }
      else if (dir == std::ios_base::cur)
      {
        auto current_pos = m_file_device.tell();
        m_file_device.seek((current_pos + pos) - (egptr() - gptr()));
      }
      else if (dir == std::ios_base::end)
      {
        auto length = m_file_device.file_length();
        m_file_device.seek(length + pos);
      }

      if (mode & std::ios_base::in)
      {
        setg(egptr(), egptr(), egptr());
      }
      if (mode & std::ios_base::out)
      {
        setp(m_buffer, m_buffer + m_buffer_size - 1);
      }
      return m_file_device.tell();
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode mode) override
    {
      m_file_device.seek(pos);
      if (mode & std::ios_base::in)
      {
        setg(egptr(), egptr(), egptr());
      }
      if (mode & std::ios_base::out)
      {
        setp(m_buffer, m_buffer + m_buffer_size - 1);
      }
      return m_file_device.tell();
    }

    int sync() override { return (overflow() == 0) ? 0 : -1; }

  private:
    file_device& m_file_device;

    char* m_buffer;
    const size_t m_buffer_size;
  };

  class base_fstream
  {
  protected:
    file_device m_file;

  public:
    explicit base_fstream() noexcept : m_file() {}
    explicit base_fstream(const std::string& filename, access_mode mode) : m_file(filename, mode) {}

    base_fstream(const base_fstream&) = delete;
    base_fstream& operator=(const base_fstream&) = delete;

    inline bool is_open() const noexcept { return m_file.is_open(); }
    inline virtual void open(const std::string& filename, access_mode mode) { m_file.open(filename, mode); }
    inline void close()
    {
      if (is_open())
      {
        close();
      }
    }

    virtual ~base_fstream() noexcept {}
  };

  class fstream : public base_fstream, public std::iostream
  {
  public:
    explicit fstream() noexcept : base_fstream(), std::iostream(new filebuf(m_file)) {}
    explicit fstream(const std::string& filename, access_mode mode = access_mode::read) : base_fstream(filename, mode), std::iostream(new filebuf(m_file)) {}
    virtual ~fstream() noexcept { delete rdbuf(); }
  };

  class ifstream : public base_fstream, public std::istream
  {
  public:
    explicit ifstream() noexcept : base_fstream(), std::istream(new filebuf(m_file)) {}
    explicit ifstream(const std::string& filename) : base_fstream(filename, access_mode::read), std::istream(new filebuf(m_file)) {}
    virtual ~ifstream() noexcept { delete rdbuf(); }

    inline void open(const std::string& filename, access_mode mode = access_mode::read) override { base_fstream::open(filename, mode); }
  };

  class ofstream : public base_fstream, public std::ostream
  {
  public:
    explicit ofstream() noexcept : base_fstream(), std::ostream(new filebuf(m_file)) {}
    explicit ofstream(const std::string& filename, access_mode mode = access_mode::write) : base_fstream(filename, mode), std::ostream(new filebuf(m_file)) {}
    virtual ~ofstream() noexcept { delete rdbuf(); }

    inline void open(const std::string& filename, access_mode mode = access_mode::write) override { base_fstream::open(filename, mode); }
  };

} // namespace physfs

#endif /*PHYSFS_CXX_STREAMS_HXX*/