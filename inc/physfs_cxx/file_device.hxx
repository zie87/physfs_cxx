#ifndef PHYSFS_CXX_FILE_DEVICE_HXX
#define PHYSFS_CXX_FILE_DEVICE_HXX

#include <iostream>
#include <physfs.h>

#include "error.hxx"

namespace physfs
{
  enum class access_mode : int
  {
    read,
    write,
    append
  };

  struct file_device
  {
  public:
    explicit file_device() noexcept : m_file(nullptr), m_filename(){};
    file_device(const std::string& file_path, access_mode mode) : m_file(nullptr), m_filename() { open(file_path, mode); }

    file_device(const file_device&) = delete;
    file_device& operator=(const file_device&) = delete;

    file_device(file_device&&) = delete;
    file_device& operator=(file_device&&) = delete;

    ~file_device() noexcept
    {
      if (is_open())
      {
        try
        {
          close();
        }
        catch (exception& e)
        {
          std::cerr << __FUNCTION__ << " Couldn't close file \"" << m_filename << "\"! : " << e.what() << std::endl;
        }
        catch (...)
        {
          std::cerr << __FUNCTION__ << " Couldn't close file \"" << m_filename << "\"! unexpected exception!" << std::endl;
        }
      }
    }

    inline void open(const std::string& filename, access_mode mode)
    {
      if (is_open())
      {
        close();
      }

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

      m_file = file;
      m_filename = filename;
    }

    inline void close()
    {
      PHYSFS_CXX_CHECK(PHYSFS_close(m_file) != 0);
      m_file = nullptr;
    }

    inline bool is_open() const noexcept { return m_file != nullptr; }
    inline bool eof() const noexcept { return (PHYSFS_eof(m_file) != 0); }

    inline std::int64_t read(void* buffer, std::uint64_t length)
    {
      auto read_size = PHYSFS_readBytes(m_file, buffer, length);
      PHYSFS_CXX_CHECK(read_size != -1);
      return read_size;
    }

    inline std::int64_t write(const void* buffer, std::uint64_t length)
    {
      auto read_size = PHYSFS_writeBytes(m_file, buffer, length);
      PHYSFS_CXX_CHECK(read_size != -1);
      return read_size;
    }

    inline std::int64_t tell()
    {
      auto offset = PHYSFS_tell(m_file);
      PHYSFS_CXX_CHECK(offset != -1);
      return offset;
    }

    inline bool flush()
    {
      PHYSFS_CXX_CHECK(PHYSFS_flush(m_file) != 0);
      return true;
    }

    inline void seek(std::uint64_t pos) { PHYSFS_CXX_CHECK(PHYSFS_seek(m_file, pos) != 0); }

    inline std::int64_t file_length()
    {

      auto length = PHYSFS_fileLength(m_file);
      PHYSFS_CXX_CHECK(length != -1);
      return length;
    }

  private:
    PHYSFS_File* m_file;
    std::string m_filename;
  };

} // namespace physfs

#endif /*PHYSFS_CXX_FILE_DEVICE_HXX*/