#pragma once
//  ---------------------------------------------
//  Routines to converts LogicLab libraries
//  ---------------------------------------------
//  #include "libraries_converter.hpp" // ll::convert_libraries()
//  ---------------------------------------------
#include <stdexcept> // std::runtime_error
#include <string_view>
#include <cassert>

#include <fmt/format.h> // fmt::format

#include "filesystem_utilities.hpp" // fs::*, fsu::*
#include "memory_mapped_file.hpp" // sys::memory_mapped_file
#include "keyvals.hpp" // MG::keyvals
#include "plc_library.hpp" // plcb::Library
//#include "fast-parser-h.hpp" // h::Parser
#include "file_write.hpp" // sys::file_write()

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace ll
{

//---------------------------------------------------------------------------
enum class file_type : std::uint8_t { unknown, h, pll, plclib };
[[nodiscard]] file_type recognize_file_type( const std::string_view file_path )
{
    if( file_path.ends_with(".h"sv) )
       {
        return file_type::h;
       }
    else if( file_path.ends_with(".pll"sv) )
       {
        return file_type::pll;
       }
    else if( file_path.ends_with(".plclib"sv) )
       {
        return file_type::plclib;
       }
    return file_type::unknown;
}


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
void prepare_output_dir(const fs::path& dir, const bool clear, fnotify_t const& notify_issue)
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
struct outpaths_t final
{
    fs::path pll, plclib;

    void set_pll(const fs::path& path)
       {
        pll = path;
        pll.replace_extension(".pll");
       }

    void set_plclib(const fs::path& path)
       {
        plclib = path;
        plclib.replace_extension(".plclib");
       }
};
[[nodiscard]] outpaths_t set_output_paths(const fs::path& input_file_path, const file_type input_file_type, fs::path output_path, const bool can_overwrite)
{
    outpaths_t output_files_paths;

    if( output_path.empty() )
       {
        output_path = input_file_path.parent_path();
       }
    else if( not fs::exists(output_path) and not output_path.has_extension() )
       {
      #ifdef _DEBUG
        fmt::print("Creating directory: {}\n", output_path.string());
      #endif
        fs::create_directories(output_path);
       }

    if( fs::is_directory(output_path) )
       {// Choosing output paths basing on input file
        switch( input_file_type )
           {
            case file_type::pll: // pll -> plclib
                output_files_paths.set_plclib(input_file_path);
                break;

            case file_type::h: // h -> pll,plclib
                output_files_paths.set_pll(input_file_path);
                output_files_paths.set_plclib(input_file_path);
                break;

            default:
                ; // No output paths
           }
       }

    else if( output_path.has_extension() )
       {// Specified the output file extension
        switch( recognize_file_type( output_path.string() ) )
           {
            case file_type::pll:
                output_files_paths.pll = output_path;
                break;

            case file_type::plclib:
                output_files_paths.plclib = output_path;
                break;

            default:
                std::runtime_error{ fmt::format("Unhandled output file type {}"sv, output_path.filename().string()) };
           }
       }

    // Ensure to not overwrite output files unless indicated
    const auto check_overwrites = [&input_file_path, can_overwrite](const fs::path output_file_path)
       {
        if( fs::exists(output_file_path) )
           {
            if( fs::equivalent(input_file_path, output_file_path) )
               {
                throw std::invalid_argument( fmt::format("Output file \"{}\" collides with original file", output_file_path.string()) );
               }
            else if( not can_overwrite )
               {
                throw std::invalid_argument( fmt::format("Won't overwrite existing file \"{}\"", output_file_path.string()) );
               }
           }
       };
    check_overwrites(output_files_paths.pll);
    check_overwrites(output_files_paths.plclib);

    return output_files_paths;
}

// stubs
namespace pll
{
//---------------------------------------------------------------------------
void parse([[maybe_unused]] const std::string_view input_bytes, [[maybe_unused]] plcb::Library& lib, [[maybe_unused]] fnotify_t const& notify_issue)
{
    throw std::runtime_error("pll::parse() not yet ready");
}

}

namespace h
{
//---------------------------------------------------------------------------
void parse([[maybe_unused]] const std::string_view input_bytes, [[maybe_unused]] plcb::Library& lib, [[maybe_unused]] fnotify_t const& notify_issue)
{
    throw std::runtime_error("h::parse() not yet ready");
}

}


//---------------------------------------------------------------------------
void convert_library(const fs::path& input_file_path, fs::path output_path, const bool can_overwrite, [[maybe_unused]] const MG::keyvals& conv_options, [[maybe_unused]] fnotify_t const& notify_issue)
{
    const std::string input_file_fullpath{ input_file_path.string() };
    const std::string input_file_basename{ input_file_path.stem().string() };
    const file_type input_file_type = recognize_file_type( input_file_fullpath );

    const auto [out_pll, out_plclib] = set_output_paths(input_file_path, input_file_type, output_path, can_overwrite);

    const sys::memory_mapped_file input_file_mapped{ input_file_fullpath.c_str() };
    const std::string_view input_file_bytes{ input_file_mapped.as_string_view() };

    plcb::Library lib( input_file_basename );

    switch( input_file_type )
       {
        case file_type::pll:
            pll::parse(input_file_bytes, lib,  notify_issue);
            break;

        case file_type::h:
            h::parse(input_file_bytes, lib,  notify_issue);
            break;

        default:
            std::runtime_error{ fmt::format("Unhandled input file type {}"sv, input_file_path.filename().string()) };
       }


//    parsefunct(bufmap.path(), bufmap.as_string_view(), lib, parse_issues, args.fussy());
//    fmt::print("    {}\n", lib.to_str());
//    // Check the result
//    lib.check(); // throws if something's wrong
//    if( lib.is_empty() )
//        {
//         notify_issue( fmt::format("{} generated an empty library",bufmap.path()) );
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


    // [parse]
    //if( input_file_type==file_type::pll )
    //   {// pll -> plclib
    //    parse_buffer(pll::parse, file_buf, input_file_path, lib, args, notify_issue);
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
    //    parse_buffer(h::parse, file_buf, input_file_path, lib, args, notify_issue);
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
