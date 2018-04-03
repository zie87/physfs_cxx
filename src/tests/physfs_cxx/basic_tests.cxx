#include <physfs_cxx/physfs.hxx>

#include <catch.hpp>

TEST_CASE("testing wrapper for physfs", "[physfs]")
{
  SECTION("test initialization of physfs")
  {
    {
      physfs::init_guard guard{};
      REQUIRE(physfs::is_init());
    }
    REQUIRE_FALSE(physfs::is_init());
  }

  SECTION("test double initialization")
  {
    {
      physfs::init();
      try
      {
        physfs::init();
      }
      catch (physfs::exception& e)
      {
        REQUIRE_THAT(e.what(), Catch::Matchers::EndsWith("already initialized"));
      }
      catch (...)
      {
        REQUIRE(false);
      }

      physfs::deinit();
    }
  }

  SECTION("test path getter of archives")
  {
    physfs::init_guard guard{};

    const std::string test_app_name("test_app");
    auto pref_path = physfs::get_pref_dir("test_org", test_app_name);
    REQUIRE_THAT(pref_path, Catch::Matchers::Contains(test_app_name));

    auto base_path = physfs::get_base_dir();
    REQUIRE_FALSE(base_path.empty());
  }

  SECTION("test mounting of archives")
  {
    physfs::init_guard guard;
    const std::string mount_point("zip_archiv");
    const std::string target_archive(std::string(TEST_DATA) + "/test_archive.zip");

    physfs::mount(target_archive, mount_point);
    REQUIRE(physfs::exists(mount_point));
    REQUIRE_THAT(physfs::get_mount_point(target_archive), Catch::Matchers::StartsWith(mount_point));

    REQUIRE(physfs::exists(mount_point + "/themeinfo.txt"));

    {
      auto files = physfs::get_search_paths();
      REQUIRE(std::find(files.begin(), files.end(), target_archive) != files.end());
    }

    {
      auto files = physfs::enumerate_files(mount_point);
      REQUIRE(files.size() == 6);
    }

    auto real_dir = physfs::get_real_dir(mount_point + "/themeinfo.txt");
    REQUIRE_THAT(real_dir, Catch::Matchers::StartsWith(TEST_DATA));

    physfs::unmount(target_archive);
  }
}