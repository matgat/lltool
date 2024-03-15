#pragma once
//  ---------------------------------------------
//  Routines to converts LogicLab libraries
//  ---------------------------------------------
//  #include "libraries_converter.hpp" // ll::convert_libraries()
//  ---------------------------------------------
#include <stdexcept> // std::runtime_error
#include <format>
#include <string_view>
#include <cassert>

#include "filesystem_utilities.hpp" // fs::*, fsu::*
#include "memory_mapped_file.hpp" // sys::memory_mapped_file
#include "options_map.hpp" // MG::options_map
#include "plc_library.hpp" // plcb::Library
#include "h_file_parser.hpp" // sipro::h_parse()
#include "pll_file_parser.hpp" // ll::pll_parse()
#include "file_write.hpp" // sys::file_write()
#include "writer_pll.hpp" // pll::write_lib()
#include "writer_plclib.hpp" // plclib::write_lib()

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
void prepare_output_dir(const fs::path& dir, const bool clear, fnotify_t const& notify_issue)
{
    if( dir.empty() )
       {
        throw std::runtime_error{"Output directory not given"};
       }
    else if( not fs::exists(dir) )
       {
        //dbg_print("Creating directory: {}\n", dir.string());
        fs::create_directories(dir);
       }
    else if( fs::is_directory(dir) )
       {
        if( clear )
           {
            [[maybe_unused]] const std::size_t removed_count = fsu::remove_files_with_suffix_in(dir, {".pll", ".plclib", ".log"});
            //if(removed_count>0) dbg_print("Cleared {} files in {}", removed_count, dir.string());

            // Notify the presence of uncleared files
            if( not fs::is_empty(dir) )
               {
                for( const fs::directory_entry& ientry : fs::directory_iterator(dir) )
                   {
                    if( ientry.is_regular_file() and not ientry.path().filename().string().starts_with('.') )
                       {
                        notify_issue( std::format("Uncleared file in output dir: \"{}\"", ientry.path().string()) );
                       }
                   }
               }
           }
       }
    else
       {
        throw std::runtime_error{ std::format("Output should be a directory: \"{}\"", dir.string()) };
       }
}


//---------------------------------------------------------------------------
struct outpaths_t final { fs::path pll, plclib; };
[[nodiscard]] outpaths_t set_output_paths(const fs::path& input_file_path, const file_type input_file_type, fs::path output_path, const bool can_overwrite)
{
    outpaths_t output_files_paths;

    if( output_path.empty() )
       {
        output_path = input_file_path.parent_path();
       }
    else if( not fs::exists(output_path) and not output_path.has_extension() )
       {
        //dbg_print("Creating directory: {}\n", output_path.string());
        fs::create_directories(output_path);
       }

    if( fs::is_directory(output_path) )
       {// Choosing output paths basing on input file
        output_path /= input_file_path.stem();
        switch( input_file_type )
           {
            case file_type::pll: // pll -> plclib
                output_files_paths.plclib = output_path;
                output_files_paths.plclib += ".plclib";
                break;

            case file_type::h: // h -> pll,plclib
                output_files_paths.pll = output_path;
                output_files_paths.pll += ".pll";
                output_files_paths.plclib = output_path;
                output_files_paths.plclib += ".plclib";
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
                throw std::runtime_error{ std::format("Unhandled output file type: \"{}\""sv, output_path.filename().string()) };
           }
       }

    // Ensure to not overwrite output files unless indicated
    const auto check_overwrites = [&input_file_path, can_overwrite](const fs::path output_file_path)
       {
        if( fs::exists(output_file_path) )
           {
            if( fs::equivalent(input_file_path, output_file_path) )
               {
                throw std::invalid_argument( std::format("Output file \"{}\" collides with original file", output_file_path.string()) );
               }
            else if( not can_overwrite )
               {
                throw std::invalid_argument( std::format("Won't overwrite existing file \"{}\"", output_file_path.string()) );
               }
           }
       };
    check_overwrites(output_files_paths.pll);
    check_overwrites(output_files_paths.plclib);

    return output_files_paths;
}


