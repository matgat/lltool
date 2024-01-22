#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
//#include <https://raw.githubusercontent.com/doctest/doctest/master/doctest/doctest.h>
#include "doctest.h"
#include "test_facilities.hpp" // test::*

#define TEST_UNITS // Include units embedded tests
//#include "system_process.hpp" // sys::*
//#include "string_utilities.hpp" // str::*
//#include "string_map.hpp" // MG::string_map<>
//#include "text.hpp" // text::*
//#include "parser-base.hpp" // MG::ParserBase
//#include "parser-xml.hpp" // xml::Parser
//#include "project_updater.hpp" // ll::update_project_libraries()



// Integration tests if got the executable path?
const fs::path exe_path{ "lltool.exe" };
//WARN_MESSAGE(fs::exists(exe_path), "Program not found for integration tests\n");
TEST_SUITE("system tests"
           * doctest::skip(not fs::exists(exe_path)))
   {
    TEST_CASE("Calling with no arguments")
       {
        CHECK_MESSAGE(test::execute_wait(exe_path, {"-q"}) == 2, "...Should give a fatal error");
       }

    TEST_CASE("Updating a nonexistent file")
       {
        CHECK_MESSAGE(test::execute_wait(exe_path, {"update", "-q", "not-existing.ppjs"}) == 2, "...Should give a fatal error");
       }

    TEST_CASE("Updating an empty project")
       {
        test::TemporaryFile empty_prj("~empty.ppjs", "");
        CHECK_MESSAGE(test::execute_wait(exe_path, {"update", "-q", empty_prj.path().string()}) == 2, "...Should give a fatal error");
       }

    TEST_CASE("Updating a project with no libs")
       {
        test::TemporaryFile nolibs_prj("~nolibs.ppjs", "<plcProject>\n<libraries>\n</libraries>\n</plcProject>\n");
        CHECK_MESSAGE(test::execute_wait(exe_path, {"update", "-q", nolibs_prj.path().string()}) == 1, "...Should raise an issue");
       }

    TEST_CASE("Indicating the project itself as output")
       {
        test::TemporaryFile nolibs_prj("~nolibs.ppjs", "<plcProject>\n<libraries>\n</libraries>\n</plcProject>\n");
        CHECK_MESSAGE(test::execute_wait(exe_path, {"update", "-q", "-o", nolibs_prj.path().string(), nolibs_prj.path().string()}) == 2, "...Should give a fatal error");
       }

    TEST_CASE("Updating an ill formed project")
       {
        test::TemporaryFile bad_prj("~bad.ppjs", "<foo>");
        CHECK_MESSAGE(test::execute_wait(exe_path, {"update", "-q", bad_prj.path().string()}) == 2, "...Should give a fatal error");
       }

    TEST_CASE("Updating a project with an ill formed lib")
       {
        test::TemporaryFile prj_with_badplclib("~prj_with_badplclib.ppjs",
                                                "<plcProject>\n"
                                                "    <libraries>\n"
                                                "        <lib link=\"true\" name=\"~bad.plclib\"></lib>\n"
                                                "    </libraries>\n"
                                                "</plcProject>\n" );
        test::TemporaryFile bad_plclib("~bad.plclib", "<lib><lib>forbidden nested</lib></lib>");
        CHECK_MESSAGE(test::execute_wait(exe_path, {"update", "-q", prj_with_badplclib.path().string(),}) == 2, "...Should give a fatal error");
       }

    TEST_CASE("Updating a project with a nonexistent lib")
       {
        test::TemporaryFile prj_with_nonextlib("~prj_with_nonextlib.ppjs",
                                                "<plcProject>\n"
                                                "    <libraries>\n"
                                                "        <lib link=\"true\" name=\"not-existing.pll\"></lib>\n"
                                                "    </libraries>\n"
                                                "</plcProject>\n");
        CHECK_MESSAGE(test::execute_wait(exe_path, {"update", "-q", prj_with_nonextlib.path().string()}) == 1, "...Should raise an issue");
       }
   } // system tests
