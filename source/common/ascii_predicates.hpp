#pragma once
//  ---------------------------------------------
//  Querying chars
//  ---------------------------------------------
//  #include "ascii_predicates.hpp" // ascii::is_*
//  ---------------------------------------------
#include <concepts> // std::same_as<>
#include <cstdint> // std::uint16_t


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace ascii //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

namespace details
{
    enum mask_t : std::uint16_t
       {
        ISLOWER = 0x0001u, // std::islower
        ISUPPER = 0x0002u, // std::isupper
        ISALPHA = 0x0004u, // std::isalpha
        ISALNUM = 0x0010u, // std::isalnum
        ISDIGIT = 0x0020u, // std::isdigit
        ISXDIGI = 0x0040u, // std::isxdigit
        ISIDENT = 0x0080u, // std::isalnum plus '_'
        ISSPACE = 0x0100u, // std::isspace
        ISBLANK = 0x0200u, // std::isspace except '\n'
        ISGRAPH = 0x0400u, // std::isgraph
        ISPUNCT = 0x0800u, // std::ispunct
        ISCNTRL = 0x1000u, // std::iscntrl
        ISPRINT = 0x2000u  // std::isprint
       };

    static_assert( sizeof(unsigned char)==1 );
    static constexpr std::uint16_t ascii_lookup_table[256] =
      { 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1300, 0x1100, 0x1300, 0x1300, 0x1300, 0x1000, 0x1000,
        0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000,
        0x2300, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00,
        0x24F0, 0x24F0, 0x24F0, 0x24F0, 0x24F0, 0x24F0, 0x24F0, 0x24F0, 0x24F0, 0x24F0, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00,
        0x2C00, 0x24D6, 0x24D6, 0x24D6, 0x24D6, 0x24D6, 0x24D6, 0x2496, 0x2496, 0x2496, 0x2496, 0x2496, 0x2496, 0x2496, 0x2496, 0x2496,
        0x2496, 0x2496, 0x2496, 0x2496, 0x2496, 0x2496, 0x2496, 0x2496, 0x2496, 0x2496, 0x2496, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C80,
        0x2C00, 0x24D5, 0x24D5, 0x24D5, 0x24D5, 0x24D5, 0x24D5, 0x2495, 0x2495, 0x2495, 0x2495, 0x2495, 0x2495, 0x2495, 0x2495, 0x2495,
        0x2495, 0x2495, 0x2495, 0x2495, 0x2495, 0x2495, 0x2495, 0x2495, 0x2495, 0x2495, 0x2495, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x1000,
        0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0,
        0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0,
        0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0,
        0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0,
        0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0,
        0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0,
        0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0,
        0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0   , 0x0 };

    [[nodiscard]] inline constexpr bool check(const unsigned char ch, const mask_t mask){ return ascii_lookup_table[ch] & mask; }
    [[nodiscard]] inline constexpr bool check(const char ch, const mask_t mask){ return check(static_cast<unsigned char>(ch), mask); }
    [[nodiscard]] inline constexpr bool check(const char32_t cp, const mask_t mask)
       {
        if( cp<128 ) [[likely]]
           {
            return check(static_cast<unsigned char>(cp), mask);
           }
        return false;
       }
}


template<typename T> concept CharLike = std::same_as<T, char> or std::same_as<T, unsigned char> or std::same_as<T, char32_t>;

template<CharLike T> [[nodiscard]] constexpr bool is_lower(const T c) noexcept { return details::check(c, details::ISLOWER); }
template<CharLike T> [[nodiscard]] constexpr bool is_upper(const T c) noexcept { return details::check(c, details::ISUPPER); }
template<CharLike T> [[nodiscard]] constexpr bool is_alpha(const T c) noexcept { return details::check(c, details::ISALPHA); }
template<CharLike T> [[nodiscard]] constexpr bool is_alnum(const T c) noexcept { return details::check(c, details::ISALNUM); }
template<CharLike T> [[nodiscard]] constexpr bool is_digit(const T c) noexcept { return details::check(c, details::ISDIGIT); }
template<CharLike T> [[nodiscard]] constexpr bool is_xdigi(const T c) noexcept { return details::check(c, details::ISXDIGI); }
template<CharLike T> [[nodiscard]] constexpr bool is_ident(const T c) noexcept { return details::check(c, details::ISIDENT); }
template<CharLike T> [[nodiscard]] constexpr bool is_space(const T c) noexcept { return details::check(c, details::ISSPACE); }
template<CharLike T> [[nodiscard]] constexpr bool is_blank(const T c) noexcept { return details::check(c, details::ISBLANK); }
template<CharLike T> [[nodiscard]] constexpr bool is_graph(const T c) noexcept { return details::check(c, details::ISGRAPH); }
template<CharLike T> [[nodiscard]] constexpr bool is_punct(const T c) noexcept { return details::check(c, details::ISPUNCT); }
template<CharLike T> [[nodiscard]] constexpr bool is_cntrl(const T c) noexcept { return details::check(c, details::ISCNTRL); }
template<CharLike T> [[nodiscard]] constexpr bool is_print(const T c) noexcept { return details::check(c, details::ISPRINT); }

