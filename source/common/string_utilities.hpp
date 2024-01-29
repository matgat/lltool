#pragma once
//  ---------------------------------------------
//  Useful utilities for strings
//  #include "string_utilities.hpp" // str::*
//  ---------------------------------------------
//#include <concepts> // std::convertible_to
#include <string>
#include <string_view>

#include "ascii_predicates.hpp" // ascii::to_lower

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace str //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//-----------------------------------------------------------------------
[[nodiscard]] constexpr std::string join_left(const char sep, const std::initializer_list<std::string_view> svs)
{
    std::string joined;

    size_t total_size = svs.size(); // The separators
    for( const std::string_view sv : svs )
       {
        total_size += sv.size(); // cppcheck-suppress useStlAlgorithm
       }

    joined.reserve(total_size);

    for( const std::string_view sv : svs )
       {
        joined += sep;
        joined += sv;
       }

    return joined;
}

//-----------------------------------------------------------------------
[[nodiscard]] constexpr std::string tolower(std::string s) noexcept
{
    for(char& ch : s)
       {
        ch = ascii::to_lower(ch);
       }
    return s;
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"string_utilities"> string_utilities_tests = []
{////////////////////////////////////////////////////////////////////////////

    ut::test("str::join_left()") = []
       {
        ut::expect( ut::that % str::join_left(';', {"a","b","c"})==";a;b;c"sv );
        ut::expect( ut::that % str::join_left(',', {})==""sv );
       };

    ut::test("str::tolower()") = []
       {
        ut::expect( ut::that % str::tolower("AbCdE fGhI 23 L")=="abcde fghi 23 l"sv );
        ut::expect( ut::that % str::tolower("a")=="a"sv );
        ut::expect( ut::that % str::tolower("A")=="a"sv );
        ut::expect( ut::that % str::tolower("1")=="1"sv );
        ut::expect( ut::that % str::tolower("")==""sv );
       };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
