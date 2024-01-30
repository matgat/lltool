#pragma once
//  ---------------------------------------------
//  Writes lib to LogicLab 'pll' format
//  ---------------------------------------------
//  #include "writer_pll.hpp" // pll::write()
//  ---------------------------------------------
#include <fmt/format.h> // fmt::format

#include "output_streamable_concept.hpp" // MG::OutputStreamable
#include "keyvals.hpp" // MG::keyvals
#include "timestamp.hpp" // MG::get_human_readable_timestamp()
#include "plc_library.hpp" // plcb::*

using namespace std::string_view_literals; // Use "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace pll
{

//---------------------------------------------------------------------------
// Write variable to pll file
inline void write(MG::OutputStreamable auto& f, const plcb::Variable& var)
{
    assert( !var.name().empty() );

    f<< '\t' << var.name();

    if( var.has_address() )
       {
        f<< " AT %"sv << var.address().type()
                      << var.address().typevar()
                      << std::to_string(var.address().index())
                      << '.'
                      << std::to_string(var.address().subindex());
       }

    f<< " : "sv;

    if( var.has_length() )
       {// STRING[ 80 ]
        f<< var.type() << "[ "sv << std::to_string(var.length()) << " ]"sv;
       }
    else if( var.is_array() )
       {// ARRAY[ 0..999 ] OF BOOL
        f<< "ARRAY[ "sv << std::to_string(var.array_startidx()) << ".."sv << std::to_string(var.array_lastidx()) << " ] OF "sv << var.type();
       }
    else
       {// DINT
        f<< var.type();
       }

    if( var.has_value() )
       {
        f<< " := "sv << var.value();
       }

    f<< ';';

    if( var.has_descr() )
       {
        f<< " { DE:\""sv << var.descr() << "\" }\n"sv;
       }
}



//---------------------------------------------------------------------------
// Write POU to pll file
inline void write(const sys::file_write& f, const plcb::Pou& pou, const std::string_view tag)
{
    f << '\n' << tag << ' ' << pou.name();
    if( pou.has_return_type() )
       {
        f << " : "sv << pou.return_type();
       }
    f<< '\n';

    if( pou.has_descr() )
       {
        f << "\n{ DE:\""sv << pou.descr() << "\" }\n"sv;
       }

    // [Variables]
    if( !pou.inout_vars().empty() )
       {
        f << "\n\tVAR_IN_OUT\n"sv;
        for( const auto& var : pou.inout_vars() ) write(f, var);
        f << "\tEND_VAR\n"sv;
       }
    if( !pou.input_vars().empty() )
       {
        f << "\n\tVAR_INPUT\n"sv;
        for( const auto& var : pou.input_vars() ) write(f, var);
        f << "\tEND_VAR\n"sv;
       }
    if( !pou.output_vars().empty() )
       {
        f << "\n\tVAR_OUTPUT\n"sv;
        for( const auto& var : pou.output_vars() ) write(f, var);
        f << "\tEND_VAR\n"sv;
       }
    if( !pou.external_vars().empty() )
       {
        f << "\n\tVAR_EXTERNAL\n"sv;
        for( const auto& var : pou.external_vars() ) write(f, var);
        f << "\tEND_VAR\n"sv;
       }
    if( !pou.local_vars().empty() )
       {
        f << "\n\tVAR\n"sv;
        for( const auto& var : pou.local_vars() ) write(f, var);
        f << "\tEND_VAR\n"sv;
       }
    if( !pou.local_constants().empty() )
       {
        f << "\n\tVAR CONSTANT\n"sv;
        for( const auto& var : pou.local_constants() ) write(f, var);
        f << "\tEND_VAR\n"sv;
       }

    // [Body]
    f << "\n\t{ CODE:"sv << pou.code_type() << " }"sv
      << pou.body();
    if( !pou.body().ends_with('\n') ) f << '\n';
    f << "END_"sv << tag << "\n\n"sv;
}


//---------------------------------------------------------------------------
// Write macro to plclib file
inline void write(const sys::file_write& f, const plcb::Macro& macro)
{
    f << "\nMACRO "sv << macro.name() << '\n';

    if( macro.has_descr() )
       {
        f << "{ DE:\""sv << macro.descr() << "\" }\n"sv;
       }

    // [Parameters]
    if( !macro.parameters().empty() )
       {
        f << "\n\tPAR_MACRO\n"sv;
        for( const auto& par : macro.parameters() )
           {
            f << '\t' << par.name() << "; { DE:\""sv << par.descr() << "\" }\n"sv;
           }
        f << "\tEND_PAR\n"sv;
       }

    // [Body]
    f << "\n\t{ CODE:"sv << macro.code_type() << " }"sv
      << macro.body();
    if( !macro.body().ends_with('\n') ) f << '\n';
    f << "END_MACRO\n\n"sv;
}


//---------------------------------------------------------------------------
// Write library to pll file
void write(const sys::file_write& f, const plcb::Library& lib, [[maybe_unused]] const MG::keyvals& options)
{
    // [Options]
    //auto xxx = options.value_of("xxx");
    const std::string_view sects_spacer = "\n\n\n"sv;
    const std::string_view blocks_spacer = "\n\n"sv;

    // [Heading]
    f<< "(*\n"sv
     << "\tname: "sv << lib.name() << '\n'
     << "\tdescr: "sv << lib.descr() << '\n'
     << "\tversion: "sv << lib.version() << '\n'
     << "\tauthor: pll::write()\n"sv;

    if( not options.contains("no-timestamp") )
       {
        f<< "\tdate: "sv << MG::get_human_readable_timestamp() << "\n\n"sv;
       }

    // Content summary
    if( !lib.global_variables().is_empty() )  f<< "\tglobal-variables: "sv << std::to_string(lib.global_variables().size()) << '\n';
    if( !lib.global_constants().is_empty() )  f<< "\tglobal-constants: "sv << std::to_string(lib.global_constants().size()) << '\n';
    if( !lib.global_retainvars().is_empty() ) f<< "\tglobal-retain-vars: "sv << std::to_string(lib.global_retainvars().size()) << '\n';
    if( !lib.functions().empty() )            f<< "\tfunctions: "sv << std::to_string(lib.functions().size()) << '\n';
    if( !lib.function_blocks().empty() )      f<< "\tfunction blocks: "sv << std::to_string(lib.function_blocks().size()) << '\n';
    if( !lib.programs().empty() )             f<< "\tprograms: "sv << std::to_string(lib.programs().size()) << '\n';
    if( !lib.macros().empty() )               f<< "\tmacros: "sv << std::to_string(lib.macros().size()) << '\n';
    if( !lib.structs().empty() )              f<< "\tstructs: "sv << std::to_string(lib.structs().size()) << '\n';
    if( !lib.typedefs().empty() )             f<< "\ttypedefs: "sv << std::to_string(lib.typedefs().size()) << '\n';
    if( !lib.enums().empty() )                f<< "\tenums: "sv << std::to_string(lib.enums().size()) << '\n';
    if( !lib.subranges().empty() )            f<< "\tsubranges: "sv << std::to_string(lib.subranges().size()) << '\n';
    //if( !lib.interfaces().empty() )           f<< "\tinterfaces: "sv << std::to_string(lib.interfaces().size()) << '\n';
    f << "*)\n"sv;

    // [Global variables]
    if( !lib.global_variables().is_empty() ||
        !lib.global_retainvars().is_empty() )
       {
        f<< sects_spacer <<
            "\t(****************************)\n"
            "\t(*                          *)\n"
            "\t(*     GLOBAL VARIABLES     *)\n"
            "\t(*                          *)\n"
            "\t(****************************)\n"sv;
        f<< blocks_spacer <<
            "\tVAR_GLOBAL\n"sv;
        for( const auto& group : lib.global_variables().groups() )
           {
            if( !group.name().empty() ) f << "\t{G:\""sv << group.name() << "\"}\n"sv;
            for( const auto& var : group.variables() ) write(f, var);
           }
        for( const auto& group : lib.global_retainvars().groups() )
           {
            if( !group.name().empty() ) f << "\t{G:\""sv << group.name() << "\"}\n"sv;
            for( const auto& var : group.variables() ) write(f, var);
           }
        f<< "\tEND_VAR\n"sv;
       }

    // [Global constants]
    if( !lib.global_constants().is_empty() )
       {
        f<< sects_spacer <<
            "\t(****************************)\n"
            "\t(*                          *)\n"
            "\t(*     GLOBAL CONSTANTS     *)\n"
            "\t(*                          *)\n"
            "\t(****************************)\n"sv;
        f<< blocks_spacer <<
            "\tVAR_GLOBAL CONSTANT\n"sv;
        for( const auto& group : lib.global_constants().groups() )
           {
            if( !group.name().empty() ) f << "\t{G:\""sv << group.name() << "\"}\n"sv;
            for( const auto& var : group.variables() ) write(f, var);
           }
        f<< "\tEND_VAR\n"sv;
       }

    // [Functions]
    if( !lib.functions().empty() )
       {
        f<< sects_spacer <<
            "\t(*********************)\n"
            "\t(*                   *)\n"
            "\t(*     FUNCTIONS     *)\n"
            "\t(*                   *)\n"
            "\t(*********************)\n"sv;
        for( const auto& pou : lib.functions() )
           {
            f<< blocks_spacer;
            write(f, pou, "FUNCTION"sv);
           }
       }

    // [FunctionBlocks]
    if( !lib.function_blocks().empty() )
       {
        f<< sects_spacer <<
            "\t(***************************)\n"
            "\t(*                         *)\n"
            "\t(*     FUNCTION BLOCKS     *)\n"
            "\t(*                         *)\n"
            "\t(***************************)\n"sv;
        for( const auto& pou : lib.function_blocks() )
           {
            f<< blocks_spacer;
            write(f, pou, "FUNCTION_BLOCK"sv);
           }
       }

    // [Programs]
    if( !lib.programs().empty() )
       {
        f<< sects_spacer <<
            "\t(********************)\n"
            "\t(*                  *)\n"
            "\t(*     PROGRAMS     *)\n"
            "\t(*                  *)\n"
            "\t(********************)\n"sv;
        for( const auto& pou : lib.programs() )
           {
            f<< blocks_spacer;
            write(f, pou, "PROGRAM"sv);
           }
       }

    // [Enums]
    if( !lib.enums().empty() )
       {
        f<< sects_spacer <<
            "\t(*****************)\n"
            "\t(*               *)\n"
            "\t(*     ENUMS     *)\n"
            "\t(*               *)\n"
            "\t(*****************)\n"sv;
        f<< blocks_spacer <<
            "TYPE\n\n"sv;
        for( const auto& en : lib.enums() )
           {
            f<< "\n\t"sv << en.name() << ": (\n"sv;
            if(!en.descr().empty()) f<< "\t\t{ DE:\""sv << en.descr() << "\" }\n"sv;
            if( !en.elements().empty() )
               {
                const auto ie_last = std::prev( en.elements().cend() );
                for( auto ie=en.elements().cbegin(); ie!=ie_last; ++ie )
                   {
                    f<< "\t\t"sv << ie->name() << " := "sv << ie->value() << ',';
                    if(!ie->descr().empty()) f<< " { DE:\""sv << ie->descr() << "\" }"sv;
                    f<< '\n';
                   }
                // Last element
                f<< "\t\t"sv << ie_last->name() << " := "sv << ie_last->value();
                if(!ie_last->descr().empty()) f<< " { DE:\""sv << ie_last->descr() << "\" }"sv;
                f<< '\n';
               }
            f<< "\t);\n"sv;
           }
        f<< "\nEND_TYPE\n"sv;
       }

    // [Typedefs]
    if( !lib.typedefs().empty() )
       {
        f<< sects_spacer <<
            "\t(********************)\n"
            "\t(*                  *)\n"
            "\t(*     TYPEDEFS     *)\n"
            "\t(*                  *)\n"
            "\t(********************)\n"sv;
        f<< blocks_spacer <<
            "TYPE\n\n"sv;
        for( const auto& tdef : lib.typedefs() )
           {
            f<< '\t' << tdef.name() << " : "sv;

            if( tdef.has_length() )
               {// STRING[ 80 ]
                f<< tdef.type() << "[ "sv << std::to_string(tdef.length()) << " ]"sv;
               }
            else if( tdef.is_array() )
               {// ARRAY[ 0..999 ] OF BOOL
                f<< "ARRAY[ "sv << std::to_string(tdef.array_startidx()) << ".."sv << std::to_string(tdef.array_lastidx()) << " ] OF "sv << tdef.type();
               }
            else
               {// DINT
                f<< tdef.type();
               }
            f<< ';';

            if(!tdef.descr().empty()) f<< " { DE:\""sv << tdef.descr() << "\" }"sv;
            f<< '\n';
           }
        f<< "\nEND_TYPE\n"sv;
       }

    // [Structs]
    if( !lib.structs().empty() )
       {
        f<< sects_spacer <<
            "\t(*******************)\n"
            "\t(*                 *)\n"
            "\t(*     STRUCTS     *)\n"
            "\t(*                 *)\n"
            "\t(*******************)\n"sv;
        f<< blocks_spacer <<
            "TYPE\n\n"sv;
        for( const auto& strct : lib.structs() )
           {
            f<< '\t' << strct.name() << " : STRUCT"sv;
            if(!strct.descr().empty()) f<< " { DE:\""sv << strct.descr() << "\" }"sv;
            f<< '\n';
            for( const auto& var : strct.members() )
               {
                f<< "\t\t"sv << var.name() << " : "sv << var.type() << ';';
                if(!var.descr().empty()) f<< " { DE:\""sv << var.descr() << "\" }"sv;
                f<< '\n';
               }
            f<< "\tEND_STRUCT;\n\n"sv;
           }
        f<< "\nEND_TYPE\n"sv;
       }

    // [Subranges]
    if( !lib.subranges().empty() )
       {
        f<< sects_spacer <<
            "\t(*********************)\n"
            "\t(*                   *)\n"
            "\t(*     SUBRANGES     *)\n"
            "\t(*                   *)\n"
            "\t(*********************)\n"sv;
        f<< blocks_spacer <<
            "TYPE\n\n"sv;
        for( const auto& subr : lib.subranges() )
           {
            f<< '\t' << subr.name() << " : "sv << subr.type()
             << " ("sv << std::to_string(subr.min_value()) << ".."sv << std::to_string(subr.max_value()) << ");"sv;
            if(!subr.descr().empty()) f<< " { DE:\""sv << subr.descr() << "\" }"sv;
            f<< '\n';
           }
        f<< "\nEND_TYPE\n"sv;
       }

    // [Macros]
    if( !lib.macros().empty() )
       {
        f<< sects_spacer <<
            "\t(********************)\n"
            "\t(*                  *)\n"
            "\t(*      MACROS      *)\n"
            "\t(*                  *)\n"
            "\t(********************)\n"
            "\n"sv;
        for( const auto& macro : lib.macros() )
           {
            f<< blocks_spacer;
            write(f, macro);
           }
       }

    // [Interfaces]
    //if( !lib.interfaces().empty() )
    //   {
    //    f<< sects_spacer <<
    //        "\t(**********************)\n"
    //        "\t(*                    *)\n"
    //        "\t(*     INTERFACES     *)\n"
    //        "\t(*                    *)\n"
    //        "\t(**********************)\n"sv;
    //    for( const auto& intfc : lib.interfaces() )
    //       {
    //        write(f, intfc);
    //       }
    //   }
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
#include <sstream> // std::stringstream
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"writer_pll"> writer_pll_tests = []
{

ut::test("pll::write(plcb::Variable)") = []
   {
    plcb::Variable var;

    var.set_name( "varname" );
    var.set_type( "INT" );
    //var.set_length( 80 );
    var.set_descr( "testing variable" );

    var.address().set_type( 'M' );
    var.address().set_typevar( 'B' );
    var.address().set_index( 700 );
    var.address().set_subindex( 320 );

    std::stringstream out;
    pll::write(out, var);
    ut::expect( ut::that % out.str() == "\tvarname AT %MB700.320 : INT; { DE:\"testing variable\" }\n"sv );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
