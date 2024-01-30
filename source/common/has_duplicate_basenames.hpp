#pragma once
//  ---------------------------------------------
//  Tells if in a sequence of paths there are
//  more than one same basenames
//  ---------------------------------------------
//  #include "has_duplicate_basenames.hpp" // MG::has_duplicate_basenames()
//  ---------------------------------------------
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <optional>

#include "os-detect.hpp" // MS_WINDOWS, POSIX
#if defined(MS_WINDOWS)
  #include "string_utilities.hpp" // str::to_lower()
#endif

namespace fs = std::filesystem;
namespace rng = std::ranges;


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

namespace details
{
    //-----------------------------------------------------------------------
    [[nodiscard]] std::string path2stem(const fs::path& p)
       {
      #if defined(MS_WINDOWS)
        return str::tolower(p.stem().string());
      #else
        return p.stem().string();
      #endif
       };
}


//---------------------------------------------------------------------------
[[nodiscard]] bool has_duplicate_basenames(const std::vector<fs::path>& paths)
{
    std::unordered_set<std::string> stems;
    return rng::any_of(paths | rng::views::transform(details::path2stem), [&](const std::string& stem){ return !stems.insert(stem).second; });
}


//---------------------------------------------------------------------------
[[nodiscard]] std::optional<std::string> find_duplicate_basename(const std::vector<fs::path>& paths)
{
    auto stems_view = paths | std::views::transform(details::path2stem) | std::views::common;
    std::vector<std::string> sorted_stems(stems_view.begin(), stems_view.end());
    std::ranges::sort(sorted_stems);
    if( const auto it=rng::adjacent_find(sorted_stems); it!=sorted_stems.end() )
       {
        return *it;
       }
    return {};
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"has_duplicate_basenames"> has_duplicate_basenames_tests = []
{////////////////////////////////////////////////////////////////////////////

    ut::test("MG::has_duplicate_basenames()") = []
       {
        ut::expect( not MG::has_duplicate_basenames( {"/path/to/file1.txt", "/path/to/file2.h", "/path3/to/file3.c"}) );

      #if defined(MS_WINDOWS)
        ut::expect( MG::has_duplicate_basenames( {"/path/to/file1.txt", "/path/to/File2.h", "/path3/to/File1.c"}) );
      #else
        ut::expect( MG::has_duplicate_basenames( {"/path/to/file1.txt", "/path/to/File2.h", "/path3/to/file1.c"}) );
      #endif
       };

    ut::test("MG::find_duplicate_basename()") = []
       {
        ut::expect( not MG::find_duplicate_basename( {"/path/to/file1.txt", "/path/to/file2.h", "/path3/to/file3.c"}) );

      #if defined(MS_WINDOWS)
        ut::expect( MG::find_duplicate_basename( {"/path/to/file1.txt", "/path/to/File2.h", "/path3/to/File1.c"}).value()=="file1"sv );
      #else
        ut::expect( MG::find_duplicate_basename( {"/path/to/file1.txt", "/path/to/File2.h", "/path3/to/file1.c"}).value()=="file1"sv );
      #endif
       };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
