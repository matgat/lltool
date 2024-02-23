#pragma once
//  ---------------------------------------------
//  ANSI escape codes for terminal output
//  ---------------------------------------------
//  #include "ansi_escape_codes.hpp" // ANSI_RED, ...
//  ---------------------------------------------

//#define ANSI_RESET        "\\033[0m"  // Reset all
//#define ANSI_RESETBOLD    "\\033[21m" // Reset Bold
//#define ANSI_RESETDIM     "\\033[22m" // Reset Dim
//#define ANSI_RESETUNDER   "\\033[24m" // Reset Underlined
//#define ANSI_RESETBLINK   "\\033[25m" // Reset Blink
//#define ANSI_RESETREVERSE "\\033[27m" // Reset Reverse
//#define ANSI_RESETHIDDEN  "\\033[28m" // Reset Hidden

// Formatting
//#define ANSI_BOLD     "\\033[1m" // Bold
//#define ANSI_DIM      "\\033[2m" // Dim
//#define ANSI_UNDER    "\\033[4m" // Underlined
//#define ANSI_BLINK    "\\033[5m" // Blink
//#define ANSI_REVERSE  "\\033[7m" // Reverse
//#define ANSI_HIDDEN   "\\033[8m" // Hidden

// Foreground colors
#define ANSI_DEFAULT  "\\033[39m" // Default color
//#define ANSI_BLACK    "\\033[30m" // Black
#define ANSI_RED      "\\033[31m" // Red
//#define ANSI_GREEN    "\\033[32m" // Green
//#define ANSI_YELLOW   "\\033[33m" // Yellow
#define ANSI_BLUE     "\\033[34m" // Blue
#define ANSI_MAGENTA  "\\033[35m" // Magenta
#define ANSI_CYAN     "\\033[36m" // Cyan
//#define ANSI_LGRAY    "\\033[37m" // Light Gray
//#define ANSI_DGRAY    "\\033[90m" // Dark Gray
//#define ANSI_LRED     "\\033[91m" // Light Red
//#define ANSI_LGREEN   "\\033[92m" // Light Green
//#define ANSI_LYELLOW  "\\033[93m" // Light Yellow
//#define ANSI_LBLUE    "\\033[94m" // Light Blue
//#define ANSI_LMAGENTA "\\033[95m" // Light Magenta
//#define ANSI_LCYAN    "\\033[96m" // Light Cyan
//#define ANSI_WHITE    "\\033[97m" // White

// Background colors
//#define ANSI_BKGDEFAULT  "\\033[49m"  // Default Background Color
//#define ANSI_BKGBLACK    "\\033[40m"  // Background Black
//#define ANSI_BKGRED      "\\033[41m"  // Background Red
//#define ANSI_BKGGREEN    "\\033[42m"  // Background Green
//#define ANSI_BKGYELLOW   "\\033[43m"  // Background Yellow
//#define ANSI_BKGBLUE     "\\033[44m"  // Background Blue
//#define ANSI_BKGMAGENTA  "\\033[45m"  // Background Magenta
//#define ANSI_BKGCYAN     "\\033[46m"  // Background Cyan
//#define ANSI_BKGLGRAY    "\\033[47m"  // Background LightGray
//#define ANSI_BKGDGRAY    "\\033[100m" // Background DarkGray
//#define ANSI_BKGLRED     "\\033[101m" // Background LightRed
//#define ANSI_BKGLGREEN   "\\033[102m" // Background LightGreen
//#define ANSI_BKGLYELLOW  "\\033[103m" // Background LightYellow
//#define ANSI_BKGLBLUE    "\\033[104m" // Background LightBlue
//#define ANSI_BKGLMAGENTA "\\033[105m" // Background LightMagenta
//#define ANSI_BKGLCYAN    "\\033[106m" // Background LightCyan
//#define ANSI_BKGWHITE    "\\033[107m" // Background White


