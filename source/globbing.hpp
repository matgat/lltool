#pragma once
//  ---------------------------------------------
//  Match glob patterns
//  ---------------------------------------------
//  #include "globbing.hpp" // MG::glob_match()
//  ---------------------------------------------
#include <string_view>
using namespace std::literals; // "..."sv

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
[[nodiscard]] constexpr bool contains_wildcards(const std::string_view sv) noexcept
{
    return sv.find_first_of("*?"sv)!=std::string_view::npos;
}

//-----------------------------------------------------------------------
// Returns true if text matches glob-like pattern with wildcards (*, ?)
[[nodiscard]] bool glob_match(const std::string_view text, const std::string_view glob, const char dont_match ='\0') noexcept
{
    // 'dont_match': character not matched by any wildcards
    std::size_t i_text=0, i_text_match_asterisk=text.size();
    std::size_t i_glob=0, i_glob_after_asterisk=std::string_view::npos;
    while( i_text<text.size() )
       {
        if( i_glob<glob.size() and glob[i_glob]=='*' )
           {// new '*'-loop: backup positions in pattern and text
            i_text_match_asterisk = i_text;
            i_glob_after_asterisk = ++i_glob;
           }
        else if( i_glob<glob.size() and (glob[i_glob]==text[i_text] or (glob[i_glob]=='?' and text[i_text]!=dont_match)) )
           {// Character matched
            ++i_text;
            ++i_glob;
           }
        else if( i_glob_after_asterisk==std::string_view::npos or (i_text_match_asterisk<text.size() and text[i_text_match_asterisk]==dont_match) )
           {// No match
            return false;
           }
        else
           {// '*'-loop: backtrack after the last '*'
            if( i_text_match_asterisk<text.size() )
               {
                i_text = ++i_text_match_asterisk;
               }
            i_glob = i_glob_after_asterisk;
           }
       }

    // Ignore trailing asterisks
    while(i_glob<glob.size() and glob[i_glob]=='*') ++i_glob;

    // At end of text means success if nothing else is left to match
    return i_glob>=glob.size();
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
static ut::suite<"globbing"> globbing_tests = []
{////////////////////////////////////////////////////////////////////////////

    ut::test("MG::contains_wildcards()") = []
       {
        ut::expect( not MG::contains_wildcards("abcde"sv) );
        ut::expect( not MG::contains_wildcards(""sv) );
        ut::expect( MG::contains_wildcards("ab*de"sv) );
        ut::expect( MG::contains_wildcards("ab?de"sv) );
        ut::expect( MG::contains_wildcards("*"sv) );
        ut::expect( MG::contains_wildcards("?"sv) );
        ut::expect( MG::contains_wildcards("?*"sv) );
       };

    ut::test("MG::glob_match()") = []
       {
        ut::expect( not MG::glob_match("abcd 1234"sv, ""sv) );
        ut::expect( not MG::glob_match("abcd 1234"sv, "?"sv) );
        ut::expect( MG::glob_match("abcd 1234"sv, "*"sv) );
        ut::expect( MG::glob_match("abcd 1234"sv, "ab*"sv) );
        ut::expect( MG::glob_match("abcd 1234"sv, "a?c*"sv) );
        ut::expect( not MG::glob_match("abcd 1234"sv, "a?d*"sv) );
        ut::expect( MG::glob_match("abcd 1234"sv, "a??d*"sv) );
        ut::expect( MG::glob_match("abcd 1234"sv, "a??d 1234"sv) );
        ut::expect( MG::glob_match("abcd 1234"sv, "a??d 123*"sv) );
        ut::expect( MG::glob_match("abcd 1234"sv, "a??d 1234*"sv) );
        ut::expect( not MG::glob_match("abcd 1234"sv, "a?d 1234*"sv) );
       };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
