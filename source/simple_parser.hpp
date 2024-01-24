#pragma once
//  ---------------------------------------------
//  A map of string pairs
//  ---------------------------------------------
//  #include "simple_parser.hpp" // MG::simple_parser
//  ---------------------------------------------
#include <cctype> // std::isspace, ...
#include <string_view>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
class simpleparser_t
{
 private:
    const std::string_view input;
    std::size_t i = 0;

 public:
    explicit simpleparser_t(const std::string_view sv) noexcept
      : input(sv)
       {}

    [[nodiscard]] bool got_var_name() noexcept
       {
        while( true )
           {
            if( got('%') )
               {// Possible %var_name%
                i_chunkend = pos();
                next();
                if( got_varname_token() and got('%') )
                   {
                    next();
                    return true;
                   }
               }
            else if( got('$') )
               {
                i_chunkend = pos();
                next();
                if( got('{') )
                   {// Possible ${var_name}
                    next();
                    if( got_varname_token() and got('}') )
                       {
                        next();
                        return true;
                       }
                   }
                else if( got_varname_token() )
                   {// Possible $var_name
                    return true;
                   }
               }
            else if( not next() )
               {
                break;
               }
           }
        return false;
       }

 protected:
    [[nodiscard]] std::size_t pos() const noexcept { return i; }
    [[nodiscard]] bool got(const char ch) const noexcept { return i<input.size() and input[i]==ch; }
    [[maybe_unused]] bool next() noexcept { return ++i<input.size(); }
    [[nodiscard]] bool got_identifier_char() const noexcept { return i<input.size() and (std::isalnum(input[i]) or input[i]=='_'); }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"MG::simple_parser"> simple_parser_tests = []
{////////////////////////////////////////////////////////////////////////////

//    ut::test("MG::keyvals") = []
//       {
//        ut::expect( ut::that % true );
//       };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
