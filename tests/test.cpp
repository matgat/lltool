#define BOOST_UT_DISABLE_MODULE
//#include <https://raw.githubusercontent.com/boost-experimental/ut/master/include/boost/ut.hpp>
#include "ut.hpp" // import boost.ut;
namespace ut = boost::ut;
#include "test_facilities.hpp" // test::*

#define TEST_UNITS
#include "system_process.hpp" // sys::*
#include "ascii_predicates.hpp" // ascii::is_*
#include "ascii_simple_parser.hpp" // ascii::simple_parser
#include "string_utilities.hpp" // str::*
#include "globbing.hpp" // MG::glob_match()
#include "file_globbing.hpp" // MG::file_glob()
#include "string_map.hpp" // MG::string_map<>
#include "vectmap.hpp" // MG::vectmap<>
#include "text.hpp" // text::*
#include "text-parser-base.hpp" // text::ParserBase
#include "text-parser-xml.hpp" // text::xml::Parser
#include "project_updater.hpp" // ll::update_project_libraries()
//#include "libraries_converter.hpp" // ll::convert_libraries()

int main() {}
