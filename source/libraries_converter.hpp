#pragma once
//  ---------------------------------------------
//  Routines to converts LogicLab libraries
//  ---------------------------------------------
//  #include "libraries_converter.hpp" // ll::convert_libraries()
//  ---------------------------------------------
#include <cassert>
#include <stdexcept> // std::runtime_error
//#include <string>
#include <string_view>
//#include <vector>
//#include <optional>
//#include <fmt/format.h> // fmt::*

#include "filesystem_utilities.hpp" // fs::*, fsu::*
#include "memory_mapped_file.hpp" // sys::memory_mapped_file
#include "plc_library.hpp" // plcb::Library
//#include "fast-parser-h.hpp" // h::Parser
#include "file_write.hpp" // sys::file_write()

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace ll
{

//---------------------------------------------------------------------------
enum class file_type : std::uint8_t { unknown, h, pll };
[[nodiscard]] file_type recognize_input_file_type( const std::string_view file_path )
{
    if( file_path.ends_with(".h"sv) )
       {
        return file_type::h;
       }
    else if( file_path.ends_with(".pll"sv) )
       {
        return file_type::pll;
       }
    return file_type::unknown;
}



//---------------------------------------------------------------------------
//template<typename F> void parse_buffer(F parsefunct, const sys::MemoryMappedFile& bufmap, const fs::path& pth, plcb::Library& lib, const Arguments& args, std::vector<std::string>& issues)
//{
//    std::vector<std::string> parse_issues;
//    try{
//        parsefunct(bufmap.path(), bufmap.as_string_view(), lib, parse_issues, args.fussy());
//       }
//    catch( parse::error& e)
//       {
//        sys::edit_text_file( e.file_path(), e.line(), e.pos() );
//        throw;
//       }
//    if( args.verbose() )
//       {
//        fmt::print("    {}\n", lib.to_str());
//       }
//
//    // Handle parsing issues
//    if( not parse_issues.empty() )
//       {
//        // Append to overall issues list
//        issues.push_back( fmt::format("____Parsing of {}",bufmap.path()) );
//        issues.insert(issues.end(), parse_issues.begin(), parse_issues.end());
//        // Log in a file (in output folder)
//        const fs::path log_file_dir = args.output_isdir() ? args.output() : args.output().parent_path();
//        const std::string log_file_path{ (log_file_dir / pth.filename()).concat(".log").string() };
//        sys::file_write log_file_write( log_file_path.c_str() );
//        log_file_write << sys::get_formatted_time_stamp() << '\n';
//        log_file_write << "[Parse log of "sv << bufmap.path() << "]\n"sv;
//        for(const std::string& issue : parse_issues)
//           {
//            log_file_write << "[!] "sv << issue << '\n';
//           }
//        sys::launch_file( log_file_path );
//       }
//
//    // Check the result
//    lib.check(); // throws if something's wrong
//    if( lib.is_empty() )
//        {
//         issues.push_back( fmt::format("{} generated an empty library",bufmap.path()) );
//        }
//
//    // Manipulate the result
//    //const auto [ctime, mtime] = sys::get_file_dates(bufmap.path());
//    //fmt::print("created:{} modified:{}\n", sys::get_formatted_time_stamp(ctime), sys::get_formatted_time_stamp(mtime));
//    //lib.set_dates(ctime, mtime);
//    if( args.options().contains("sort")) // args.options().value_of("sort")=="name"
//       {
//        lib.sort();
//       }
//}


//---------------------------------------------------------------------------
// Write PLC library to plclib format
//void write_plclib(const plcb::Library& lib, const std::string& out_pth_str, const Arguments& args)
//{
//    if( args.verbose() )
//       {
//        fmt::print("    Writing plclib to: {}\n", out_pth_str);
//       }
//    sys::file_write out_file_write(out_pth_str.c_str());
//    plclib::write(out_file_write, lib, args.options());
//}

//---------------------------------------------------------------------------
// Write PLC library to pll format
//void write_pll(const plcb::Library& lib, const std::string& out_pth_str, const Arguments& args)
//{
//    if( args.verbose() )
//       {
//        fmt::print("    Writing pll to: {}\n", out_pth_str);
//       }
//    sys::file_write out_file_write(out_pth_str.c_str());
//    pll::write(out_file_write, lib, args.options());
//}

//---------------------------------------------------------------------------
// Write PLC library to...
//void write(const plcb::Library& lib, const fs::path& out_pth, const Arguments& args)
//{
//    const std::string out_ext{ str::tolower(out_pth.extension().string()) };
//    if( out_ext==".plclib" )
//       {
//        write_plclib(lib, out_pth.string(), args);
//       }
//    else if( out_ext==".pll" )
//       {
//        write_pll(lib, out_pth.string(), args);
//       }
//    else
//       {
//        throw std::runtime_error(fmt::format("Unknown extension {} of output file {}", out_ext, out_pth.filename().string()));
//       }
//}


//---------------------------------------------------------------------------
void prepare_converted_libs_output_dir(const fs::path& dir, const bool clear, fnotify_t const& notify_issue)
{
    if( dir.empty() )
       {
        throw std::runtime_error("Output directory not given");
       }
    else if( not fs::exists(dir) )
       {
      #ifdef _DEBUG
        fmt::print("Creating directory: {}\n", dir.string());
      #endif
        fs::create_directories(dir);
       }
    else if( fs::is_directory(dir) )
       {
        if( clear )
           {
            [[maybe_unused]] const std::size_t removed_count = fsu::remove_files_with_suffix_in(dir, {".pll", ".plclib", ".log"});
          #ifdef _DEBUG
            if(removed_count>0) fmt::print("Cleared {} files in {}", removed_count, dir.string());
          #endif

            // Notify the presence of uncleared files
            if( not fs::is_empty(dir) )
               {
                for( const fs::directory_entry& ientry : fs::directory_iterator(dir) )
                   {
                    if( ientry.is_regular_file() and not ientry.path().filename().string().starts_with('.') )
                       {
                        notify_issue( fmt::format("Uncleared file in output dir: {}", ientry.path().string()) );
                       }
                   }
               }
           }
       }
    else
       {
        throw std::runtime_error(fmt::format("Output should be a directory: {}", dir.string()));
       }
}

//---------------------------------------------------------------------------
//void get_converted_lib_output(const fs::path& input_file_path, const file_type input_file_type, fs::path& output_file_path, const bool can_overwrite)
//{
//    // can_overwrite
//    if( output_file_path.empty() )
//       {
//        // I'll have to write the following files:
//        //fs::path pll_file_path{ input_file_path }; pll_file_path.replace_extension(".pll");
//        //fs::path plclib_file_path{ input_file_path }; plclib_file_path.replace_extension(".plclib");
//       }
//    else if( fs::exists(output_file_path) )
//       {
//        if( fs::is_directory(output_file_path) )
//           {// output_file_path can't be the same directory of the input file
//            if( xxx )
//               {
//                throw std::runtime_error( fmt::format("Output directory \"{}\" collides with original file", output_file_path.string()) );
//               }
//           }
//        else if( fs::equivalent(input_files.front(), output_file_path) )
//           {
//            throw std::runtime_error( fmt::format("Output file \"{}\" collides with original file", output_file_path.string()) );
//           }
//       }
//}


//---------------------------------------------------------------------------
void convert_library(const fs::path& input_file_path, [[maybe_unused]] fs::path output_file_path, [[maybe_unused]] const bool can_overwrite, [[maybe_unused]] fnotify_t const& notify_issue)
{
    const std::string input_file_fullpath{ input_file_path.string() };
    [[maybe_unused]] const file_type input_file_type = recognize_input_file_type( input_file_fullpath );
    //check_converted_lib_output(input_file_path, input_file_type, output_file_path, can_overwrite);

    const sys::memory_mapped_file input_file_mapped{ input_file_fullpath.c_str() };
    const std::string_view input_file_bytes{ input_file_mapped.as_string_view() };

    const std::string input_file_basename{ input_file_path.stem().string() };
    plcb::Library lib( input_file_basename );

    throw std::runtime_error("\"convert\" not yet ready");

    // [parse]
    //if( input_file_type==file_type::pll )
    //   {// pll -> plclib
    //    parse_buffer(pll::parse, file_buf, input_file_path, lib, args, issues);
    //
    //    if( args.output_isdir() )
    //       {// Converting into separate files
    //        const fs::path output_file_path{ args.output() / fmt::format("{}.plclib", input_file_basename) }; replace_extension
    //        write_plclib(lib, output_file_path.string(), args);
    //       }
    //    else if( args.files().size()==1 )
    //       {// Write single input into specified output file
    //        write(lib, args.output(), args);
    //       }
    //    else
    //       {// Multiple input files into specified output file
    //        throw std::runtime_error(fmt::format("Combine into single file {} not yet supported", args.output().string()));
    //       }
    //   }
    //else if( input_file_type==file_type::h )
    //   {// h -> pll,plclib
    //    parse_buffer(h::parse, file_buf, input_file_path, lib, args, issues);
    //
    //    if( args.output_isdir() )
    //       {// Converting into separate files
    //        const fs::path out_pll_pth{ args.output() / fmt::format("{}.pll", input_file_basename) };
    //        write_pll(lib, out_pll_pth.string(), args);
    //
    //        const fs::path out_plclib_pth{ args.output() / fmt::format("{}.plclib", input_file_basename) };
    //        write_plclib(lib, out_plclib_pth.string(), args);
    //       }
    //    else if( args.files().size()==1 )
    //       {// Write single input into specified output file
    //        write(lib, args.output(), args);
    //       }
    //    else
    //       {// Multiple input files into specified output file
    //        throw std::runtime_error(fmt::format("Combine into single file {} not yet supported", args.output().string()));
    //       }
    //   }
    //else
    //   {
    //    std::runtime_error{ fmt::format("Unhandled file type {}"sv, input_file_path.filename().string()) };
    //   }
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"libraries_converter"> libraries_converter_tests = []
{

    //"ll::update_project_libraries()"
    //ut::test("Updating a nonexistent file") = []
    //   {
    //    ut::expect( ut::throws([]{ ll::update_project_libraries({"not-existing.ppjs"}, {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
    //   };
    //
    //ut::test("Updating an empty project") = []
    //   {
    //    test::TemporaryFile empty_prj("~empty.ppjs", "");
    //    ut::expect( ut::throws([&empty_prj]{ ll::update_project_libraries(empty_prj.path(), {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
    //   };
    //
    //ut::test("Updating a project with no libs") = []
    //   {
    //    test::TemporaryFile nolibs_prj("~nolibs.ppjs", "<plcProject>\n<libraries>\n</libraries>\n</plcProject>\n");
    //    int num_issues = 0;
    //    auto notify_issue = [&num_issues](std::string&&) noexcept { ++num_issues; };
    //    ll::update_project_libraries(nolibs_prj.path(), {}, notify_issue);
    //    ut::expect( ut::that % num_issues==1 ) << "should raise an issue\n";
    //   };
    //
    //ut::test("Using the project itself as output") = []
    //   {
    //    test::TemporaryFile fake_prj("~fake_prj.ppjs", "<plcProject>\n<libraries>\n</libraries>\n</plcProject>\n");
    //    ut::expect( ut::throws([&fake_prj]{ ll::update_project_libraries(fake_prj.path(), fake_prj.path(), [](std::string&&)noexcept{}); }) ) << "should throw\n";
    //   };
    //
    //ut::test("Updating an ill formed project") = []
    //   {
    //    test::TemporaryFile bad_prj("~bad.ppjs", "<foo>");
    //    ut::expect( ut::throws([&bad_prj]{ ll::update_project_libraries(bad_prj.path(), {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
    //   };
    //
    //ut::test("Updating a project with an ill formed lib") = []
    //   {
    //    test::TemporaryFile bad_plclib("~bad.plclib", "<lib><lib>forbidden nested</lib></lib>");
    //    test::TemporaryFile prj_with_badplclib("~prj_with_badplclib.ppjs",
    //                                            "<plcProject>\n"
    //                                            "    <libraries>\n"
    //                                            "        <lib link=\"true\" name=\"~bad.plclib\"></lib>\n"
    //                                            "    </libraries>\n"
    //                                            "</plcProject>\n" );
    //    ut::expect( ut::throws([&bad_plclib, &prj_with_badplclib]{ ll::update_project_libraries(prj_with_badplclib.path(), {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
    //   };
    //
    //ut::test("Updating a project with a nonexistent lib") = []
    //   {
    //    test::TemporaryFile existing("~existing.pll", "content");
    //    test::TemporaryFile prj_with_nonextlib("~prj_with_nonextlib.ppjs",
    //                                            "<plcProject>\n"
    //                                            "    <libraries>\n"
    //                                            "        <lib link=\"true\" name=\"~existing.pll\"></lib>\n"
    //                                            "        <lib link=\"true\" name=\"not-existing.pll\"></lib>\n"
    //                                            "    </libraries>\n"
    //                                            "</plcProject>\n");
    //    int num_issues = 0;
    //    auto notify_issue = [&num_issues](std::string&&) noexcept { ++num_issues; };
    //    ll::update_project_libraries(prj_with_nonextlib.path(), {}, notify_issue);
    //    ut::expect( ut::that % num_issues==1 ) << "should raise an issue\n";
    //   };
    //
    //ut::test("update simple") = []
    //   {
    //    test::TemporaryDirectory tmp_dir;
    //    tmp_dir.add_file("pll1.pll", "abc");
    //    tmp_dir.add_file("pll2.pll", "def");
    //    const auto prj_file = tmp_dir.add_file("prj.ppjs",
    //        "\uFEFF"
    //        "<plcProject>\n"
    //        "    <libraries>\n"
    //        "        <lib link=\"true\" name=\"pll1.pll\"><![CDATA[prev]]></lib>\n"
    //        "        <lib link=\"true\" name=\"pll2.pll\"></lib>\n"
    //        "    </libraries>\n"
    //        "</plcProject>\n"sv);
    //
    //    const std::string_view expected =
    //        "\uFEFF"
    //        "<plcProject>\n"
    //        "    <libraries>\n"
    //        "        <lib link=\"true\" name=\"pll1.pll\"><![CDATA[abc]]></lib>\n"
    //        "        <lib link=\"true\" name=\"pll2.pll\"><![CDATA[def]]></lib>\n"
    //        "    </libraries>\n"
    //        "</plcProject>\n"sv;
    //
    //    ll::update_project_libraries(prj_file.path(), {}, [](std::string&&)noexcept{});
    //
    //    ut::expect( ut::that % prj_file.has_content(expected) );
    //   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
