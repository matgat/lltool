#pragma once
//  ---------------------------------------------
//  Extract PLC elements from a Sipro header
//  ---------------------------------------------
//  #include "h_file_parser.hpp" // sipro::h_parse()
//  ---------------------------------------------

#include "sipro.hpp" // sipro::Register
#include "plc_library.hpp" // plcb::*
#include "h_parser.hpp" // h::Parser, h::Define



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sipro {
{

//---------------------------------------------------------------------------
void export_register(const sipro::Register& reg, const h::Define& def, std::vector<plcb::Variable>& vars)
{
    plcb::Variable var;

    var.set_name( def.label() );
    var.set_type( reg.iec_type() );
    if( reg.is_va() ) var.set_length( reg.get_va_length() );
    if( def.has_comment() ) var.set_descr( def.comment() );

    var.address().set_type( reg.iec_address_type() );
    var.address().set_typevar( reg.iec_address_vartype() );
    var.address().set_index( reg.iec_address_index() );
    var.address().set_subindex( reg.index() );

    vars.push_back( var );
}


//---------------------------------------------------------------------------
void export_constant(const h::Define& def, std::vector<plcb::Variable>& consts)
{
    plcb::Variable var;

    var.set_name( def.label() );
    var.set_type( def.comment_predecl() );

    var.set_value( def.value() );
    if( def.has_comment() ) var.set_descr( def.comment() );

    consts.push_back( var );
}


//---------------------------------------------------------------------------
// Parse a Sipro header file
void h_parse(const std::string& file_path, const std::string_view buf, plcb::Library& lib, fnotify_t const& notify_issue)
{
    h::Parser parser(file_path, buf, issues, fussy);
    parser.set_on_notify_issue(notify_issue);
    parser.set_file_path( file_path );

    // Prepare the library containers for header data
    auto& vars = lib.global_variables().groups().emplace_back();
    vars.set_name("Header_Variables");
    auto& consts = lib.global_constants().groups().emplace_back();
    consts.set_name("Header_Constants");

    while( const h::Define def = parser.next_define() )
       {
        //DLOG1("h::Define - label=\"{}\" value=\"{}\" comment=\"{}\" predecl=\"{}\"\n", def.label(), def.value(), str::iso_latin1_to_utf8(def.comment()), def.comment_predecl())

        // Must export these:
        //
        // Sipro registers
        // vnName     vn1782  // descr
        //             ? Sipro register
        //
        // Numeric constants
        // LABEL     123       // [INT] Descr
        //            ? Value       ? IEC61131-3 type

        // Check if it's a Sipro register
        if( const sipro::Register reg(def.value());
            reg.is_valid() )
           {
            export_register(reg, def, vars.mutable_variables());
           }

        // Check if it's a numeric constant to be exported
        else if( def.value_is_number() )
           {
            // Must be exported to PLC?
            if( plc::is_num_type(def.comment_predecl()) )
               {
                export_constant(def, consts.mutable_variables());
               }
            //else
            //   {
            //    issues.push_back(fmt::format("{} value not exported: {}={} ({})", def.comment_predecl(), def.label(), def.value()));
            //   }
           }

        //else if( superfussy )
        //   {
        //    issues.push_back(fmt::format("h::Define not exported: {}={}"sv, def.label(), def.value()));
        //   }
       }

    if( vars.variables().empty() and consts.variables().empty() )
       {
        notify_issue( fmt::format("No exportable defines found in {}", file_path) );
       }
}

} //::::::::::::::::::::::::::::::::: sipro :::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
static ut::suite<"h_file_parser"> h_file_parser_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("sipro::h_parse()") = []
   {
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