template<CharLike T> [[nodiscard]] constexpr bool is_endline(const T c) noexcept { return c==static_cast<T>('\n'); }
template<CharLike T> [[nodiscard]] constexpr bool is_always_false(const T) noexcept { return false; }


template<CharLike auto CH>
[[nodiscard]] constexpr bool is(const decltype(CH) ch) noexcept
   {
    return ch==CH;
   }

template<CharLike auto CH1, decltype(CH1)... CHS>
[[nodiscard]] constexpr bool is_any_of(const decltype(CH1) ch)
   {
    return ch==CH1 or ((ch==CHS) or ...);
   }

template<CharLike auto CH1, decltype(CH1)... CHS>
[[nodiscard]] constexpr bool is_none_of(const decltype(CH1) ch)
   {
    return ch!=CH1 and ((ch!=CHS) and ...);
   }

template<CharLike auto CH1, decltype(CH1)... CHS>
[[nodiscard]] constexpr bool is_space_or_any_of(const decltype(CH1) ch)
   {
    return is_space(ch) or is_any_of<CH1, CHS ...>(ch);
   }

template<CharLike auto CH1, decltype(CH1)... CHS>
[[nodiscard]] constexpr bool is_alnum_or_any_of(const decltype(CH1) ch)
   {
    return is_alnum(ch) or is_any_of<CH1, CHS ...>(ch);
   }

template<CharLike auto CH1, decltype(CH1)... CHS>
[[nodiscard]] constexpr bool is_punct_and_none_of(const decltype(CH1) ch)
   {
    return is_punct(ch) and is_none_of<CH1, CHS ...>(ch);
   }


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"MG::ascii_predicates"> ascii_predicates_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("basic") = []
   {
    ut::expect( ut::that % not ascii::is_always_false('a') );
    ut::expect( ut::that % not ascii::is_endline('a') );
    ut::expect( ut::that % not ascii::is_space('a') );
    ut::expect( ut::that % not ascii::is_blank('a') );
    ut::expect( ut::that % not ascii::is_digit('a') );
    ut::expect( ut::that % ascii::is_alpha('a') );
    ut::expect( ut::that % ascii::is_alnum('a') );
    ut::expect( ut::that % not ascii::is_punct('a') );

    ut::expect( ut::that % not ascii::is_always_false('2') );
    ut::expect( ut::that % not ascii::is_endline('2') );
    ut::expect( ut::that % not ascii::is_space('2') );
    ut::expect( ut::that % not ascii::is_blank('2') );
    ut::expect( ut::that % ascii::is_digit('2') );
    ut::expect( ut::that % not ascii::is_alpha('2') );
    ut::expect( ut::that % ascii::is_alnum('2') );
    ut::expect( ut::that % not ascii::is_punct('2') );

    ut::expect( ut::that % not ascii::is_always_false('\t') );
    ut::expect( ut::that % not ascii::is_endline('\t') );
    ut::expect( ut::that % ascii::is_space('\t') );
    ut::expect( ut::that % ascii::is_blank('\t') );
    ut::expect( ut::that % not ascii::is_digit('\t') );
    ut::expect( ut::that % not ascii::is_alpha('\t') );
    ut::expect( ut::that % not ascii::is_alnum('\t') );
    ut::expect( ut::that % not ascii::is_punct('\t') );

    ut::expect( ut::that % not ascii::is_always_false('\n') );
    ut::expect( ut::that % ascii::is_endline('\n') );
    ut::expect( ut::that % ascii::is_space('\n') );
    ut::expect( ut::that % not ascii::is_blank('\n') );
    ut::expect( ut::that % not ascii::is_digit('\n') );
    ut::expect( ut::that % not ascii::is_alpha('\n') );
    ut::expect( ut::that % not ascii::is_alnum('\n') );
    ut::expect( ut::that % not ascii::is_punct('\n') );

    ut::expect( ut::that % not ascii::is_always_false(';') );
    ut::expect( ut::that % not ascii::is_endline(';') );
    ut::expect( ut::that % not ascii::is_space(';') );
    ut::expect( ut::that % not ascii::is_blank(';') );
    ut::expect( ut::that % not ascii::is_digit(';') );
    ut::expect( ut::that % not ascii::is_alpha(';') );
    ut::expect( ut::that % not ascii::is_alnum(';') );
    ut::expect( ut::that % ascii::is_punct(';') );

    ut::expect( ut::that % not ascii::is_always_false('\xE0') );
    ut::expect( ut::that % not ascii::is_endline('\xE0') );
    ut::expect( ut::that % not ascii::is_space('\xE0') );
    ut::expect( ut::that % not ascii::is_blank('\xE0') );
    ut::expect( ut::that % not ascii::is_digit('\xE0') );
    ut::expect( ut::that % not ascii::is_alpha('\xE0') );
    ut::expect( ut::that % not ascii::is_alnum('\xE0') );
    ut::expect( ut::that % not ascii::is_punct('\xE0') );
   };

