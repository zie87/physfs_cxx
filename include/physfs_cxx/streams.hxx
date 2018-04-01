#ifndef PHYSFS_CXX_STREAMS_HXX
#define PHYSFS_CXX_STREAMS_HXX

#include <physfs.h>

#include <fstream>
#include <iostream>

#include "error.hxx"

namespace physfs
{
  namespace detail
  {
    inline void set_write_dir(const char* write_dir) { PHYSFS_CXX_CHECK(PHYSFS_setWriteDir(write_dir) != 0); }
  } /*detail*/

  inline void disable_writing() { detail::set_write_dir(nullptr); }
  inline void set_write_dir(const std::string& write_dir) { detail::set_write_dir(write_dir.c_str()); }

  inline std::string get_write_dir() noexcept
  {
    const char* write_dir = PHYSFS_getWriteDir();
    if (write_dir == nullptr)
    {
      return std::string{""};
    }
    return std::string{write_dir};
  }

  enum class access_mode : int
  {
    read,
    write,
    append
  };

  namespace detail
  {
    inline PHYSFS_File* open_file(const std::string& filename, access_mode mode)
    {

      PHYSFS_File* file = nullptr;
      if (mode == access_mode::read)
      {
        file = PHYSFS_openRead(filename.c_str());
      }
      else if (mode == access_mode::write)
      {
        file = PHYSFS_openWrite(filename.c_str());
      }
      else if (mode == access_mode::append)
      {
        file = PHYSFS_openAppend(filename.c_str());
      }
      PHYSFS_CXX_CHECK(file != nullptr);
      return file;
    }

    inline void close(PHYSFS_File* file) { PHYSFS_CXX_CHECK(PHYSFS_close(file) != 0); }

    inline bool eof(PHYSFS_File* file) noexcept { return (PHYSFS_eof(file) != 0); }

    inline std::int64_t read(PHYSFS_File* file, void* buffer, std::uint64_t length)
    {
      auto read_size = PHYSFS_readBytes(file, buffer, length);
      PHYSFS_CXX_CHECK(read_size != -1);
      return read_size;
    }

    inline std::int64_t write(PHYSFS_File* file, const void* buffer, std::uint64_t length)
    {
      auto read_size = PHYSFS_writeBytes(file, buffer, length);
      PHYSFS_CXX_CHECK(read_size != -1);
      return read_size;
    }

    inline std::int64_t tell(PHYSFS_File* file)
    {
      auto offset = PHYSFS_tell(file);
      PHYSFS_CXX_CHECK(offset != -1);
      return offset;
    }

    inline void seek(PHYSFS_File* file, std::uint64_t pos) { PHYSFS_CXX_CHECK(PHYSFS_seek(file, pos) != 0); }

    inline std::int64_t file_length(PHYSFS_File* file)
    {

      auto length = PHYSFS_fileLength(file);
      PHYSFS_CXX_CHECK(length != -1);
      return length;
    }

    class fbuf : public std::streambuf
    {
    private:
      int_type underflow()
      {
        if (eof(m_file))
        {
          return traits_type::eof();
        }
        auto bytes_read = read(m_file, m_buffer, m_buffer_size);
        if (bytes_read < 1)
        {
          return traits_type::eof();
        }
        setg(m_buffer, m_buffer, m_buffer + bytes_read);
        return (unsigned char)*gptr();
      }

      pos_type seekoff(off_type pos, std::ios_base::seekdir dir, std::ios_base::openmode mode)
      {
        if (dir == std::ios_base::beg)
        {
          seek(m_file, pos);
        }
        else if (dir == std::ios_base::cur)
        {
          seek(m_file, (tell(m_file) + pos) - (egptr() - gptr()));
        }
        else if (dir == std::ios_base::end)
        {
          seek(m_file, file_length(m_file) + pos);
        }

        if (mode & std::ios_base::in)
        {
          setg(egptr(), egptr(), egptr());
        }
        if (mode & std::ios_base::out)
        {
          setp(m_buffer, m_buffer);
        }
        return tell(m_file);
      }

      pos_type seekpos(pos_type pos, std::ios_base::openmode mode)
      {
        seek(m_file, pos);
        if (mode & std::ios_base::in)
        {
          setg(egptr(), egptr(), egptr());
        }
        if (mode & std::ios_base::out)
        {
          setp(m_buffer, m_buffer);
        }
        return tell(m_file);
      }

      int_type overflow(int_type c = traits_type::eof())
      {
        if (pptr() == pbase() && c == traits_type::eof())
        {
          return 0; // no-op
        }
        if (write(m_file, pbase(), pptr() - pbase()) < 1)
        {
          return traits_type::eof();
        }
        if (c != traits_type::eof())
        {
          if (write(m_file, &c, 1) < 1)
          {
            return traits_type::eof();
          }
        }

        return 0;
      }

      int sync() { return overflow(); }

      char* m_buffer;
      const size_t m_buffer_size;

    protected:
      PHYSFS_File* const m_file;

    public:
      fbuf(const fbuf& other) = delete;
      fbuf& operator=(const fbuf& other) = delete;

      fbuf(PHYSFS_File* file, std::size_t buffer_size = 2048) : m_buffer(nullptr), m_buffer_size(buffer_size), m_file(file)
      {

        m_buffer = new char[buffer_size];
        char* end = m_buffer + buffer_size;
        setg(end, end, end);
        setp(m_buffer, end);
      }

      ~fbuf()
      {
        sync();
        delete[] m_buffer;
      }
    };

    class base_fstream
    {
    protected:
      PHYSFS_File* m_file;

    public:
      explicit base_fstream(const std::string& filename, access_mode mode) : m_file(open_file(filename.c_str(), mode)) {}

      base_fstream(const base_fstream&) = delete;
      base_fstream& operator=(const base_fstream&) = delete;

      inline std::size_t length() const { return static_cast<std::size_t>(file_length(m_file)); }

      virtual ~base_fstream() noexcept
      {

        try
        {
          if (m_file != nullptr)
          {
            close(m_file);
          }
        }
        catch (::physfs::exception& e)
        {
          std::cerr << __FUNCTION__ << " couldn't close file: " << e.what() << std::endl;
        }
        catch (...)
        {
          std::cerr << __FUNCTION__ << " couldn't close file unexpected exception " << std::endl;
        }
      }
    };

  } /*detail*/

  class ifstream : public detail::base_fstream, public std::istream
  {
  public:
    explicit ifstream(const std::string& filename) : detail::base_fstream(filename, access_mode::read), std::istream(new detail::fbuf(m_file)) {}
    virtual ~ifstream() noexcept { delete rdbuf(); }
  };

  class ofstream : public detail::base_fstream, public std::ostream
  {
  public:
    explicit ofstream(const std::string& filename, access_mode mode = access_mode::write)
        : detail::base_fstream(filename, mode), std::ostream(new detail::fbuf(m_file))
    {
    }
    virtual ~ofstream() noexcept { delete rdbuf(); }
  };

  class fstream : public detail::base_fstream, public std::iostream
  {
  public:
    explicit fstream(const std::string& filename, access_mode mode = access_mode::read)
        : detail::base_fstream(filename, mode), std::iostream(new detail::fbuf(m_file))
    {
    }
    virtual ~fstream() noexcept { delete rdbuf(); }
  };

} /*physfs*/

#endif /*PHYSFS_CXX_STREAMS_HXX*/