//---------------------------------------------------------------------------
void parse_library(plcb::Library& lib, const std::string& input_file_fullpath, const file_type input_file_type, const std::string_view input_file_bytes, const MG::options_map& conv_options, fnotify_t const& notify_issue)
{
    if( input_file_bytes.empty() )
       {
        notify_issue( std::format("\"{}\" is empty", input_file_fullpath) );
       }

    switch( input_file_type )
       {
        case file_type::pll:
            ll::pll_parse(input_file_fullpath, input_file_bytes, lib, notify_issue);
            break;

        case file_type::h:
            sipro::h_parse(input_file_fullpath, input_file_bytes, lib, notify_issue);
            break;

        default:
            throw std::runtime_error{ std::format("Unhandled input file \"{}\""sv, input_file_fullpath) };
       }

    if( lib.is_empty() )
       {
        notify_issue( std::format("\"{}\" generated an empty library", input_file_fullpath) );
       }

    if( conv_options.contains("sort") )
       {
        lib.sort();
       }

    //dbg_print("    {}\n", lib.get_summary());
}


//---------------------------------------------------------------------------
bool write_library(const plcb::Library& lib, const fs::path& out_pll, const fs::path& out, const MG::options_map& conv_options)
{
    bool something_done = false;

    if( not out_pll.empty() )
       {
        sys::file_write out_file{ out_pll.string().c_str() };
        out_file.set_buffer_size(4_MB);
        pll::write_lib(out_file, lib, conv_options);
        something_done = true;
       }

    if( not out.empty() )
       {
        sys::file_write out_file{ out.string().c_str() };
        out_file.set_buffer_size(4_MB);
        plclib::write_lib(out_file, lib, conv_options);
        something_done = true;
       }

    return something_done;
}


