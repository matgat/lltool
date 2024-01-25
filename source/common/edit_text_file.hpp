#pragma once
//  ---------------------------------------------
//  Edit a text file
//  #include "edit_text_file.hpp" // sys::edit_text_file()
//  ---------------------------------------------
#include "system_process.hpp" // sys::*


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
void edit_text_file(const std::string& pth, const std::size_t line) noexcept
{
    if( pth.empty() ) return;

  #ifdef MS_WINDOWS
    [[maybe_unused]] static bool once = [](){ sys::add_to_path_expanding_vars({ "%UserProfile%\\Apps\\npp", "%ProgramFiles%\\notepad++"}); return true; }();
    sys::shell_execute("notepad++.exe", {"-nosession", fmt::format("-n{}", line), pth});
  #else
    sys::execute("mousepad", fmt::format("--line={}",line), pth);
  #endif
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
