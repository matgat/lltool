#pragma once
//  ---------------------------------------------
//  Until there will be
//  int main( std::span<std::string_view> args )
//  ---------------------------------------------
//  #include "args_extractor.hpp" // MG::args_extractor
//  ---------------------------------------------
#include <stdexcept> // std::invalid_argument
#include <cassert>
#include <string_view>
#include <functional> // std::function

#include <fmt/format.h> // fmt::*


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

template<typename T>
concept InvokableStrChr = requires(T t, std::string_view sv, char c) { { t(sv, c) } -> std::same_as<void>; };


/////////////////////////////////////////////////////////////////////////////
class args_extractor final
{
    const char* const * const m_argv;
    const int m_argc;
    int m_curridx = 1; // argv[0] is the executable path

 public:
    args_extractor(const int argc, const char* const argv[]) noexcept
      : m_argv(argv)
      , m_argc(argc)
       {}

    void next() noexcept
       {
        ++m_curridx;
       }

    [[nodiscard]] bool has_current() const noexcept
       {
        return m_curridx<m_argc;
       }

    [[nodiscard]] std::string_view current() const noexcept
       {
        assert( has_current() );
        return std::string_view{ m_argv[m_curridx] };
       }

    [[nodiscard]] static bool is_switch(const std::string_view arg) noexcept
       {
        return arg.size()>1 and arg[0]=='-';
       }

    [[nodiscard]] static std::size_t get_switch_prefix_size(const std::string_view arg) noexcept
       {
        assert( is_switch(arg) );
        return arg[1]=='-' ? 2 : 1;
       }

    //-----------------------------------------------------------------------
    std::function <void(const std::string_view, const char)> apply_switch_by_name_or_char;
    void apply_switch(std::string_view arg)
       {
        const std::size_t switch_prfx_size = get_switch_prefix_size(arg);
        arg.remove_prefix(switch_prfx_size);
        if( switch_prfx_size==1 )
           {// Single char, possibly more than one
            for(const char ch : arg )
               {
                apply_switch_by_name_or_char(""sv, ch);
               }
           }
        else
           {// Full name
            apply_switch_by_name_or_char(arg, '\0');
           }
       }

    //-----------------------------------------------------------------------
    std::string_view get_next_value_of( const std::string_view arg )
       {
        next(); // Expecting a string next
        if( not has_current() )
           {
            throw std::invalid_argument( fmt::format("Missing value after {}", arg) );
           }
        std::string_view str = current();
        if( is_switch(str) )
           {
            throw std::invalid_argument( fmt::format("Missing value after {} before {}", arg, str) );
           }
        return str;
       }
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
