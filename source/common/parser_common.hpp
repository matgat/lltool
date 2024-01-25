#pragma once
//  ---------------------------------------------
//  Common definitions used by parsers
//  ---------------------------------------------
//  #include "parser_common.hpp" // parse::error, parse::flags
//  ---------------------------------------------
#include <cstdint> // std::uint8_t
#include <stdexcept> // std::exception, std::runtime_error
#include <string>
#include <string_view>

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace parse //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class error final : public std::exception
{
 private:
    std::string m_msg;
    std::string m_file;
    std::size_t m_line;

 public:
    explicit error(std::string&& msg, const std::string_view fil, const std::size_t lin) noexcept
       : m_msg{std::move(msg)}
       , m_file{fil}
       , m_line{lin}
        {}

    std::string const& file() const noexcept { return m_file; }
    std::size_t line() const noexcept { return m_line; }

    char const* what() const noexcept override { return m_msg.c_str(); }
};


/////////////////////////////////////////////////////////////////////////////
// parse_function(..., const parse::flags flags =parse::flag::NONE)
// if( flags & parse::flag::SKIP_STOPPER ) ...
//using flags = std::uint8_t;
//namespace flag
//{
//    enum : flags
//    {
//       NONE = 0x0
//      ,SKIP_STOPPER = 0x1 // Skip the codepoint that ended the collect
//       //,RESERVED = 0x2 // Reserved
//       //,RESERVED = 0x4 // Reserved
//       //,RESERVED = 0x8 // Reserved
//    };
//}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
