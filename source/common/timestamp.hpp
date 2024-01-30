#pragma once
//  ---------------------------------------------
//  Returns a human readable timestamp
//  ---------------------------------------------
//  #include "timestamp.hpp" // MG::get_human_readable_timestamp()
//  ---------------------------------------------
#include <string>
#include <chrono>

#include <fmt/format.h> 
#include <fmt/chrono.h>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
[[nodiscard]] std::string get_human_readable_timestamp()
{
    //return fmt::format("{}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    return fmt::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
