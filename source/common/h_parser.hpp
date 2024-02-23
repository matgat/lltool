#pragma once
//  ---------------------------------------------
//  Parses a file containing a list of c-like
//  preprocessor defines
//  ---------------------------------------------
//  #include "h_parser.hpp" // h::Parser
//  ---------------------------------------------
#include <charconv> // std::from_chars

#include "plain_parser_base.hpp" // plain::ParserBase
#include "string_utilities.hpp" // str::trim_right()


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace h //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
// Descriptor of a '#define' entry in a buffer
class Define final
{
 private:
    std::string_view m_label;
    std::string_view m_value;
    std::string_view m_comment;
    std::string_view m_comment_predecl;

 public:
    constexpr void clear() noexcept
       {
        m_label = {};
        m_value = {};
        m_comment = {};
        m_comment_predecl = {};
       }

    [[nodiscard]] explicit constexpr operator bool() const noexcept { return !m_value.empty(); }

    [[nodiscard]] constexpr std::string_view label() const noexcept { return m_label; }
    constexpr void set_label(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error("Empty define label");
           }
        m_label = sv;
       }

    [[nodiscard]] constexpr std::string_view value() const noexcept { return m_value; }
    constexpr void set_value(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error("Empty define value");
           }
        m_value = sv;
       }


    [[nodiscard]] constexpr bool has_comment() const noexcept { return !m_comment.empty(); }
    [[nodiscard]] constexpr std::string_view comment() const noexcept { return m_comment; }
    constexpr void set_comment(const std::string_view sv) noexcept { m_comment = sv; }

    [[nodiscard]] constexpr bool has_comment_predecl() const noexcept { return !m_comment_predecl.empty(); }
    [[nodiscard]] constexpr std::string_view comment_predecl() const noexcept { return m_comment_predecl; }
    constexpr void set_comment_predecl(const std::string_view sv) noexcept { m_comment_predecl = sv; }

    [[nodiscard]] bool value_is_number() const noexcept
       {
        double result;
        const auto i_end = m_value.data() + m_value.size();
        auto [i, ec] = std::from_chars(m_value.data(), i_end, result);
        return ec==std::errc() and i==i_end;
       }
};



/////////////////////////////////////////////////////////////////////////////
class Parser final : public plain::ParserBase<char>
{         using inherited = plain::ParserBase<char>;
 private:
    Define m_curr_def;

 public:
    Parser(const std::string_view buf)
      : inherited(buf)
       {}

    //-----------------------------------------------------------------------
    [[nodiscard]] const Define& next_define()
       {
        m_curr_def.clear();
        try{
            while( true )
               {
                inherited::skip_blanks();
                if( not inherited::has_codepoint() )
                   {// No more data
                    break;
                   }
                else if( eat_line_comment_start() )
                   {
                    inherited::skip_line();
                   }
                else if( eat_block_comment_start() )
                   {
                    skip_block_comment();
                   }
                else if( inherited::got_endline() )
                   {
                    inherited::get_next();
                   }
                else if( inherited::eat_token("#define"sv) )
                   {
                    collect_define(m_curr_def);
                    break;
                   }
                else
                   {
                    throw create_parse_error( fmt::format("Unexpected content: {}", str::escape(inherited::get_rest_of_line())) );
                   }
               }
           }
        catch( parse::error& )
           {
            throw;
           }
        catch( std::exception& e )
           {
            throw create_parse_error(e.what());
           }

        return m_curr_def;
       }


