#define BOOST_UT_DISABLE_MODULE
//#include <https://raw.githubusercontent.com/boost-experimental/ut/master/include/boost/ut.hpp>
#include "ut.hpp" // import boost.ut;
namespace ut = boost::ut;
#include "test_facilities.hpp" // test::*

#define TEST_UNITS // Include units embedded tests
#include "system_process.hpp" // sys::*
#include "string_utilities.hpp" // str::*
#include "string_map.hpp" // MG::string_map<>
#include "text.hpp" // text::*
#include "parser-base.hpp" // MG::ParserBase
#include "parser-xml.hpp" // xml::Parser
#include "project_updater.hpp" // ll::update_project_libraries()

int main() {}
