#pragma once
//  ---------------------------------------------
//  Useful utilities for strings
//  ---------------------------------------------
//  #include "string_utilities.hpp" // str::*
//  ---------------------------------------------
//#include <concepts> // std::convertible_to
#include <string>
#include <string_view>

#include "ascii_predicates.hpp" // ascii::*

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace str //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
//template<std::same_as<std::string_view>... Args>
//[[nodiscard]] constexpr std::string concat(const std::string_view first, Args... others, const char delim)
//{
//    std::string s;
//    const std::size_t totsiz = std::size(first) + (std::size(others) + ...) + sizeof...(others);
//    s.reserve(totsiz);
//    s += first;
//    ((s+=delim, s+=others), ...);
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
[[nodiscard]] constexpr std::string to_lower(std::string s) noexcept
{
    for(char& ch : s) ch = ascii::to_lower(ch);
    return s;
}

//-----------------------------------------------------------------------
//[[nodiscard]] constexpr bool compare_nocase(const std::string_view sv1, const std::string_view sv2) noexcept
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

//-----------------------------------------------------------------------
//[[nodiscard]] constexpr std::string_view trim_left(std::string_view sv)
//{
//    sv.remove_prefix(std::distance(sv.cbegin(), std::find_if_not(sv.cbegin(), sv.cend(), ascii::is_space<char>)));
//    return sv;
//}

//-----------------------------------------------------------------------
[[nodiscard]] constexpr std::string_view trim_right(std::string_view sv)
{
    const auto d = std::distance(sv.crbegin(), std::find_if_not(sv.crbegin(), sv.crend(), ascii::is_space<char>));
    sv.remove_suffix( static_cast<std::size_t>(d) );
    return sv;
}

//---------------------------------------------------------------------------
//constexpr std::string& trim_left(std::string& s)
//{
//    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), ascii::is_space<char>));
//    return s;
//}

//---------------------------------------------------------------------------
//constexpr std::string& trim_right(std::string& s)
//{
//    s.erase(std::find_if_not(s.rbegin(), s.rend(), ascii::is_space<char>).base(), s.end());
//    return s;
//}


//---------------------------------------------------------------------------
// Represent not printable characters
[[nodiscard]] constexpr std::string escape(const char ch) noexcept
   {
    std::string s(1,ch);
    switch( ch )
       {
        case '\n': s = "\\n"; break;
        case '\r': s = "\\r"; break;
        case '\t': s = "\\t"; break;
        //case '\f': s = "\\f"; break;
        //case '\v': s = "\\v"; break;
        //case '\a': s = "\\a"; break;
        //case '\b': s = "\\b"; break;
        case '\0': s = "\\0"; break;
       }
    return s;
   }
//---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string escape(const std::string_view sv) noexcept
   {
    std::string s;
    s.reserve( sv.size() );
    for(const char ch : sv)
       {
        s += escape(ch);
       }
    return s;
   }


//---------------------------------------------------------------------------
[[nodiscard]] /*constexpr*/ std::string quoted(const std::string_view sv, const char quot ='\"')
{
    if( sv.contains(quot) )
       {// TODO: Should escape internal double quotes
        throw std::runtime_error( std::format("str::quoted: {} contains already {}"sv, sv, quot) );
       }
    return std::format("{0}{1}{0}"sv, quot, sv);
}


//---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string_view unquoted(const std::string_view sv, const char quot ='\"') noexcept
{
    if( sv.size()>=2u and sv.front()==quot and sv.back()==quot )
       {
        return sv.substr(1u, sv.size()-2u);
       }
    return sv;
}