ut::test("templated") = []
   {
    ut::expect( ut::that % ascii::is<'a'>('a') );
    ut::expect( ut::that % ascii::is_any_of<'a','\xE0',';'>('a') );
    ut::expect( ut::that % not ascii::is_none_of<'a','\xE0',';'>('a') );
    ut::expect( ut::that % ascii::is_space_or_any_of<'a','\xE0',';'>('a') );
    ut::expect( ut::that % ascii::is_alnum_or_any_of<'a','\xE0',';'>('a') );
    ut::expect( ut::that % not ascii::is_punct_and_none_of<','>('a') );

    ut::expect( ut::that % not ascii::is<'a'>('b') );
    ut::expect( ut::that % not ascii::is_any_of<'a','\xE0',';'>('b') );
    ut::expect( ut::that % ascii::is_none_of<'a','\xE0',';'>('b') );
    ut::expect( ut::that % not ascii::is_space_or_any_of<'a','\xE0',';'>('b') );
    ut::expect( ut::that % ascii::is_alnum_or_any_of<'a','\xE0',';'>('b') );
    ut::expect( ut::that % not ascii::is_punct_and_none_of<','>('b') );

    ut::expect( ut::that % not ascii::is<'a'>(',') );
    ut::expect( ut::that % not ascii::is_any_of<'a','\xE0',';'>(',') );
    ut::expect( ut::that % ascii::is_none_of<'a','\xE0',';'>(',') );
    ut::expect( ut::that % not ascii::is_space_or_any_of<'a','\xE0',';'>(',') );
    ut::expect( ut::that % not ascii::is_alnum_or_any_of<'a','\xE0',';'>(',') );
    ut::expect( ut::that % not ascii::is_punct_and_none_of<','>(',') );

    ut::expect( ut::that % not ascii::is<'a'>(';') );
    ut::expect( ut::that % ascii::is_any_of<'a','\xE0',';'>(';') );
    ut::expect( ut::that % not ascii::is_none_of<'a','\xE0',';'>(';') );
    ut::expect( ut::that % ascii::is_space_or_any_of<'a','\xE0',';'>(';') );
    ut::expect( ut::that % ascii::is_alnum_or_any_of<'a','\xE0',';'>(';') );
    ut::expect( ut::that % ascii::is_punct_and_none_of<','>(';') );

    ut::expect( ut::that % not ascii::is<'a'>(' ') );
    ut::expect( ut::that % not ascii::is_any_of<'a','\xE0',';'>(' ') );
    ut::expect( ut::that % ascii::is_none_of<'a','\xE0',';'>(' ') );
    ut::expect( ut::that % ascii::is_space_or_any_of<'a','\xE0',';'>(' ') );
    ut::expect( ut::that % not ascii::is_alnum_or_any_of<'a','\xE0',';'>(' ') );
    ut::expect( ut::that % not ascii::is_punct_and_none_of<','>(' ') );

    ut::expect( ut::that % not ascii::is<'a'>('\xE0') );
    ut::expect( ut::that % ascii::is_any_of<'a','\xE0',';'>('\xE0') );
    ut::expect( ut::that % not ascii::is_none_of<'a','\xE0',';'>('\xE0') );
    ut::expect( ut::that % ascii::is_space_or_any_of<'a','\xE0',';'>('\xE0') );
    ut::expect( ut::that % ascii::is_alnum_or_any_of<'a','\xE0',';'>('\xE0') );
    ut::expect( ut::that % not ascii::is_punct_and_none_of<','>('\xE0') );

    ut::expect( ut::that % not ascii::is<'a'>('\xE1') );
    ut::expect( ut::that % not ascii::is_any_of<'a','\xE0',';'>('\xE1') );
    ut::expect( ut::that % ascii::is_none_of<'a','\xE0',';'>('\xE1') );
    ut::expect( ut::that % not ascii::is_space_or_any_of<'a','\xE0',';'>('\xE1') );
    ut::expect( ut::that % not ascii::is_alnum_or_any_of<'a','\xE0',';'>('\xE1') );
    ut::expect( ut::that % not ascii::is_punct_and_none_of<','>('\xE1') );
   };

