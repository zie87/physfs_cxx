#ifndef PHYSFS_CXX_PHYSFS_HXX
#define PHYSFS_CXX_PHYSFS_HXX

#include <physfs.h>

#include <string>
#include <vector>

#include <iostream>

#include "error.hxx"

namespace physfs
{
  enum class filetype : std::underlying_type_t<PHYSFS_FileType>
  {
    regular = PHYSFS_FILETYPE_REGULAR,
    directory = PHYSFS_FILETYPE_DIRECTORY,
    symlink = PHYSFS_FILETYPE_SYMLINK,
    other = PHYSFS_FILETYPE_OTHER
  };

  class file_stat
  {
  public:
    explicit file_stat(const PHYSFS_Stat& values) noexcept
        : m_size(values.filesize), m_modtime(values.modtime), m_createtime(values.createtime), m_accesstime(values.accesstime),
          m_type(static_cast<filetype>(values.filetype)), m_readonly(values.readonly != 0)
    {
    }

    inline filetype type() const noexcept { return m_type; }
    inline std::int64_t size() const noexcept { return m_size; }
    inline std::int64_t modification_time() const noexcept { return m_modtime; }
    inline std::int64_t creation_time() const noexcept { return m_createtime; }
    inline std::int64_t access_time() const noexcept { return m_accesstime; }

    inline bool is_readonly() const noexcept { return m_readonly; }

  private:
    std::int64_t m_size;
    std::int64_t m_modtime;
    std::int64_t m_createtime;
    std::int64_t m_accesstime;
    filetype m_type;
    bool m_readonly;
  };

  using file_list = std::vector<std::string>;

  inline void init(const char* argv0 = nullptr) { PHYSFS_CXX_CHECK(PHYSFS_init(argv0) != 0); }
  inline void init(const std::string& argv0) { init(argv0.c_str()); }

  inline void deinit() { PHYSFS_CXX_CHECK(PHYSFS_deinit() != 0); }
  inline bool is_init() noexcept { return (PHYSFS_isInit() != 0); }

  struct init_guard
  {
    init_guard() : init_guard(nullptr) {}
    explicit init_guard(const char* argv0) { init(argv0); }
    explicit init_guard(const std::string& argv0) { init(argv0); }

    init_guard(init_guard&&) noexcept = default;
    init_guard& operator=(init_guard&&) noexcept = default;

    ~init_guard() noexcept
    {
      try
      {
        deinit();
      }
      catch (exception& e)
      {
        std::cerr << __FUNCTION__ << " Couldn't deinit physfs! : " << e.what() << std::endl;
      }
      catch (...)
      {
        std::cerr << __FUNCTION__ << " Couldn't deinit physfs! unexpected exception!" << std::endl;
      }
    }
  };

  inline void permit_symbolic_links(bool allow) noexcept { PHYSFS_permitSymbolicLinks((allow ? 1 : 0)); }
  inline bool symbolic_links_permitted() noexcept { return (PHYSFS_symbolicLinksPermitted() != 0); }

  inline std::string get_base_dir()
  {
    const char* dir_name = PHYSFS_getBaseDir();
    PHYSFS_CXX_CHECK(dir_name != nullptr);
    return std::string(dir_name);
  }

  inline std::string get_pref_dir(const std::string& org, const std::string& app)
  {
    const char* dir_name = PHYSFS_getPrefDir(org.c_str(), app.c_str());
    PHYSFS_CXX_CHECK(dir_name != nullptr);
    return std::string(dir_name);
  }

  inline std::string get_real_dir(const std::string& target)
  {
    const char* dir_name = PHYSFS_getRealDir(target.c_str());
    PHYSFS_CXX_CHECK(dir_name != nullptr);
    return std::string(dir_name);
  }

  inline std::string get_mount_point(const std::string& target)
  {
    const char* dir_name = PHYSFS_getMountPoint(target.c_str());
    PHYSFS_CXX_CHECK(dir_name != nullptr);
    return std::string(dir_name);
  }

  namespace detail
  {
    inline file_list convert_to_vector(char** list)
    {
      PHYSFS_CXX_CHECK(list != nullptr);
      file_list files{};
      for (char** iterator = list; *iterator != nullptr; ++iterator)
      {
        files.push_back(*iterator);
      }
      PHYSFS_freeList(list);
      return files;
    }
  } // namespace detail

  inline file_list get_search_paths() { return detail::convert_to_vector(PHYSFS_getSearchPath()); }
  inline file_list enumerate_files(const std::string& dir) { return detail::convert_to_vector(PHYSFS_enumerateFiles(dir.c_str())); }

  inline bool exists(const std::string& filename) noexcept { return (PHYSFS_exists(filename.c_str()) != 0); }
  inline void remove(const std::string& filename) { PHYSFS_CXX_CHECK(PHYSFS_delete(filename.c_str()) != 0); }

  inline file_stat get_file_stat(const std::string& filename)
  {
    PHYSFS_Stat stat;
    PHYSFS_CXX_CHECK(PHYSFS_stat(filename.c_str(), &stat) != 0);
    return file_stat(stat);
  }

  inline std::int64_t get_file_size(const std::string& filename) { return get_file_stat(filename).size(); }

  inline bool is_readonly(const std::string& filename) { return get_file_stat(filename).is_readonly(); }
  inline bool is_regular_file(const std::string& filename) { return get_file_stat(filename).type() == filetype::regular; }
  inline bool is_symlink(const std::string& filename) { return get_file_stat(filename).type() == filetype::symlink; }
  inline bool is_directory(const std::string& filename) { return get_file_stat(filename).type() == filetype::directory; }

  inline void make_directory(const std::string& path) { PHYSFS_CXX_CHECK(PHYSFS_mkdir(path.c_str()) != 0); }

  inline void mount(const std::string& target, bool append = true) { PHYSFS_CXX_CHECK(PHYSFS_mount(target.c_str(), nullptr, (append ? 1 : 0)) != 0); }
  inline void mount(const std::string& target, const std::string& mount_point, bool append = true)
  {
    PHYSFS_CXX_CHECK(PHYSFS_mount(target.c_str(), mount_point.c_str(), (append ? 1 : 0)) != 0);
  }

  inline void unmount(const std::string& target) { PHYSFS_CXX_CHECK(PHYSFS_unmount(target.c_str()) != 0); }

  namespace detail
  {
    inline void set_write_dir(const char* write_dir) { PHYSFS_CXX_CHECK(PHYSFS_setWriteDir(write_dir) != 0); }
  } // namespace detail

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

} // namespace physfs

#endif /*PHYSFS_CXX_PHYSFS_HXX*/