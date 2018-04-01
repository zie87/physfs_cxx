#ifndef PHYSFS_CXX_ERROR_HXX
#define PHYSFS_CXX_ERROR_HXX

#include <physfs.h>

#include <stdexcept>

namespace physfs
{
  class exception : public std::runtime_error
  {
  public:
    static inline std::string last_error()
    {
      auto error_code = PHYSFS_getLastErrorCode();
      std::string result("PHYSFS ERROR: ");
      result += PHYSFS_getErrorByCode(error_code);
      return result;
    }

    exception() : std::runtime_error(last_error()) {}
    exception(const std::string& str) : std::runtime_error(str) {}
    virtual ~exception() = default;
  };

#ifndef PHYSFS_CXX_CHECK
#define PHYSFS_CXX_CHECK(condition)                                                                                                                            \
  do                                                                                                                                                           \
  {                                                                                                                                                            \
    if (!(condition)) throw exception();                                                                                                                       \
  } while (false)
#endif /*PHYSFS_CXX_CHECK*/

} /*physfs*/

#endif /*PHYSFS_CXX_ERROR_HXX*/