/*

#include <string_view>
using namespace std::literals; // "..."sv

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace ansi
{

inline static constexpr std::string_view Reset        = "\\033[0m"sv;  // Reset all
inline static constexpr std::string_view ResetBold    = "\\033[21m"sv; // Reset Bold
inline static constexpr std::string_view ResetDim     = "\\033[22m"sv; // Reset Dim
inline static constexpr std::string_view ResetUnder   = "\\033[24m"sv; // Reset Underlined
inline static constexpr std::string_view ResetBlink   = "\\033[25m"sv; // Reset Blink
inline static constexpr std::string_view ResetReverse = "\\033[27m"sv; // Reset Reverse
inline static constexpr std::string_view ResetHidden  = "\\033[28m"sv; // Reset Hidden

// Formatting
inline static constexpr std::string_view Bold     = "\\033[1m"sv; // Bold
inline static constexpr std::string_view Dim      = "\\033[2m"sv; // Dim
inline static constexpr std::string_view Under    = "\\033[4m"sv; // Underlined
inline static constexpr std::string_view Blink    = "\\033[5m"sv; // Blink
inline static constexpr std::string_view Reverse  = "\\033[7m"sv; // Reverse
inline static constexpr std::string_view Hidden   = "\\033[8m"sv; // Hidden

// Foreground colors
inline static constexpr std::string_view Default  = "\\033[39m"sv; // Default
inline static constexpr std::string_view Black    = "\\033[30m"sv; // Black
inline static constexpr std::string_view Red      = "\\033[31m"sv; // Red
inline static constexpr std::string_view Green    = "\\033[32m"sv; // Green
inline static constexpr std::string_view Yellow   = "\\033[33m"sv; // Yellow
inline static constexpr std::string_view Blue     = "\\033[34m"sv; // Blue
inline static constexpr std::string_view Magenta  = "\\033[35m"sv; // Magenta
inline static constexpr std::string_view Cyan     = "\\033[36m"sv; // Cyan
inline static constexpr std::string_view LGray    = "\\033[37m"sv; // Light Gray
inline static constexpr std::string_view DGray    = "\\033[90m"sv; // Dark Gray
inline static constexpr std::string_view LRed     = "\\033[91m"sv; // Light Red
inline static constexpr std::string_view LGreen   = "\\033[92m"sv; // Light Green
inline static constexpr std::string_view LYellow  = "\\033[93m"sv; // Light Yellow
inline static constexpr std::string_view LBlue    = "\\033[94m"sv; // Light Blue
inline static constexpr std::string_view LMagenta = "\\033[95m"sv; // Light Magenta
inline static constexpr std::string_view LCyan    = "\\033[96m"sv; // Light Cyan
inline static constexpr std::string_view White    = "\\033[97m"sv; // White

// Background colors
inline static constexpr std::string_view BkgDefault  = "\\033[49m"sv;  // Background Default
inline static constexpr std::string_view BkgBlack    = "\\033[40m"sv;  // Background Black
inline static constexpr std::string_view BkgRed      = "\\033[41m"sv;  // Background Red
inline static constexpr std::string_view BkgGreen    = "\\033[42m"sv;  // Background Green
inline static constexpr std::string_view BkgYellow   = "\\033[43m"sv;  // Background Yellow
inline static constexpr std::string_view BkgBlue     = "\\033[44m"sv;  // Background Blue
inline static constexpr std::string_view BkgMagenta  = "\\033[45m"sv;  // Background Magenta
inline static constexpr std::string_view BkgCyan     = "\\033[46m"sv;  // Background Cyan
inline static constexpr std::string_view BkgLGray    = "\\033[47m"sv;  // Background LightGray
inline static constexpr std::string_view BkgDGray    = "\\033[100m"sv; // Background DarkGray
inline static constexpr std::string_view BkgLRed     = "\\033[101m"sv; // Background LightRed
inline static constexpr std::string_view BkgLGreen   = "\\033[102m"sv; // Background LightGreen
inline static constexpr std::string_view BkgLYellow  = "\\033[103m"sv; // Background LightYellow
inline static constexpr std::string_view BkgLBlue    = "\\033[104m"sv; // Background LightBlue
inline static constexpr std::string_view BkgLMagenta = "\\033[105m"sv; // Background LightMagenta
inline static constexpr std::string_view BkgLCyan    = "\\033[106m"sv; // Background LightCyan
inline static constexpr std::string_view BkgWhite    = "\\033[107m"sv; // Background White

}//:::::::::::::::::::::::::::::::::: ansi ::::::::::::::::::::::::::::::::::

*/