ut::test("my blanks tests") = []
   {
    ut::expect( ut::that % ascii::is_space(' ')  and ascii::is_blank(' ') and not ascii::is_endline(' ') );
    ut::expect( ut::that % ascii::is_space('\t') and ascii::is_blank('\t') and not ascii::is_endline('\t') );
    ut::expect( ut::that % ascii::is_space('\n') and not ascii::is_blank('\n') and ascii::is_endline('\n') );
    ut::expect( ut::that % ascii::is_space('\r') and ascii::is_blank('\r') and not ascii::is_endline('\r') );
    ut::expect( ut::that % ascii::is_space('\v') and ascii::is_blank('\v') and not ascii::is_endline('\v') );
    ut::expect( ut::that % ascii::is_space('\f') and ascii::is_blank('\f') and not ascii::is_endline('\f') );
    ut::expect( ut::that % not ascii::is_space('\b') and not ascii::is_blank('\b') and not ascii::is_endline('\b') );
   };

ut::test("char32_t") = []
   {
    ut::expect( not ascii::is_space(U'a') );
    ut::expect( not ascii::is_blank(U'a') );
    ut::expect( not ascii::is_endline(U'a') );
    ut::expect( not ascii::is_digit(U'a') );
    ut::expect( ascii::is_alpha(U'a') );
    ut::expect( not ascii::is_punct(U'a') );

    ut::expect( not ascii::is_space(U'à') );
    ut::expect( not ascii::is_blank(U'à') );
    ut::expect( not ascii::is_endline(U'à') );
    ut::expect( not ascii::is_digit(U'à') );
    ut::expect( not ascii::is_alpha(U'à') );
    ut::expect( not ascii::is_punct(U'à') );

    ut::expect( not ascii::is_space(U'2') );
    ut::expect( not ascii::is_blank(U'2') );
    ut::expect( not ascii::is_endline(U'2') );
    ut::expect( ascii::is_digit(U'2') );
    ut::expect( not ascii::is_alpha(U'2') );
    ut::expect( not ascii::is_punct(U'2') );

    ut::expect( ascii::is_space(U' ') );
    ut::expect( ascii::is_blank(U' ') );
    ut::expect( not ascii::is_endline(U' ') );
    ut::expect( not ascii::is_digit(U' ') );
    ut::expect( not ascii::is_alpha(U' ') );
    ut::expect( not ascii::is_punct(U' ') );

    ut::expect( ascii::is_space(U'\r') );
    ut::expect( ascii::is_blank(U'\r') );
    ut::expect( not ascii::is_endline(U'\r') );
    ut::expect( not ascii::is_digit(U'\r') );
    ut::expect( not ascii::is_alpha(U'\r') );
    ut::expect( not ascii::is_punct(U'\r') );

    ut::expect( ascii::is_space(U'\n') );
    ut::expect( not ascii::is_blank(U'\n') );
    ut::expect( ascii::is_endline(U'\n') );
    ut::expect( not ascii::is_digit(U'\n') );
    ut::expect( not ascii::is_alpha(U'\n') );
    ut::expect( not ascii::is_punct(U'\n') );

    ut::expect( not ascii::is_space(U'▙') );
    ut::expect( not ascii::is_blank(U'▙') );
    ut::expect( not ascii::is_endline(U'▙') );
    ut::expect( not ascii::is_digit(U'▙') );
    ut::expect( not ascii::is_alpha(U'▙') );
    ut::expect( not ascii::is_punct(U'▙') );

    ut::expect( not ascii::is_space(U'❸') );
    ut::expect( not ascii::is_blank(U'❸') );
    ut::expect( not ascii::is_endline(U'❸') );
    ut::expect( not ascii::is_digit(U'❸') );
    ut::expect( not ascii::is_alpha(U'❸') );
    ut::expect( not ascii::is_punct(U'❸') );

    ut::expect( ascii::is<U'♦'>(U'♦') and not ascii::is<U'♥'>(U'♦') );
    ut::expect( ascii::is_any_of<U'♥',U'♦',U'♠',U'♣'>(U'♣') and not ascii::is_any_of<U'♥',U'♦',U'♠',U'♣'>(U'☺') );
    ut::expect( not ascii::is_none_of<U'♥',U'♦',U'♠',U'♣'>(U'♣') and ascii::is_none_of<U'♥',U'♦',U'♠',U'♣'>(U'☺') );
    ut::expect( ascii::is_space_or_any_of<U'a',U'b'>(U'a') and ascii::is_space_or_any_of<U'a',U'b'>(U' ') and not ascii::is_space_or_any_of<U'a',U'b'>(U'c') );
    ut::expect( ascii::is_space_or_any_of<U'a',U'\xE0',U';'>(U';') );
    ut::expect( ascii::is_punct_and_none_of<U'-'>(U';') and not ascii::is_punct_and_none_of<U'-'>(U'a') and not ascii::is_punct_and_none_of<U'-'>(U'-') );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



// https://gcc.godbolt.org/z/eKPMxjz5z
//#include <iostream>
//#include <iomanip>
//#include <format>
//#include <cctype>
//#include <cstdint>
//struct LookupTableGenerator
//{
//    std::uint16_t lookup_table[256] = {};
//
//    LookupTableGenerator()
//       {
//        for(int i=0; i<256; ++i)
//           {
//            lookup_table[i] = generate_item(static_cast<unsigned char>(i));
//           }
//       }
//
//    static std::uint16_t generate_item(unsigned char c)
//       {
//        enum masks : std::uint16_t
//        {
//            ISLOWER = 0x0001u, // std::islower
//            ISUPPER = 0x0002u, // std::isupper
//            ISALPHA = 0x0004u, // std::isalpha
//            ISALNUM = 0x0010u, // std::isalnum
//            ISDIGIT = 0x0020u, // std::isdigit
//            ISXDIGI = 0x0040u, // std::isxdigit
//            ISIDENT = 0x0080u, // std::isalnum plus '_'
//            ISSPACE = 0x0100u, // std::isspace
//            ISBLANK = 0x0200u, // std::isblank
//            ISGRAPH = 0x0400u, // std::isgraph
//            ISPUNCT = 0x0800u, // std::ispunct
//            ISCNTRL = 0x1000u, // std::iscntrl
//            ISPRINT = 0x2000u  // std::isprint
//        };
//
//        std::uint16_t mask = 0;
//        if( std::islower(c) )  mask += ISLOWER;
//        if( std::isupper(c) )  mask += ISUPPER;
//        if( std::isalpha(c) )  mask += ISALPHA;
//        if( std::isalnum(c) )  mask += ISALNUM;
//        if( std::isdigit(c) )  mask += ISDIGIT;
//        if( std::isxdigit(c) ) mask += ISXDIGI;
//        if( std::isalnum(c) or c=='_' ) mask += ISIDENT;
//        if( std::isspace(c) )  mask += ISSPACE;
//        if( std::isspace(c) and c!='\n' )  mask += ISBLANK; // std::isblank(c)
//        if( std::isgraph(c) )  mask += ISGRAPH;
//        if( std::ispunct(c) )  mask += ISPUNCT;
//        if( std::iscntrl(c) )  mask += ISCNTRL;
//        if( std::isprint(c) )  mask += ISPRINT;
//        return mask;
//       }
//
//    void print_table()
//       {
//        for(int i=0; i<256; ++i)
//           {
//            std::cout << std::hex << "\\x" << i << " (" << char(i) << ") 0x" << lookup_table[i] << '\n';
//           }
//       }
//
//    void print_array()
//       {
//        std::cout << "static constexpr std::uint16_t ascii_lookup_table[256] = \n";
//        std::cout << std::left;
//        for(int i=0; i<256; ++i)
//           {
//            std::cout << std::setw(6) << std::format("0x{:X}",lookup_table[i]) << ", ";
//            if((i+1)%16 == 0) std::cout << '\n';
//           }
//       }
//};
//
//int main()
//{
//    LookupTableGenerator tbl;
//    //tbl.print_table();
//    tbl.print_array();
//}
