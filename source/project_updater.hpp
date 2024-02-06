#pragma once
//  ---------------------------------------------
//  Updates the libraries in a LogicLab project
//  ---------------------------------------------
//  #include "project_updater.hpp" // ll::update_project_libraries()
//  ---------------------------------------------
#include <cassert>
#include <stdexcept> // std::runtime_error
#include <string>
#include <string_view>
#include <vector>
#include <optional>

#include <fmt/format.h> // fmt::*

#include "filesystem_utilities.hpp" // fs::*, fsu::*
#include "memory_mapped_file.hpp" // sys::memory_mapped_file
#include "unicode_text.hpp" // utxt::*
#include "text_parser_xml.hpp" // text::xml::Parser
#include "file_write.hpp" // sys::file_write()

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace ll
{

// Note: valid for both ppjs and plcprj project types
static constexpr std::u32string_view libraries_tag_name = U"libraries"sv;
static constexpr std::u32string_view library_tag_name = U"lib"sv;


//---------------------------------------------------------------------------
enum class library_type : std::uint8_t { unknown, pll, plclib };
[[nodiscard]] library_type recognize_library_type( const std::u32string_view file_path )
{
    if( file_path.ends_with(U".pll"sv) )
       {
        return library_type::pll;
       }
    else if( file_path.ends_with(U".plclib"sv) )
       {
        return library_type::plclib;
       }
    return library_type::unknown;
}


//---------------------------------------------------------------------------
struct lib_t final
   {
    fs::path path;
    std::size_t chunk_start = 0u;
    std::size_t chunk_end = 0u;
    library_type type = library_type::unknown;

    lib_t(fs::path&& pth, const library_type typ) noexcept
      : path{ std::move(pth) }
      , type{ typ }
       {}
   };

using libs_t = std::vector<lib_t>;



//---------------------------------------------------------------------------
template<utxt::Enc ENC>
[[nodiscard]] libs_t collect_linked_libs(const std::string_view bytes, std::string&& file_path, fnotify_t const& notify_issue)
{
    libs_t libs;

    text::xml::Parser<ENC> parser{bytes};
    parser.set_on_notify_issue(notify_issue);
    parser.set_file_path( std::move(file_path) );

    // Both project types (ppjs, plcprj) are a xml file that contains the following structure:
    // plcProject/sources/libraries/lib[link,name]

    class prj_parser_t
    {
     private:
        text::xml::Parser<ENC>& parser;

     public:
        explicit prj_parser_t(text::xml::Parser<ENC>& prs) noexcept
          : parser(prs)
           {
            parser.options().set_collect_comment_text(false);
            parser.options().set_collect_text_sections(false);
           }

        void seek_open_tag(const std::u32string_view tag_name)
           {
            while( parser.next_event() )
               {
                if( parser.curr_event().is_open_tag(tag_name) )
                   {
                    return;
                   }
               }
            throw parser.create_parse_error( fmt::format("Invalid project (<{}> not found)"sv, utxt::to_utf8(tag_name)), 1 );
           }

        void seek_close_tag(const std::u32string_view tag_name)
           {
            const auto start_line = parser.curr_line();
            do {
                if( parser.curr_event().is_close_tag(tag_name) )
                   {
                    return;
                   }
                else if( parser.curr_event().is_open_tag(tag_name) )
                   {
                    throw parser.create_parse_error( fmt::format("Unexpected nested <{}>"sv, utxt::to_utf8(tag_name)) );
                   }
               }
            while( parser.next_event() );

            throw parser.create_parse_error( fmt::format("Unclosed <{}>"sv, utxt::to_utf8(tag_name)), start_line );
           }

        std::optional<lib_t> check_and_collect_lib_data() noexcept
           {
            assert( parser.curr_event().is_open_tag(library_tag_name) );

            std::optional<lib_t> lib_data;

            // I'll collect only libraries with attribute link="true"
            if( not parser.curr_event().has_attribute_with_value(U"link"sv, U"true"sv) )
               {
                parser.notify_issue( fmt::format("Skipping library (need link=\"true\" in line {})"sv, parser.curr_line()) );
                return lib_data;
               }

            const auto name = parser.curr_event().attributes().value_of(U"name"sv);
            if( not name.has_value() or not name->get().has_value() )
               {
                parser.notify_issue( fmt::format("Skipping unnamed library (expected name=\"...\" in line {})"sv, parser.curr_line()) );
                return lib_data;
               }

            const std::u32string_view name_value{ name->get().value() };
            if( name_value.empty() )
               {
                parser.notify_issue( fmt::format("Skipping library with empty name (name=\"\" in line {})"sv, parser.curr_line()) );
                return lib_data;
               }

            std::error_code ec;
            fs::path lib_path = fs::absolute(fs::path{name_value}, ec); // Uses fs::current_path()
            if( ec )
               {
                parser.notify_issue( fmt::format("Skipping broken linked library (name=\"{}\" in line {}): {}"sv, utxt::to_utf8(name_value), parser.curr_line(), ec.message()) );
                return lib_data;
               }
            if( not fs::exists(lib_path) )
               {
                parser.notify_issue( fmt::format("Skipping broken linked library (name=\"{}\" path=\"{}\" in line {})"sv, utxt::to_utf8(name_value), lib_path.string(), parser.curr_line()) );
                return lib_data;
               }

            const library_type lib_type = recognize_library_type(name_value);
            if( lib_type==library_type::unknown )
               {
                parser.notify_issue( fmt::format("Unrecognized library (name=\"{}\" in line {})"sv, utxt::to_utf8(name_value), parser.curr_line()) );
               }

            lib_data.emplace( std::move(lib_path), lib_type );
            return lib_data;
           }

        void collect_lib_and_put_in(libs_t& libs)
           {
            std::optional<lib_t> lib_data = check_and_collect_lib_data();
            if( not lib_data.has_value() )
               {// Skipping this lib
                parser.next_event(); // Skip opening tag
                seek_close_tag(library_tag_name);
                return;
               }

            // Now I'll retrieve start and end of the library data chunk
            parser.next_event(); // Skip opening tag
            lib_data.value().chunk_start = parser.curr_event().start_byte_offset();
            seek_close_tag(library_tag_name);
            lib_data.value().chunk_end = parser.curr_event().start_byte_offset();

            libs.push_back( std::move(lib_data.value()) );
           }

    } prj_parser(parser);


    // Expecting a <libraries> tag
    prj_parser.seek_open_tag(libraries_tag_name);

    // Collect contained <libs>
    while( parser.next_event() )
       {
        if( parser.curr_event().is_open_tag(library_tag_name) )
           {
            prj_parser.collect_lib_and_put_in( libs );
           }
        else if( parser.curr_event().is_close_tag(libraries_tag_name) )
           {
            break;
           }
       }

    if( libs.empty() )
       {
        notify_issue("No libraries found");
       }

    return libs;
}
//---------------------------------------------------------------------------
[[nodiscard]] libs_t collect_linked_libs(const std::string_view bytes, const utxt::Enc bytes_enc, std::string&& file_path, fnotify_t const& notify_issue)
{
    TEXT_DISPATCH_TO_ENC(bytes_enc, collect_linked_libs<, >(bytes, std::move(file_path), notify_issue))
}

//---------------------------------------------------------------------------
// Parse original project detecting contained libs
[[nodiscard]] libs_t parse_project_file( const fs::path& project_file_path, const std::string_view project_file_bytes, utxt::Enc project_bytes_enc, fnotify_t const& notify_issue )
{
    // Temporarily switch current path to properly resolve libraries paths relative to project file
    fsu::CurrentPathLocalChanger curr_path_changed( project_file_path.parent_path() );

    return collect_linked_libs(project_file_bytes, project_bytes_enc, project_file_path.string(), notify_issue);
}


//---------------------------------------------------------------------------
template<utxt::Enc ENC>
[[nodiscard]] std::string_view get_plclib_content(const std::string_view plclib_bytes, std::string&& file_path)
{
    text::xml::Parser<ENC> parser{plclib_bytes};
    parser.set_file_path( std::move(file_path) );

    // Seeking <lib>
    while( parser.next_event() and not parser.curr_event().is_open_tag(library_tag_name) );
    if( not parser.curr_event() )
       {
        throw parser.create_parse_error( fmt::format("Invalid plclib (<{}> not found)"sv, utxt::to_utf8(library_tag_name)), 1 );
       }
    const auto start_line = parser.curr_line();
    parser.next_event();
    const auto chunk_start = parser.curr_event().start_byte_offset();

    // Seeking </lib>
    do {
        if( parser.curr_event().is_close_tag(library_tag_name) )
           {
            break;
           }
        else if( parser.curr_event().is_open_tag(library_tag_name) )
           {
            throw parser.create_parse_error( fmt::format("Invalid plclib (unexpected nested <{}>)"sv, utxt::to_utf8(library_tag_name)) );
           }
       }
    while( parser.next_event() );
    if( not parser.curr_event() )
       {
        throw parser.create_parse_error( fmt::format("Invalid plclib (unclosed <{}>)"sv, utxt::to_utf8(library_tag_name)), start_line );
       }

    return plclib_bytes.substr(chunk_start, parser.curr_event().start_byte_offset()-chunk_start);
}
//---------------------------------------------------------------------------
[[nodiscard]] std::string_view get_plclib_content(const std::string_view plclib_bytes, std::string&& file_path)
{
    const auto [plclib_bytes_enc, bom_size] = utxt::detect_encoding_of(plclib_bytes);

    TEXT_DISPATCH_TO_ENC(plclib_bytes_enc, get_plclib_content<, >(plclib_bytes, std::move(file_path)))
}


//---------------------------------------------------------------------------
template<utxt::Enc out_enc>
void insert_library(const lib_t& lib, sys::file_write& out_file)
{
    const sys::memory_mapped_file lib_file_mapped{ lib.path.string().c_str() };
    std::string reencoded_buf;

    if( lib.type==library_type::plclib )
       {// Insert the content of <lib> tag
        const std::string_view content_to_insert = get_plclib_content( lib_file_mapped.as_string_view(), lib.path.string() );
        out_file << utxt::encode_if_necessary_as<out_enc>(content_to_insert, reencoded_buf);
       }
    else
       {// Insert the file content (excluding BOM) in a CDATA section
        out_file << utxt::encode_as<out_enc>(U"<![CDATA["sv);

        out_file << utxt::encode_if_necessary_as<out_enc>(lib_file_mapped.as_string_view(), reencoded_buf, utxt::flag::SKIP_BOM);

        out_file << utxt::encode_as<out_enc>(U"]]>"sv);
       }
}
//---------------------------------------------------------------------------
void insert_library(const lib_t& lib, sys::file_write& out_file, const utxt::Enc original_bytes_enc)
{
    TEXT_DISPATCH_TO_ENC(original_bytes_enc, insert_library<, >(lib, out_file))
}


//---------------------------------------------------------------------------
void write_project_file(const fs::path& output_file_path, const std::string_view original_bytes, const utxt::Enc original_bytes_enc, const libs_t& libs)
{
    sys::file_write out_file{ output_file_path.string().c_str() };
    out_file.set_buffer_size(4_MB);
    std::size_t i_chunk_byte_offset = 0;

    for( const auto& lib: libs)
       {
      #ifdef _DEBUG
        fmt::print("lib {} at chunk {}-{}\n", lib.path.string(), lib.chunk_start, lib.chunk_end);
      #endif
        // Note: No need to re-encode when copying the original content
        out_file << original_bytes.substr(i_chunk_byte_offset, lib.chunk_start-i_chunk_byte_offset);
        i_chunk_byte_offset = lib.chunk_end;

        insert_library(lib, out_file, original_bytes_enc);
       }

    out_file << original_bytes.substr(i_chunk_byte_offset);
}


//---------------------------------------------------------------------------
void parse_and_rewrite_project( const fs::path& project_file_path, const fs::path& output_file_path, fnotify_t const& notify_issue )
{
    const sys::memory_mapped_file project_file_mapped{ project_file_path.string().c_str() };
    const std::string_view project_file_bytes{ project_file_mapped.as_string_view() };
    if( project_file_bytes.empty() )
       {
        throw std::runtime_error{"No data to parse (empty file?)"};
       }

    const auto [project_bytes_enc, bom_size] = utxt::detect_encoding_of(project_file_bytes);

    const libs_t libs = parse_project_file(project_file_path, project_file_bytes, project_bytes_enc, notify_issue);

    try{
        write_project_file(output_file_path, project_file_bytes, project_bytes_enc, libs);
       }
    catch(...)
       {// Housekeeping, don't leave a half-baked project around
        if( fsu::exists(output_file_path) and not fsu::equivalent(project_file_path,output_file_path) )
           {
            fs::remove(output_file_path);
           }
        throw;
       }
}


//---------------------------------------------------------------------------
void update_project_libraries( const fs::path& project_file_path, fs::path output_file_path, fnotify_t const& notify_issue )
{
    const bool overwrite_original = output_file_path.empty();
    if( overwrite_original )
       {
        output_file_path = fsu::get_a_temporary_path_for(project_file_path);
       }

    // Ensure that the output file is not the original project!
    if( fs::exists(output_file_path) and fs::equivalent(project_file_path,output_file_path) )
       {
        throw std::runtime_error{ fmt::format("Specified output \"{}\" collides with original file", output_file_path.string()) };
       }

    parse_and_rewrite_project(project_file_path, output_file_path, notify_issue);

    if( overwrite_original )
       {
        //fsu::backup_file(project_file_path);
        fs::copy_file(output_file_path, project_file_path, fs::copy_options::overwrite_existing);
        fs::remove(output_file_path);
       }
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"project_updater"> project_updater_tests = []
{

ut::test("ll::update_project_libraries()") = []
   {
    ut::should("Updating a nonexistent file") = []
       {
        ut::expect( ut::throws([]{ ll::update_project_libraries({"not-existing.ppjs"}, {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
       };

    ut::should("Updating an empty project") = []
       {
        test::TemporaryFile empty_prj("~empty.ppjs", "");
        ut::expect( ut::throws([&empty_prj]{ ll::update_project_libraries(empty_prj.path(), {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
       };

    ut::should("Updating a project with no libs") = []
       {
        test::TemporaryFile nolibs_prj("~nolibs.ppjs", "<plcProject>\n<libraries>\n</libraries>\n</plcProject>\n");
        int num_issues = 0;
        auto notify_issue = [&num_issues](std::string&&) noexcept { ++num_issues; };
        ll::update_project_libraries(nolibs_prj.path(), {}, notify_issue);
        ut::expect( ut::that % num_issues==1 ) << "should raise an issue\n";
       };

    ut::should("Using the project itself as output") = []
       {
        test::TemporaryFile fake_prj("~fake_prj.ppjs", "<plcProject>\n<libraries>\n</libraries>\n</plcProject>\n");
        ut::expect( ut::throws([&fake_prj]{ ll::update_project_libraries(fake_prj.path(), fake_prj.path(), [](std::string&&)noexcept{}); }) ) << "should throw\n";
       };

    ut::should("Updating an ill formed project") = []
       {
        test::TemporaryFile bad_prj("~bad.ppjs", "<foo>");
        ut::expect( ut::throws([&bad_prj]{ ll::update_project_libraries(bad_prj.path(), {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
       };

    ut::should("Updating a project with an ill formed lib") = []
       {
        test::TemporaryFile bad_plclib("~bad.plclib", "<lib><lib>forbidden nested</lib></lib>");
        test::TemporaryFile prj_with_badplclib("~prj_with_badplclib.ppjs",
                                                "<plcProject>\n"
                                                "    <libraries>\n"
                                                "        <lib link=\"true\" name=\"~bad.plclib\"></lib>\n"
                                                "    </libraries>\n"
                                                "</plcProject>\n" );
        ut::expect( ut::throws([&bad_plclib, &prj_with_badplclib]{ ll::update_project_libraries(prj_with_badplclib.path(), {}, [](std::string&&)noexcept{}); }) ) << "should throw\n";
       };

    ut::should("Updating a project with a nonexistent lib") = []
       {
        test::TemporaryFile existing("~existing.pll", "content");
        test::TemporaryFile prj_with_nonextlib("~prj_with_nonextlib.ppjs",
                                                "<plcProject>\n"
                                                "    <libraries>\n"
                                                "        <lib link=\"true\" name=\"~existing.pll\"></lib>\n"
                                                "        <lib link=\"true\" name=\"not-existing.pll\"></lib>\n"
                                                "    </libraries>\n"
                                                "</plcProject>\n");
        int num_issues = 0;
        auto notify_issue = [&num_issues](std::string&&) noexcept { ++num_issues; };
        ll::update_project_libraries(prj_with_nonextlib.path(), {}, notify_issue);
        ut::expect( ut::that % num_issues==1 ) << "should raise an issue\n";
       };

    ut::should("update simple") = []
       {
        test::TemporaryDirectory tmp_dir;
        tmp_dir.add_file("pll1.pll", "abc");
        tmp_dir.add_file("pll2.pll", "def");
        const auto prj_file = tmp_dir.add_file("prj.ppjs",
            "\uFEFF"
            "<plcProject>\n"
            "    <libraries>\n"
            "        <lib link=\"true\" name=\"pll1.pll\"><![CDATA[prev]]></lib>\n"
            "        <lib link=\"true\" name=\"pll2.pll\"></lib>\n"
            "    </libraries>\n"
            "</plcProject>\n"sv);

        const std::string_view expected =
            "\uFEFF"
            "<plcProject>\n"
            "    <libraries>\n"
            "        <lib link=\"true\" name=\"pll1.pll\"><![CDATA[abc]]></lib>\n"
            "        <lib link=\"true\" name=\"pll2.pll\"><![CDATA[def]]></lib>\n"
            "    </libraries>\n"
            "</plcProject>\n"sv;

        ll::update_project_libraries(prj_file.path(), {}, [](std::string&&)noexcept{});

        ut::expect( ut::that % prj_file.has_content(expected) );
       };
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
