﻿#pragma once
//  ---------------------------------------------
//  Facility to expand environment variables
//  #include "expand_env_vars.hpp" // sys::expand_env_vars()
//  ---------------------------------------------
#include <string_view>
#include <string>
#include <optional>
#include <cctype> // std::isalnum()
#include <cstdlib> // std::getenv, ::getenv_s()


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

namespace details
   {
    using var_value_t = std::optional<std::string>;
    using f_resolve_var_t = var_value_t (*)(const std::string_view);
   }


//---------------------------------------------------------------------------
[[nodiscard]] details::var_value_t resolve_var_getenv(const std::string_view var_name)
{
    // Unfortunately getenv API requires a null terminated string,
    // so I have to instantiate a std::string
    const std::string var_name_str = std::string(var_name);

  #if defined(MS_WINDOWS)
    // std::getenv deemed unsafe, the Microsoft safe version requires a copy
    // Check if variable is present
    std::size_t val_size_plus_null = 0;
    if( 0==::getenv_s(&val_size_plus_null, nullptr, 0, var_name_str.c_str()) and val_size_plus_null>0 )
       {
        std::string var_value;
        var_value.resize(val_size_plus_null-1);
        if( 0==::getenv_s(&val_size_plus_null, var_value.data(), val_size_plus_null, var_name_str.c_str()) )
           {
            return { std::move(var_value) };
           }
       }
  #else
    if( const char* const val_cstr = std::getenv(var_name_str.c_str()) )
       {
        return { std::string(val_cstr) };
       }
  #endif

    return {};
}


//---------------------------------------------------------------------------
template<details::f_resolve_var_t resolve_var =resolve_var_getenv>
[[nodiscard]] std::string expand_env_vars(const std::string_view input)
{
    std::string output;
    output.reserve( 2 * input.size() ); // An arbitrary guess

    class parser_t final
    {
     private:
        const std::string_view input;
        std::size_t i = 0;
        std::size_t i_chunkstart = 0;
        std::size_t i_chunkend = 0;
        std::size_t i_varstart = 0;
        std::size_t i_varend = 0;

     public:
        explicit parser_t(const std::string_view sv) noexcept
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
                else if( !next() )
                   {
                    break;
                   }
               }
            return false;
           }

        [[nodiscard]] std::string_view chunk_before() const noexcept { return input.substr(i_chunkstart, i_chunkend-i_chunkstart); }
        [[nodiscard]] std::string_view var_name() const noexcept { return input.substr(i_varstart, i_varend-i_varstart); }
        [[nodiscard]] std::string_view remaining_chunk() const noexcept { return input.substr(i_chunkstart); }
        void var_was_substituted() noexcept { i_chunkstart = pos(); }

    private:
        [[nodiscard]] std::size_t pos() const noexcept { return i; }
        [[nodiscard]] bool got(const char ch) const noexcept { return i<input.size() and input[i]==ch; }
        [[maybe_unused]] bool next() noexcept { return ++i<input.size(); }

        [[nodiscard]] bool got_varname_initial_char() const noexcept { return i<input.size() and std::isalnum(input[i]); }
        [[nodiscard]] bool got_varname_char() const noexcept { return i<input.size() and (std::isalnum(input[i]) or input[i]=='_'); }
        [[nodiscard]] bool got_varname_token() noexcept
           {
            i_varstart = pos();
            if( got_varname_initial_char() )
               {
                next();
                while( got_varname_char() ) next();
               }
            i_varend = pos();
            return i_varend>i_varstart;
           }
    } parser(input);

    while( parser.got_var_name() )
       {
        if( const auto val = resolve_var(parser.var_name()) )
           {
            output += parser.chunk_before();
            output += val.value();
            parser.var_was_substituted();
           }
       }
    output += parser.remaining_chunk();

    return output;
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
#include <map>
//---------------------------------------------------------------------------
[[nodiscard]] sys::details::var_value_t resolve_var_test(const std::string_view var_name)
{
    static const std::map<std::string,std::string,std::less<void>> m =
       {
        {"foo", "FOO"},
        {"bar", "BAR"}
       };

    if( const auto it = m.find(var_name); it!=m.end() )
       {
        return { it->second };
       }
    return {};
}
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"sys::expand_env_vars()"> sys_expand_env_vars_tests = []
{////////////////////////////////////////////////////////////////////////////
    using namespace std::literals; // "..."sv
    using ut::expect;
    using ut::that;

    ut::test("no expansions") = []
       {
        expect( that % sys::expand_env_vars<resolve_var_test>(""sv)==""sv );
        expect( that % sys::expand_env_vars<resolve_var_test>("foo"sv)=="foo"sv );
       };

    ut::test("single expansions") = []
       {
        expect( that % sys::expand_env_vars<resolve_var_test>("%foo%"sv)=="FOO"sv );
        expect( that % sys::expand_env_vars<resolve_var_test>("$foo"sv)=="FOO"sv );
        expect( that % sys::expand_env_vars<resolve_var_test>("${foo}"sv)=="FOO"sv );
       };

    ut::test("multiple expansions") = []
       {
        expect( that % sys::expand_env_vars<resolve_var_test>("/%foo%/%bad%/%foo/fo%o/%foo%%bar%/%foo%%bad%/%foo%"sv)=="/FOO/%bad%/%foo/fo%o/FOOBAR/FOO%bad%/FOO"sv );
        expect( that % sys::expand_env_vars<resolve_var_test>("/$foo/$bad/$fooo/fo$o/$foo$bar/$foo$bad/$foo"sv)=="/FOO/$bad/$fooo/fo$o/FOOBAR/FOO$bad/FOO"sv );
        expect( that % sys::expand_env_vars<resolve_var_test>("/${foo}/${bad}/${foo/fo${o/${foo}${bar}/${foo}${bad}/${foo}"sv)=="/FOO/${bad}/${foo/fo${o/FOOBAR/FOO${bad}/FOO"sv );

        expect( that % sys::expand_env_vars<resolve_var_test>("%foo%/foo/$bar/bar"sv)=="FOO/foo/BAR/bar"sv );
        expect( that % sys::expand_env_vars<resolve_var_test>("%bad%/bar/$bad/bar/${bar} /"sv)=="%bad%/bar/$bad/bar/BAR /"sv );
        expect( that % sys::expand_env_vars<resolve_var_test>("fo%o/%foo%/ba$r/bar%bar%$bar${bar}bar"sv)=="fo%o/FOO/ba$r/barBARBARBARbar"sv );
       };

    ut::test("os specific variable") = []
       {
      #include "os-detect.hpp" // MS_WINDOWS, POSIX
      #if defined(MS_WINDOWS)
        auto tolower = [](std::string s) constexpr -> std::string
           {
            for(char& c : s) c = static_cast<char>(std::tolower(c));
            return s;
           };
        expect( that % tolower(sys::expand_env_vars("%WINDIR%-typical"sv))=="c:\\windows-typical"sv);
      #elif defined(POSIX)
        expect( that % sys::expand_env_vars("$SHELL-typical"sv)=="/bin/bash-typical"sv);
      #endif
       };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