//---------------------------------------------------------------------------
// Replace all occurrences in a string
//constexpr void replace_all(std::string& s, const std::string& from, const std::string& to)
//{
//    //std::string::size_type i = 0;
//    //while( (i = s.find(from, i)) != std::string::npos )
//    //   {
//    //    s.replace(i, from.size(), to);
//    //    i += to.size();
//    //   }
//
//    std::string sout;
//    sout.reserve(s.size());
//    std::string::size_type i, i_start=0;
//    while( (i = s.find(from, i_start)) != std::string::npos )
//       {
//        sout.append(s, i_start, i-i_start);
//        sout.append(to);
//        i_start = i + from.size();
//       }
//    sout.append(s, i_start); //, std::string::npos);
//    s.swap(sout);
//}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//void print_strings(std::convertible_to<std::string_view> auto&& ...s)
//{
//    for(const auto sv : std::initializer_list<std::string_view>{ s... }) std::print("{}\n",sv);
//}
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"string_utilities"> string_utilities_tests = []
{////////////////////////////////////////////////////////////////////////////

//ut::test("str::concat()") = []
//   {
//    ut::expect( ut::that % str::concat("a","b","c",';'})=="a;b;c"sv );
//    ut::expect( ut::that % str::concat("a",';'})=="a;b;c"sv );
//    ut::expect( ut::that % str::concat("",';')==""sv );
//   };

ut::test("str::join_left()") = []
   {
    ut::expect( ut::that % str::join_left(';', {"a","b","c"})==";a;b;c"sv );
    ut::expect( ut::that % str::join_left(',', {})==""sv );
   };

ut::test("str::to_lower()") = []
   {
    ut::expect( ut::that % str::to_lower("AbCdE fGhI 23 L")=="abcde fghi 23 l"sv );
    ut::expect( ut::that % str::to_lower("a")=="a"sv );
    ut::expect( ut::that % str::to_lower("A")=="a"sv );
    ut::expect( ut::that % str::to_lower("1")=="1"sv );
    ut::expect( ut::that % str::to_lower("")==""sv );
   };

ut::test("str::trim_right()") = []
   {
    ut::expect( ut::that % str::trim_right(" abc \t \r"sv)==" abc"sv );
    ut::expect( ut::that % str::trim_right(" abc\n"sv)==" abc"sv );
    ut::expect( ut::that % str::trim_right(" abc"sv)==" abc"sv );
    ut::expect( ut::that % str::trim_right("abc"sv)=="abc"sv );
    ut::expect( ut::that % str::trim_right("\t \r"sv)==""sv );
    ut::expect( ut::that % str::trim_right(" "sv)==""sv );
    ut::expect( ut::that % str::trim_right("a"sv)=="a"sv );
    ut::expect( ut::that % str::trim_right(""sv)==""sv );
   };

ut::test("str::escape()") = []
   {
    ut::expect( ut::that % str::escape("1\n2\t3\0"sv)=="1\\n2\\t3\\0"sv );
    ut::expect( ut::that % str::escape("\r"sv)=="\\r"sv );
    ut::expect( ut::that % str::escape("a"sv)=="a"sv );
    ut::expect( ut::that % str::escape(""sv)==""sv );
   };

ut::test("str::quoted()") = []
   {
    ut::expect( ut::that % str::quoted("abc"sv)=="\"abc\""sv );
    ut::expect( ut::that % str::quoted(""sv)=="\"\""sv );
    ut::expect( ut::throws([]{ [[maybe_unused]] auto s = str::quoted("a\"bc"sv); }) ) << "quoting a string containing quotes\n";
   };

ut::test("str::unquoted()") = []
   {
    ut::expect( ut::that % str::unquoted("\"abc\""sv)=="abc"sv );
    ut::expect( ut::that % str::unquoted("\"a\""sv)=="a"sv );
    ut::expect( ut::that % str::unquoted("\"\""sv)==""sv );

    ut::expect( ut::that % str::unquoted(" \"abc\""sv)==" \"abc\""sv );
    ut::expect( ut::that % str::unquoted("\"abc\" "sv)=="\"abc\" "sv );
    ut::expect( ut::that % str::unquoted(" \"abc\" "sv)==" \"abc\" "sv );

    ut::expect( ut::that % str::unquoted("#\"abc\""sv)=="#\"abc\""sv );
    ut::expect( ut::that % str::unquoted("\"abc\"#"sv)=="\"abc\"#"sv );
    ut::expect( ut::that % str::unquoted("#\"abc\"#"sv)=="#\"abc\"#"sv );

    ut::expect( ut::that % str::unquoted("a\""sv)=="a\""sv );
    ut::expect( ut::that % str::unquoted("\"a"sv)=="\"a"sv );
    ut::expect( ut::that % str::unquoted("\""sv)=="\""sv );

    ut::expect( ut::that % str::unquoted("abc"sv)=="abc"sv );
    ut::expect( ut::that % str::unquoted("a"sv)=="a"sv );
    ut::expect( ut::that % str::unquoted(""sv)==""sv );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
