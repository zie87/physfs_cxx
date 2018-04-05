#include <physfs_cxx/physfs.hxx>
#include <physfs_cxx/streams.hxx>

#include <catch.hpp>

std::size_t length(physfs::ifstream& file)
{
  auto pos = file.tellg();
  file.seekg(0, std::ios::end);
  auto size = file.tellg();
  file.seekg(pos);

  // auto size = file.length();
  return size;
}

TEST_CASE("testing stream access for physfs", "[physfs]")
{
  physfs::init_guard guard{};

  const std::string archiv_mount_point("zip_archiv");
  const std::string target_archive(std::string(TEST_DATA) + "/test_archive.zip");

  physfs::mount(target_archive, archiv_mount_point);

  const std::string data_mount_point("test_data");
  physfs::mount(TEST_DATA, "test_data");

  SECTION("test write dir setting")
  {
    physfs::disable_writing();
    REQUIRE(physfs::get_write_dir().empty());

    physfs::set_write_dir(TEST_DATA);
    REQUIRE_THAT(physfs::get_write_dir(), Catch::Matchers::Equals(TEST_DATA));
  }

  SECTION("test file reading")
  {
    const std::string theme_info_file(archiv_mount_point + "/themeinfo.txt");
    REQUIRE(physfs::exists(theme_info_file));

    physfs::ifstream infile(theme_info_file);
    REQUIRE(length(infile) == 19);

    std::string line;
    std::vector<std::string> line_content;
    while (std::getline(infile, line))
    {
      line_content.push_back(line);
    }

    REQUIRE(line_content.size() == 2);
    REQUIRE_THAT(line_content[0], Catch::Matchers::StartsWith("Ilya Baranovsky"));
  }

  SECTION("test writing of files")
  {
    physfs::set_write_dir(TEST_DATA);
    const std::string test_file("test_file.txt");

    if (physfs::exists(test_file))
    {
      physfs::remove(test_file);
    }

    std::string input_string("this is the test content");
    {
      physfs::ofstream outfile(test_file);
      outfile << input_string;
    }

    {
      physfs::ifstream infile(test_file);

      std::string line;
      while (std::getline(infile, line))
      {
        REQUIRE_THAT(line, Catch::Matchers::Equals(input_string));
      }
    }
    physfs::remove(test_file);
  }

  SECTION("test witchcraft write bug")
  {
    physfs::set_write_dir(TEST_DATA);
    const std::string test_file("test_file_withcraft_bug.dat");
    const int repeations = 50;

    if (physfs::exists(test_file))
    {
      physfs::remove(test_file);
    }

    {
      physfs::ofstream outfile(test_file);

      for (int i = 0; i < repeations; ++i)
      {
        outfile << i << "\n";
      }
    }

    {
      physfs::ifstream infile(test_file);

      std::string line;
      int count = 0;
      while (std::getline(infile, line))
      {
        std::string pattern = std::to_string(++count);
        REQUIRE_THAT(line, Catch::Matchers::Equals(pattern));
      }

      REQUIRE(count == (repeations - 1));
    }
    physfs::remove(test_file);
  }
}