﻿#pragma once
//  ---------------------------------------------
//  Simple parsing utils
//  ---------------------------------------------
//  #include "ascii_parsing_utils.hpp" // ascii::extract*<>
//  ---------------------------------------------
#include <type_traits> // std::is_*_v<T>

#include "ascii_simple_lexer.hpp" // ascii::simple_lexer
#include "string_conversions.hpp" // str::to_num<>()



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
template<typename T>
struct extracted_t final
{
    T extracted;
    std::string_view remaining;
};

//---------------------------------------------------------------------------
template<typename T1, typename T2>
struct extracted_pair_t final
{
    T1 extracted1;
    T2 extracted2;
    std::string_view remaining;
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace ascii //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
template<typename T>
[[nodiscard]] constexpr T extract(ascii::simple_lexer<char>& lexer)
{
    T result;

    if constexpr (std::is_integral_v<T>)
       {
        result = str::to_num<T>( lexer.get_digits() );
        // = str::to_num<T>( lexer.get_while(ascii::is_digit_or_any_of<'+','-'>) );
       }
    else if constexpr (std::is_floating_point_v<T>)
       {
        result = str::to_num<T>( lexer.get_while(ascii::is_float) );
       }
    //else if constexpr (std::is_same_v<T, std::string>)
    //else if constexpr (std::is_convertible_v<T, std::string_view>)
    else
       {
        static_assert( std::is_same_v<T, std::string_view> );
        result = lexer.get_until(ascii::is_punct);
       }

    return result;
}


//---------------------------------------------------------------------------
template<typename T>
[[nodiscard]] constexpr MG::extracted_t<T> extract(const std::string_view sv)
{
    MG::extracted_t<T> result{};

    ascii::simple_lexer lexer(sv);
    lexer.skip_any_space();
    result.extracted = extract<T>(lexer);
    result.remaining = lexer.remaining();

    return result;
}


//---------------------------------------------------------------------------
template<typename T1, typename T2>
[[nodiscard]] constexpr MG::extracted_pair_t<T1,T2> extract_pair(const std::string_view sv)
{
    MG::extracted_pair_t<T1,T2> result;

    ascii::simple_lexer lexer(sv);
    result.extracted1 = extract<T1>(lexer);
    lexer.skip_while( ascii::is_space_or_punct );
    result.extracted2 = extract<T2>(lexer);
    result.remaining = lexer.remaining();

    return result;
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
template<typename T> [[nodiscard]] bool operator==(const MG::extracted_t<T>& a, const MG::extracted_t<T>& b) noexcept { return a.extracted==b.extracted and a.remaining==b.remaining; }
template<typename T> [[maybe_unused]] std::ostream& operator<<(std::ostream& os, const MG::extracted_t<T>& e) { os << "'" << e.extracted << " '" << e.remaining << "'\n"; return os; }
template<typename T1, typename T2> [[nodiscard]] bool operator==(const MG::extracted_pair_t<T1,T2>& a, const MG::extracted_pair_t<T1,T2>& b) noexcept { return a.extracted1==b.extracted1 and a.extracted2==b.extracted2 and a.remaining==b.remaining; }
template<typename T1, typename T2> [[maybe_unused]] std::ostream& operator<<(std::ostream& os, const MG::extracted_pair_t<T1,T2>& e) { os << "'" << e.extracted1 << "', '" << e.extracted2 << " '" << e.remaining << "'\n"; return os; }
template<typename T, typename U> constexpr bool are_same_type = std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

/////////////////////////////////////////////////////////////////////////////
static ut::suite<"ascii_parsing_utils"> ascii_parsing_utils_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("ascii::extract<>") = []
   {
    const auto [extracted, remaining] = ascii::extract<int>("123abc");
    ut::expect( are_same_type<decltype(extracted), int> );
    ut::expect( are_same_type<decltype(remaining), std::string_view> );
    ut::expect( ut::that % extracted==123 and remaining=="abc"sv );

    ut::expect( ascii::extract<int>("42")==MG::extracted_t{42, ""sv} );
    ut::expect( ascii::extract<int>("42.123")==MG::extracted_t{42, ".123"sv} );
    ut::expect( ascii::extract<int>("42a")==MG::extracted_t{42, "a"sv} );
    ut::expect( ascii::extract<double>("42.123mm")==MG::extracted_t{42.123, "mm"sv} );
    ut::expect( ascii::extract<std::string_view>("42.123mm")==MG::extracted_t{"42"sv, ".123mm"sv} );

    ut::expect( ut::throws([]{ [[maybe_unused]] auto ret = ascii::extract<int>(""); }) ) << "should throw\n";
    ut::expect( ut::throws([]{ [[maybe_unused]] auto ret = ascii::extract<int>("aaa"); }) ) << "should throw\n";
    ut::expect( ut::throws([]{ [[maybe_unused]] auto ret = ascii::extract<double>("aaa"); }) ) << "should throw\n";
   };

ut::test("ascii::extract_pair<>") = []
   {
    const auto [extracted1, extracted2, remaining] = ascii::extract_pair<double,int>("1.2;3abc");
    ut::expect( are_same_type<decltype(extracted1), double> );
    ut::expect( are_same_type<decltype(extracted2), int> );
    ut::expect( are_same_type<decltype(remaining), std::string_view> );
    ut::expect( ut::that % extracted1==1.2 and extracted2==3 and remaining=="abc"sv );

    ut::expect( ascii::extract_pair<int,int>("2,8")==MG::extracted_pair_t{2, 8, ""sv} );
    ut::expect( ascii::extract_pair<double,int>("2.4;42")==MG::extracted_pair_t{2.4, 42, ""sv} );
    ut::expect( ascii::extract_pair<double,int>("2.4;42b")==MG::extracted_pair_t{2.4, 42, "b"sv} );
    ut::expect( ascii::extract_pair<double,int>("1.1;1.1")==MG::extracted_pair_t{1.1, 1, ".1"sv} );
    ut::expect( ascii::extract_pair<int,double>("1.1;1.1")==MG::extracted_pair_t{1, 1.0, ";1.1"sv} );
    ut::expect( ascii::extract_pair<double,std::string_view>("2.4;42b")==MG::extracted_pair_t{2.4, "42b"sv, ""sv} );
    ut::expect( ascii::extract_pair<std::string_view,std::string_view>("2.4;42b")==MG::extracted_pair_t{"2"sv, "4"sv, ";42b"sv} );

    ut::expect( ut::throws([]{ [[maybe_unused]] auto ret = ascii::extract_pair<int,int>("a,1"); }) ) << "should throw\n";
    ut::expect( ut::throws([]{ [[maybe_unused]] auto ret = ascii::extract_pair<int,int>("1,a"); }) ) << "should throw\n";
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