//---------------------------------------------------------------------------
void convert_library(const fs::path& input_file_path, fs::path output_path, const bool can_overwrite, const MG::options_map& conv_options, fnotify_t const& notify_issue)
{
    const std::string input_file_fullpath{ input_file_path.string() };
    const std::string input_file_basename{ input_file_path.stem().string() };

    const file_type input_file_type = recognize_file_type( input_file_fullpath );
    const auto [out_pll, out] = set_output_paths(input_file_path, input_file_type, output_path, can_overwrite);

    plcb::Library lib( input_file_basename );
    const sys::memory_mapped_file input_file_mapped{ input_file_fullpath.c_str() }; // This must live until the end
    parse_library(lib, input_file_fullpath, input_file_type, input_file_mapped.as_string_view(), conv_options, notify_issue);
    lib.throw_if_incoherent();

    const bool something_done = write_library(lib, out_pll, out, conv_options);
    if( not something_done )
       {
        notify_issue( std::format("Nothing to do for: \"{}\""sv, input_file_fullpath) );
       }
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"libraries_converter"> libraries_converter_tests = []
{

ut::test("reparse a header") = []
   {
    // Parse a defines header
    plcb::Library parsed_lib("sample_def_header"sv);
    sipro::h_parse(parsed_lib.name(), sample_def_header, parsed_lib, [](std::string&&)noexcept{});
    // Write to pll
    MG::string_write out;
    pll::write_lib(out, parsed_lib, {});
    // Parse written pll
    plcb::Library parsed_lib2(parsed_lib.name());
    ll::pll_parse(parsed_lib2.name(), out.str(), parsed_lib2, [](std::string&&)noexcept{});
    // Compare
    ut::expect( parsed_lib == parsed_lib2 );
   };


ut::test("reparse a pll") = []
   {
    // Parse a pll
    plcb::Library parsed_lib("sample-lib"sv);
    ll::pll_parse(parsed_lib.name(), sample_lib_pll, parsed_lib, [](std::string&&)noexcept{});
    // Rewrite to pll
    MG::string_write out;
    pll::write_lib(out, parsed_lib, {});
    // Parse written pll
    plcb::Library parsed_lib2(parsed_lib.name());
    ll::pll_parse(parsed_lib2.name(), out.str(), parsed_lib2, [](std::string&&)noexcept{});
    // Compare
    ut::expect( parsed_lib == parsed_lib2 );
   };


ut::test("ll::convert_library()") = []
   {
    ut::should("converting a nonexistent pll") = []
       {
        test::TemporaryFile in("~not-existent.pll");
        test::TemporaryFile out("~out.plclib");
        ut::expect( ut::throws([&]{ ll::convert_library(in.path().string(), out.path(), false, {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
       };

    ut::should("converting an unknown library") = []
       {
        test::TemporaryFile in("~unknown.xxx");
        test::TemporaryFile out("~out.plclib");
        ut::expect( ut::throws([&]{ ll::convert_library(in.path().string(), out.path(), false, {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
       };

    ut::should("converting to unknown format") = []
       {
        test::TemporaryFile in("~in.pll");
        test::TemporaryFile out("~out.xxx");
        ut::expect( ut::throws([&]{ ll::convert_library(in.path().string(), out.path(), false, {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
       };

    ut::should("writing to original file") = []
       {
        test::TemporaryFile in("~in.pll",   "PROGRAM Prg\n"
                                            "    { CODE:ST }Body\n"
                                            "END_PROGRAM\n"sv);
        ut::expect( ut::throws([&]{ ll::convert_library(in.path().string(), in.path(), false, {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
       };

    ut::should("writing to existent file") = []
       {
        test::TemporaryDirectory dir;
        auto in = dir.create_file("~in.pll","PROGRAM Prg\n"
                                            "    { CODE:ST }Body\n"
                                            "END_PROGRAM\n"sv);
        auto out = dir.create_file("~out.plclib", "<lib>\n</lib>\n"sv);
        ut::expect( ut::throws([&]{ ll::convert_library(in.path().string(), out.path(), false, {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
       };

    ut::should("writing to input directory") = []
       {
        test::TemporaryDirectory dir;
        dir.create_file("~in1.h","#define vqEx1 vq1 // descr 1\n"sv);
        dir.create_file("~in2.h","#define vqEx2 vq2 // descr 2\n"sv);
        auto in = dir.decl_file("*.h");
        ut::expect( ut::throws([&]{ ll::convert_library(in.path().string(), dir.path(), true, {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
       };

    ut::should("converting an empty pll") = []
       {
        test::TemporaryDirectory dir;
        auto in = dir.create_file("~empty.pll", "\n"sv);
        struct issues_t final { int num=0; void operator()(std::string&&) noexcept {++num;}; } issues;
        ll::convert_library(in.path().string(), {}, false, MG::options_map{"plclib-indent:2"}, std::ref(issues));
        ut::expect( ut::that % issues.num==1 ) << "should rise an issue related to empty lib\n";
        auto out = dir.decl_file("~empty.plclib");
        ut::expect( out.exists() );
        ut::expect( ut::that % out.content() == "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
                                                "<plcLibrary schemaVersion=\"2.8\">\n"
                                                "\t<lib version=\"1.0.0\" name=\"~empty\" fullXml=\"true\">\n"
                                                "\t\t<!-- author=\"plclib::write()\" -->\n"
                                                "\t\t<descr>PLC library</descr>\n"
                                                "\t\t<libWorkspace>\n"
                                                "\t\t\t<folder name=\"~empty\" id=\"2386\">\n"
                                                "\t\t\t</folder>\n"
                                                "\t\t</libWorkspace>\n"
                                                "\t\t<globalVars/>\n"
                                                "\t\t<retainVars/>\n"
                                                "\t\t<constantVars/>\n"
                                                "\t\t<functions/>\n"
                                                "\t\t<functionBlocks/>\n"
                                                "\t\t<programs/>\n"
                                                "\t\t<macros/>\n"
                                                "\t\t<structs/>\n"
                                                "\t\t<typedefs/>\n"
                                                "\t\t<enums/>\n"
                                                "\t\t<subranges/>\n"
                                                "\t</lib>\n"
                                                "</plcLibrary>\n"sv );
       };

    ut::should("converting a simple pll specifying output") = []
       {
        test::TemporaryDirectory dir;
        auto in = dir.create_file("~in.pll","PROGRAM Prg\n"
                                            "    { CODE:ST }Body\n"
                                            "END_PROGRAM\n"sv);
        auto out = dir.decl_file("~out.plclib");
        struct issues_t final { int num=0; void operator()(std::string&& msg) noexcept {++num; ut::log << msg << '\n';}; } issues;
        ll::convert_library(in.path().string(), out.path(), false, MG::options_map{"plclib-indent:2"}, std::ref(issues));
        ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
        ut::expect( out.exists() );
        ut::expect( ut::that % out.content() == "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
                                                "<plcLibrary schemaVersion=\"2.8\">\n"
                                                "\t<lib version=\"1.0.0\" name=\"~in\" fullXml=\"true\">\n"
                                                "\t\t<!-- author=\"plclib::write()\" -->\n"
                                                "\t\t<descr>PLC library</descr>\n"
                                                "\t\t<!--\n"
                                                "\t\t\tprograms: 1\n"
                                                "\t\t-->\n"
                                                "\t\t<libWorkspace>\n"
                                                "\t\t\t<folder name=\"~in\" id=\"698\">\n"
                                                "\t\t\t\t<Pou name=\"Prg\"/>\n"
                                                "\t\t\t</folder>\n"
                                                "\t\t</libWorkspace>\n"
                                                "\t\t<globalVars/>\n"
                                                "\t\t<retainVars/>\n"
                                                "\t\t<constantVars/>\n"
                                                "\t\t<functions/>\n"
                                                "\t\t<functionBlocks/>\n"
                                                "\t\t<programs>\n"
                                                "\t\t\t<program name=\"Prg\" version=\"1.0.0\" creationDate=\"0\" lastModifiedDate=\"0\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\">\n"
                                                "\t\t\t\t<vars/>\n"
                                                "\t\t\t\t<iecDeclaration active=\"FALSE\"/>\n"
                                                "\t\t\t\t<sourceCode type=\"ST\">\n"
                                                "\t\t\t\t\t<![CDATA[Body]]>\n"
                                                "\t\t\t\t</sourceCode>\n"
                                                "\t\t\t</program>\n"
                                                "\t\t</programs>\n"
                                                "\t\t<macros/>\n"
                                                "\t\t<structs/>\n"
                                                "\t\t<typedefs/>\n"
                                                "\t\t<enums/>\n"
                                                "\t\t<subranges/>\n"
                                                "\t</lib>\n"
                                                "</plcLibrary>\n"sv );
       };

    ut::should("converting sample header") = []
       {
        test::TemporaryDirectory dir;
        auto in = dir.create_file("sample-def.h", sample_def_header);

        try{
            struct issues_t final { int num=0; void operator()(std::string&& msg) noexcept {++num; ut::log << msg << '\n';}; } issues;
            ll::convert_library(in.path().string(), {}, false, MG::options_map{"plclib-indent:2"}, std::ref(issues));
            ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
           }
        catch( parse::error& e )
           {
            ut::expect(false) << std::format("Exception: {} (line {})\n", e.what(), e.line());
           }

        auto out_pll = dir.decl_file("sample-def.pll");
        ut::expect( out_pll.exists() );
        ut::expect( ut::that % out_pll.content() == "(*\n"
                                                    "\tname: sample-def\n"
                                                    "\tdescr: PLC library\n"
                                                    "\tversion: 1.0.0\n"
                                                    "\tauthor: pll::write()\n"
                                                    "\tglobal-variables: 5\n"
                                                    "\tglobal-constants: 3\n"
                                                    "*)\n"
                                                    "\n"
                                                    "\n"
                                                    "\n"
                                                    "\t(****************************)\n"
                                                    "\t(*                          *)\n"
                                                    "\t(*     GLOBAL VARIABLES     *)\n"
                                                    "\t(*                          *)\n"
                                                    "\t(****************************)\n"
                                                    "\n"
                                                    "\tVAR_GLOBAL\n"
                                                    "\t{G:\"Header_Variables\"}\n"
                                                    "\tvbSample AT %MB300.123 : BOOL; { DE:\"A vb var\" }\n"
                                                    "\tvnSample AT %MW400.123 : INT; { DE:\"A vn var\" }\n"
                                                    "\tvqSample AT %MD500.123 : DINT; { DE:\"A vq var\" }\n"
                                                    "\tvaSample AT %MB700.0 : STRING[ 80 ]; { DE:\"A va var\" }\n"
                                                    "\tvdSample AT %ML600.11 : LREAL; { DE:\"A vd var\" }\n"
                                                    "\tEND_VAR\n"
                                                    "\n"
                                                    "\n"
                                                    "\n"
                                                    "\t(****************************)\n"
                                                    "\t(*                          *)\n"
                                                    "\t(*     GLOBAL CONSTANTS     *)\n"
                                                    "\t(*                          *)\n"
                                                    "\t(****************************)\n"
                                                    "\n"
                                                    "\tVAR_GLOBAL CONSTANT\n"
                                                    "\t{G:\"Header_Constants\"}\n"
                                                    "\tint : INT := 42; { DE:\"An int constant\" }\n"
                                                    "\tdint : DINT := 100; { DE:\"A dint constant\" }\n"
                                                    "\tdouble : LREAL := 12.3; { DE:\"A double constant\" }\n"
                                                    "\tEND_VAR\n"sv );

        auto out_plclib = dir.decl_file("sample-def.plclib");
        ut::expect( out_plclib.exists() );
        ut::expect( ut::that % out_plclib.content() ==  "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
                                                        "<plcLibrary schemaVersion=\"2.8\">\n"
                                                        "\t<lib version=\"1.0.0\" name=\"sample-def\" fullXml=\"true\">\n"
                                                        "\t\t<!-- author=\"plclib::write()\" -->\n"
                                                        "\t\t<descr>PLC library</descr>\n"
                                                        "\t\t<!--\n"
                                                        "\t\t\tglobal-variables: 5\n"
                                                        "\t\t\tglobal-constants: 3\n"
                                                        "\t\t-->\n"
                                                        "\t\t<libWorkspace>\n"
                                                        "\t\t\t<folder name=\"sample-def\" id=\"5616\">\n"
                                                        "\t\t\t\t<GlobalVars name=\"Header_Constants\"/>\n"
                                                        "\t\t\t\t<GlobalVars name=\"Header_Variables\"/>\n"
                                                        "\t\t\t</folder>\n"
                                                        "\t\t</libWorkspace>\n"
                                                        "\t\t<globalVars>\n"
                                                        "\t\t\t<group name=\"Header_Variables\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\" version=\"1.0.0\">\n"
                                                        "\t\t\t\t<var name=\"vbSample\" type=\"BOOL\">\n"
                                                        "\t\t\t\t\t<descr>A vb var</descr>\n"
                                                        "\t\t\t\t\t<address type=\"M\" typeVar=\"B\" index=\"300\" subIndex=\"123\"/>\n"
                                                        "\t\t\t\t</var>\n"
                                                        "\t\t\t\t<var name=\"vnSample\" type=\"INT\">\n"
                                                        "\t\t\t\t\t<descr>A vn var</descr>\n"
                                                        "\t\t\t\t\t<address type=\"M\" typeVar=\"W\" index=\"400\" subIndex=\"123\"/>\n"
                                                        "\t\t\t\t</var>\n"
                                                        "\t\t\t\t<var name=\"vqSample\" type=\"DINT\">\n"
                                                        "\t\t\t\t\t<descr>A vq var</descr>\n"
                                                        "\t\t\t\t\t<address type=\"M\" typeVar=\"D\" index=\"500\" subIndex=\"123\"/>\n"
                                                        "\t\t\t\t</var>\n"
                                                        "\t\t\t\t<var name=\"vaSample\" type=\"STRING\" length=\"80\">\n"
                                                        "\t\t\t\t\t<descr>A va var</descr>\n"
                                                        "\t\t\t\t\t<address type=\"M\" typeVar=\"B\" index=\"700\" subIndex=\"0\"/>\n"
                                                        "\t\t\t\t</var>\n"
                                                        "\t\t\t\t<var name=\"vdSample\" type=\"LREAL\">\n"
                                                        "\t\t\t\t\t<descr>A vd var</descr>\n"
                                                        "\t\t\t\t\t<address type=\"M\" typeVar=\"L\" index=\"600\" subIndex=\"11\"/>\n"
                                                        "\t\t\t\t</var>\n"
                                                        "\t\t\t</group>\n"
                                                        "\t\t</globalVars>\n"
                                                        "\t\t<retainVars/>\n"
                                                        "\t\t<constantVars>\n"
                                                        "\t\t\t<group name=\"Header_Constants\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\" version=\"1.0.0\">\n"
                                                        "\t\t\t\t<const name=\"int\" type=\"INT\">\n"
                                                        "\t\t\t\t\t<descr>An int constant</descr>\n"
                                                        "\t\t\t\t\t<initValue>42</initValue>\n"
                                                        "\t\t\t\t</const>\n"
                                                        "\t\t\t\t<const name=\"dint\" type=\"DINT\">\n"
                                                        "\t\t\t\t\t<descr>A dint constant</descr>\n"
                                                        "\t\t\t\t\t<initValue>100</initValue>\n"
                                                        "\t\t\t\t</const>\n"
                                                        "\t\t\t\t<const name=\"double\" type=\"LREAL\">\n"
                                                        "\t\t\t\t\t<descr>A double constant</descr>\n"
                                                        "\t\t\t\t\t<initValue>12.3</initValue>\n"
                                                        "\t\t\t\t</const>\n"
                                                        "\t\t\t</group>\n"
                                                        "\t\t</constantVars>\n"
                                                        "\t\t<iecVarsDeclaration>\n"
                                                        "\t\t\t<group name=\"Header_Constants\">\n"
                                                        "\t\t\t\t<iecDeclaration active=\"FALSE\"/>\n"
                                                        "\t\t\t</group>\n"
                                                        "\t\t\t<group name=\"Header_Variables\">\n"
                                                        "\t\t\t\t<iecDeclaration active=\"FALSE\"/>\n"
                                                        "\t\t\t</group>\n"
                                                        "\t\t</iecVarsDeclaration>\n"
                                                        "\t\t<functions/>\n"
                                                        "\t\t<functionBlocks/>\n"
                                                        "\t\t<programs/>\n"
                                                        "\t\t<macros/>\n"
                                                        "\t\t<structs/>\n"
                                                        "\t\t<typedefs/>\n"
                                                        "\t\t<enums/>\n"
                                                        "\t\t<subranges/>\n"
                                                        "\t</lib>\n"
                                                        "</plcLibrary>\n"sv );
       };

    ut::should("converting sample pll") = []
       {
        test::TemporaryDirectory dir;
        auto in = dir.create_file("sample-lib.pll", sample_lib_pll);

        try{
            struct issues_t final { int num=0; void operator()(std::string&& msg) noexcept {++num; ut::log << msg << '\n';}; } issues;
            ll::convert_library(in.path().string(), {}, false, MG::options_map{"plclib-indent:2"}, std::ref(issues));
            ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
           }
        catch( parse::error& e )
           {
            ut::expect(false) << std::format("Exception: {} (line {})\n", e.what(), e.line());
           }

        auto out = dir.decl_file("sample-lib.plclib");
        ut::expect( out.exists() );
        ut::expect( ut::that % out.content() == sample_lib_plclib );
       };
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
