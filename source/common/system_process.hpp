#pragma once
//  ---------------------------------------------
//  System facilities to launch processes
//  ---------------------------------------------
//  #include "system_process.hpp" // sys::*
//  ---------------------------------------------
#include <concepts> // std::convertible_to
//#include <cstdio> // std::popen
#include <stdexcept> // std::runtime_error
#include <format>

#include "string_utilities.hpp" // str::join_left
#include "system_base.hpp" // sys::*

#if defined(MS_WINDOWS)
  #include <cstdlib> // ::_putenv_s (mswin)
  #include <shellapi.h> // ShellExecuteExA, FindExecutableA
#elif defined(POSIX)
  #include <unistd.h> // exec*, fork, ...
  #include <sys/wait.h> // waitpid
#endif


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

#if defined(MS_WINDOWS)

//---------------------------------------------------------------------------
[[nodiscard]] std::string find_executable_by_file(const std::string& doc) noexcept
{
    char buf[MAX_PATH + 1] = {'\0'};
    ::FindExecutableA(doc.c_str(), NULL, buf);
    return std::string(buf);
}

//---------------------------------------------------------------------------
void shell_execute(const char* const pth, const std::initializer_list<std::string_view> args ={}) noexcept
{
    const std::string joined_args = str::join_left(' ', args);

    SHELLEXECUTEINFOA ShExecInfo = {0};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    ShExecInfo.fMask = 0;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = "open";
    ShExecInfo.lpFile = pth;
    ShExecInfo.lpParameters = joined_args.c_str();
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;
    ::ShellExecuteExA(&ShExecInfo);
}

//---------------------------------------------------------------------------
[[maybe_unused]] int shell_execute_wait(const char* const pth, const std::initializer_list<std::string_view> args ={})
{
    const bool show = true;
    const bool wait = true;
    const std::string joined_args = str::join_left(' ', args);

    SHELLEXECUTEINFOA ShExecInfo = {0};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = (wait ? SEE_MASK_NOCLOSEPROCESS : 0) // SEE_MASK_DEFAULT
                       // | SEE_MASK_FLAG_NO_UI // Do not show error dialog in case of exe not found
                       // | SEE_MASK_DOENVSUBST // Substitute environment vars
                       // cppcheck-suppress badBitmaskCheck
                       | (show ? 0 : SEE_MASK_FLAG_NO_UI);
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpFile = pth;
    ShExecInfo.lpParameters = joined_args.c_str();
    ShExecInfo.lpDirectory = NULL; // base_dir.empty() ? NULL : base_dir.c_str();
    ShExecInfo.nShow = show ? SW_SHOW : SW_HIDE;
    ShExecInfo.hInstApp = NULL;
    if( not ::ShellExecuteExA(&ShExecInfo) )
       {
        throw std::runtime_error{ std::format("Cannot run {}: {}", pth, sys::get_lasterr_msg()) };
       }

    DWORD ret = 0xFFFFFFFF; // A default value to be interpreted as "no exit code"

    if( wait )
       {
        do {
            const DWORD WaitResult = ::WaitForSingleObject(ShExecInfo.hProcess, 500);
            if( WaitResult==WAIT_TIMEOUT )
               {// Still executing...
                ::Sleep(100);
               }
            else if( WaitResult==WAIT_OBJECT_0 )
               {
                break;
               }
            else
               {
                ::CloseHandle(ShExecInfo.hProcess); // SEE_MASK_NOCLOSEPROCESS
                throw std::runtime_error{ std::format("ShellExecuteEx: spawn error of {}", pth) };
               }
           }
        while(true);

        // If here process ended
        ::GetExitCodeProcess(ShExecInfo.hProcess, &ret);
        ::CloseHandle(ShExecInfo.hProcess); // SEE_MASK_NOCLOSEPROCESS
       }

    return ret;
}

#endif // MS_WINDOWS


//---------------------------------------------------------------------------
// Same effect as double click on a file
void launch_file(const std::string& pth) noexcept
{
  #if defined(MS_WINDOWS)
    shell_execute( pth.c_str() );
  #elif defined(POSIX)
    if( const auto pid=fork(); pid==0 ) // pid_t
       {
        execlp("xdg-open", "xdg-open", pth.c_str(), nullptr);
       }
  #endif
}


//---------------------------------------------------------------------------
#if defined(MS_WINDOWS)
template<std::convertible_to<std::string_view> ...Args>
#elif defined(POSIX) // Needs null terminated strings
template<std::convertible_to<std::string> ...Args>
#endif
void execute(const char* const exe, Args&&... args) noexcept
{
  #if defined(MS_WINDOWS)
    shell_execute( exe, { std::string_view{std::forward<Args>(args)}... } );
  #elif defined(POSIX)
    if( const auto pid=fork(); pid==0 ) // pid_t
       {
        struct loc final
           {// Extract char pointer for posix api exec*
            static const char* c_str(const char* const s) noexcept { return s; }
            static const char* c_str(const std::string& s) noexcept { return s.c_str(); }
           };
        execlp(exe, exe, loc::c_str(std::forward<Args>(args))..., nullptr);
       }
  #endif
}


//---------------------------------------------------------------------------
//[[maybe_unused]] int execute_wait(const char* const cmd)
//{
//    std::FILE* const pipe = std::popen(cmd, "r");
//    if( std::FILE* const pipe = std::popen(cmd, "r") )
//       {
//        return WEXITSTATUS(std::pclose(pipe));
//       }
//    return -1;
//}


//---------------------------------------------------------------------------
#if defined(MS_WINDOWS)
template<std::convertible_to<std::string_view> ...Args>
#elif defined(POSIX) // Needs null terminated strings
template<std::convertible_to<std::string> ...Args>
#endif
[[maybe_unused]] int execute_wait(const char* const exe, Args&&... args) noexcept
{
  #if defined(MS_WINDOWS)
    try{
        return shell_execute_wait( exe, { std::string_view{std::forward<Args>(args)}... } );
       }
    catch(...){}
  #elif defined(POSIX)
    const auto pid_child = fork(); // pid_t
    if( pid_child==0 )
       {// Fork successful, inside child process
        struct loc final
           {// Extract char pointer for posix api exec*
            static const char* c_str(const char* const s) noexcept { return s; }
            static const char* c_str(const std::string& s) noexcept { return s.c_str(); }
           };
        execlp(exe, exe, loc::c_str(std::forward<Args>(args))..., nullptr);
       }
    else if( pid_child!=-1 )
       {// Fork successful, inside parent process: wait child
        pid_t pid;
        int status;
        do {
            pid = waitpid(pid_child, &status, WUNTRACED | WCONTINUED);
            if(pid==-1) return -2; // waitpid error
            else if( WIFEXITED(status) ) return WEXITSTATUS(status); // Exited, return result
            else if( WIFSIGNALED(status) ) return -1; // Killed by WTERMSIG(status)
            else if( WIFSTOPPED(status) ) return -1; // Stopped by WSTOPSIG(status)
            //else if( WIFCONTINUED(status) ) continue;
           }
        while( not WIFEXITED(status) and !WIFSIGNALED(status) );
        //if(pid != pid_child) // Failed: Child process vanished
       }
    //else // Fork failed
  #endif
    return -3; // Something failed
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"system_process"> system_process_tests = []
{////////////////////////////////////////////////////////////////////////////

//ut::test("sys::xxx()") = []
//   {
//    ut::expect( ut::that % true );
//   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
