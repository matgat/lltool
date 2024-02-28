#define BOOST_UT_DISABLE_MODULE
//#include <https://raw.githubusercontent.com/boost-ext/ut/master/include/boost/ut.hpp>
#include "ut.hpp" // import boost.ut;
namespace ut = boost::ut;

#define TEST_UNITS
#include "test_facilities.hpp" // test::*

#include "ascii_parsing_utils.hpp" // ascii::extract_pair<>()
#include "ascii_predicates.hpp" // ascii::is_*
#include "ascii_simple_lexer.hpp" // ascii::simple_lexer
#include "expand_env_vars.hpp" // sys::expand_env_vars()
#include "file_globbing.hpp" // MG::file_glob()
#include "file_write.hpp" // sys::file_write()
#include "filesystem_utilities.hpp" // fs::*, fsu::*
#include "globbing.hpp" // MG::glob_match()
#include "h_parser.hpp" // h::Parser
#include "has_duplicate_basenames.hpp" // MG::has_duplicate_basenames()
#include "keyvals.hpp" // MG::keyvals
#include "memory_mapped_file.hpp" // sys::memory_mapped_file
#include "plain_parser_base.hpp" // plain::ParserBase
#include "string_conversions.hpp" // str::to_num<>()
#include "string_map.hpp" // MG::string_map<>
#include "string_utilities.hpp" // str::*
#include "system_base.hpp" // sys::*
#include "system_process.hpp" // sys::*
#include "text_parser_base.hpp" // text::ParserBase
#include "text_parser_xml.hpp" // text::xml::Parser
#include "timestamp.hpp" // MG::get_human_readable_timestamp()
#include "vectmap.hpp" // MG::vectmap<>
#include "unicode_text.hpp" // utxt::*

#include "sipro.hpp" // sipro::*
#include "h_file_parser.hpp" // sipro::h_parse()
#include "pll_file_parser.hpp" // ll::pll_parse()
#include "writer_pll.hpp" // pll::write_lib()
#include "writer_plclib.hpp" // plclib::write_lib()
#include "project_updater.hpp" // ll::update_project_libraries()
#include "libraries_converter.hpp" // ll::convert_libraries()

int main()
{
    ut::expect(true);
}
