#pragma once
//  ---------------------------------------------
//  'OutputStreamable' concept
//  ---------------------------------------------
//  #include "output_streamable_concept.hpp" // MG::OutputStreamable
//  ---------------------------------------------
#include <string_view>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

template<typename T>
concept OutputStreamable =
    requires(T ss, const std::string_view sv, const char ch)
       {
        ss << sv << ch << sv;
       };

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
