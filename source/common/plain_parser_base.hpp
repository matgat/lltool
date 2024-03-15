#pragma once
//  ---------------------------------------------
//  Base class offering facilities to parse a
//  text buffer interpreting each element as a
//  single codepoint (fixed-length encoding)
//  allowing faster operations with no memory
//  allocations
//  ---------------------------------------------
//  #include "plain_parser_base.hpp" // plain::ParserBase
//  ---------------------------------------------
#include <cassert>
#include <concepts> // std::same_as<>, std::predicate<>, std::signed_integral
#include <limits> // std::numeric_limits<>
#include <type_traits> // std::make_unsigned_t<>
#include <format>

#include "parsers_common.hpp" // parse::error
#include "fnotify_type.hpp" // fnotify_t
#include "string_utilities.hpp" // str::escape()
#include "ascii_predicates.hpp" // ascii::is_*



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace plain
{

/////////////////////////////////////////////////////////////////////////////
template<ascii::CharLike Char =char>
class ParserBase
{
    using string_view = std::basic_string_view<Char>;

 public:
    struct context_t final
       {
        std::size_t line;
        std::size_t offset;
        Char curr_codepoint;
       };
    constexpr context_t save_context() const noexcept
       {
        return { m_line, m_offset, m_curr_codepoint };
       }
    constexpr void restore_context(const context_t& context) noexcept
       {
        m_line = context.line;
        m_offset = context.offset;
        m_curr_codepoint = context.curr_codepoint;
       }

 private:
    const string_view m_buf;
    std::size_t m_line = 1; // Current line number
    std::size_t m_offset = 0; // Index of current codepoint
    Char m_curr_codepoint = '\0'; // Current extracted codepoint
    fnotify_t m_on_notify_issue = default_notify;
    std::string m_file_path; // Just for create_parse_error()

 public:
    explicit constexpr ParserBase(const string_view buf)
      : m_buf{buf}
       {
        // See first codepoint
        if( m_offset<m_buf.size() )
           {
            m_curr_codepoint = m_buf[m_offset];
            check_and_skip_bom();
           }
       }

    // Prevent copy or move
    ParserBase(const ParserBase&) =delete;
    ParserBase& operator=(const ParserBase&) =delete;
    ParserBase(ParserBase&&) =delete;
    ParserBase& operator=(ParserBase&&) =delete;

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr std::size_t curr_line() const noexcept { return m_line; }
    [[nodiscard]] constexpr std::size_t curr_offset() const noexcept { return m_offset; }

    [[nodiscard]] constexpr string_view get_view_between(const std::size_t from_byte_pos, const std::size_t to_byte_pos) const noexcept
       {
        assert( from_byte_pos<=to_byte_pos );
        return m_buf.substr(from_byte_pos, to_byte_pos-from_byte_pos);
       }
    [[nodiscard]] constexpr string_view get_view_of_next(const std::size_t len) const noexcept
       {
        return m_buf.substr(curr_offset(), len);
       }

    //-----------------------------------------------------------------------
    static constexpr void default_notify(std::string&&) {}
    constexpr void set_on_notify_issue(fnotify_t const& f) { m_on_notify_issue = f; }
    constexpr void notify_issue(const std::string_view msg) const
       {
        m_on_notify_issue( std::format("[{}:{}] {}"sv, m_file_path, m_line, msg) ); // m_offset
       }
    constexpr void set_file_path(const std::string& pth)
       {
        m_file_path = pth;
       }
     [[nodiscard]] parse::error create_parse_error(std::string&& msg) const noexcept
       {
        return create_parse_error(std::move(msg), m_line);
       }
     [[nodiscard]] parse::error create_parse_error(std::string&& msg, const std::size_t ln_idx) const noexcept
       {
        return parse::error(std::move(msg), m_file_path.empty() ? "buffer"s : m_file_path, ln_idx);
       }

    //-----------------------------------------------------------------------
    // Extract next codepoint from buffer
    [[maybe_unused]] constexpr bool get_next() noexcept
       {
        if( ascii::is_endline(m_curr_codepoint) ) ++m_line;
        if( ++m_offset<m_buf.size() ) [[likely]]
           {
            m_curr_codepoint = m_buf[m_offset];
            return true;
           }
        m_curr_codepoint = static_cast<Char>('\0');
        m_offset = m_buf.size();
        return false;
       }
    [[nodiscard]] constexpr Char curr_codepoint() const noexcept
       {
        return m_curr_codepoint;
       }
    //-----------------------------------------------------------------------
    // Querying current codepoint
    [[nodiscard]] constexpr bool has_codepoint() const noexcept
       {
        return m_offset<m_buf.size();
       }
    [[nodiscard]] constexpr bool got(const Char cp) const noexcept
       {
        return curr_codepoint()==cp;
       }
    [[nodiscard]] constexpr bool got_endline() const noexcept
       {
        return ascii::is_endline(curr_codepoint());
       }
    [[nodiscard]] constexpr bool got_space() const noexcept
       {
        return ascii::is_space(curr_codepoint());
       }
    [[nodiscard]] constexpr bool got_blank() const noexcept
       {
        return ascii::is_blank(curr_codepoint());
       }
    [[nodiscard]] constexpr bool got_alpha() const  noexcept
       {
        return ascii::is_alpha(curr_codepoint());
       }
    [[nodiscard]] constexpr bool got_digit() const  noexcept
       {
        return ascii::is_digit(curr_codepoint());
       }
    [[nodiscard]] constexpr bool got_punct() const noexcept
       {
        return ascii::is_punct(curr_codepoint());
       }
    //-----------------------------------------------------------------------
    [[nodiscard]] bool constexpr got(const string_view sv) const noexcept
       {
        return sv==get_view_of_next(sv.size());
       }
    //-----------------------------------------------------------------------
    template<Char CH1, Char... CHS>
    [[nodiscard]] constexpr bool got_any_of() const noexcept
       {
        return ascii::is_any_of<CH1, CHS ...>(curr_codepoint());
       }
    //-----------------------------------------------------------------------
    template<std::predicate<const Char> CodepointPredicate =decltype(ascii::is_always_false<Char>)>
    [[nodiscard]] constexpr bool got(CodepointPredicate is) const noexcept
       {
        return is(curr_codepoint());
       }

    //-----------------------------------------------------------------------
    template<std::predicate<const Char> CodepointPredicate =decltype(ascii::is_always_false<Char>)>
    constexpr void skip_while(CodepointPredicate is) noexcept
       {
        while( is(curr_codepoint()) and get_next() );
       }

    //-----------------------------------------------------------------------
    template<std::predicate<const Char> CodepointPredicate =decltype(ascii::is_always_false<Char>)>
    constexpr void skip_until(CodepointPredicate is) noexcept
       {
        while( not is(curr_codepoint()) and get_next() );
       }

    template<std::predicate<const Char> CodepointPredicate =decltype(ascii::is_always_false<Char>)>
    [[nodiscard]] constexpr string_view get_while(CodepointPredicate is) noexcept
       {
        const std::size_t i_start = curr_offset();
        while( is(curr_codepoint()) and get_next() );
        return get_view_between(i_start, curr_offset());
       }

    //-----------------------------------------------------------------------
    //const auto bytes = parser.get_until(ascii::is_any_of<'=',':'>, ascii::is_endline);
    template<std::predicate<const Char> CodepointPredicate =decltype(ascii::is_always_false<Char>)>
    [[nodiscard]] constexpr string_view get_until(CodepointPredicate is_end, CodepointPredicate is_unexpected =ascii::is_always_false<Char>)
       {
        const auto start = save_context();
        while( not is_end(curr_codepoint()) )
           {
            if( is_unexpected(curr_codepoint()) )
               {
                const Char offending_codepoint = curr_codepoint();
                restore_context( start ); // Strong guarantee
                throw create_parse_error( std::format("Unexpected character '{}'"sv, str::escape(offending_codepoint)) );
               }
            else if( not get_next() )
               {// No more data
                if( is_end(curr_codepoint()) )
                   {// End predicate tolerates end of data
                    break;
                   }
                else
                   {
                    restore_context( start ); // Strong guarantee
                    throw create_parse_error( "Unexpected end (termination not found)" );
                   }
               }
           }
        return get_view_between(start.offset, curr_offset());
       }
    //-----------------------------------------------------------------------
    template<std::predicate<const Char> CodepointPredicate =decltype(ascii::is_always_false<Char>)>
    [[nodiscard]] constexpr string_view get_until_and_skip(CodepointPredicate is_end, CodepointPredicate is_unexpected =ascii::is_always_false<Char>)
       {
        string_view sv = get_until(is_end, is_unexpected);
        get_next(); // Skip termination codepoint
        return sv;
       }
    //-----------------------------------------------------------------------
    template<Char end_codepoint>
    [[nodiscard]] constexpr string_view get_until()
       {
        return get_until_and_skip(ascii::is<end_codepoint>, ascii::is_always_false<Char>);
       }
    //-----------------------------------------------------------------------
    template<Char end_codepoint>
    [[nodiscard]] constexpr string_view get_until_or_endline()
       {
        return get_until(ascii::is_any_of<end_codepoint,'\n','\0'>, ascii::is_always_false<Char>);
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr string_view get_until(const string_view sv)
       {
        const auto start = save_context();
        do {
            if( eat(sv) )
               {
                return get_view_between(start.offset, curr_offset()-sv.size());
               }
           }
        while( get_next() );
        restore_context( start ); // Strong guarantee
        throw create_parse_error(std::format("Unclosed content (\"{}\" not found)",sv), start.line);
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr string_view get_until_newline_token(const string_view tok)
       {
        const auto start = save_context();
        return get_until_newline_token(tok, start);
       }
    [[nodiscard]] constexpr string_view get_until_newline_token(const string_view tok, const context_t& start)
       {
        while( has_codepoint() ) [[likely]]
           {
            if( got_endline() )
               {
                get_next();
                skip_blanks();
                const std::size_t candidate_end = curr_offset();
                if( eat_token(tok) )
                   {
                    return get_view_between(start.offset, candidate_end);
                   }
               }
            else
               {
                get_next();
               }
           }
        restore_context( start ); // Strong guarantee
        throw create_parse_error(std::format("Unclosed content (\"{}\" not found)",tok), start.line);
       }

    constexpr void skip_blanks() noexcept { skip_while(ascii::is_blank<Char>); }
    constexpr void skip_any_space() noexcept { skip_while(ascii::is_space<Char>); }
    constexpr void skip_line() noexcept { skip_until(ascii::is_endline<Char>); get_next(); }
    [[nodiscard]] constexpr string_view get_rest_of_line() noexcept { return get_until_and_skip(ascii::is_any_of<Char('\n'),'\0'>); }
    [[nodiscard]] constexpr string_view get_until_space_or_end() noexcept { return get_until_and_skip(ascii::is_space_or_any_of<Char('\0')>); }
    [[nodiscard]] constexpr string_view get_notspace() noexcept { return get_until(ascii::is_space_or_any_of<Char('\0')>); }
    [[nodiscard]] constexpr string_view get_alphabetic() noexcept { return get_while(ascii::is_alpha<Char>); }
    [[nodiscard]] constexpr string_view get_alnums() noexcept { return get_while(ascii::is_alnum<Char>); }
    [[nodiscard]] constexpr string_view get_identifier() noexcept { return get_while(ascii::is_ident<Char>); }
    [[nodiscard]] constexpr string_view get_digits() noexcept { return get_while(ascii::is_digit<Char>); }
    [[nodiscard]] constexpr string_view get_float() noexcept { return get_while(ascii::is_float<Char>); }

    //-----------------------------------------------------------------------
    // Called when line is supposed to end
    constexpr void check_and_eat_endline()
       {
        if( got_endline() )
           {
            get_next();
           }
        else
           {
            throw create_parse_error( std::format("Unexpected content '{}' at line end"sv, str::escape(get_rest_of_line())) );
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr bool eat(const Char cp) noexcept
       {
        if( got(cp) )
           {
            get_next();
            return true;
           }
        return false;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr bool eat(const string_view sv) noexcept
       {
        assert( not sv.contains('\n') );
        if( sv==get_view_of_next(sv.size()) )
           {
            advance_of( sv.size() );
            return true;
           }
        return false;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr bool eat_token(const string_view sv) noexcept
       {
        assert( not sv.contains('\n') );
        if( sv==get_view_of_next(sv.size()) )
           {
            // It's a token if next char is not identifier
            const std::size_t i_next = m_offset + sv.size();
            if( i_next>=m_buf.size() or not ascii::is_ident(m_buf[i_next]) )
               {
                advance_of( sv.size() );
                return true;
               }
           }
        return false;
       }


    //-----------------------------------------------------------------------
    // Read a (base10) positive integer literal
    template<std::unsigned_integral Uint =std::size_t>
    [[nodiscard]] constexpr Uint extract_index()
       {
        if( not got_digit() )
           {
            throw create_parse_error( std::format("Invalid char '{}' in number literal"sv, str::escape(curr_codepoint())) );
           }

        Uint result = ascii::value_of_digit(curr_codepoint());
        constexpr Uint base = 10u;
        constexpr Uint overflow_limit = ((std::numeric_limits<Uint>::max() - (base - 1u)) / (2 * base)) - 1u; // A little conservative
        while( get_next() and got_digit() )
           {
            if( result>overflow_limit )
               {
                throw create_parse_error( std::format("Integer literal too big ({}x{} would be dangerously near {})", result, base, std::numeric_limits<Uint>::max()/2) );
               }
            result = static_cast<Uint>((base*result) + ascii::value_of_digit(curr_codepoint()));
           }
        return result;
       }


    //-----------------------------------------------------------------------
    // Read a (base10) integer literal
    template<std::signed_integral Int =int>
    [[nodiscard]] constexpr Int extract_integer()
       {
        Int sign = 1;
        if( got('+') )
           {
            if( not get_next() )
               {
                throw create_parse_error("Invalid integer '+'");
               }
           }
        else if( got('-') )
           {
            sign = -1;
            if( not get_next() )
               {
                throw create_parse_error("Invalid integer '-'");
               }
           }

        using Uint = std::make_unsigned_t<Int>;
        return sign * static_cast<Int>( extract_index<Uint>() );
       }

    //-----------------------------------------------------------------------
    // Read a (base10) float literal
    [[nodiscard]] constexpr double extract_float()
       {
        // [sign]
        double sign = 1.0;
        if( got('-') )
           {
            sign = -1.0;
            if( not get_next() )
               {
                throw create_parse_error("Invalid float '-'");
               }
           }
        else if( got('+') )
           {
            if( not get_next() )
               {
                throw create_parse_error("Invalid float '+'");
               }
           }

        // [mantissa - integer part]
        double mantissa = 0.0;
        if( got_digit() )
           {
            mantissa = ascii::value_of_digit(curr_codepoint());
            while( get_next() and got_digit() )
               {
                mantissa = (10.0 * mantissa) + ascii::value_of_digit(curr_codepoint());
               }
           }
        // [mantissa - fractional part]
        if( got('.') )
           {
            double k = 0.1; // shift of decimal part
            if( get_next() and got_digit() )
               {
                do {
                    mantissa += k * ascii::value_of_digit(curr_codepoint());
                    k *= 0.1;
                   }
                while( get_next() and got_digit() );
               }
           }

        // [exponent]
        int exp = 0;
        if( got_any_of<'E','e'>() )
           {
            get_next();
            try{
                exp = extract_integer<int>();
               }
            catch(...)
               {
                throw create_parse_error("Invalid exponent");
               }
           }

        const auto pow10 = [](const int n) noexcept -> double
           {
            double result = 1.0;
            if( n>=0 )
               {
                for(int i=n; i>0; --i) result *= 10.0;
               }
            else
               {
                for(int i=-n; i>0; --i) result *= 10.0;
                result = 1.0 / result;
               }
            return result;
           };

        return sign * mantissa * pow10( exp );
       }

 private:
    //-----------------------------------------------------------------------
    void constexpr advance_of(const std::size_t codepoints_num)
       {
        assert( codepoints_num>0 );
        m_offset += codepoints_num-1;
        // Note: assuming same m_line
        get_next();
       }

    //-----------------------------------------------------------------------
    void constexpr check_and_skip_bom()
       {
        if constexpr( std::same_as<Char, char32_t> )
           {
            if( got(U'\uFEFF') )
               {
                m_offset += 1;
               }
           }
        //else if constexpr( std::same_as<Char, char16_t> )
        //   {
        //    if( got(u"\x0000\xFEFF"sv) or got(u"\xFEFF\x0000"sv) )
        //       {
        //        throw std::runtime_error("utf-32 not supported, convert to utf-8");
        //       }
        //    else if( got(u'\xFEFF') )
        //       {
        //        m_offset += 1;
        //       }
        //   }
        else // char, unsigned char, char8_t
           {
            // +-----------+-------------+
            // | Encoding  |   Bytes     |
            // |-----------|-------------|
            // | utf-8     | EF BB BF    |
            // | utf-16-be | FE FF       |
            // | utf-16-le | FF FE       |
            // | utf-32-be | 00 00 FE FF |
            // | utf-32-le | FF FE 00 00 |
            // +-----------+-------------+
            if( got("\xFF\xFE\x00\x00"sv) or got("\x00\x00\xFE\xFF"sv) )
               {
                throw std::runtime_error("utf-32 not supported, convert to utf-8");
               }
            else if( got("\xFF\xFE"sv) or got("\xFE\xFF"sv) )
               {
                throw std::runtime_error("utf-16 not supported, convert to utf-8");
               }
            else if( got("\xEF\xBB\xBF"sv) )
               {
                m_offset += 3;
               }
           }
       }
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <cstdint> // std::uint16_t, ...
#include "ansi_escape_codes.hpp" // ANSI_RED, ...
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"plain::ParserBase"> plain_parser_base_tests = []
{////////////////////////////////////////////////////////////////////////////

//auto notify_sink = [](const std::string_view msg) -> void { ut::log << ANSI_BLUE "parser: " ANSI_DEFAULT << msg; };

ut::test("basic stuff") = []
   {
    plain::ParserBase<char> parser{"abc\ndef\n"sv};

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==0u );
    ut::expect( ut::that % parser.curr_line()==1u );
    ut::expect( ut::that % parser.curr_codepoint()=='a' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==1u );
    ut::expect( ut::that % parser.curr_line()==1u );
    ut::expect( ut::that % parser.curr_codepoint()=='b' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==2u );
    ut::expect( ut::that % parser.curr_line()==1u );
    ut::expect( ut::that % parser.curr_codepoint()=='c' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==3u );
    ut::expect( ut::that % parser.curr_line()==1u );
    ut::expect( ut::that % parser.curr_codepoint()=='\n' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==4u );
    ut::expect( ut::that % parser.curr_line()==2u );
    ut::expect( ut::that % parser.curr_codepoint()=='d' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==5u );
    ut::expect( ut::that % parser.curr_line()==2u );
    ut::expect( ut::that % parser.curr_codepoint()=='e' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==6u );
    ut::expect( ut::that % parser.curr_line()==2u );
    ut::expect( ut::that % parser.curr_codepoint()=='f' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==7u );
    ut::expect( ut::that % parser.curr_line()==2u );
    ut::expect( ut::that % parser.curr_codepoint()=='\n' );
    ut::expect( not parser.get_next() );

    ut::expect( not parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==8u );
    ut::expect( ut::that % parser.curr_line()==3u );
    ut::expect( ut::that % parser.curr_codepoint()=='\0' );
    ut::expect( not parser.get_next() );

    ut::expect( not parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==8u );
    ut::expect( ut::that % parser.curr_line()==3u );
    ut::expect( ut::that % parser.curr_codepoint()=='\0' );
   };


ut::test("get_view_of_next()") = []
   {
    plain::ParserBase<char> parser{"abc"sv};
    ut::expect( ut::that % parser.get_view_of_next(1)=="a"sv );
    ut::expect( ut::that % parser.get_view_of_next(2)=="ab"sv );
    ut::expect( ut::that % parser.get_view_of_next(3)=="abc"sv );
    ut::expect( ut::that % parser.get_view_of_next(4)=="abc"sv );
    ut::expect( ut::that % parser.get_view_of_next(100)=="abc"sv );
    ut::expect( parser.get_next() );
    ut::expect( ut::that % parser.get_view_of_next(1)=="b"sv );
    ut::expect( ut::that % parser.get_view_of_next(2)=="bc"sv );
    ut::expect( ut::that % parser.get_view_of_next(3)=="bc"sv );
    ut::expect( ut::that % parser.get_view_of_next(4)=="bc"sv );
    ut::expect( ut::that % parser.get_view_of_next(100)=="bc"sv );
    ut::expect( parser.get_next() );
    ut::expect( ut::that % parser.get_view_of_next(1)=="c"sv );
    ut::expect( ut::that % parser.get_view_of_next(2)=="c"sv );
    ut::expect( ut::that % parser.get_view_of_next(3)=="c"sv );
    ut::expect( ut::that % parser.get_view_of_next(4)=="c"sv );
    ut::expect( ut::that % parser.get_view_of_next(100)=="c"sv );
    ut::expect( not parser.get_next() );
    ut::expect( ut::that % parser.get_view_of_next(1)==""sv );
    ut::expect( ut::that % parser.get_view_of_next(2)==""sv );
    ut::expect( ut::that % parser.get_view_of_next(3)==""sv );
    ut::expect( ut::that % parser.get_view_of_next(4)==""sv );
    ut::expect( ut::that % parser.get_view_of_next(100)==""sv );
   };


ut::test("get_view_between()") = []
   {
    plain::ParserBase<char> parser{"abc"sv};
    ut::expect( ut::that % parser.get_view_between(0,1)=="a"sv );
    ut::expect( ut::that % parser.get_view_between(0,2)=="ab"sv );
    ut::expect( ut::that % parser.get_view_between(0,3)=="abc"sv );
    ut::expect( ut::that % parser.get_view_between(0,4)=="abc"sv );
    ut::expect( ut::that % parser.get_view_between(0,100)=="abc"sv );
    ut::expect( ut::that % parser.get_view_between(1,2)=="b"sv );
    ut::expect( ut::that % parser.get_view_between(1,100)=="bc"sv );
    ut::expect( ut::that % parser.get_view_between(2,3)=="c"sv );
    ut::expect( ut::that % parser.get_view_between(2,100)=="c"sv );
   };


ut::test("no data to parse") = []
   {
    ut::should("empty char buffer") = []
       {
        plain::ParserBase<char> parser{""sv};
        ut::expect( not parser.has_codepoint() );
       };

    ut::should("empty utf-8 buffer except bom") = []
       {
        plain::ParserBase<char> parser{"\xEF\xBB\xBF"sv};
        ut::expect( not parser.has_codepoint() );
       };

    //ut::should("empty char16_t buffer except bom") = []
    //   {
    //    plain::ParserBase<char16_t> parser{u"\xFEFF"sv};
    //    ut::expect( not parser.has_codepoint() );
    //   };

    ut::should("empty char32_t buffer except bom") = []
       {
        plain::ParserBase<char32_t> parser{U"\uFEFF"sv};
        ut::expect( not parser.has_codepoint() );
       };
   };


ut::test("rejected boms") = []
   {
    ut::expect( ut::throws([]{ [[maybe_unused]] plain::ParserBase<char> parser{"\x00\x00\xFE\xFF blah"sv}; }) ) << "char should reject utf-32-be\n";
    ut::expect( ut::throws([]{ [[maybe_unused]] plain::ParserBase<char> parser{"\xFF\xFE blah"sv}; }) ) << "char should reject utf-16-le\n";
    //ut::expect( ut::throws([]{ [[maybe_unused]] plain::ParserBase<char16_t> parser{u"\xFEFF\x0000 blah"sv}; }) ) << "char16_t should reject utf-32-be\n";
   };


ut::test("codepoint queries") = []
   {
    plain::ParserBase<char> parser{"a; 2\n"sv};

    ut::expect( parser.got('a') and parser.got(ascii::is_alpha) );
    ut::expect( parser.got("a"sv) );
    ut::expect( parser.got("a; 2"sv) );

    ut::expect( parser.get_next() );
    ut::expect( parser.got(';') and parser.got_punct() and parser.got(ascii::is_punct) );

    ut::expect( parser.get_next() );
    ut::expect( parser.got(' ') and parser.got_space() and parser.got(ascii::is_blank) );
    ut::expect( parser.got(" "sv) );
    ut::expect( parser.got(" 2"sv) );

    ut::expect( parser.get_next() );
    ut::expect( parser.got('2') and parser.got_digit() and parser.got(ascii::is_float) );
    ut::expect( parser.got("2\n"sv) );

    ut::expect( parser.get_next() );
    ut::expect( parser.got('\n') and parser.got_endline() and parser.got(ascii::is_space) );
   };


ut::test("skipping primitives") = []
   {
    plain::ParserBase<char> parser{"abc \t123d,e,f\nghi345"sv};

    parser.skip_while(ascii::is_digit);
    ut::expect( ut::that % parser.curr_codepoint()=='a' );

    parser.skip_while(ascii::is_alpha);
    ut::expect( ut::that % parser.curr_codepoint()==' ' );

    parser.skip_while(ascii::is_blank);
    ut::expect( ut::that % parser.curr_codepoint()=='1' );

    parser.skip_while(ascii::is_digit);
    ut::expect( ut::that % parser.curr_codepoint()=='d' );

    parser.skip_until(ascii::is_endline);
    ut::expect( ut::that % parser.curr_codepoint()=='\n' );

    parser.skip_until(ascii::is_digit);
    ut::expect( ut::that % parser.curr_codepoint()=='3' );

    parser.skip_until(ascii::is_digit);
    ut::expect( ut::that % parser.curr_codepoint()=='3' );

    parser.skip_until(ascii::is_alpha);
    ut::expect( not parser.has_codepoint() );
   };


ut::test("skipping functions") = []
   {
    plain::ParserBase<char> parser{" \t a \t b\n\t\t\n\nc d e f\ng"sv};

    parser.skip_blanks();
    ut::expect( ut::that % parser.curr_codepoint()=='a' );

    ut::expect( parser.get_next() );
    parser.skip_any_space();
    ut::expect( ut::that % parser.curr_codepoint()=='b' );

    ut::expect( parser.get_next() );
    parser.skip_any_space();
    ut::expect( ut::that % parser.curr_codepoint()=='c' );

    parser.skip_line();
    ut::expect( ut::that % parser.curr_codepoint()=='g' );

    parser.skip_line();
    ut::expect( not parser.has_codepoint() );

    parser.skip_line(); // skipping line at buffer end shouldn't be harmful
    ut::expect( not parser.has_codepoint() );
   };


ut::test("getting primitives") = []
   {
    plain::ParserBase<char> parser{"nam=val k2:v2 a3==b3"sv};

    ut::expect( ut::that % parser.get_while(ascii::is_ident)=="nam"sv );
    ut::expect( ut::that % parser.curr_codepoint()=='=' and parser.get_next() );
    ut::expect( ut::that % parser.get_while(ascii::is_alnum)=="val"sv );
    ut::expect( ut::that % parser.curr_codepoint()==' ' and parser.get_next() );

    ut::expect( ut::that % parser.get_until<':'>()=="k2"sv );
    ut::expect( ut::that % parser.get_until_and_skip(ascii::is_space)=="v2"sv );
    ut::expect( ut::that % parser.curr_codepoint()=='a' );

    ut::expect( ut::throws([&parser]{ [[maybe_unused]] auto sv = parser.get_until(ascii::is<';'>); }) ) << "should complain for termination not found\n";
    ut::expect( ut::that % parser.get_until(ascii::is_any_of<'\0',';'>)=="a3==b3"sv );
   };


ut::test("get_until(block)") = []
   {
    plain::ParserBase<char> parser{"**abc**\n**def***///ghi/*\n**lmn***/"sv};

    ut::expect( ut::throws([&parser]{ [[maybe_unused]] auto sv = parser.get_until("*@"sv); }) ) << "should complain for termination not found\n";
    ut::expect( ut::that % parser.get_until("*/"sv)=="**abc**\n**def**"sv );
    ut::expect( ut::that % parser.curr_line()==2u );
    ut::expect( ut::that % parser.get_until("*/"sv)=="//ghi/*\n**lmn**"sv );
    ut::expect( ut::that % parser.curr_line()==3u );
    ut::expect( not parser.has_codepoint() );
   };


ut::test("get_until_or_endline()") = []
   {
    plain::ParserBase<char> parser{"// [abc] def\n"
                                   "// [ghi lmn"sv};

    ut::expect( ut::that % parser.get_until_or_endline<']'>()=="// [abc"sv );
    ut::expect( ut::that % parser.curr_codepoint()==']' );
    ut::expect( ut::that % parser.curr_line()==1u );
    ut::expect( parser.get_next() );

    ut::expect( ut::that % parser.get_until_or_endline<']'>()==" def"sv );
    ut::expect( ut::that % parser.curr_codepoint()=='\n' );
    ut::expect( ut::that % parser.curr_line()==1u );
    ut::expect( parser.get_next() );

    ut::expect( ut::that % parser.get_until_or_endline<']'>()=="// [ghi lmn"sv );
    ut::expect( not parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_line()==2u );
   };


ut::test("getting functions") = []
   {
    plain::ParserBase<char> parser{"abc123 ... ---\n_id3:-2.3E5mm2 ..."sv};

    ut::expect( ut::that % parser.get_digits()==""sv );
    ut::expect( ut::that % parser.get_alphabetic()=="abc"sv );
    ut::expect( ut::that % parser.get_digits()=="123"sv );
    ut::expect( ut::that % parser.get_identifier()==""sv );
    ut::expect( ut::that % parser.get_notspace()==""sv );
    ut::expect( parser.get_next() );
    ut::expect( ut::that % parser.get_notspace()=="..."sv );
    ut::expect( ut::that % parser.get_rest_of_line()==" ---"sv );

    ut::expect( ut::that % parser.get_identifier()=="_id3"sv );
    ut::expect( parser.get_next() );
    ut::expect( ut::that % parser.get_float()=="-2.3E5"sv );
    ut::expect( ut::that % parser.get_alnums()=="mm2"sv );
    ut::expect( ut::that % parser.get_rest_of_line()==" ..."sv );
   };


ut::test("get_until_newline_token()") = []
   {
    plain::ParserBase<char> parser{ "start\n"
                                    "123\n"
                                    "endnot\n"
                                    "\n"
                                    "  end start2\n"
                                    "not end\n"
                                    "end"sv };

    ut::expect( ut::throws([&parser] { [[maybe_unused]] auto n = parser.get_until_newline_token("xxx"sv); }) ) << "should complain for unclosed content\n";

    ut::expect( ut::that % parser.get_until_newline_token("end"sv) == "start\n123\nendnot\n\n  "sv );
    ut::expect( ut::that % parser.curr_line()==5u );

    ut::expect( ut::that % parser.get_until_newline_token("end"sv) == " start2\nnot end\n"sv );
    ut::expect( ut::that % parser.curr_line()==7u );
    ut::expect( not parser.has_codepoint() );
   };


ut::test("endline functions") = []
   {
    plain::ParserBase<char> parser{"1  \n2  \n3  \n"sv};

    ut::expect( ut::throws([&parser]{ parser.check_and_eat_endline(); }) ) << "should complain for line not ended\n";

    ut::expect( ut::that % parser.curr_codepoint()=='2' ) << "previous line was collected\n";
    ut::expect( ut::that % parser.curr_line()==2u );
    ut::expect( not parser.got_endline() );
    ut::expect( parser.get_next() );
    ut::expect( not parser.got_endline() ) << "there are still spaces here\n";
    parser.skip_blanks();
    ut::expect( parser.got_endline() and parser.get_next() );

    ut::expect( ut::that % parser.curr_codepoint()=='3' );
    ut::expect( ut::that % parser.curr_line()==3u );
    ut::expect( parser.get_next() );
    parser.skip_blanks();
    parser.check_and_eat_endline();
    ut::expect( not parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_line()==4u );
   };


ut::test("eat functions") = []
   {
    plain::ParserBase<char> parser{"abcd 1234 efgh"sv};

    ut::expect( not parser.eat('b') );
    ut::expect( ut::that % parser.curr_codepoint()=='a' );

    ut::expect( not parser.eat("bc"sv) );
    ut::expect( ut::that % parser.curr_codepoint()=='a' );

    ut::expect( not parser.eat_token("ab"sv) );
    ut::expect( ut::that % parser.curr_codepoint()=='a' );

    ut::expect( parser.eat('a') );
    ut::expect( ut::that % parser.curr_codepoint()=='b' );

    ut::expect( parser.eat("bc"sv) );
    ut::expect( ut::that % parser.curr_codepoint()=='d' );

    ut::expect( parser.eat("d "sv) );
    ut::expect( ut::that % parser.curr_codepoint()=='1' );

    ut::expect( not parser.eat_token("123"sv) );
    ut::expect( ut::that % parser.curr_codepoint()=='1' );

    ut::expect( parser.eat_token("1234"sv) );
    ut::expect( ut::that % parser.curr_codepoint()==' ' );

    ut::expect( parser.eat(' ') );
    ut::expect( ut::that % parser.curr_codepoint()=='e' );

    ut::expect( not parser.eat_token("efghi"sv) );
    ut::expect( ut::that % parser.curr_codepoint()=='e' );

    ut::expect( parser.eat_token("efgh"sv) );
    ut::expect( not parser.has_codepoint() );
   };


ut::test("parsing numbers") = []
   {
    ut::test("1234") = []
       {
        plain::ParserBase parser{"1234"sv};
        ut::expect( ut::that % parser.extract_index() == 1234u );
       };

    ut::test("+2.3E-2") = []
       {
        plain::ParserBase parser{"+2.3E-2"sv};
        ut::expect( ut::that % parser.extract_float() == 2.3E-2 );
       };

    ut::test("huge double") = []
       {
        const double huge = std::numeric_limits<double>::max() / 10.0;
        const std::string huge_str = std::format("{:L}",huge);
        plain::ParserBase<char> parser{huge_str};
        ut::expect( ut::approx(parser.extract_float(), huge, huge*std::numeric_limits<double>::epsilon()) );
       };

    ut::test("-10300") = []
       {
        plain::ParserBase parser{"-10300"sv};
        ut::expect( ut::throws([&parser]{ [[maybe_unused]] auto n = parser.extract_index(); }) ) << "should complain for negative index\n";
        ut::expect( ut::that % parser.extract_integer() == -10'300 );
       };

    ut::test("65536 as std::uint32_t") = []
       {
        plain::ParserBase parser{"65536"sv};
        ut::expect( ut::that % parser.extract_index<std::uint32_t>() == 65'536u );
       };

    ut::test("65536 as std::uint16_t") = []
       {
        plain::ParserBase parser{"65536"sv};
        ut::expect( ut::throws([&parser]{ [[maybe_unused]] auto n = parser.extract_index<std::uint16_t>(); }) ) << "should complain for std::uint16_t literal overflow\n";
       };

    ut::test("32768 as std::int16_t") = []
       {
        plain::ParserBase parser{"32768"sv};
        ut::expect( ut::throws([&parser]{ [[maybe_unused]] auto n = parser.extract_integer<std::int16_t>(); }) ) << "should complain for std::int16_t literal overflow\n";
       };
   };


ut::test("parsing key:value entries") = []
   {
    const std::string_view buf =
        "    name: test\n"
        "    descr: a sample (parser test)\n"
        "    version : 0.5.0\n"
        "    test\n"
        "    \n"
        "    dependencies: parsers_common.hpp, fnotify_type.hpp, ascii_predicates.hpp\n"sv;
    plain::ParserBase<char> parser{buf};
    struct keyvalue_t final
       {
        std::string_view key;
        std::string_view value;

        bool get_next( plain::ParserBase<char>& parser )
           {
            while( true )
               {
                parser.skip_any_space();
                key = parser.get_identifier();
                if( not key.empty() )
                   {
                    parser.skip_blanks();
                    if( parser.got_any_of<':','='>() )
                       {
                        parser.get_next();
                        parser.skip_blanks();
                        value = parser.get_rest_of_line();
                        if( not value.empty() )
                           {
                            return true;
                           }
                       }
                   }
                else
                   {
                    return false;
                   }
               }
           }
       } entry;

    std::size_t n_event = 0u;
    while( entry.get_next(parser) )
       {
        switch( ++n_event )
           {
            case  1:
                ut::expect( ut::that % entry.key=="name"sv );
                ut::expect( ut::that % entry.value=="test"sv );
                break;

            case  2:
                ut::expect( ut::that % entry.key=="descr"sv );
                ut::expect( ut::that % entry.value=="a sample (parser test)"sv );
                break;

            case  3:
                ut::expect( ut::that % entry.key=="version"sv );
                ut::expect( ut::that % entry.value=="0.5.0"sv );
                break;

            case  4:
                ut::expect( ut::that % entry.key=="dependencies"sv );
                ut::expect( ut::that % entry.value=="parsers_common.hpp, fnotify_type.hpp, ascii_predicates.hpp"sv );
                break;

            default:
                ut::expect(false) << "unexpected key:\"" << entry.key << "\"\n";
           }
       }
    ut::expect( ut::that % n_event==4u ) << "events number should match";
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