 private:
    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat_line_comment_start() noexcept
       {
        return inherited::eat("//"sv);
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat_block_comment_start() noexcept
       {
        return inherited::eat("/*"sv);
       }

    //-----------------------------------------------------------------------
    void skip_block_comment()
       {
        [[maybe_unused]] auto cmt = inherited::get_until("*/");
       }

    //-----------------------------------------------------------------------
    void collect_define_comment( Define& def )
       {// [INT] Descr
        inherited::skip_blanks();

        // Detect possible pre-declaration in square brackets like: // [xxx] comment
        if( inherited::got('[') )
           {
            inherited::get_next();
            inherited::skip_blanks();
            std::string_view predecl = inherited::get_until_or_endline<']'>();
            if( inherited::got(']') )
               {
                def.set_comment_predecl( str::trim_right(predecl) );
                inherited::get_next();
                inherited::skip_blanks();
               }
            else
               {
                inherited::notify_issue( fmt::format("Unclosed \'[\' in the comment of define {}", def.label()) );
                def.set_comment( str::trim_right(predecl) );
                inherited::get_next();
                return;
               }
           }

        // Collect the remaining comment text
        def.set_comment( str::trim_right(inherited::get_rest_of_line()) );
        //if( def.comment().empty() )
        //   {
        //    inherited::notify_issue( fmt::format("Define {} has no comment", def.label()) );
        //   }
       }

    //-----------------------------------------------------------------------
    void collect_define( Define& def )
       {// LABEL       0  // [INT] Descr
        // vnName     vn1782  // descr [unit]

        // Contract: '#define' already eat

        // [Label]
        inherited::skip_blanks();
        def.set_label( inherited::get_identifier() );

        // [Value]
        inherited::skip_blanks();
        def.set_value( inherited::get_until_space_or_end() );

        // [Comment]
        inherited::skip_blanks();
        if( eat_line_comment_start() )
           {
            collect_define_comment( def );
           }
        //else
        //   {
        //    inherited::notify_issue( fmt::format("Define {} hasn't a comment", def.label()) );
        //   }
       }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include "ansi_escape_codes.hpp" // ANSI_RED, ...
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"h::Parser"> h_parser_tests = []
{////////////////////////////////////////////////////////////////////////////
//auto notify_sink = [](const std::string_view msg) -> void { ut::log << ANSI_BLUE "parser: " ANSI_DEFAULT << msg; };
ut::test("basic") = []
   {
    const std::string_view buf =
        "// test header\n"
        "\n"
        "//#define commented val // A commented define\n"
        "#define const 1234 // [INT] A constant\n"
        "\n"
        "#define macro  value   // A first macro\n"
        "#define macro2  value2 // A second macro \n"
        " /* block\n   comment */ \n"
        " #define  macro3 value3 // [ A third macro \n"
        "\n"
        "#define last x //  [ y 2 ]  A last macro"sv;

    h::Parser parser(buf);
    //parser.set_on_notify_issue(notify_sink);

    std::size_t n_event = 0u;
    try{
        while( const h::Define def = parser.next_define() )
           {
            //ut::log << ANSI_BLUE << def.label() << '=' << def.value() << ANSI_CYAN "(num " << n_event+1u << " line " << parser.curr_line() << ")\n" ANSI_DEFAULT;
            switch( ++n_event )
               {
                case  1:
                    ut::expect( ut::that % def.label()=="const"sv );
                    ut::expect( ut::that % def.value()=="1234"sv );
                    ut::expect( ut::that % def.comment()=="A constant"sv );
                    ut::expect( ut::that % def.comment_predecl()=="INT"sv );
                    break;

                case  2:
                    ut::expect( ut::that % def.label()=="macro"sv );
                    ut::expect( ut::that % def.value()=="value"sv );
                    ut::expect( ut::that % def.comment()=="A first macro"sv );
                    ut::expect( ut::that % def.comment_predecl()==""sv );
                    break;

                case  3:
                    ut::expect( ut::that % def.label()=="macro2"sv );
                    ut::expect( ut::that % def.value()=="value2"sv );
                    ut::expect( ut::that % def.comment()=="A second macro"sv );
                    ut::expect( ut::that % def.comment_predecl()==""sv );
                    break;

                case  4:
                    ut::expect( ut::that % def.label()=="macro3"sv );
                    ut::expect( ut::that % def.value()=="value3"sv );
                    ut::expect( ut::that % def.comment()=="A third macro"sv );
                    ut::expect( ut::that % def.comment_predecl()==""sv );
                    break;

                case  5:
                    ut::expect( ut::that % def.label()=="last"sv );
                    ut::expect( ut::that % def.value()=="x"sv );
                    ut::expect( ut::that % def.comment()=="A last macro"sv );
                    ut::expect( ut::that % def.comment_predecl()=="y 2"sv );
                    break;

                default:
                    ut::expect(false) << "unexpected define:\"" << def.label() << "\"\n";
               }
           }
       }
    catch( parse::error& e )
       {
        ut::log << ANSI_MAGENTA "Exception: " ANSI_RED << e.what() << ANSI_DEFAULT "(event " << n_event << " line " << e.line() << ")\n";
       }
    ut::expect( ut::that % n_event==5u ) << "events number should match";
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
