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
#include <concepts> // std::same_as<>, std::predicate<>

#include <fmt/format.h> // fmt::format

#include "parsers_common.hpp" // parse::error
#include "fnotify_type.hpp" // fnotify_t
#include "string_utilities.hpp" // str::escape
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

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr std::size_t curr_line() const noexcept { return m_line; }
    [[nodiscard]] constexpr std::size_t curr_offset() const noexcept { return m_offset; }

    [[nodiscard]] constexpr string_view get_view_between(const std::size_t from_byte_pos, const std::size_t to_byte_pos) const noexcept
       {
        assert( from_byte_pos<=to_byte_pos and to_byte_pos<=m_byte_buf.size() );
        return m_buf.substr(from_byte_pos, to_byte_pos-from_byte_pos);
       }
    [[nodiscard]] constexpr string_view get_view_of_next(const std::size_t len) const noexcept
       {
        return m_buf.substr(curr_offset(), len);
       }

    //-----------------------------------------------------------------------
    static constexpr void default_notify(std::string&&) {} // { fmt::print(fmt::runtime(msg)); }
    constexpr void set_on_notify_issue(fnotify_t const& f) { m_on_notify_issue = f; }
    constexpr void notify_issue(const std::string_view msg) const
       {
        //fmt::print("  ! {}\n"sv, msg);
        m_on_notify_issue( fmt::format("{} (line {} offset {})"sv, msg, m_line, m_offset) );
       }
    constexpr void set_file_path(const std::string& pth)
       {
        m_file_path = pth;
       }
     [[nodiscard]] parse::error create_parse_error(std::string&& msg) const noexcept
       {
        return create_parse_error(std::move(msg), m_line);
       }
     [[nodiscard]] parse::error create_parse_error(std::string&& msg, const std::size_t l) const noexcept
       {
        if( m_file_path.empty() )
           {// I'm probably parsing a buffer
            return parse::error(std::move(msg), "buffer", l);
           }
        return parse::error(std::move(msg), m_file_path, l);
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
    [[nodiscard]] constexpr bool got_digit() const  noexcept
       {
        return ascii::is_digit(curr_codepoint());
       }
    [[nodiscard]] constexpr bool got_punct() const noexcept
       {
        return ascii::is_punct(curr_codepoint());
       }
    //-----------------------------------------------------------------------
    [[nodiscard]] bool constexpr got(const string_view sv) noexcept
       {
        return sv==get_view_of_next(sv.length());
       }
    //-----------------------------------------------------------------------
    template<Char CH1, Char... CHS>
    [[nodiscard]] constexpr bool got_any_of() noexcept
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
        do {
            if( is_end(curr_codepoint()) ) [[unlikely]]
               {
                break;
               }
            else if( is_unexpected(curr_codepoint()) ) [[unlikely]]
               {
                const Char offending_codepoint = curr_codepoint();
                restore_context( start ); // Strong guarantee
                throw create_parse_error( fmt::format("Unexpected character '{}'"sv, str::escape(offending_codepoint)) );
               }
            else if( not get_next() ) [[unlikely]]
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
        while( true );

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
    //const auto bytes = parser.get_bytes_until<'*','/'>();
    template<Char end_seq1, Char end_seq2, Char... end_seqtail>
    [[nodiscard]] constexpr string_view get_until()
       {
        using end_block_arr_t = std::array<Char, 2+sizeof...(end_seqtail)>;
        static constexpr end_block_arr_t end_block_arr{end_seq1, end_seq2, end_seqtail...};
        constexpr string_view end_block(end_block_arr.data(), end_block_arr.size());
        using preceding_match_t = std::array<bool, end_block_arr.size()-1u>;
        static constexpr preceding_match_t preceding_match = [](const end_block_arr_t& end_blk) constexpr
           {
            preceding_match_t a;
            a[0] = true;
            for(std::size_t i=1; i<a.size(); ++i)
               {
                a[i] = a[i-1] and end_blk[i-1]==end_blk[i];
               }
            return a;
           }(end_block_arr);


        const auto start = save_context();
        std::size_t content_end_offset = start.offset;
        std::size_t i = 0; // Matching codepoint index
        do {
            if( got(end_block[i]) )
               {// Matches a codepoint in end_block
                // If it's the first of end_block...
                if( i==0 )
                   {// ...Store the offset of the possible end of collected content
                    content_end_offset = m_offset;
                   }
                // If it's the last of end_block...
                if( ++i>=end_block.size() )
                   {// ...I'm done
                    get_next(); // Skip last end_block codepoint
                    return get_view_between(start.offset, content_end_offset);
                   }
               }
            else if( i>0 )
               {// Last didn't match and there was a partial match
                // Check what of the partial match can be salvaged
                do {
                    if( got(end_block[--i]) )
                       {// Last matches a codepoint in end block
                        if( preceding_match[i] )
                           {
                            content_end_offset = m_offset - i;
                            ++i;
                            break;
                           }
                       }
                   }
                while( i>0 );
               }
           }
        while( get_next() ); [[likely]]

        restore_context( start ); // Strong guarantee
        throw create_parse_error(fmt::format("Unclosed content (\"{}\" not found)",str::escape(end_block)), start.line);
       }

    constexpr void skip_blanks() noexcept { skip_while(ascii::is_blank<Char>); }
    constexpr void skip_any_space() noexcept { skip_while(ascii::is_space<Char>); }
    constexpr void skip_line() noexcept { skip_until(ascii::is_endline<Char>); get_next(); }
    [[nodiscard]] constexpr string_view get_rest_of_line() noexcept { return get_until_and_skip(ascii::is_any_of<Char('\n'),'\0'>); }
    [[nodiscard]] constexpr string_view get_until_space_or_end() noexcept { return get_until_and_skip(ascii::is_space_or_any_of<Char('\0')>); }
    [[nodiscard]] constexpr string_view get_alphabetic() noexcept { return get_while(ascii::is_alpha<Char>); }
    [[nodiscard]] constexpr string_view get_alnums() noexcept { return get_while(ascii::is_alnum<Char>); }
    [[nodiscard]] constexpr string_view get_identifier() noexcept { return get_while(ascii::is_ident<Char>); }
    [[nodiscard]] constexpr string_view get_digits() noexcept { return get_while(ascii::is_digit<Char>); }
    [[nodiscard]] constexpr string_view get_float() noexcept { return get_while(ascii::is_float<Char>); }

    //-----------------------------------------------------------------------
    // Called when line is supposed to end: nothing more than spaces allowed
    constexpr void skip_endline()
       {
        skip_blanks();
        if( got_endline() )
           {
            get_next();
           }
        else
           {
            throw create_parse_error( fmt::format("Unexpected content '{}' at line end"sv, str::escape(get_rest_of_line())) );
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
        if( sv==get_view_of_next(sv.length()) )
           {
            advance_of( sv.length() );
            return true;
           }
        return false;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr bool eat_token(const string_view sv) noexcept
       {
        assert( not sv.contains('\n') );
        if( sv==get_view_of_next(sv.length()) )
           {
            // It's a token if next char is not identifier
            const std::size_t i_next = m_offset + sv.length();
            if( i_next>=m_buf.size() or not ascii::is_ident(m_buf[i_next]) )
               {
                advance_of( sv.length() );
                return true;
               }
           }
        return false;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr string_view get_until_newline_token(const string_view tok)
       {
        const auto start = save_context();
        do {
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
           }
        while( get_next() );
        restore_context( start ); // Strong guarantee
        throw create_parse_error(fmt::format("Unclosed content (\"{}\" not found)",tok), start.line);
       }

    //-----------------------------------------------------------------------
    // Read a (base10) positive integer literal
    [[nodiscard]] constexpr std::size_t extract_index()
       {
        if( not got_digit() )
           {
            throw create_parse_error( fmt::format("Invalid char '{}' in index"sv, utxt::to_utf8(curr_codepoint())) );
           }

        std::size_t result = (curr_codepoint() - '0');
        constexpr std::size_t base = 10u;
        while( get_next() and got_digit() )
           {
            //assert( result < (std::numeric_limits<std::size_t>::max - (curr_codepoint()-'0')) / base ); // Check overflows
            result = (base*result) + (curr_codepoint() - '0');
           }
        return result;
       }


    //-----------------------------------------------------------------------
    // Read a (base10) integer literal
    [[nodiscard]] constexpr int extract_integer()
       {
        int sign = 1;
        if( got('+') )
           {
            if( not get_next() )
               {
                throw create_parse_error("Invalid integer \'+\'");
               }
           }
        else if( got('-') )
           {
            sign = -1;
            if( not get_next() )
               {
                throw create_parse_error("Invalid integer \'+\'");
               }
           }

        if( not got_digit() )
           {
            throw create_parse_error(fmt::format("Invalid char \'{}\' in integer", curr_codepoint()));
           }
        int result = (curr_codepoint() - '0');
        const int base = 10;
        while( get_next() and got_digit() )
           {
            result = (base*result) + (curr_codepoint() - '0');
           }
        return sign * result;
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
                throw create_parse_error("Invalid float \'-\'");
               }
           }
        else if( got('+') )
           {
            if( not get_next() )
               {
                throw create_parse_error("Invalid float \'+\'");
               }
           }

        // [mantissa - integer part]
        double mantissa = 0.0;
        if( got_digit() )
           {
            mantissa = (curr_codepoint() - '0');
            while( get_next() && got_digit() )
               {
                mantissa = (10.0 * mantissa) + (curr_codepoint() - '0');
               }
           }
        // [mantissa - fractional part]
        if( got('.') )
           {
            double k = 0.1; // shift of decimal part
            if( get_next() && got_digit() )
               {
                do {
                    mantissa += k * (curr_codepoint() - '0');
                    k *= 0.1;
                   }
                while( get_next() and got_digit() );
               }
           }

        // [exponent]
        int exp = 0;
        if( got_any_of<'E','e'>() )
           {
            int exp_sign = 1;
            if( get_next() )
               {
                // [exponent sign]
                if( got('-') )
                   {
                    exp_sign = -1;
                    if( not get_next() )
                       {
                        throw create_parse_error("Invalid float \'...E-\'");
                       }
                   }
                else if( got('+') )
                   {
                    if( not get_next() )
                       {
                        throw create_parse_error("Invalid float \'...E+\'");
                       }
                   }

                // [exponent value]
                while( got_digit() )
                   {
                    exp = (10 * exp) + static_cast<int>((curr_codepoint() - '0'));
                    get_next();
                   }
                exp *= exp_sign;
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
        if constexpr(std::same_as<Char, char32_t>)
           {
            if( got(U'\uFEFF') )
               {
                m_offset += 1;
               }
           }
        //else if constexpr(std::same_as<Char, char16_t>)
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
static ut::suite<"plain::ParserBase"> plain_parser_base_tests = []
{////////////////////////////////////////////////////////////////////////////

auto notify_sink = [](const std::string_view msg) -> void { ut::log << "\033[33m" "parser: " "\033[0m" << msg; };

ut::test("basic stuff") = []
   {
    plain::ParserBase<char> parser{"abc\ndef\n"sv};

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==0 );
    ut::expect( ut::that % parser.curr_line()==1 );
    ut::expect( ut::that % parser.curr_codepoint()=='a' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==1 );
    ut::expect( ut::that % parser.curr_line()==1 );
    ut::expect( ut::that % parser.curr_codepoint()=='b' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==2 );
    ut::expect( ut::that % parser.curr_line()==1 );
    ut::expect( ut::that % parser.curr_codepoint()=='c' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==3 );
    ut::expect( ut::that % parser.curr_line()==1 );
    ut::expect( ut::that % parser.curr_codepoint()=='\n' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==4 );
    ut::expect( ut::that % parser.curr_line()==2 );
    ut::expect( ut::that % parser.curr_codepoint()=='d' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==5 );
    ut::expect( ut::that % parser.curr_line()==2 );
    ut::expect( ut::that % parser.curr_codepoint()=='e' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==6 );
    ut::expect( ut::that % parser.curr_line()==2 );
    ut::expect( ut::that % parser.curr_codepoint()=='f' );
    ut::expect( parser.get_next() );

    ut::expect( parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==7 );
    ut::expect( ut::that % parser.curr_line()==2 );
    ut::expect( ut::that % parser.curr_codepoint()=='\n' );
    ut::expect( not parser.get_next() );

    ut::expect( not parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==8 );
    ut::expect( ut::that % parser.curr_line()==3 );
    ut::expect( ut::that % parser.curr_codepoint()=='\0' );
    ut::expect( not parser.get_next() );

    ut::expect( not parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_offset()==8 );
    ut::expect( ut::that % parser.curr_line()==3 );
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

ut::test("getting block") = []
   {
    plain::ParserBase<char> parser{"**abc**\n**def***///ghi/*\n**lmn***/"sv};

    ut::expect( ut::throws([&parser]{ [[maybe_unused]] auto sv = parser.get_until<'*','@'>(); }) ) << "should complain for termination not found\n";
    ut::expect( ut::that % parser.get_until<'*','/'>()=="**abc**\n**def**"sv );
    ut::expect( ut::that % parser.curr_line()==2 );
    ut::expect( ut::that % parser.get_until<'*','/'>()=="//ghi/*\n**lmn**"sv );
    ut::expect( ut::that % parser.curr_line()==3 );
    ut::expect( not parser.has_codepoint() );
   };

ut::test("getting functions") = []
   {
    plain::ParserBase<char> parser{"abc123 ...\n_id3:-2.3E5mm2 ..."sv};

    ut::expect( ut::that % parser.get_alphabetic()=="abc"sv );
    ut::expect( ut::that % parser.get_digits()=="123"sv );
    ut::expect( ut::that % parser.get_rest_of_line()==" ..."sv );

    ut::expect( ut::that % parser.get_identifier()=="_id3"sv );
    ut::expect( parser.get_next() );
    ut::expect( ut::that % parser.get_float()=="-2.3E5"sv );
    ut::expect( ut::that % parser.get_alnums()=="mm2"sv );
    ut::expect( ut::that % parser.get_rest_of_line()==" ..."sv );
   };

ut::test("end line functions") = []
   {
    plain::ParserBase<char> parser{"1  \n2  \n3  \n"sv};

    ut::expect( ut::throws([&parser]{ parser.skip_endline(); }) ) << "should complain for line not ended\n";

    ut::expect( ut::that % parser.curr_codepoint()=='2' ) << "previous line was collected\n";
    ut::expect( ut::that % parser.curr_line()==2 );
    ut::expect( not parser.got_endline() );
    ut::expect( parser.get_next() );
    ut::expect( not parser.got_endline() ) << "there are still spaces here\n";
    parser.skip_blanks();
    ut::expect( parser.got_endline() and parser.get_next() );

    ut::expect( ut::that % parser.curr_codepoint()=='3' );
    ut::expect( ut::that % parser.curr_line()==3 );
    ut::expect( parser.get_next() );
    parser.skip_endline();
    ut::expect( not parser.has_codepoint() );
    ut::expect( ut::that % parser.curr_line()==4 );
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

ut::test("get_until_newline_token()") = []
   {
    plain::ParserBase<char> parser{ "start\n"
                                    "123\n"
                                    "endnot\n"
                                    "  end start2\n"
                                    "not end\n"
                                    "end"sv };

    ut::expect( ut::throws([&parser]{ [[maybe_unused]] auto n = parser.get_until_newline_token("xxx"sv); }) ) << "should complain for unclosed content\n";
    ut::expect( ut::that % parser.get_until_newline_token("end"sv) == "start\n123\nendnot\n  "sv );
    ut::expect( ut::that % parser.curr_line()==4 );
    ut::expect( ut::that % parser.get_until_newline_token("end"sv) == " start2\nnot end\n"sv );
    ut::expect( ut::that % parser.curr_line()==6 );
    ut::expect( not parser.has_codepoint() );
   };

ut::test("parsing numbers") = []
   {
    plain::ParserBase<char> parser{"1234 -10300 +2.3E-2"sv};

    ut::expect( ut::that % parser.extract_index() == 1234u );
    parser.skip_blanks();

    ut::expect( ut::throws([&parser]{ [[maybe_unused]] auto n = parser.extract_index(); }) ) << "index has no sign\n";
    ut::expect( ut::that % parser.extract_integer() == -10'300 );

    parser.skip_blanks();
    ut::expect( ut::that % parser.extract_float() == 2.3E-2 );

    ut::expect( not parser.has_codepoint() );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
