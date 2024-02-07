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
//#include "h_file_parser.hpp" // sipro::h_parse()
//#include "pll_file_parser.hpp" // ll::pll_parse()
#include "file_write.hpp" // sys::file_write()
#include "writer_pll.hpp" // pll::write_lib()
#include "writer_plclib.hpp" // plclib::write_lib()


using namespace std::literals; // "..."sv


// stubs
    namespace ll
    {
    void pll_parse([[maybe_unused]] const std::string& pth, [[maybe_unused]] const std::string_view input_bytes, [[maybe_unused]] plcb::Library& lib, [[maybe_unused]] fnotify_t const& notify_issue)
    {
        throw std::runtime_error{"pll::parse() not yet ready"};
    }
    }

    namespace sipro
    {
    void h_parse([[maybe_unused]] const std::string& pth, [[maybe_unused]] const std::string_view input_bytes, [[maybe_unused]] plcb::Library& lib, [[maybe_unused]] fnotify_t const& notify_issue)
    {
        throw std::runtime_error{"h::parse() not yet ready"};
    }
    }


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
        throw std::runtime_error{ fmt::format("Output should be a directory: {}", dir.string()) };
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
                throw std::runtime_error{ fmt::format("Unhandled output file type {}"sv, output_path.filename().string()) };
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


//---------------------------------------------------------------------------
void parse_library(plcb::Library& lib, const std::string& input_file_fullpath, const file_type input_file_type, const std::string_view input_file_bytes, const MG::keyvals& conv_options, fnotify_t const& notify_issue)
{
    if( input_file_bytes.empty() )
       {
        notify_issue( fmt::format("{} is empty", input_file_fullpath) );
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
            throw std::runtime_error{ fmt::format("Unhandled input file {}"sv, input_file_fullpath) };
       }

    if( lib.is_empty() )
       {
        notify_issue( fmt::format("{} generated an empty library", input_file_fullpath) );
       }

    if( conv_options.contains("sort") )
       {
        lib.sort();
       }

  #ifdef _DEBUG
    fmt::print("    {}\n", lib.get_summary());
  #endif
}


//---------------------------------------------------------------------------
bool write_library(const plcb::Library& lib, const fs::path& out_pll, const fs::path& out_plclib, const MG::keyvals& conv_options)
{
    bool something_done = false;

    if( not out_pll.empty() )
       {
        sys::file_write out_file{ out_pll.string().c_str() };
        out_file.set_buffer_size(4_MB);
        pll::write_lib(out_file, lib, conv_options);
        something_done = true;
       }

    if( not out_plclib.empty() )
       {
        sys::file_write out_file{ out_pll.string().c_str() };
        out_file.set_buffer_size(4_MB);
        plclib::write_lib(out_file, lib, conv_options);
        something_done = true;
       }

    return something_done;
}


//---------------------------------------------------------------------------
void convert_library(const fs::path& input_file_path, fs::path output_path, const bool can_overwrite, const MG::keyvals& conv_options, fnotify_t const& notify_issue)
{
    const std::string input_file_fullpath{ input_file_path.string() };
    const std::string input_file_basename{ input_file_path.stem().string() };

    const file_type input_file_type = recognize_file_type( input_file_fullpath );
    const auto [out_pll, out_plclib] = set_output_paths(input_file_path, input_file_type, output_path, can_overwrite);

    plcb::Library lib( input_file_basename );
    const sys::memory_mapped_file input_file_mapped{ input_file_fullpath.c_str() }; // This must live until the end
    parse_library(lib, input_file_fullpath, input_file_type, input_file_mapped.as_string_view(), conv_options, notify_issue);
    lib.throw_if_incoherent();

    const bool something_done = write_library(lib, out_pll, out_plclib, conv_options);
    if( not something_done )
       {
        notify_issue( fmt::format("Nothing to do for {}"sv, input_file_fullpath) );
       }
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"libraries_converter"> libraries_converter_tests = []
{


};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
