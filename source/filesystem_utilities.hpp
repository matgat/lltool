#pragma once
//  ---------------------------------------------
//  filesystem utilities
//  #include "filesystem_utilities.hpp" // fs::*, fsu::*
//  ---------------------------------------------
#include <filesystem> // std::filesystem
namespace fs = std::filesystem;

#include <fmt/format.h> // fmt::format


//---------------------------------------------------------------------------
constexpr auto operator""_MB(const unsigned long long int MB_size) noexcept
{
    return MB_size * 0x100000ull; // [bytes]
}


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace fsu //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
[[nodiscard]] bool exists(const std::filesystem::path& pth) noexcept
{
    std::error_code ec;
    return std::filesystem::exists(pth, ec) && !ec;
}

//---------------------------------------------------------------------------
[[nodiscard]] bool equivalent(const std::filesystem::path& pth1, const std::filesystem::path& pth2) noexcept
{
    std::error_code ec;
    return std::filesystem::equivalent(pth1, pth2, ec) && !ec;
}


//---------------------------------------------------------------------------
[[nodiscard]] fs::path get_a_temporary_path_for(const fs::path& file)
{
    std::error_code ec;
    fs::path temp_path{ fs::temp_directory_path(ec) };
    if(ec) temp_path = file.parent_path();
    temp_path /= fmt::format("~{}.tmp", file.filename().string());
    return temp_path;
}

//---------------------------------------------------------------------------
[[maybe_unused]] fs::path backup_file(const fs::path& file_to_backup)
{
    fs::path backup_path{ file_to_backup };
    backup_path += ".bck";
    if( fs::exists(backup_path) ) [[unlikely]]
       {
        int n = 0;
        do {
            backup_path.replace_extension( fmt::format(".{}.bck", ++n) );
           }
        while( fs::exists(backup_path) );
       }

    fs::copy_file(file_to_backup, backup_path);
    return backup_path;
}


//---------------------------------------------------------------------------
//[[nodiscard]] bool are_paths_equivalent(const fs::path& pth1, const fs::path& pth2)
//{
//    // Unfortunately fs::equivalent() needs existing files
//    if( fs::exists(pth1) && fs::exists(pth2) )
//       {
//        return fs::equivalent(pth1,pth2);
//       }
//    // The following is not perfect:
//    //   .'fs::absolute' implementation may need the file existence
//    //   .'fs::weakly_canonical' may need to be called multiple times
//  #if defined(MS_WINDOWS)
//    // Windows filesystem is case insensitive
//    // For case insensitive comparison could use std::memicmp (<cstring>)
//    const auto tolower = [](std::string&& s) noexcept -> std::string
//       {
//        for(char& c : s) c = static_cast<char>(std::tolower(c));
//        return s;
//       };
//    return tolower(fs::weakly_canonical(fs::absolute(pth1)).string()) ==
//           tolower(fs::weakly_canonical(fs::absolute(pth2)).string());
//  #else
//    return fs::weakly_canonical(fs::absolute(pth1)) ==
//           fs::weakly_canonical(fs::absolute(pth2));
//  #endif
//}


//---------------------------------------------------------------------------
//[[nodiscard]] std::time_t get_last_write_time_epoch(const fs::path& pth)
//{
//    const fs::file_time_type wt = fs::last_write_time(pth);
//    return std::chrono::system_clock::to_time_t(wt);
//}


/////////////////////////////////////////////////////////////////////////////
class CurrentPathLocalChanger final
{
 private:
    fs::path original_path;

 public:
    explicit CurrentPathLocalChanger(const fs::path& new_path)
      : original_path(fs::current_path())
       {
        fs::current_path(new_path);
        //fmt::print("Path changed to: {}\n", fs::current_path().string());
       }

    ~CurrentPathLocalChanger()
       {
        fs::current_path( std::move(original_path) );
        //fmt::print("Path restored to: {}\n", fs::current_path().string());
       }

    CurrentPathLocalChanger(const CurrentPathLocalChanger&) = delete; // Prevent copy
    CurrentPathLocalChanger& operator=(const CurrentPathLocalChanger&) = delete;
};




}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"filesystem_utilities"> filesystem_utilities_tests = []
{////////////////////////////////////////////////////////////////////////////
    using ut::expect;
    using ut::that;

    //ut::test("fsu::backup_file") = []
    //   {
    //    expect( that % true );
    //   };
};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
