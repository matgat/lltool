#pragma once
//  ---------------------------------------------
//  Parses a file containing a list of c-like
//  preprocessor defines
//  ---------------------------------------------
//  #include "h_parser.hpp" // h::Parser
//  ---------------------------------------------

#include "plain_parser_base.hpp" // plain::ParserBase


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
    [[nodiscard]] operator bool() const noexcept { return !m_value.empty(); }

    [[nodiscard]] std::string_view label() const noexcept { return m_label; }
    void set_label(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error("Empty define label");
           }
        m_label = sv;
       }

    [[nodiscard]] std::string_view value() const noexcept { return m_value; }
    void set_value(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error("Empty define value");
           }
        m_value = sv;
       }

    [[nodiscard]] bool value_is_number() const noexcept
       {
        double result;
        const auto i_end = m_value.data() + m_value.size();
        auto [i, ec] = std::from_chars(m_value.data(), i_end, result);
        return ec==std::errc() and i==i_end;
       }

    [[nodiscard]] bool has_comment() const noexcept { return !m_comment.empty(); }
    [[nodiscard]] std::string_view comment() const noexcept { return m_comment; }
    void set_comment(const std::string_view sv) noexcept { m_comment = sv; }

    [[nodiscard]] bool has_comment_predecl() const noexcept { return !m_comment_predecl.empty(); }
    [[nodiscard]] std::string_view comment_predecl() const noexcept { return m_comment_predecl; }
    void set_comment_predecl(const std::string_view sv) noexcept { m_comment_predecl = sv; }


};



/////////////////////////////////////////////////////////////////////////////
class Parser final : public plain::ParserBase
{
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
                if( i>=siz )
                   {// No more data!
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
                else if( inherited::eat_line_end() )
                   {// An empty line
                   }
                else if( inherited::eat_token("#define"sv) )
                   {
                    collect_define(def);
                    break;
                   }
                else
                   {
                    notify_error("Unexpected content: {}", str::escape(inherited::get_rest_of_line()));
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
        if( i<(siz-1) and buf[i]=='/' and buf[i+1]=='/' )
           {
            i += 2; // Skip "//"
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat_block_comment_start() noexcept
       {
        if( i<(siz-1) and buf[i]=='/' and buf[i+1]=='*' )
           {
            i += 2; // Skip "/*"
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    void skip_block_comment()
       {
        const std::size_t line_start = line; // Store current line
        const std::size_t i_start = i; // Store current position
        while( i<i_last )
           {
            if( buf[i]=='*' and buf[i+1]=='/' )
               {
                i += 2; // Skip "*/"
                return;
               }
            else if( buf[i]=='\n' )
               {
                ++line;
               }
            ++i;
           }
        throw create_parse_error("Unclosed block comment", line_start, i_start);
       }


    //-----------------------------------------------------------------------
    void collect_define( Define& def )
       {// LABEL       0  // [INT] Descr
        // vnName     vn1782  // descr [unit]

        // Contract: '#define' already eat

        // [Label]
        inherited::skip_blanks();
        def.set_label( collect_identifier() );
        // [Value]
        inherited::skip_blanks();
        def.set_value( collect_token() );
        // [Comment]
        inherited::skip_blanks();
        if( eat_line_comment_start() and i<siz )
           {
            inherited::skip_blanks();
            const std::size_t i_start = i; // Start of overall comment string

            // Detect possible pre-declarator in square brackets like: // [xxx] comment
            if( i<siz and buf[i]=='[' )
               {
                ++i; // Skip '['
                inherited::skip_blanks();
                const std::size_t i_pre_start = i; // Start of pre-declaration
                std::size_t i_pre_end = i; // One-past-end of pre-declaration
                while( true )
                   {
                    if( i>=siz or buf[i]=='\n' )
                       {
                        notify_error("Unclosed initial \'[\' in the comment of define {}", def.label());
                        def.set_comment( std::string_view(buf+i_start, i_pre_end-i_start) );
                        break;
                       }
                    else if( buf[i]==']' )
                       {
                        def.set_comment_predecl( std::string_view(buf+i_pre_start, i_pre_end-i_pre_start) );
                        ++i; // Skip ']'
                        break;
                       }
                    else
                       {
                        if( not is_blank(buf[i]) ) i_pre_end = ++i;
                        else ++i;
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

                def.set_comment( std::string_view(buf+i_txt_start, i_txt_end-i_txt_start) );
               }
           }
        //else if(fussy)
        //   {
        //    notify_error("Define {} hasn't a comment", def.label());
        //   }

        // Expecting a line end here
        if( not inherited::eat_line_end() )
           {
            notify_error("Unexpected content after define {}: {}", def.label(), str::escape(inherited::get_rest_of_line()));
           }

        //DLOG2("  [*] Collected define: label=\"{}\" value=\"{}\" comment=\"{}\"\n", def.label(), def.value(), def.comment())
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
