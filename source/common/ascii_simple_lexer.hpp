#pragma once
//  ---------------------------------------------
//  Base class to parse strings
//  ---------------------------------------------
//  #include "ascii_simple_lexer.hpp" // ascii::simple_lexer
//  ---------------------------------------------
#include <concepts> // std::predicate
#include <string_view>

#include "ascii_predicates.hpp" // ascii::is_*

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace ascii //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
template<ascii::CharLike Chr =char>
class simple_lexer
{
 public:
    using string_view = std::basic_string_view<Chr>;
    const string_view input;
 private:
    std::size_t i = 0;

 public:
    constexpr explicit simple_lexer(const string_view buf) noexcept
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

    [[nodiscard]] constexpr bool got(const Chr ch) const noexcept { return i<input.size() and input[i]==ch; }
    [[nodiscard]] constexpr bool got_endline() const noexcept { return got('\n'); }
    [[nodiscard]] constexpr bool got_space() const noexcept { return i<input.size() and ascii::is_space(input[i]); }
    [[nodiscard]] constexpr bool got_blank() const noexcept { return i<input.size() and ascii::is_blank(input[i]); }
    [[nodiscard]] constexpr bool got_digit() const noexcept { return i<input.size() and ascii::is_digit(input[i]); }
    [[nodiscard]] constexpr bool got_alpha() const noexcept { return i<input.size() and ascii::is_alpha(input[i]); }
    [[nodiscard]] constexpr bool got_alnum() const noexcept { return i<input.size() and ascii::is_alnum(input[i]); }
    [[nodiscard]] constexpr bool got_punct() const noexcept { return i<input.size() and ascii::is_punct(input[i]); }
    [[nodiscard]] constexpr bool got_ident() const noexcept { return i<input.size() and ascii::is_ident(input[i]); }

    //template<Chr... CHS> [[nodiscard]] constexpr bool got_any_of() noexcept { return i<input.size() and ((input[i]==CHS) or ...); }
    //template<Chr... CHS> [[nodiscard]] constexpr bool got_none_of() noexcept { return i<input.size() and ((input[i]!=CHS) and ...); }

    [[maybe_unused]] constexpr bool eat(const Chr ch) noexcept { if(got(ch)){get_next(); return true;} return false; }

    constexpr void skip_any_space() noexcept { while( got_space() and get_next() ); }
    constexpr void skip_blanks() noexcept { while( got_blank() and get_next() ); }

    template<std::predicate<const Chr> CharPredicate =decltype(ascii::is_always_false<Chr>)>
    [[nodiscard]] constexpr bool got(CharPredicate is) const noexcept { return i<input.size() and is(input[i]); }

    template<std::predicate<const Chr> CharPredicate =decltype(ascii::is_always_false<Chr>)>
    constexpr void skip_while(CharPredicate is) noexcept
       { while( got(is) and get_next() ); }
       
    template<std::predicate<const Chr> CharPredicate =decltype(ascii::is_always_false<Chr>)>
    constexpr void skip_until(CharPredicate is) noexcept
       { while( not got(is) and get_next() ); }
       
    template<std::predicate<const Chr> CharPredicate =decltype(ascii::is_always_false<Chr>)>
    [[nodiscard]] constexpr string_view get_while(CharPredicate is) noexcept
       {
        const std::size_t i_start = pos();
        while( got(is) and get_next() );
        return {input.data()+i_start, pos()-i_start};
       }

    template<std::predicate<const Chr> CharPredicate =decltype(ascii::is_always_false<Chr>)>
    [[nodiscard]] constexpr string_view get_until(CharPredicate is) noexcept
       {
        const std::size_t i_start = pos();
        while( not got(is) and get_next() );
        return {input.data()+i_start, pos()-i_start};
       }
       
    [[nodiscard]] constexpr string_view get_alphabetic() noexcept { return get_while(ascii::is_alpha<Chr>); }
    [[nodiscard]] constexpr string_view get_alnums() noexcept { return get_while(ascii::is_alnum<Chr>); }
    [[nodiscard]] constexpr string_view get_identifier() noexcept { return get_while(ascii::is_ident<Chr>); }
    [[nodiscard]] constexpr string_view get_digits() noexcept { return get_while(ascii::is_digit<Chr>); }
    
    [[nodiscard]] constexpr string_view remaining() const noexcept { return input.substr(i); }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"ascii::simple_lexer"> simple_lexer_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("basic members") = []
   {
    ascii::simple_lexer lexer("abc, def \t 123\n\n456"sv);

    ut::expect( ut::that % lexer.got_data() and lexer.got('a') );
    ut::expect( ut::that % lexer.get_identifier()=="abc"sv );
    ut::expect( ut::that % lexer.got_punct() and lexer.eat(',') );
    lexer.skip_blanks();
    ut::expect( ut::that % lexer.got('d') );
    ut::expect( ut::that % lexer.get_identifier()=="def"sv );
    ut::expect( ut::that % lexer.got_blank() );
    lexer.skip_blanks();
    ut::expect( ut::that % lexer.got(ascii::is_digit) );
    ut::expect( ut::that % lexer.got('1') );
    ut::expect( ut::that % lexer.get_digits()=="123"sv );
    ut::expect( ut::that % lexer.got_endline() );
    lexer.skip_any_space();
    ut::expect( ut::that % lexer.got('4') );
    ut::expect( ut::that % lexer.get_digits()=="456"sv );
    ut::expect( ut::that % not lexer.got_data() );
   };

ut::test("inheriting") = []
   {

    class csv_lexer_t final : public ascii::simple_lexer<char>
    {
     private:
        std::size_t i_element_start = 0;
        std::size_t i_element_end = 0;

     public:
        explicit csv_lexer_t(const std::string_view buf) noexcept
          : ascii::simple_lexer<char>(buf)
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

    csv_lexer_t lexer("abc, def ghi , , ,\"lmn\", ,;123,"sv);
    int n = 0;
    while( lexer.got_element() and n<100 )
       {
        switch( ++n )
           {
            case 1:
                ut::expect( ut::that % lexer.element()=="abc"sv );
                break;

            case 2:
                ut::expect( ut::that % lexer.element()=="def"sv );
                break;

            case 3:
                ut::expect( ut::that % lexer.element()=="ghi"sv );
                break;

            case 4:
                ut::expect( ut::that % lexer.element()=="\"lmn\""sv );
                break;

            case 5:
                ut::expect( ut::that % lexer.element()==";123"sv );
                break;
           }
       }
    ut::expect( ut::that % n==5 );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
