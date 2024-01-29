#pragma once
//  ---------------------------------------------
//  A map of string pairs
//  ---------------------------------------------
//  #include "ascii_simple_parser.hpp" // ascii::simple_parser
//  ---------------------------------------------
#include <concepts> // std::predicate
#include <string_view>

#include "ascii_predicates.hpp" // ascii::is_*

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace ascii //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class simple_parser
{
 public:
    const std::string_view input;
 private:
    std::size_t i = 0;

 public:
    constexpr explicit simple_parser(const std::string_view buf) noexcept
      : input(buf)
       {}

    [[nodiscard]] constexpr std::size_t pos() const noexcept { return i; }
    [[nodiscard]] constexpr bool got_data() const noexcept { return i<input.size(); }
    [[maybe_unused]] constexpr bool get_next() noexcept
       {
        if( i<input.size() ) [[likely]]
           {
            return ++i<input.size();
           }
        return false; // 'i' points to one past next
       }

    [[nodiscard]] constexpr bool got(const char ch) const noexcept { return i<input.size() and input[i]==ch; }
    [[nodiscard]] constexpr bool got_endline() const noexcept { return got('\n'); }
    [[nodiscard]] constexpr bool got_space() const noexcept { return i<input.size() and ascii::is_space(input[i]); }
    [[nodiscard]] constexpr bool got_blank() const noexcept { return i<input.size() and ascii::is_blank(input[i]); }
    [[nodiscard]] constexpr bool got_digit() const noexcept { return i<input.size() and ascii::is_digit(input[i]); }
    [[nodiscard]] constexpr bool got_alpha() const noexcept { return i<input.size() and ascii::is_alpha(input[i]); }
    [[nodiscard]] constexpr bool got_alnum() const noexcept { return i<input.size() and ascii::is_alnum(input[i]); }
    [[nodiscard]] constexpr bool got_punct() const noexcept { return i<input.size() and ascii::is_punct(input[i]); }
    [[nodiscard]] constexpr bool got_ident() const noexcept { return i<input.size() and ascii::is_ident(input[i]); }

    //template<char... CHS> [[nodiscard]] constexpr bool got_any_of() { return i<input.size() and ((input[i]==CHS) or ...); }
    //template<char... CHS> [[nodiscard]] constexpr bool got_none_of() { return i<input.size() and ((input[i]!=CHS) and ...); }

    [[maybe_unused]] constexpr bool eat(const char ch) noexcept { if(got(ch)){get_next(); return true;} return false; }

    constexpr void skip_any_space() noexcept { while( got_space() and get_next() ); }
    constexpr void skip_blanks() noexcept { while( got_blank() and get_next() ); }

    template<std::predicate<const char> CharPredicate =decltype(ascii::is_always_false<char>)>
    [[nodiscard]] constexpr bool got(CharPredicate is) const noexcept { return i<input.size() and is(input[i]); }

    template<std::predicate<const char> CharPredicate =decltype(ascii::is_always_false<char>)>
    [[nodiscard]] constexpr std::string_view get_while(CharPredicate is) noexcept
       {
        const std::size_t i_start = pos();
        while( got(is) and get_next() );
        return {input.data()+i_start, pos()-i_start};
       }

    [[nodiscard]] constexpr std::string_view get_alphabetic() noexcept { return get_while(ascii::is_alpha<char>); }
    [[nodiscard]] constexpr std::string_view get_alnums() noexcept { return get_while(ascii::is_alnum<char>); }
    [[nodiscard]] constexpr std::string_view get_digits() noexcept { return get_while(ascii::is_digit<char>); }
    [[nodiscard]] constexpr std::string_view get_identifier() noexcept { return get_while(ascii::is_ident<char>); }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"ascii::simple_parser"> simple_parser_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("basic members") = []
   {
    ascii::simple_parser parser("abc, def \t 123\n\n456"sv);

    ut::expect( ut::that % parser.got_data() and parser.got('a') );
    ut::expect( ut::that % parser.get_identifier()=="abc"sv );
    ut::expect( ut::that % parser.got_punct() and parser.eat(',') );
    parser.skip_blanks();
    ut::expect( ut::that % parser.got('d') );
    ut::expect( ut::that % parser.get_identifier()=="def"sv );
    ut::expect( ut::that % parser.got_blank() );
    parser.skip_blanks();
    ut::expect( ut::that % parser.got(ascii::is_digit<char>) );
    ut::expect( ut::that % parser.got('1') );
    ut::expect( ut::that % parser.get_digits()=="123"sv );
    ut::expect( ut::that % parser.got_endline() );
    parser.skip_any_space();
    ut::expect( ut::that % parser.got('4') );
    ut::expect( ut::that % parser.get_digits()=="456"sv );
    ut::expect( ut::that % not parser.got_data() );
   };

ut::test("inheriting") = []
   {

    class csv_parser_t final : public ascii::simple_parser
    {
     private:
        std::size_t i_element_start = 0;
        std::size_t i_element_end = 0;

     public:
        explicit csv_parser_t(const std::string_view buf) noexcept
          : ascii::simple_parser(buf)
           {}

        [[nodiscard]] bool got_element() noexcept
           {
            while( got_data() )
               {
                skip_any_space();
                if( got_element_token() )
                   {
                    return true;
                   }
                get_next();
               }
            return false;
           }

        [[nodiscard]] bool got_element_token() noexcept
           {
            i_element_start = pos();
            while( not got(ascii::is_space_or_any_of<','>) and get_next() ) ;
            i_element_end = pos();
            return i_element_end>i_element_start;
           }

        [[nodiscard]] std::string_view element() const noexcept { return input.substr(i_element_start, i_element_end-i_element_start); }
    };

    csv_parser_t parser("abc, def ghi , , ,\"lmn\", ,;123,"sv);
    int n = 0;
    while( parser.got_element() and n<100 )
       {
        switch( ++n )
           {
            case 1:
                ut::expect( ut::that % parser.element()=="abc"sv );
                break;

            case 2:
                ut::expect( ut::that % parser.element()=="def"sv );
                break;

            case 3:
                ut::expect( ut::that % parser.element()=="ghi"sv );
                break;

            case 4:
                ut::expect( ut::that % parser.element()=="\"lmn\""sv );
                break;

            case 5:
                ut::expect( ut::that % parser.element()==";123"sv );
                break;
           }
       }
    ut::expect( ut::that % n==5 );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
