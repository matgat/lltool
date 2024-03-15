#pragma once
//  ---------------------------------------------
//  Open a text file with external editor
//  ---------------------------------------------
//  #include "edit_text_file.hpp" // sys::edit_text_file()
//  ---------------------------------------------
#include <format>
#include "system_process.hpp" // sys::*


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
void edit_text_file(const std::string& pth, const std::size_t line =1u) noexcept
{
    if( pth.empty() ) return;

  #ifdef MS_WINDOWS
    [[maybe_unused]] static bool once = [](){ sys::add_to_path_expanding_vars({ "%ProgramFiles%\\notepad++",
                                                                                "%UserProfile%\\Apps\\npp",
                                                                                "C:\\Macotec\\Apps\\Notepad++"}); return true; }();
    sys::shell_execute("notepad++.exe", {"-nosession", std::format("-n{}", line), pth});
  #else
    sys::execute("mousepad", std::format("--line={}",line), pth);
  #endif
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
