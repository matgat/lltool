#pragma once
//  ---------------------------------------------
//  filesystem utilities
//  #include "filesystem_utilities.hpp" // fs::*, fsu::*
//  ---------------------------------------------
#include <vector>
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
// Non-throwing version of fs::exists
[[nodiscard]] bool exists(const std::filesystem::path& pth) noexcept
{
    std::error_code ec;
    return std::filesystem::exists(pth, ec) and !ec;
}

//---------------------------------------------------------------------------
// Non-throwing version of fs::equivalent
[[nodiscard]] bool equivalent(const std::filesystem::path& pth1, const std::filesystem::path& pth2) noexcept
{
    std::error_code ec;
    return std::filesystem::equivalent(pth1, pth2, ec) and !ec;
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
[[nodiscard]] bool ends_with_one_of(const std::string_view file_path, const std::initializer_list<std::string_view> suffixes) noexcept
{
    for( const auto suffix : suffixes )
       {
        if( file_path.ends_with(suffix) )
           {
            return true;
           }
       }
    return false;
}


//---------------------------------------------------------------------------
[[nodiscard]] std::vector<std::string> list_filenames_in_dir(const fs::path& dir)
{
    std::vector<std::string> files_in_directory;

    for( const fs::directory_entry& ientry : fs::directory_iterator(dir, fs::directory_options::follow_directory_symlink) )
       {
        if( ientry.is_regular_file() )
           {
            files_in_directory.push_back( ientry.path().filename().string() );
           }
       }

    return files_in_directory;
}


//---------------------------------------------------------------------------
// ex. const auto removed_count = remove_files_with_suffix_in("C:/dir", {".tmp", ".bck"});
[[maybe_unused]] std::size_t remove_files_with_suffix_in(const std::filesystem::path& dir, const std::initializer_list<std::string_view> suffixes)
{
    std::size_t removed_items_count { 0 };

    for( const fs::directory_entry& ientry : fs::directory_iterator(dir) )
       {
        if( ientry.is_regular_file() and ends_with_one_of(ientry.path().filename().string(), suffixes) )
           {
            removed_items_count += fs::remove(ientry.path());
           }
       }

    return removed_items_count;
}


//---------------------------------------------------------------------------
//[[nodiscard]] bool are_paths_equivalent(const fs::path& pth1, const fs::path& pth2)
//{
//    // Unfortunately fs::equivalent() needs existing files
//    if( fs::exists(pth1) and fs::exists(pth2) )
//       {
//        return fs::equivalent(pth1,pth2);
//       }
//    // The following is not perfect:
//    //  'fs::absolute' implementation may need the file existence
//    //  'fs::weakly_canonical' may need to be called multiple times
//  #if defined(MS_WINDOWS)
//    // Windows filesystem is case insensitive
//    // For case insensitive comparison could use std::memicmp (<cstring>)
//    return str::tolower(fs::weakly_canonical(fs::absolute(pth1)).string()) ==
//           str::tolower(fs::weakly_canonical(fs::absolute(pth2)).string());
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

    ut::test("fsu::list_filenames_in_dir()") = []
       {
        test::TemporaryDirectory dir;
        std::vector<std::string> lst = {
                                        dir.add_file("abc", "_").path().filename().string(),
                                        dir.add_file("def", "_").path().filename().string(),
                                        dir.add_file("ghi.txt", "_").path().filename().string(),
                                        dir.add_file("lmn.txt", "_").path().filename().string(),
                                       };
        ut::expect( test::have_same_elements(fsu::list_filenames_in_dir(dir.path()), std::move(lst)) );
       };

    ut::test("fsu::remove_files_with_suffix_in()") = []
       {
        test::TemporaryDirectory dir;
        std::vector<std::string> rem = {
                                        dir.add_file("a", "_").path().filename().string(),
                                        dir.add_file("b", "_").path().filename().string(),
                                        dir.add_file("a.txt", "_").path().filename().string(),
                                        dir.add_file("b.txt", "_").path().filename().string(),
                                       };
        std::vector<std::string> del = {
                                        dir.add_file("a.bck", "_").path().filename().string(),
                                        dir.add_file("a.tmp", "_").path().filename().string(),
                                        dir.add_file("b.bck", "_").path().filename().string(),
                                        dir.add_file("b.tmp", "_").path().filename().string()
                                       };
        std::vector<std::string> tot = test::join(rem, del);

        ut::expect( test::have_same_elements(fsu::list_filenames_in_dir(dir.path()), std::move(tot)) );
        ut::expect( ut::that % fsu::remove_files_with_suffix_in(dir.path(), {".tmp", ".bck"}) == 4 );
        ut::expect( test::have_same_elements(fsu::list_filenames_in_dir(dir.path()), std::move(rem)) );
       };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
