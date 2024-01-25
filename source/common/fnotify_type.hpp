#pragma once
//  ---------------------------------------------
//  Abstraction of a callable that takes
//  temporary string
//  ---------------------------------------------
//  #include "fnotify_type.hpp" // fnotify_t
//  ---------------------------------------------
#include <functional> // std::function

using fnotify_t = std::function<void(std::string&&)>;
