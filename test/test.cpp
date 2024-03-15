#define BOOST_UT_DISABLE_MODULE
//#include <https://raw.githubusercontent.com/boost-ext/ut/master/include/boost/ut.hpp>
#include "ut.hpp" // import boost.ut;
namespace ut = boost::ut;

#define TEST_UNITS
#include "test_facilities.hpp" // test::*

// The includes in main.cpp:
#include "arguments.hpp"
#include "issues_collector.hpp"
#include "edit_text_file.hpp"
#include "project_updater.hpp"
#include "libraries_converter.hpp"

int main()
{
    ut::expect(true);
}
