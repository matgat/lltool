#pragma once
//  ---------------------------------------------
//  Basic system facilities
//  #include "system_base.hpp" // sys::*
//  ---------------------------------------------
#include <string>
#include <string_view>

#include "os-detect.hpp" // MS_WINDOWS, POSIX
#include "string_utilities.hpp" // str::join_left
#include "expand_env_vars.hpp" // sys::expand_env_vars()

#if defined(MS_WINDOWS)
  #include <cstdlib> // ::_putenv_s (mswin)
  #include <Windows.h> // DWORD, ...
#elif defined(POSIX)
  //#include <unistd.h> // ...
#endif


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

#if defined(MS_WINDOWS)

//---------------------------------------------------------------------------
void add_to_path(std::initializer_list<std::string_view> folders) noexcept
{
    const std::string folder_list = str::join_left(';', folders);
    ::_putenv_s("PATH", folder_list.c_str());
}

//---------------------------------------------------------------------------
void add_to_path_expanding_vars(std::initializer_list<std::string_view> folders) noexcept
{
    const std::string folder_list = str::join_left(';', folders);
    ::_putenv_s("PATH", sys::expand_env_vars(folder_list).c_str());
}

// DWORD dwErrVal = GetLastError();
// std::error_code ec (dwErrVal, std::system_category());
// throw std::system_error(ec, "Exception occurred");

//---------------------------------------------------------------------------
// Format system error message
[[nodiscard]] std::string get_lasterr_msg(DWORD e =0) noexcept
{
    //#include <system_error>
    //std::string message = std::system_category().message(e);

    if(e==0) e = ::GetLastError(); // ::WSAGetLastError()
    const DWORD buf_siz = 1024;
    TCHAR buf[buf_siz];
    const DWORD siz =
        ::FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_IGNORE_INSERTS|
                         FORMAT_MESSAGE_MAX_WIDTH_MASK,
                         nullptr,
                         e,
                         0, // MAKELANGID deprecated
                         buf,
                         buf_siz,
                         nullptr );

    return siz>0 ? std::string(buf, siz)
                 : std::string("Unknown error ") + std::to_string(e);
}


#endif // MS_WINDOWS



}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"system_base"> system_base_tests = []
{////////////////////////////////////////////////////////////////////////////
//    using namespace std::literals; // "..."sv
//    using ut::expect;
//    using ut::that;
//
//    ut::test("sys::xxx()") = []
//       {
//        expect( that % true );
//       };
};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
