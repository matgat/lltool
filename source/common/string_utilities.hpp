﻿#pragma once
//  ---------------------------------------------
//  Useful utilities for strings
//  ---------------------------------------------
//  #include "string_utilities.hpp" // str::*
//  ---------------------------------------------
//#include <concepts> // std::convertible_to
#include <string>
#include <string_view>

#include "ascii_predicates.hpp" // ascii::to_lower()

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace str //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
//template<std::convertible_to<std::string_view>... Args>
//[[nodiscard]] constexpr std::string concat(Args&&... args, const char delim)
//{
//    std::string s;
//    const std::size_t totsiz = sizeof...(args) + (std::size(args) + ...);
//    s.reserve(totsiz);
//    ((s+=args, s+=delim), ...);
//    if(!s.empty()) s.resize(s.size()-1);
//    return s;
//}

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
    for(char& ch : s) ch = ascii::to_lower(ch);
    return s;
}

//-----------------------------------------------------------------------
//[[nodiscard]] bool compare_nocase(const std::string_view sv1, const std::string_view sv2) noexcept
//{
//    if( sv1.size()!=sv2.size() ) return false;
//    //#if defined(MS_WINDOWS)
//    //#include <cstring>
//    //return( ::_memicmp( sv1.data(), sv2.data(), sv1.size())==0 );
//    for( std::size_t i=0; i<sv1.size(); ++i )
//        if( ascii::to_lower(sv1[i]) !=
//            ascii::to_lower(sv2[i]) ) return false;
//    return true;
//}


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
