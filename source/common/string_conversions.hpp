#pragma once
//  ---------------------------------------------
//  Functions for string conversions
//  ---------------------------------------------
//  #include "string_conversions.hpp" // str::to_num<>()
//  ---------------------------------------------
#include <stdexcept> // std::runtime_error
#include <string_view>
#include <charconv> // std::from_chars


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace str //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
// Convert a string_view to number
template<typename T>
[[nodiscard]] constexpr T to_num(const std::string_view sv)
{
    T result;
    const auto it_end = sv.data() + sv.size();
    const auto [it, ec] = std::from_chars(sv.data(), it_end, result);
    if( ec!=std::errc() || it!=it_end )
       {
        throw std::runtime_error{ fmt::format("\"{}\" is not a valid number", sv) };
       }
    return result;
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"string_conversions"> string_conversions_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("str::to_num<>") = []
   {
    ut::expect( ut::that % str::to_num<int>("42")==42 );
    ut::expect( ut::that % str::to_num<double>("42.1")==42.1 );

    ut::expect( ut::throws([]{ [[maybe_unused]] const auto n = str::to_num<int>("42.1"); }) ) << "should throw\n";
    ut::expect( ut::throws([]{ [[maybe_unused]] const auto n = str::to_num<int>("42a"); }) ) << "should throw\n";
    ut::expect( ut::throws([]{ [[maybe_unused]] const auto n = str::to_num<int>(""); }) ) << "should throw\n";
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
