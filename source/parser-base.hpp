#pragma once
//  ---------------------------------------------
//  Common encoding agnostic parsing facilities
//  ---------------------------------------------
#include <cassert>
#include <cstdint> // std::uint8_t
#include <stdexcept> // std::exception, std::runtime_error
#include <concepts>
#include <functional> // std::function
#include <array>
#include <string>
#include <string_view>
#include <fmt/format.h> // fmt::format

#include "text.hpp" // text::*

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace text
{


/////////////////////////////////////////////////////////////////////////////
class parse_error final : public std::exception
{
 private:
    std::string m_msg;
    std::string m_file;
    std::size_t m_line;

 public:
    explicit parse_error(std::string&& msg, const std::string_view fil, const std::size_t lin) noexcept
       : m_msg{std::move(msg)}
       , m_file{fil}
       , m_line{lin}
        {}

    std::string const& file() const noexcept { return m_file; }
    std::size_t line() const noexcept { return m_line; }

    char const* what() const noexcept override { return m_msg.c_str(); }
};


/////////////////////////////////////////////////////////////////////////////
using parse_flags_t = std::uint8_t;
namespace parse_flag
{
    enum : parse_flags_t
    {
       NONE = 0x0
      ,SKIP_STOPPER = 0x1 // Skip the codepoint that ended the collect
       //,RESERVED = 0x2 // Reserved
       //,RESERVED = 0x4 // Reserved
       //,RESERVED = 0x8 // Reserved
    };
}

using fnotify_t = std::function<void(std::string&&)>;


/////////////////////////////////////////////////////////////////////////////
template<text::Enc ENC>
class ParserBase
{
 private:
    using bytes_buffer_t = text::bytes_buffer_t<ENC>;

 public:
    struct context_t final
       {
        std::size_t line;
        std::size_t offset;
        std::size_t curr_codepoint_byte_offset;
        char32_t curr_codepoint;
        bytes_buffer_t::context_t buf_context;
       };
    constexpr context_t save_context() const noexcept
       {
        return { m_line, m_offset, m_curr_codepoint_byte_offset, m_curr_codepoint, m_buf.save_context() };
       }
    constexpr void restore_context(const context_t context) noexcept
       {
        m_line = context.line;
        m_offset = context.offset;
        m_curr_codepoint_byte_offset = context.curr_codepoint_byte_offset;
        m_curr_codepoint = context.curr_codepoint;
        m_buf.restore_context( context.buf_context );
       }

 private:
    bytes_buffer_t m_buf;
    std::size_t m_line = 1; // Current line number
    std::size_t m_offset = 0; // Index of next extracted codepoint
    std::size_t m_curr_codepoint_byte_offset = 0; // Index of the first byte of the last extracted codepoint
    char32_t m_curr_codepoint = text::null_codepoint; // Current extracted character
    fnotify_t m_on_notify_issue = default_notify;
    std::string m_file_path; // Just for create_parse_error

 public:
    //static const text::Enc INPUT_ENCODING = ENC;
    explicit constexpr ParserBase(const std::string_view bytes) noexcept
      : m_buf{bytes}
       {
        [[maybe_unused]] const bool has_next = get_next(); // Read first codepoint
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr bool has_bytes() const noexcept { return m_buf.has_bytes(); }
    [[nodiscard]] constexpr std::size_t curr_line() const noexcept { return m_line; }
    [[nodiscard]] constexpr std::size_t curr_offset() const noexcept { return m_offset; }
    [[nodiscard]] constexpr std::size_t curr_byte_offset() const noexcept { return m_buf.byte_pos(); }
    [[nodiscard]] constexpr std::size_t curr_codepoint_byte_offset() const noexcept { return m_curr_codepoint_byte_offset; }
    [[nodiscard]] constexpr char32_t curr_codepoint() const noexcept { return m_curr_codepoint; }

 public:
    //-----------------------------------------------------------------------
    static constexpr void default_notify(std::string&&) {} // { fmt::print(fmt::runtime(msg)); }
    constexpr void set_on_notify_issue(fnotify_t const& f) { m_on_notify_issue = f; }
    constexpr void notify_issue(const std::string_view msg) const
       {
        //fmt::print("  ! {}\n"sv, msg);
        m_on_notify_issue( fmt::format("{} (line {} offset {})"sv, msg, m_line, m_offset) );
       }
    constexpr void set_file_path(std::string&& pth)
       {
        std::swap(m_file_path, pth);
       }
     [[nodiscard]] parse_error create_parse_error(std::string&& msg) const noexcept
       {
        return create_parse_error(std::move(msg), m_line);
       }
     [[nodiscard]] parse_error create_parse_error(std::string&& msg, const std::size_t l) const noexcept
       {
        if( m_file_path.empty() )
           {// I'm probably parsing a buffer
            return parse_error(std::move(msg), "buffer", l);
           }
        return parse_error(std::move(msg), m_file_path, l);
       }

    //-----------------------------------------------------------------------
    // Extract next codepoint from buffer
    [[nodiscard]] constexpr bool get_next() noexcept
       {
        if( m_buf.has_codepoint() ) [[likely]]
           {
            m_curr_codepoint_byte_offset = m_buf.byte_pos();
            if( text::is_endline(m_curr_codepoint) ) ++m_line;
            m_curr_codepoint = m_buf.extract_codepoint();
            ++m_offset;
            //notify_issue(fmt::format("'{}'"sv, text::to_utf8(curr_codepoint())));
            return true;
           }
        else if( m_buf.has_bytes() )
           {// Truncated codepoint!
            m_curr_codepoint = text::err_codepoint;
            m_buf.set_as_depleted();
            notify_issue("! Truncated codepoint"sv);
            return true;
           }
        m_curr_codepoint = text::null_codepoint;
        return false;
       }

    //-----------------------------------------------------------------------
    // Querying current codepoint
    [[nodiscard]] constexpr bool has_codepoint() noexcept
       {
        return m_curr_codepoint!=text::null_codepoint;
       }
    [[nodiscard]] constexpr bool got(const char32_t cp) noexcept
       {
        return m_curr_codepoint==cp;
       }
    [[nodiscard]] constexpr bool isnt(const char32_t cp) noexcept
       {
        return m_curr_codepoint!=cp;
       }
    [[nodiscard]] constexpr bool got_endline() noexcept
       {
        return text::is_endline(m_curr_codepoint);
       }
    [[nodiscard]] constexpr bool got_space() noexcept
       {
        return text::is_space(m_curr_codepoint);
       }
    [[nodiscard]] constexpr bool got_blank() noexcept
       {
        return text::is_blank(m_curr_codepoint);
       }
    [[nodiscard]] constexpr bool got_digit() noexcept
       {
        return text::is_digit(m_curr_codepoint);
       }
    [[nodiscard]] constexpr bool got_punct() noexcept
       {
        return text::is_punct(m_curr_codepoint);
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
        while( !got_endline() and get_next() ) ;
        [[maybe_unused]] const bool has_next = get_next(); // Skip also end line
       }

    //-----------------------------------------------------------------------
    // Line should end: nothing more than spaces allowed
    constexpr void skip_endline()
       {
        skip_blanks();
        if( not eat_endline() )
           {
            throw create_parse_error( fmt::format("Unexpected content '{}' at line end"sv, text::to_utf8(curr_codepoint())) );
           }
       }

    //-----------------------------------------------------------------------
    [[maybe_unused]] constexpr bool eat_endline() noexcept
       {
        if( got_endline() )
           {
            [[maybe_unused]] const bool has_next = get_next();
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr bool eat(const char32_t cp) noexcept
       {
        if( got(cp) )
           {
            [[maybe_unused]] const bool has_next = get_next();
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
        static_assert( !ustr.contains(U'\n') ); // Otherwise should handle line counter
        static constexpr std::string bytes_to_eat = text::encode_as<ENC>(ustr);
        if( m_buf.get_current_view().starts_with(bytes_to_eat) )
           {
            m_buf.advance_of( bytes_to_eat.size() );
            m_offset += ustr.size();
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
                    [[maybe_unused]] const bool has_next = get_next();
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


    //-----------------------------------------------------------------------
    //const auto bytes = parser.collect_bytes_until(text::is_any_of<U'=',U':'>, text::is_endline);
    template<std::predicate<const char32_t> CodepointPredicate =decltype(text::is_always_false)>
    [[nodiscard]] constexpr std::string_view collect_bytes_until(CodepointPredicate is_end, CodepointPredicate is_unexpected =text::is_always_false, const parse_flags_t flags =parse_flag::NONE)
       {
        const auto start = save_context();
        do {
            if( is_end(curr_codepoint()) ) [[unlikely]]
               {
                std::string_view collected = m_buf.get_view_between(start.curr_codepoint_byte_offset, m_curr_codepoint_byte_offset);
                 if( flags & parse_flag::SKIP_STOPPER )
                   {
                    [[maybe_unused]] const bool has_next = get_next(); // Skip termination codepoint
                   }
                return collected;
               }
            else if( is_unexpected(curr_codepoint()) ) [[unlikely]]
               {
                break;
               }
           }
        while( get_next() ); [[likely]]

        restore_context( start ); // Strong guarantee
        throw create_parse_error( has_codepoint() ? fmt::format("Unexpected character '{}'"sv, text::to_utf8(curr_codepoint()))
                                                  : "Unexpected end (termination not found)"s );
       }


    //-----------------------------------------------------------------------
    //const auto bytes = parser.collect_bytes_until<U'*',U'/'>();
    template<char32_t end_seq1, char32_t end_seq2, char32_t... end_seqtail>
    [[nodiscard]] constexpr std::string_view collect_bytes_until()
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
                a[i] = a[i-1] && end_blk[i-1]==end_blk[i];
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
                    //notify_issue( fmt::format("matched! cp:{} collected:{}", text::to_utf8(curr_codepoint()), m_buf.get_slice_between(start.curr_codepoint_byte_offset, content_end_byte_pos)) );
                    [[maybe_unused]] const bool has_next = get_next(); // Skip last end_block codepoint
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
            //notify_issue( fmt::format("cp:{} collected:{} matched:{}", text::to_utf8(curr_codepoint()), m_buf.get_slice_between(start.curr_codepoint_byte_offset, content_end_byte_pos), text::to_utf8(end_block.substr(0, i))) );
           }
        while( get_next() ); [[likely]]

        restore_context( start ); // Strong guarantee
        throw create_parse_error( fmt::format("Should be closed by {}"sv, text::to_utf8(end_block)) );
       }

    //-----------------------------------------------------------------------
    template<char32_t end_codepoint>
    [[nodiscard]] constexpr std::string_view collect_bytes_until()
       {
        return collect_bytes_until(text::is<end_codepoint>, text::is_always_false, parse_flag::SKIP_STOPPER);
       }


    //-----------------------------------------------------------------------
    template<std::predicate<const char32_t> CodepointPredicate =decltype(text::is_always_false)>
    [[nodiscard]] constexpr std::u32string collect_until(CodepointPredicate is_end, CodepointPredicate is_unexpected =text::is_always_false, const parse_flags_t flags =parse_flag::NONE)
       {
        const std::string_view bytes = collect_bytes_until(is_end, is_unexpected, flags);
        return text::to_utf32<ENC>(bytes);
       }

    //-----------------------------------------------------------------------
    template<char32_t end_codepoint>
    [[nodiscard]] constexpr std::u32string collect_until()
       {
        return collect_until(text::is<end_codepoint>, text::is_always_false, parse_flag::SKIP_STOPPER);
       }

    //-----------------------------------------------------------------------
    template<char32_t end_seq1, char32_t end_seq2, char32_t... end_seqtail>
    [[nodiscard]] constexpr std::u32string collect_until()
       {
        const std::string_view bytes = collect_bytes_until<end_seq1, end_seq2, end_seqtail...>();
        return text::to_utf32<ENC>(bytes);
       }


    //-----------------------------------------------------------------------
    // Read a (base10) positive integer literal
    [[nodiscard]] constexpr std::size_t extract_index()
       {
        if( not got_digit() )
           {
            throw create_parse_error( fmt::format("Invalid char '{}' in index"sv, text::to_utf8(curr_codepoint())) );
           }

        std::size_t result = (curr_codepoint()-U'0');
        constexpr std::size_t base = 10u;
        while( get_next() and got_digit() )
           {
            //assert( result < (std::numeric_limits<std::size_t>::max - (curr_codepoint()-U'0')) / base ); // Check overflows
            result = (base*result) + (curr_codepoint()-U'0');
           }
        return result;
       }
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
static ut::suite<"text::ParserBase"> ParserBase_tests = []
{////////////////////////////////////////////////////////////////////////////
    using namespace std::literals; // "..."sv
    //using namespace ut::literals; // _ul
    using ut::expect;
    using ut::that;
    using ut::throws;
    using enum text::Enc;

    auto notify_sink = [](const std::string_view msg) -> void { ut::log << "\033[33m" "parser: " "\033[0m" << msg; };

    ut::test("empty") = []
       {
        text::ParserBase<UTF8> parser{""sv};

        expect( not parser.has_codepoint() and parser.curr_codepoint()==text::null_codepoint );
       };


    ut::test("simple utf-8") = []
       {
        text::ParserBase<UTF8> parser{"\x61\xC3\xA0\x20\xC2\xB1\xE2\x88\x86"sv}; // u8"aà ±∆"

        expect( parser.has_codepoint() and parser.got(U'a') );
        expect( parser.get_next() and parser.got(U'à') );
        expect( parser.get_next() and parser.got(U' ') );
        expect( parser.get_next() and parser.got(U'±') );
        expect( parser.get_next() and parser.got(U'∆') );
        expect( not parser.get_next() and parser.got(text::null_codepoint) );
       };


    ut::test("simple utf-16le") = []
       {
        text::ParserBase<UTF16LE> parser{"\x61\x00\xE0\x00\x20\x00\xB1\x00\x06\x22"sv}; // u"aà ±∆"

        expect( parser.has_codepoint() and parser.got(U'a') );
        expect( parser.get_next() and parser.got(U'à') );
        expect( parser.get_next() and parser.got(U' ') );
        expect( parser.get_next() and parser.got(U'±') );
        expect( parser.get_next() and parser.got(U'∆') );
        expect( not parser.get_next() and parser.got(text::null_codepoint) );
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
        expect( parser.eat_endline() and parser.curr_line()==2 );

        expect( parser.got(U'2') );
        expect( parser.get_next() and parser.got_blank() );
        parser.skip_blanks();
        expect( parser.eat_endline() and parser.curr_line()==3 );

        expect( parser.got(U'3') and parser.curr_line()==3 and parser.get_next() );
        parser.skip_any_space();

        expect( parser.got(U'4') and parser.curr_line()==4 );
        parser.skip_line();

        expect( parser.got(U'5') and parser.curr_line()==5 and parser.get_next() );
        parser.skip_endline();

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
        expect( throws<text::parse_error>([&parser] { [[maybe_unused]] auto n = parser.collect_bytes_until<U'☺'>(); }) ) << "missing closing character should throw\n";
        expect( parser.collect_bytes_until<U'>'>()=="tag"sv and parser.curr_line()==2u and parser.got(U'a') );
        expect( parser.eat(U"a=\"") );
        expect( throws<text::parse_error>([&parser] { [[maybe_unused]] auto n = parser.collect_until(text::is<U'*'>, text::is_endline, text::parse_flag::SKIP_STOPPER); }) ) << "missing closing character in same line should throw\n";
        expect( parser.collect_until(text::is<U'\"'>, text::is_endline, text::parse_flag::SKIP_STOPPER)==U""sv and parser.got(U' ') );

        parser.skip_blanks();
        expect( parser.eat(U"b=\"") );
        expect( parser.collect_until(text::is<U'\"'>, text::is_endline)==U"str"sv and parser.eat(U'\"') and parser.eat(U"</"sv) );
        expect( parser.collect_bytes_until<U'>'>()=="tag"sv and parser.eat_endline() );

        expect( parser.eat(U"/*") and parser.curr_line()==3u );
        expect( parser.collect_bytes_until<U'*',U'/'>()=="block\ncomment"sv );
        expect( parser.eat(U"end") );

        expect( parser.collect_until<U'<',U'!',U'-',U'-'>()==U"\nblah "sv and parser.got(U'a') );
        expect( parser.collect_until<U'-',U'-',U'>'>()==U"another\nblock\ncomment"sv and not parser.has_bytes() );
       };

    ut::test("end block edge case 1") = [&notify_sink]
       {
        text::ParserBase<UTF8> parser{ "****/a"sv };
        parser.set_on_notify_issue(notify_sink);
        expect( parser.collect_bytes_until<U'*',U'/'>()=="***"sv and parser.got(U'a') );
       };

    ut::test("end block edge case 2") = [&notify_sink]
       {
        text::ParserBase<UTF8> parser{ "----->a"sv };
        parser.set_on_notify_issue(notify_sink);
        expect( parser.collect_bytes_until<U'-',U'-',U'>'>()=="---"sv and parser.got(U'a') );
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
        expect( throws<text::parse_error>([&parser] { [[maybe_unused]] auto n = parser.extract_index(); }) ) << "invalid index should throw\n";
        expect( parser.eat(U'h') );
        expect( that % parser.extract_index()==1u );
        expect( parser.eat_endline() and parser.curr_line()==3u );

        expect( parser.eat(U'c') and parser.eat(U'=') );
        expect( that % parser.extract_index()==12u );
       };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
