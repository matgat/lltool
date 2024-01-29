#define BOOST_UT_DISABLE_MODULE
//#include <https://raw.githubusercontent.com/boost-experimental/ut/master/include/boost/ut.hpp>
#include "ut.hpp" // import boost.ut;
namespace ut = boost::ut;

#define TEST_UNITS
#include "test_facilities.hpp" // test::*

#include "ascii_predicates.hpp" // ascii::is_*
#include "ascii_simple_parser.hpp" // ascii::simple_parser
#include "expand_env_vars.hpp" // sys::expand_env_vars()
#include "system_process.hpp" // sys::*
#include "string_utilities.hpp" // str::*
#include "globbing.hpp" // MG::glob_match()
#include "file_globbing.hpp" // MG::file_glob()
#include "vectmap.hpp" // MG::vectmap<>
#include "string_map.hpp" // MG::string_map<>
#include "keyvals.hpp" // MG::keyvals
#include "text.hpp" // text::*
#include "text-parser-base.hpp" // text::ParserBase
#include "text-parser-xml.hpp" // text::xml::Parser
#include "project_updater.hpp" // ll::update_project_libraries()
#include "libraries_converter.hpp" // ll::convert_libraries()

int main()
{
    ut::expect(true);
}
