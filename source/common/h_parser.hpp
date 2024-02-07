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
    [[nodiscard]] constexpr operator bool() const noexcept { return !m_value.empty(); }

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

    [[nodiscard]] constexpr bool value_is_number() const noexcept
       {
        double result;
        const auto i_end = m_value.data() + m_value.size();
        auto [i, ec] = std::from_chars(m_value.data(), i_end, result);
        return ec==std::errc() and i==i_end;
       }

    [[nodiscard]] constexpr bool has_comment() const noexcept { return !m_comment.empty(); }
    [[nodiscard]] constexpr std::string_view comment() const noexcept { return m_comment; }
    constexpr void set_comment(const std::string_view sv) noexcept { m_comment = sv; }

    [[nodiscard]] constexpr bool has_comment_predecl() const noexcept { return !m_comment_predecl.empty(); }
    [[nodiscard]] constexpr std::string_view comment_predecl() const noexcept { return m_comment_predecl; }
    constexpr void set_comment_predecl(const std::string_view sv) noexcept { m_comment_predecl = sv; }
};



/////////////////////////////////////////////////////////////////////////////
class Parser final : public plain::ParserBase<char>
{         using inherited = plain::ParserBase<char>;
 public:
    Parser(const std::string_view buf)
      : plain::ParserBase(buf)
       {}

    //-----------------------------------------------------------------------
    [[nodiscard]] Define next_define()
       {
        Define def;

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
                    collect_define(def);
                    break;
                   }
                else
                   {
                    throw create_parse_error( fmt::format("Unexpected content: {}", str::escape(inherited::get_rest_of_line())) );
                   }
               }
           }
        catch(parse_error&)
           {
            throw;
           }
        catch(std::exception& e)
           {
            throw create_parse_error(e.what());
           }

        return def;
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
        [[maybe_unused]] auto cmt = inherited::get_until<'*','/'>();
       }

    //-----------------------------------------------------------------------
    void collect_define_comment( Define& def )
       {// [INT] Descr
        inherited::skip_blanks();
        const std::size_t i_start = inherited::curr_offset(); // Start of overall comment string

        // Detect possible pre-declaration in square brackets like: // [xxx] comment
        if( inherited::got('[') )
           {
            inherited::get_next();
            inherited::skip_blanks();
            const std::size_t i_pre_start = inherited::curr_offset(); // Start of pre-declaration
            std::size_t i_pre_end = i_pre_start; // One-past-end of pre-declaration
            while( true )
               {
                if( not inherited::has_codepoint() or inherited::got_endline() )
                   {
                    notify_issue( fmt::format("Unclosed initial \'[\' in the comment of define {}", def.label()) );
                    def.set_comment( str::trim_right(inherited::get_view_between(i_start, i_pre_end)) );
                    break;
                   }
                else if( inherited::got(']') )
                   {
                    def.set_comment_predecl( str::trim_right(inherited::get_view_between(i_pre_start, i_pre_end)) );
                    inherited::get_next();
                    break;
                   }
                else
                   {
                    inherited::get_next();
                   }
               }
            inherited::skip_blanks();
           }

        // Collect the remaining comment text
        if( not def.has_comment() and i<siz and buf[i]!='\n' )
           {
            const std::size_t i_txt_start = i; // Start of comment text
            std::size_t i_txt_end = i; // One-past-end of comment text
            do {
                if( buf[i]=='\n' )
                   {// Line finished
                    break;
                   }
                else
                   {
                    if( not is_blank(buf[i]) ) i_txt_end = ++i;
                    else ++i;
                   }
               }
            while( i<siz );

            def.set_comment( inherited::get_view_between(i_txt_start, i_txt_end) );
           }
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
        //    notify_issue( fmt::format("Define {} hasn't a comment", def.label()) );
        //   }

        // Expecting a line end here
        if( inherited::got_endline() )
           {
            inherited::get_next();
           }
        else
           {
            throw create_parse_error( fmt::format("Unexpected content after define {}: {}", def.label(), str::escape(inherited::get_rest_of_line())) );
           }
       }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
static ut::suite<"h::Parser"> h_parser_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("basic") = []
   {
    ut::expect( true );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
