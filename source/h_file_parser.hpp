﻿#pragma once
//  ---------------------------------------------
//  Extract PLC elements from a Sipro header
//  ---------------------------------------------
//  #include "h_file_parser.hpp" // sipro::h_parse()
//  ---------------------------------------------

#include "sipro.hpp" // sipro::Register
#include "plc_library.hpp" // plcb::*
#include "h_parser.hpp" // h::Parser, h::Define


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sipro
{

//---------------------------------------------------------------------------
void export_register(const sipro::Register& reg, const h::Define& def, std::vector<plcb::Variable>& vars)
{
    plcb::Variable& var = vars.emplace_back();

    var.set_name( def.label() );
    var.set_type( reg.iec_type() );
    if( reg.is_va() )
       {
        var.set_length( reg.get_va_length() );
       }
    if( def.has_comment() )
       {
        var.set_descr( def.comment() );
       }
    var.address().set_type( reg.iec_address_type() );
    var.address().set_typevar( reg.iec_address_vartype() );
    var.address().set_index( reg.iec_address_index() );
    var.address().set_subindex( reg.index() );
}


//---------------------------------------------------------------------------
void export_constant(const h::Define& def, std::vector<plcb::Variable>& consts)
{
    plcb::Variable& var = consts.emplace_back();

    var.set_name( def.label() );
    var.set_type( def.comment_predecl() );

    var.set_value( def.value() );
    if( def.has_comment() )
       {
        var.set_descr( def.comment() );
       }
}


//---------------------------------------------------------------------------
// Parse a Sipro header file
void h_parse(const std::string& file_path, const std::string_view buf, plcb::Library& lib, fnotify_t const& notify_issue)
{
    h::Parser parser{buf};
    parser.set_on_notify_issue(notify_issue);
    parser.set_file_path( file_path );

    // Prepare the library containers for exported data
    auto& vars = lib.global_variables().groups().emplace_back();
    vars.set_name("Header_Variables");
    auto& consts = lib.global_constants().groups().emplace_back();
    consts.set_name("Header_Constants");

    while( const h::Define& def = parser.next_define() )
       {
        // Check if it's a define that must be exported
        if( const sipro::Register reg(def.value());
            reg.is_valid() )
           {// Got something like "vnName vn1782 // descr"
            export_register(reg, def, vars.mutable_variables());
           }
        else if( def.value_is_number() and plc::is_num_type(def.comment_predecl()) )
           {// got something like "LABEL 123 // [INT] descr"
            export_constant(def, consts.mutable_variables());
           }
       }

    if( vars.variables().empty() and consts.variables().empty() )
       {
        notify_issue( fmt::format("No exportable defines found in \"{}\"", file_path) );
       }
}

} //::::::::::::::::::::::::::::::::: sipro :::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"h_file_parser"> h_file_parser_tests = []
{////////////////////////////////////////////////////////////////////////////
ut::test("sipro::h_parse()") = []
   {
    plcb::Library lib("test"sv);
    const std::string_view buf =
        "#define vbSample vb123 // A vb var\n"
        "#define vnSample vn123 // A vn var\n"
        "#define vqSample vq123 // A vq var\n"
        "#define vaSample va123 // A va var\n"
        "#define vdSample vd123 // A vd var\n"
        "#define macro value // A not exported define\n"
        "#define int 42 // [INT] An int constant\n"
        "#define dint 100 // [DINT] A dint constant\n"
        "#define double 12.3 // [LREAL] A double constant\n"
        "#define num 43 // A not exported number\n"
        "\n"sv;
    sipro::h_parse("", buf, lib, [](std::string&&)noexcept{});

    ut::expect( lib.global_retainvars().is_empty() );
    ut::expect( lib.functions().empty() );
    ut::expect( lib.function_blocks().empty() );
    ut::expect( lib.programs().empty() );
    ut::expect( lib.macros().empty() );
    ut::expect( lib.structs().empty() );
    ut::expect( lib.typedefs().empty() );
    ut::expect( lib.enums().empty() );
    ut::expect( lib.subranges().empty() );

    const auto& gvars_groups = lib.global_variables().groups();
    ut::expect( ut::fatal(gvars_groups.size() == 1u) );
    const auto& gvars = gvars_groups.front().variables();
    ut::expect( ut::fatal(gvars.size() == 5u) );

    ut::expect( ut::that % plcb::to_string(gvars[0]) == "vbSample BOOL 'A vb var' <MB300.123>"sv );
    ut::expect( ut::that % plcb::to_string(gvars[1]) == "vnSample INT 'A vn var' <MW400.123>"sv );
    ut::expect( ut::that % plcb::to_string(gvars[2]) == "vqSample DINT 'A vq var' <MD500.123>"sv );
    ut::expect( ut::that % plcb::to_string(gvars[3]) == "vaSample STRING[80] 'A va var' <MB700.123>"sv );
    ut::expect( ut::that % plcb::to_string(gvars[4]) == "vdSample LREAL 'A vd var' <ML600.123>"sv );

    const auto& gconsts_groups = lib.global_constants().groups();
    ut::expect( ut::fatal(gconsts_groups.size() == 1u) );
    const auto& gconsts = gconsts_groups.front().variables();
    ut::expect( ut::fatal(gconsts.size() == 3u) );

    ut::expect( ut::that % plcb::to_string(gconsts[0]) == "int INT 'An int constant' (=42)"sv );
    ut::expect( ut::that % plcb::to_string(gconsts[1]) == "dint DINT 'A dint constant' (=100)"sv );
    ut::expect( ut::that % plcb::to_string(gconsts[2]) == "double LREAL 'A double constant' (=12.3)"sv );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
