#pragma once
//  ---------------------------------------------
//  Base class offering facilities to parse a
//  unicode text buffer supporting variable-length
//  encodings
//  ---------------------------------------------
//  #include "text_parser_base.hpp" // text::ParserBase, parse::*
//  ---------------------------------------------
#include <cassert>
#include <concepts> // std::predicate
#include <limits> // std::numeric_limits<>
#include <array>

#include <fmt/format.h> // fmt::format

#include "parsers_common.hpp" // parse::error
#include "fnotify_type.hpp" // fnotify_t
#include "unicode_text.hpp" // utxt::*
#include "ascii_predicates.hpp" // ascii::is_*



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace text
{

/////////////////////////////////////////////////////////////////////////////
template<utxt::Enc ENC>
class ParserBase
{
    using bytes_buffer_t = utxt::bytes_buffer_t<ENC>;

 public:
    struct context_t final
       {
        std::size_t line;
        std::size_t curr_codepoint_byte_offset;
        char32_t curr_codepoint;
        bytes_buffer_t::context_t buf_context;
       };
    constexpr context_t save_context() const noexcept
       {
        return { m_line, m_curr_codepoint_byte_offset, m_curr_codepoint, m_buf.save_context() };
       }
    constexpr void restore_context(const context_t& context) noexcept
       {
        m_line = context.line;
        m_curr_codepoint_byte_offset = context.curr_codepoint_byte_offset;
        m_curr_codepoint = context.curr_codepoint;
        m_buf.restore_context( context.buf_context );
       }

 private:
    bytes_buffer_t m_buf;
    std::size_t m_line = 1; // Current line number
    //std::size_t m_curr_codepoint_offset = 0;
    std::size_t m_curr_codepoint_byte_offset = 0; // Index of the first byte of the last extracted codepoint
    char32_t m_curr_codepoint = utxt::codepoint::null; // Current extracted codepoint
    fnotify_t m_on_notify_issue = default_notify;
    std::string m_file_path; // Just for create_parse_error()

 public:
    explicit constexpr ParserBase(const std::string_view bytes) noexcept
      : m_buf{bytes}
       {
        get_next(); // Read first codepoint
       }

    // Prevent copy or move
    ParserBase(const ParserBase&) =delete;
    ParserBase& operator=(const ParserBase&) =delete;
    ParserBase(ParserBase&&) =delete;
    ParserBase& operator=(ParserBase&&) =delete;

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr bool has_bytes() const noexcept { return m_buf.has_bytes(); }
    [[nodiscard]] constexpr std::size_t curr_line() const noexcept { return m_line; }
    //[[nodiscard]] constexpr std::size_t curr_codepoint_offset() const noexcept { return m_curr_codepoint_offset; }
    [[nodiscard]] constexpr std::size_t curr_byte_offset() const noexcept { return m_buf.byte_pos(); }
    [[nodiscard]] constexpr std::size_t curr_codepoint_byte_offset() const noexcept { return m_curr_codepoint_byte_offset; }


    //-----------------------------------------------------------------------
    static constexpr void default_notify(std::string&&) {} // { fmt::print(fmt::runtime(msg)); }
    constexpr void set_on_notify_issue(fnotify_t const& f) { m_on_notify_issue = f; }
    constexpr void notify_issue(const std::string_view msg) const
       {
        //fmt::print("  ! {}\n"sv, msg);
        m_on_notify_issue( fmt::format("{} (line {})"sv, msg, m_line) );
       }
    constexpr void set_file_path(std::string&& pth)
       {
        std::swap(m_file_path, pth);
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
        if( m_buf.has_codepoint() ) [[likely]]
           {
            m_curr_codepoint_byte_offset = m_buf.byte_pos();
            if( ascii::is_endline(m_curr_codepoint) ) ++m_line;
            m_curr_codepoint = m_buf.extract_codepoint();
            //++m_curr_codepoint_offset;
            //notify_issue(fmt::format("'{}'"sv, utxt::to_utf8(curr_codepoint())));
            return true;
           }
        else if( m_buf.has_bytes() )
           {// Truncated codepoint!
            m_curr_codepoint = utxt::codepoint::invalid;
            m_buf.set_as_depleted();
            notify_issue("! Truncated codepoint"sv);
            return true;
           }
        m_curr_codepoint = utxt::codepoint::null;
        return false;
       }
    [[nodiscard]] constexpr char32_t curr_codepoint() const noexcept
       {
        return m_curr_codepoint;
       }
    //-----------------------------------------------------------------------
    // Querying current codepoint
    [[nodiscard]] constexpr bool has_codepoint() const noexcept
       {
        return m_curr_codepoint!=utxt::codepoint::null;
       }
    [[nodiscard]] constexpr bool got(const char32_t cp) const noexcept
       {
        return m_curr_codepoint==cp;
       }
    [[nodiscard]] constexpr bool got_endline() const noexcept
       {
        return ascii::is_endline(m_curr_codepoint);
       }
    [[nodiscard]] constexpr bool got_space() const noexcept
       {
        return ascii::is_space(m_curr_codepoint);
       }
    [[nodiscard]] constexpr bool got_blank() const noexcept
       {
        return ascii::is_blank(m_curr_codepoint);
       }
    [[nodiscard]] constexpr bool got_digit() const  noexcept
       {
        return ascii::is_digit(m_curr_codepoint);
       }
    [[nodiscard]] constexpr bool got_punct() const noexcept
       {
        return ascii::is_punct(m_curr_codepoint);
       }
    template<std::predicate<const char32_t> CodepointPredicate =decltype(ascii::is_always_false<char32_t>)>
    [[nodiscard]] constexpr bool got(CodepointPredicate is) const noexcept
       {
        return is(curr_codepoint());
       }

    //-----------------------------------------------------------------------
    template<std::predicate<const char32_t> CodepointPredicate =decltype(ascii::is_always_false<char32_t>)>
    constexpr void skip_while(CodepointPredicate is) noexcept
       {
        while( is(curr_codepoint()) and get_next() );
       }

    //-----------------------------------------------------------------------
    template<std::predicate<const char32_t> CodepointPredicate =decltype(ascii::is_always_false<char32_t>)>
    constexpr void skip_until(CodepointPredicate is) noexcept
       {
        while( is(curr_codepoint()) and get_next() );
       }

    //-----------------------------------------------------------------------
    // Skip spaces except new line
    constexpr void skip_blanks() noexcept
       {
        while( got_blank() and get_next() ) ;
       }

    //-----------------------------------------------------------------------
    // Skip any space, including new line
    constexpr void skip_any_space() noexcept // aka skip_empty_lines()
       {
        while( got_space() and get_next() ) ;
       }

    //-----------------------------------------------------------------------
    constexpr void skip_line() noexcept
       {
        while( not got_endline() and get_next() ) ;
        get_next(); // Skip also end line
       }

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
            throw create_parse_error( fmt::format("Unexpected content '{}' at line end"sv, utxt::to_utf8(curr_codepoint())) );
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr bool eat(const char32_t cp) noexcept
       {
        if( got(cp) )
           {
            get_next();
            return true;
           }
        return false;
       }

    //-----------------------------------------------------------------------
    //if( parser.eat<U'C',U'D',U'A',U'T',U'A'>() ) ...
    template<char32_t cp1, char32_t cp2, char32_t... cpn>
    [[nodiscard]] constexpr bool eat()
       {
        using ustr_arr_t = std::array<char32_t, 2+sizeof...(cpn)>;
        static constexpr ustr_arr_t ustr_arr{cp1, cp2, cpn...};
        static constexpr std::u32string_view ustr(ustr_arr.data(), ustr_arr.size());
        static_assert( not ustr.contains(U'\n') ); // Otherwise should handle line counter
        static constexpr std::string bytes_to_eat = utxt::encode_as<ENC>(ustr);
        if( m_buf.get_current_view().starts_with(bytes_to_eat) )
           {
            advance_of( bytes_to_eat.size() ); // , ustr.size()
            return true;
           }
        return false;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr bool eat(const std::u32string_view sv) noexcept
       {
        assert( sv.size()>1 );
        if( got(sv[0]) )
           {
            const auto context = save_context();
            std::size_t i = 0;
            while( true )
               {
                if( ++i>=sv.size() )
                   {
                    get_next();
                    return true;
                   }
                else if( not get_next() or not got(sv[i]) )
                   {
                    break;
                   }
               }
            restore_context( context );
           }
        return false;
       }

    //template<std::predicate<const char32_t> CodepointPredicate =decltype(ascii::is_always_false<char32_t>)>
    //[[nodiscard]] constexpr std::string_view get_bytes_while(CodepointPredicate is) noexcept
    //   {
    //    const std::size_t i_start = curr_offset();
    //    while( is(curr_codepoint()) and get_next() );
    //    return m_buf.get_view_between(i_start, curr_offset());
    //   }

    //-----------------------------------------------------------------------
    //const auto bytes = parser.get_bytes_until(ascii::is_any_of<U'=',U':'>, ascii::is_endline<char32_t>);
    template<std::predicate<const char32_t> CodepointPredicate =decltype(ascii::is_always_false<char32_t>)>
    [[nodiscard]] constexpr std::string_view get_bytes_until(CodepointPredicate is_end, CodepointPredicate is_unexpected =ascii::is_always_false<char32_t>)
       {
        const auto start = save_context();
        while( not is_end(curr_codepoint()) )
           {
            if( is_unexpected(curr_codepoint()) )
               {
                const char32_t offending_codepoint = curr_codepoint();
                restore_context( start ); // Strong guarantee
                throw create_parse_error( fmt::format("Unexpected character '{}'"sv, utxt::to_utf8(offending_codepoint)) );
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

        return m_buf.get_view_between(start.curr_codepoint_byte_offset, m_curr_codepoint_byte_offset);
       }
    //-----------------------------------------------------------------------
    template<std::predicate<const char32_t> CodepointPredicate =decltype(ascii::is_always_false<char32_t>)>
    [[nodiscard]] constexpr std::string_view get_bytes_until_and_skip(CodepointPredicate is_end, CodepointPredicate is_unexpected =ascii::is_always_false<char32_t>)
       {
        std::string_view sv = get_bytes_until(is_end, is_unexpected);
        get_next(); // Skip termination codepoint
        return sv;
       }
    //-----------------------------------------------------------------------
    template<char32_t end_codepoint>
    [[nodiscard]] constexpr std::string_view get_bytes_until()
       {
        return get_bytes_until_and_skip(ascii::is<end_codepoint>, ascii::is_always_false<char32_t>);
       }

    //-----------------------------------------------------------------------
    //const auto bytes = parser.get_bytes_until<U'*',U'/'>();
    template<char32_t end_seq1, char32_t end_seq2, char32_t... end_seqtail>
    [[nodiscard]] constexpr std::string_view get_bytes_until()
       {
        using end_block_arr_t = std::array<char32_t, 2+sizeof...(end_seqtail)>;
        static constexpr end_block_arr_t end_block_arr{end_seq1, end_seq2, end_seqtail...};
        constexpr std::u32string_view end_block(end_block_arr.data(), end_block_arr.size());
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
        std::size_t content_end_byte_pos = start.curr_codepoint_byte_offset;
        std::size_t i = 0; // Matching codepoint index
        do {
            if( got(end_block[i]) )
               {// Matches a codepoint in end_block
                // If it's the first of end_block...
                if( i==0 )
                   {// ...Store the offset of the possible end of collected content
                    content_end_byte_pos = m_curr_codepoint_byte_offset;
                   }
                // If it's the last of end_block...
                if( ++i>=end_block.size() )
                   {// ...I'm done
                    //notify_issue( fmt::format("matched! cp:{} collected:{}", utxt::to_utf8(curr_codepoint()), m_buf.get_slice_between(start.curr_codepoint_byte_offset, content_end_byte_pos)) );
                    get_next(); // Skip last end_block codepoint
                    return m_buf.get_view_between(start.curr_codepoint_byte_offset, content_end_byte_pos);
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
                            content_end_byte_pos = m_curr_codepoint_byte_offset - i;
                            ++i;
                            break;
                           }
                       }
                   }
                while( i>0 );
               }
            //notify_issue( fmt::format("cp:{} collected:{} matched:{}", utxt::to_utf8(curr_codepoint()), m_buf.get_slice_between(start.curr_codepoint_byte_offset, content_end_byte_pos), utxt::to_utf8(end_block.substr(0, i))) );
           }
        while( get_next() );

        restore_context( start ); // Strong guarantee
        throw create_parse_error(fmt::format("Unclosed content (\"{}\" not found)",utxt::to_utf8(end_block)), start.line);
       }


    //-----------------------------------------------------------------------
    template<std::predicate<const char32_t> CodepointPredicate =decltype(ascii::is_always_false<char32_t>)>
    [[nodiscard]] constexpr std::u32string collect_until(CodepointPredicate is_end, CodepointPredicate is_unexpected =ascii::is_always_false<char32_t>)
       {
        const std::string_view bytes = get_bytes_until(is_end, is_unexpected);
        return utxt::to_utf32<ENC>(bytes);
       }
    template<std::predicate<const char32_t> CodepointPredicate =decltype(ascii::is_always_false<char32_t>)>
    [[nodiscard]] constexpr std::u32string collect_until_and_skip(CodepointPredicate is_end, CodepointPredicate is_unexpected =ascii::is_always_false<char32_t>)
       {
        std::u32string sv = collect_until(is_end, is_unexpected);
        get_next(); // Skip termination codepoint
        return sv;
       }

    //-----------------------------------------------------------------------
    template<char32_t end_codepoint>
    [[nodiscard]] constexpr std::u32string collect_until()
       {
        return collect_until_and_skip(ascii::is<end_codepoint>, ascii::is_always_false<char32_t>);
       }

    //-----------------------------------------------------------------------
    template<char32_t end_seq1, char32_t end_seq2, char32_t... end_seqtail>
    [[nodiscard]] constexpr std::u32string collect_until()
       {
        const std::string_view bytes = get_bytes_until<end_seq1, end_seq2, end_seqtail...>();
        return utxt::to_utf32<ENC>(bytes);
       }


    //-----------------------------------------------------------------------
    // Read a (base10) positive integer literal
    template<std::unsigned_integral Uint =std::size_t>
    [[nodiscard]] constexpr Uint extract_index()
       {
        if( not got_digit() )
           {
            throw create_parse_error( fmt::format("Invalid char '{}' in number literal"sv, utxt::to_utf8(curr_codepoint())) );
           }

        Uint result = ascii::value_of_digit(curr_codepoint());
        constexpr Uint base = 10u;
        constexpr Uint overflow_limit = ((std::numeric_limits<Uint>::max() - (base - 1u)) / (2 * base)) - 1u; // A little conservative
        while( get_next() and got_digit() )
           {
            if( result>overflow_limit )
               {
                throw create_parse_error( fmt::format("Integer literal too big ({}x{} would be dangerously near {})", result, base, std::numeric_limits<Uint>::max()/2) );
               }
            result = static_cast<Uint>((base*result) + ascii::value_of_digit(curr_codepoint()));
           }
        return result;
       }

 private:
    //-----------------------------------------------------------------------
    void advance_of(const std::size_t bytes_num)
       {
        assert( bytes_num>0 );
        m_buf.advance_of( bytes_num );
        // Note: assuming same m_line
        //m_curr_codepoint_offset += codepoints_num;
        get_next();
       }
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include "ansi_escape_codes.hpp" // ANSI_RED, ...
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"text::ParserBase"> text_parser_base_tests = []
{////////////////////////////////////////////////////////////////////////////
using ut::expect;
using ut::that;
using ut::throws;
using enum utxt::Enc;

auto notify_sink = [](const std::string_view msg) -> void { ut::log << ANSI_BLUE "parser: " ANSI_DEFAULT << msg; };

ut::test("empty") = []
   {
    text::ParserBase<UTF8> parser{""sv};

    expect( not parser.has_codepoint() and parser.curr_codepoint()==utxt::codepoint::null );
   };


ut::test("simple utf-8") = []
   {
    text::ParserBase<UTF8> parser{"\x61\xC3\xA0\x20\xC2\xB1\xE2\x88\x86"sv}; // u8"aà ±∆"

    expect( parser.has_codepoint() and parser.got(U'a') );
    expect( parser.get_next() and parser.got(U'à') );
    expect( parser.get_next() and parser.got(U' ') );
    expect( parser.get_next() and parser.got(U'±') );
    expect( parser.get_next() and parser.got(U'∆') );
    expect( not parser.get_next() and parser.got(utxt::codepoint::null) );
   };


ut::test("simple utf-16le") = []
   {
    text::ParserBase<UTF16LE> parser{"\x61\x00\xE0\x00\x20\x00\xB1\x00\x06\x22"sv}; // u"aà ±∆"

    expect( parser.has_codepoint() and parser.got(U'a') );
    expect( parser.get_next() and parser.got(U'à') );
    expect( parser.get_next() and parser.got(U' ') );
    expect( parser.get_next() and parser.got(U'±') );
    expect( parser.get_next() and parser.got(U'∆') );
    expect( not parser.get_next() and parser.got(utxt::codepoint::null) );
   };

ut::test("context and eat") = []
   {
    text::ParserBase<UTF8> parser{ "abcdef"sv };
    const auto start = parser.save_context();
    expect( parser.eat(U"abc") and parser.eat(U"def") and not parser.has_bytes() );
    //expect( parser.eat<U'a',U'b',U'c'>() and parser.eat<U'd',U'e',U'f'>() and not parser.has_bytes() );
    parser.restore_context( start );
    expect( parser.eat(U"abc") and parser.eat(U"def") and not parser.has_bytes() );
    //expect( parser.eat<U'a',U'b',U'c'>() and parser.eat<U'd',U'e',U'f'>() and not parser.has_bytes() );
   };

ut::test("context and eat utf16") = []
   {
    text::ParserBase<UTF16BE> parser{ "\0a" "\0b" "\0c" "\0d" "\0e" "\0f"sv };
    const auto start = parser.save_context();
    expect( parser.eat(U"abc") and parser.eat(U"def") and not parser.has_bytes() );
    //expect( parser.eat<U'a',U'b',U'c'>() and parser.eat<U'd',U'e',U'f'>() and not parser.has_bytes() );
    parser.restore_context( start );
    expect( parser.eat(U"abc") and parser.eat(U"def") and not parser.has_bytes() );
    //expect( parser.eat<U'a',U'b',U'c'>() and parser.eat<U'd',U'e',U'f'>() and not parser.has_bytes() );
   };

ut::test("eating spaces") = []
   {
    text::ParserBase<UTF8> parser
       {
        "1\n"
        "2  \t\t  \r\n"
        "3 \t \n"
        "4 blah blah\r\n"
        "5\r\n"
        "6\t\t \r \t F\r\n"
        "   \n"
        "\t\t\n"
        "9 blah\n"sv
       };

    expect( parser.has_codepoint() and parser.got(U'1') );
    parser.skip_blanks();
    parser.skip_any_space();
    expect( not parser.got_endline() and parser.got(U'1') and parser.curr_line()==1 );
    expect( parser.get_next() and parser.got_endline() and parser.curr_line()==1 );
    expect( parser.get_next() and parser.curr_line()==2 );

    expect( parser.got(U'2') );
    expect( parser.get_next() and parser.got_blank() );
    parser.skip_blanks();
    expect( parser.got_endline() and parser.get_next() and parser.curr_line()==3 );

    expect( parser.got(U'3') and parser.curr_line()==3 and parser.get_next() );
    parser.skip_any_space();

    expect( parser.got(U'4') and parser.curr_line()==4 );
    parser.skip_blanks();
    parser.skip_line();

    expect( parser.got(U'5') and parser.curr_line()==5 and parser.get_next() );
    parser.skip_blanks();
    parser.check_and_eat_endline();

    expect( parser.got(U'6') and parser.curr_line()==6 and parser.get_next() );
    parser.skip_blanks();
    expect( parser.got(U'F') and parser.curr_line()==6 and parser.get_next() );
    parser.skip_any_space();

    expect( parser.got(U'9') and parser.curr_line()==9 );
    parser.skip_line();

    expect( not parser.has_bytes() and parser.curr_line()==9 );
   };


ut::test("parse utilities") = [&notify_sink]
   {
    text::ParserBase<UTF8> parser
       {
        "abc123\n"
        "<tag>a=\"\" b=\"str\"</tag>\n"
        "/*block\n"
        "comment*/end\n"
        "blah <!--another\n"
        "block\n"
        "comment-->"sv
       };
    parser.set_on_notify_issue(notify_sink);
    expect( parser.has_codepoint() );

    expect( not parser.eat(U"abb") and not parser.eat(U"abcd") and parser.eat(U"abc") and parser.got(U'1') and parser.curr_line()==1u );
    expect( parser.eat(U'1') and not parser.eat(U'3') and parser.eat(U'2') and parser.eat(U'3') and parser.got(U'\n') and parser.curr_line()==1u );
    expect( parser.eat(U'\n') and parser.curr_line()==2u );

    expect( parser.eat(U'<') and parser.curr_line()==2u );
    expect( throws<parse::error>([&parser] { [[maybe_unused]] auto sv = parser.get_bytes_until<U'☺'>(); }) ) << "missing closing character should throw\n";
    expect( parser.get_bytes_until<U'>'>()=="tag"sv and parser.curr_line()==2u and parser.got(U'a') );
    expect( parser.eat(U"a=\"") );
    expect( throws<parse::error>([&parser] { [[maybe_unused]] auto sv = parser.collect_until_and_skip(ascii::is<U'*'>, ascii::is_endline<char32_t>); }) ) << "missing closing character in same line should throw\n";
    expect( parser.collect_until_and_skip(ascii::is<U'\"'>, ascii::is_endline<char32_t>)==U""sv and parser.got(U' ') );

    parser.skip_blanks();
    expect( parser.eat(U"b=\"") );
    expect( parser.collect_until(ascii::is<U'\"'>, ascii::is_endline<char32_t>)==U"str"sv and parser.eat(U'\"') and parser.eat(U"</"sv) );
    expect( parser.get_bytes_until<U'>'>()=="tag"sv and parser.got_endline() and parser.get_next() );

    expect( parser.eat(U"/*") and parser.curr_line()==3u );
    expect( parser.get_bytes_until<U'*',U'/'>()=="block\ncomment"sv );
    expect( parser.eat(U"end") );

    expect( parser.collect_until<U'<',U'!',U'-',U'-'>()==U"\nblah "sv and parser.got(U'a') );
    expect( parser.collect_until<U'-',U'-',U'>'>()==U"another\nblock\ncomment"sv and not parser.has_bytes() );
   };

ut::test("end block edge case 1") = [&notify_sink]
   {
    text::ParserBase<UTF8> parser{ "****/a"sv };
    parser.set_on_notify_issue(notify_sink);
    expect( parser.get_bytes_until<U'*',U'/'>()=="***"sv and parser.got(U'a') );
   };

ut::test("end block edge case 2") = [&notify_sink]
   {
    text::ParserBase<UTF8> parser{ "----->a"sv };
    parser.set_on_notify_issue(notify_sink);
    expect( parser.get_bytes_until<U'-',U'-',U'>'>()=="---"sv and parser.got(U'a') );
   };

ut::test("numbers") = [&notify_sink]
   {
    text::ParserBase<UTF8> parser
       {
        "a=1234mm\n"
        "b=h1\n"
        "c=12"sv
       };
    parser.set_on_notify_issue(notify_sink);

    expect( parser.has_codepoint() and parser.eat(U'a') and parser.eat(U'=') );
    expect( that % parser.extract_index()==1234u );
    expect( parser.got(U'm') and parser.curr_line()==1u );
    parser.skip_line();

    expect( parser.eat(U'b') and parser.eat(U'=') and parser.curr_line()==2 );
    expect( throws<parse::error>([&parser] { [[maybe_unused]] auto i = parser.extract_index(); }) ) << "invalid index should throw\n";
    expect( parser.eat(U'h') );
    expect( that % parser.extract_index()==1u );
    expect( parser.got_endline() and parser.get_next() and parser.curr_line()==3u );

    expect( parser.eat(U'c') and parser.eat(U'=') );
    expect( that % parser.extract_index()==12u );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
