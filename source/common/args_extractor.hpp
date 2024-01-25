#pragma once
//  ---------------------------------------------
//  Until there will be
//  int main( std::span<std::string_view> args )
//  ---------------------------------------------
//  #include "args_extractor.hpp" // MG::args_extractor
//  ---------------------------------------------
#include <cassert>
#include <string_view>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

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
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
