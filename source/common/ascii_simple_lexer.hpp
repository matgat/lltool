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
    using string_view = std::basic_string_view<Chr>;
 public:
    const string_view input;
 private:
    std::size_t m_pos;
    Chr m_current;

 public:
    constexpr explicit simple_lexer(const string_view buf) noexcept
      : input(buf)
      , m_pos(0u)
      , m_current( buf.empty() ? static_cast<Chr>('\0') : buf[0u] )
       {}

    [[nodiscard]] constexpr std::size_t pos() const noexcept { return m_pos; }
    [[nodiscard]] constexpr Chr current_char() const noexcept { return m_current; }
    [[nodiscard]] constexpr bool got_data() const noexcept { return m_pos<input.size(); }
    [[maybe_unused]] constexpr bool get_next() noexcept
       {
        if( ++m_pos<input.size() ) [[likely]]
           {
            m_current = input[m_pos];
            return true;
           }
        m_current = static_cast<Chr>('\0');
        m_pos = input.size();
        return false;
       }

    template<std::predicate<const Chr> CharPredicate =decltype(ascii::is_always_false<Chr>)>
    [[nodiscard]] constexpr bool got(CharPredicate is) const noexcept { return m_pos<input.size() and is(input[m_pos]); }

    template<std::predicate<const Chr> CharPredicate =decltype(ascii::is_always_false<Chr>)>
    constexpr void skip_while(CharPredicate is) noexcept
       { while( got(is) and get_next() ); }

    template<std::predicate<const Chr> CharPredicate =decltype(ascii::is_always_false<Chr>)>
    constexpr void skip_until(CharPredicate is) noexcept
       { while( not got(is) and get_next() ); }

    template<std::predicate<const Chr> CharPredicate =decltype(ascii::is_always_false<Chr>)>
    [[nodiscard]] constexpr string_view get_while(CharPredicate is) noexcept
       {
        const std::size_t pos_start = pos();
        while( got(is) and get_next() );
        return {input.data()+pos_start, pos()-pos_start};
       }

    template<std::predicate<const Chr> CharPredicate =decltype(ascii::is_always_false<Chr>)>
    [[nodiscard]] constexpr string_view get_until(CharPredicate is) noexcept
       {
        const std::size_t pos_start = pos();
        while( not got(is) and get_next() );
        return {input.data()+pos_start, pos()-pos_start};
       }

    [[nodiscard]] constexpr bool got(const Chr ch) const noexcept { return m_current==ch; }
    [[nodiscard]] constexpr bool got_endline() const noexcept { return got('\n'); }
    [[nodiscard]] constexpr bool got_space() const noexcept { return ascii::is_space(m_current); }
    [[nodiscard]] constexpr bool got_blank() const noexcept { return ascii::is_blank(m_current); }
    [[nodiscard]] constexpr bool got_digit() const noexcept { return ascii::is_digit(m_current); }
    [[nodiscard]] constexpr bool got_alpha() const noexcept { return ascii::is_alpha(m_current); }
    [[nodiscard]] constexpr bool got_alnum() const noexcept { return ascii::is_alnum(m_current); }
    [[nodiscard]] constexpr bool got_punct() const noexcept { return ascii::is_punct(m_current); }
    [[nodiscard]] constexpr bool got_ident() const noexcept { return ascii::is_ident(m_current); }

    [[nodiscard]] constexpr string_view get_alphas() noexcept { return get_while(ascii::is_alpha<Chr>); }
    [[nodiscard]] constexpr string_view get_alnums() noexcept { return get_while(ascii::is_alnum<Chr>); }
    [[nodiscard]] constexpr string_view get_identifier() noexcept { return get_while(ascii::is_ident<Chr>); }
    [[nodiscard]] constexpr string_view get_digits() noexcept { return get_while(ascii::is_digit<Chr>); }

    constexpr void skip_any_space() noexcept { while( got_space() and get_next() ); }
    constexpr void skip_blanks() noexcept { while( got_blank() and get_next() ); }
    constexpr void skip_nonalnums() noexcept { while( not got_alnum() and get_next() ); }
    constexpr void skip_nonalphas() noexcept { while( not got_alpha() and get_next() ); }
    constexpr void skip_nondigits() noexcept { while( not got_digit() and get_next() ); }

    [[maybe_unused]] constexpr bool eat(const Chr ch) noexcept { if(got(ch)){get_next(); return true;} return false; }
    [[nodiscard]] constexpr string_view remaining() const noexcept { return input.substr(m_pos); }
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

    ut::expect( lexer.got_data() and lexer.got('a') );
    ut::expect( ut::that % lexer.get_identifier()=="abc"sv );
    ut::expect( lexer.got_punct() and lexer.eat(',') );
    lexer.skip_blanks();
    ut::expect( lexer.got('d') );
    ut::expect( ut::that % lexer.get_identifier()=="def"sv );
    ut::expect( lexer.got_blank() );
    lexer.skip_blanks();
    ut::expect( lexer.got('1') );
    ut::expect( ut::that % lexer.get_digits()=="123"sv );
    ut::expect( lexer.got_endline() );
    lexer.skip_any_space();
    ut::expect( lexer.got('4') );
    ut::expect( ut::that % lexer.get_digits()=="456"sv );
    ut::expect( not lexer.got_data() );
   };

ut::test("predicates") = []
   {
    ascii::simple_lexer lexer(" /**/ prfx42 \tdef=0"sv);
    ut::expect( lexer.got(ascii::is<' '>) );
    lexer.skip_until(ascii::is_alnum);
    ut::expect( lexer.got(ascii::is<'p'>) );
    ut::expect( ut::that % lexer.get_while(ascii::is_alpha)=="prfx"sv );
    ut::expect( lexer.got(ascii::is<'4'>) );
    ut::expect( ut::that % lexer.get_until(ascii::is_space)=="42"sv );
    ut::expect( lexer.got(ascii::is<' '>) );
    lexer.skip_while(ascii::is_space);
    ut::expect( lexer.got(ascii::is<'d'>) );
    ut::expect( ut::that % lexer.get_until(ascii::is_punct)=="def"sv );
    ut::expect( lexer.got(ascii::is<'='>) );
    lexer.skip_while(ascii::is_punct);
    ut::expect( lexer.got(ascii::is<'0'>) );
    ut::expect( ut::that % lexer.get_while(ascii::is_digit)=="0"sv );
    ut::expect( not lexer.got_data() );
   };

ut::test("inheriting") = []
   {

    class csv_lexer_t final : public ascii::simple_lexer<char>
    {
     private:
        std::string_view m_elem;

     public:
        explicit csv_lexer_t(const std::string_view buf) noexcept
          : ascii::simple_lexer<char>(buf)
           {}

        [[nodiscard]] bool got_element() noexcept
           {
            while( got_data() )
               {
                skip_any_space();
                if( m_elem = get_until(ascii::is_space_or_any_of<','>);
                    not m_elem.empty() )
                   {
                    return true;
                   }
                get_next();
               }
            return false;
           }

        [[nodiscard]] std::string_view element() const noexcept { return m_elem; }
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
