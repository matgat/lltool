#pragma once
//  ---------------------------------------------
//  Set some unified preprocessor macros
//  depending on host operating system
//  ---------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
  #define MS_WINDOWS 1
  #undef POSIX
#elif defined(__unix__) || defined(__linux__)
  #undef MS_WINDOWS
  #define POSIX 1
#else
  #undef MS_WINDOWS
  #undef POSIX
#endif
