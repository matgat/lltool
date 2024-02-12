#pragma once
//  ---------------------------------------------
//  Utilities concerning integral types
//  temporary string
//  ---------------------------------------------
//  #include "integral_types_utilities.hpp" // MG::SmallerUnsignedIntegral<>
//  ---------------------------------------------
#include <cstdint> // std::int16_t, ...
#include <concepts> // std::integral
#include <limits> // std::numeric_limits<>
#include <type_traits> // std::make_unsigned_t<>

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG
{
    
template<typename T> struct SmallerUnsignedIntegral { using type = void; };
template<> struct SmallerUnsignedIntegral<std::int64_t> { using type = std::uint32_t; };
template<> struct SmallerUnsignedIntegral<std::int32_t> { using type = std::uint16_t; };
template<> struct SmallerUnsignedIntegral<std::int16_t> { using type = std::uint8_t; };

template<std::unsigned_integral T> struct make_unsigned { using type = std::make_unsigned_t<T>; };


//---------------------------------------------------------------------------
// "Almost safe" number cast (avoids narrowing)
template<std::integral T1, std::integral T2>
[[nodiscard]] constexpr inline T2 safe_num_cast(const T1 v) noexcept
{
    if( v<std::numeric_limits<T2>::lowest() ) [[unlikely]]
       {
        return std::numeric_limits<T2>::lowest();
       }
    else if( v>std::numeric_limits<T2>::max() ) [[unlikely]]
       {
        return std::numeric_limits<T2>::max();
       }
    return static_cast<T2>(v);
}



}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
#include <type_traits>
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"integral_types_utilities"> integral_types_utilities_tests = []
{////////////////////////////////////////////////////////////////////////////

    using SmallerUint = typename MG::SmallerUnsignedIntegral<int>::type;
    static_assert(sizeof(SmallerUint) < sizeof(int));
    //static_assert(std::is_same_v<SmallerUint, unsigned short int>);

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////