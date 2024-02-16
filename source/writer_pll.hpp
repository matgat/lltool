#pragma once
//  ---------------------------------------------
//  Writes lib to LogicLab 'pll' format
//  ---------------------------------------------
//  #include "writer_pll.hpp" // pll::write_lib()
//  ---------------------------------------------
#include <cassert>

#include "keyvals.hpp" // MG::keyvals
#include "timestamp.hpp" // MG::get_human_readable_timestamp()
#include "plc_library.hpp" // plcb::*
#include "output_streamable_concept.hpp" // MG::OutputStreamable

using namespace std::string_view_literals; // Use "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace pll
{

//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Type& typ)
{
    if( typ.has_length() )
       {// STRING[ 80 ]
        f<< typ.name() << "[ "sv << std::to_string(typ.length()) << " ]"sv;
       }
    else if( typ.is_array() )
       {// ARRAY[ 0..999 ] OF BOOL
        f<< "ARRAY[ "sv << std::to_string(typ.array_startidx()) << ".."sv << std::to_string(typ.array_lastidx()) << " ] OF "sv << typ.name();
       }
    else
       {// DINT
        f<< typ.name();
       }
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Variable& var)
{
    assert( not var.name().empty() );

    f<< '\t' << var.name();

    if( var.has_address() )
       {
        f<< " AT %"sv << var.address().zone()
                      << var.address().typevar()
                      << std::to_string(var.address().index())
                      << '.'
                      << std::to_string(var.address().subindex());
       }

    f<< " : "sv;
    write(f, var.type());

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
inline void write(MG::OutputStreamable auto& f, const plcb::Pou& pou, const std::string_view tag)
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
    if( not pou.inout_vars().empty() )
       {
        f << "\n\tVAR_IN_OUT\n"sv;
        for( const auto& var : pou.inout_vars() ) write(f, var);
        f << "\tEND_VAR\n"sv;
       }
    if( not pou.input_vars().empty() )
       {
        f << "\n\tVAR_INPUT\n"sv;
        for( const auto& var : pou.input_vars() ) write(f, var);
        f << "\tEND_VAR\n"sv;
       }
    if( not pou.output_vars().empty() )
       {
        f << "\n\tVAR_OUTPUT\n"sv;
        for( const auto& var : pou.output_vars() ) write(f, var);
        f << "\tEND_VAR\n"sv;
       }
    if( not pou.external_vars().empty() )
       {
        f << "\n\tVAR_EXTERNAL\n"sv;
        for( const auto& var : pou.external_vars() ) write(f, var);
        f << "\tEND_VAR\n"sv;
       }
    if( not pou.local_vars().empty() )
       {
        f << "\n\tVAR\n"sv;
        for( const auto& var : pou.local_vars() ) write(f, var);
        f << "\tEND_VAR\n"sv;
       }
    if( not pou.local_constants().empty() )
       {
        f << "\n\tVAR CONSTANT\n"sv;
        for( const auto& var : pou.local_constants() ) write(f, var);
        f << "\tEND_VAR\n"sv;
       }

    // [Body]
    f << "\n\t{ CODE:"sv << pou.code_type() << " }"sv
      << pou.body();
    if( not pou.body().ends_with('\n') ) f << '\n';
    f << "END_"sv << tag << "\n"sv;
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Struct& strct)
{
    f<< "\n\t"sv << strct.name() << " : STRUCT"sv;

    if( strct.has_descr() )
       {
        f<< " { DE:\""sv << strct.descr() << "\" }"sv;
       }
    f<< '\n';

    for( const auto& memb : strct.members() )
       {
        f<< "\t\t"sv << memb.name() << " : "sv;
        write(f, memb.type());
        f<< ';';

        if( memb.has_descr() )
           {
            f<< " { DE:\""sv << memb.descr() << "\" }"sv;
           }
        f<< '\n';
       }
    f<< "\tEND_STRUCT;\n"sv;
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Enum& enm)
{
    f<< "\n\t"sv << enm.name() << ": (\n"sv;
    if( enm.has_descr() )
       {
        f<< "\t\t{ DE:\""sv << enm.descr() << "\" }\n"sv;
       }
    if( not enm.elements().empty() )
       {
        const auto it_last = std::prev( enm.elements().cend() );
        for( auto it=enm.elements().cbegin(); it!=it_last; ++it )
           {
            f<< "\t\t"sv << it->name() << " := "sv << it->value() << ',';
            if( it->has_descr() )
               {
                f<< " { DE:\""sv << it->descr() << "\" }"sv;
               }
            f<< '\n';
           }
        // Last element
        f<< "\t\t"sv << it_last->name() << " := "sv << it_last->value();
        if( it_last->has_descr() )
           {
            f<< " { DE:\""sv << it_last->descr() << "\" }"sv;
           }
        f<< '\n';
       }
    f<< "\t);\n"sv;
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::TypeDef& tdef)
{
    f<< "\n\t"sv << tdef.name() << " : "sv;
    write(f, tdef.type());
    f<< ';';
    if( tdef.has_descr() )
       {
        f<< " { DE:\""sv << tdef.descr() << "\" }"sv;
       }
    f<< '\n';
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Subrange& subrng)
{
    f<< "\n\t"sv << subrng.name() << " : "sv << subrng.type_name()
     << " ("sv
     << std::to_string(subrng.min_value()) << ".."sv
     << std::to_string(subrng.max_value())
     << ");"sv;

    if( subrng.has_descr() )
       {
        f<< " { DE:\""sv << subrng.descr() << "\" }"sv;
       }
    f<< '\n';
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Macro& macro)
{
    f << "\nMACRO "sv << macro.name() << '\n';

    if( macro.has_descr() )
       {
        f << "{ DE:\""sv << macro.descr() << "\" }\n"sv;
       }

    // [Parameters]
    if( not macro.parameters().empty() )
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
    if( not macro.body().ends_with('\n') ) f << '\n';
    f << "END_MACRO\n"sv;
}



//---------------------------------------------------------------------------
void write_lib(MG::OutputStreamable auto& f, const plcb::Library& lib, const MG::keyvals& options)
{
    // [Options]
    const std::string_view sects_spacer = "\n\n\n"sv; // options.value_or("pll-sects-spacer", "\n\n\n"sv);

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
    if( not lib.global_variables().is_empty() )  f<< "\tglobal-variables: "sv << std::to_string(lib.global_variables().vars_count()) << '\n';
    if( not lib.global_constants().is_empty() )  f<< "\tglobal-constants: "sv << std::to_string(lib.global_constants().vars_count()) << '\n';
    if( not lib.global_retainvars().is_empty() ) f<< "\tglobal-retain-vars: "sv << std::to_string(lib.global_retainvars().vars_count()) << '\n';
    if( not lib.functions().empty() )            f<< "\tfunctions: "sv << std::to_string(lib.functions().size()) << '\n';
    if( not lib.function_blocks().empty() )      f<< "\tfunction blocks: "sv << std::to_string(lib.function_blocks().size()) << '\n';
    if( not lib.programs().empty() )             f<< "\tprograms: "sv << std::to_string(lib.programs().size()) << '\n';
    if( not lib.macros().empty() )               f<< "\tmacros: "sv << std::to_string(lib.macros().size()) << '\n';
    if( not lib.structs().empty() )              f<< "\tstructs: "sv << std::to_string(lib.structs().size()) << '\n';
    if( not lib.typedefs().empty() )             f<< "\ttypedefs: "sv << std::to_string(lib.typedefs().size()) << '\n';
    if( not lib.enums().empty() )                f<< "\tenums: "sv << std::to_string(lib.enums().size()) << '\n';
    if( not lib.subranges().empty() )            f<< "\tsubranges: "sv << std::to_string(lib.subranges().size()) << '\n';
    //if( not lib.interfaces().empty() )           f<< "\tinterfaces: "sv << std::to_string(lib.interfaces().size()) << '\n';
    f << "*)\n"sv;

    // [Global variables]
    if( not lib.global_variables().is_empty() or
        not lib.global_retainvars().is_empty() )
       {
        f<< sects_spacer <<
            "\t(****************************)\n"
            "\t(*                          *)\n"
            "\t(*     GLOBAL VARIABLES     *)\n"
            "\t(*                          *)\n"
            "\t(****************************)\n"sv;
        f<< "\n\tVAR_GLOBAL\n"sv;
        for( const auto& group : lib.global_variables().groups() )
           {
            if( not group.name().empty() ) f << "\t{G:\""sv << group.name() << "\"}\n"sv;
            for( const auto& var : group.variables() ) write(f, var);
           }
        for( const auto& group : lib.global_retainvars().groups() )
           {
            if( not group.name().empty() ) f << "\t{G:\""sv << group.name() << "\"}\n"sv;
            for( const auto& var : group.variables() ) write(f, var);
           }
        f<< "\tEND_VAR\n"sv;
       }

    // [Global constants]
    if( not lib.global_constants().is_empty() )
       {
        f<< sects_spacer <<
            "\t(****************************)\n"
            "\t(*                          *)\n"
            "\t(*     GLOBAL CONSTANTS     *)\n"
            "\t(*                          *)\n"
            "\t(****************************)\n"sv;
        f<< "\n\tVAR_GLOBAL CONSTANT\n"sv;
        for( const auto& group : lib.global_constants().groups() )
           {
            if( not group.name().empty() ) f << "\t{G:\""sv << group.name() << "\"}\n"sv;
            for( const auto& var : group.variables() ) write(f, var);
           }
        f<< "\tEND_VAR\n"sv;
       }

    // [Functions]
    if( not lib.functions().empty() )
       {
        f<< sects_spacer <<
            "\t(*********************)\n"
            "\t(*                   *)\n"
            "\t(*     FUNCTIONS     *)\n"
            "\t(*                   *)\n"
            "\t(*********************)\n"sv;
        for( const auto& pou : lib.functions() )
           {
            write(f, pou, "FUNCTION"sv);
           }
       }

    // [FunctionBlocks]
    if( not lib.function_blocks().empty() )
       {
        f<< sects_spacer <<
            "\t(***************************)\n"
            "\t(*                         *)\n"
            "\t(*     FUNCTION BLOCKS     *)\n"
            "\t(*                         *)\n"
            "\t(***************************)\n"sv;
        for( const auto& pou : lib.function_blocks() )
           {
            write(f, pou, "FUNCTION_BLOCK"sv);
           }
       }

    // [Programs]
    if( not lib.programs().empty() )
       {
        f<< sects_spacer <<
            "\t(********************)\n"
            "\t(*                  *)\n"
            "\t(*     PROGRAMS     *)\n"
            "\t(*                  *)\n"
            "\t(********************)\n"sv;
        for( const auto& pou : lib.programs() )
           {
            write(f, pou, "PROGRAM"sv);
           }
       }

    // [Enums]
    if( not lib.enums().empty() )
       {
        f<< sects_spacer <<
            "\t(*****************)\n"
            "\t(*               *)\n"
            "\t(*     ENUMS     *)\n"
            "\t(*               *)\n"
            "\t(*****************)\n"sv;
        f<< "\nTYPE\n"sv;
        for( const auto& enm : lib.enums() )
           {
            write(f, enm);
           }
        f<< "\nEND_TYPE\n"sv;
       }

    // [Typedefs]
    if( not lib.typedefs().empty() )
       {
        f<< sects_spacer <<
            "\t(********************)\n"
            "\t(*                  *)\n"
            "\t(*     TYPEDEFS     *)\n"
            "\t(*                  *)\n"
            "\t(********************)\n"sv;
        f<< "\nTYPE\n"sv;
        for( const auto& tdef : lib.typedefs() )
           {
            write(f, tdef);
           }
        f<< "\nEND_TYPE\n"sv;
       }

    // [Structs]
    if( not lib.structs().empty() )
       {
        f<< sects_spacer <<
            "\t(*******************)\n"
            "\t(*                 *)\n"
            "\t(*     STRUCTS     *)\n"
            "\t(*                 *)\n"
            "\t(*******************)\n"sv;
        f<< "\nTYPE\n"sv;
        for( const auto& strct : lib.structs() )
           {
            write(f, strct);
           }
        f<< "\nEND_TYPE\n"sv;
       }

    // [Subranges]
    if( not lib.subranges().empty() )
       {
        f<< sects_spacer <<
            "\t(*********************)\n"
            "\t(*                   *)\n"
            "\t(*     SUBRANGES     *)\n"
            "\t(*                   *)\n"
            "\t(*********************)\n"sv;
        f<< "\nTYPE\n"sv;
        for( const auto& subrng : lib.subranges() )
           {
            write(f, subrng);
           }
        f<< "\nEND_TYPE\n"sv;
       }

    // [Macros]
    if( not lib.macros().empty() )
       {
        f<< sects_spacer <<
            "\t(********************)\n"
            "\t(*                  *)\n"
            "\t(*      MACROS      *)\n"
            "\t(*                  *)\n"
            "\t(********************)\n"sv;
        for( const auto& macro : lib.macros() )
           {
            write(f, macro);
           }
       }

    // [Interfaces]
    //if( not lib.interfaces().empty() )
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
#include "string_write.hpp" // MG::string_write
/////////////////////////////////////////////////////////////////////////////
inline static constexpr std::string_view sample_lib_pll =
    "(*\n"
    "\tname: sample-lib\n"
    "\tdescr: sample library\n"
    "\tversion: 1.0.2\n"
    "\tauthor: pll::write()\n"
    "\tglobal-variables: 2\n"
    "\tglobal-constants: 2\n"
    "\tfunctions: 1\n"
    "\tfunction blocks: 1\n"
    "\tprograms: 2\n"
    "\tmacros: 1\n"
    "\tstructs: 1\n"
    "\ttypedefs: 2\n"
    "\tenums: 1\n"
    "\tsubranges: 1\n"
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
    "\t{G:\"globs\"}\n"
    "\tgvar1 : DINT; { DE:\"gvar1 descr\" }\n"
    "\tgvar2 : ARRAY[ 0..99 ] OF INT; { DE:\"gvar2 descr\" }\n"
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
    "\tconst1 : INT := 42; { DE:\"gvar1 descr\" }\n"
    "\tconst2 : LREAL := 3.14; { DE:\"gvar2 descr\" }\n"
    "\tEND_VAR\n"
    "\n"
    "\n"
    "\n"
    "\t(*********************)\n"
    "\t(*                   *)\n"
    "\t(*     FUNCTIONS     *)\n"
    "\t(*                   *)\n"
    "\t(*********************)\n"
    "\n"
    "FUNCTION fn_name : INT\n"
    "\n"
    "{ DE:\"testing fn\" }\n"
    "\n"
    "\tVAR_INPUT\n"
    "\tin1 : DINT; { DE:\"in1 descr\" }\n"
    "\tin2 : LREAL; { DE:\"in2 descr\" }\n"
    "\tEND_VAR\n"
    "\n"
    "\tVAR CONSTANT\n"
    "\tconst1 : DINT := 42; { DE:\"const1 descr\" }\n"
    "\tEND_VAR\n"
    "\n"
    "\t{ CODE:ST }body\n"
    "END_FUNCTION\n"
    "\n"
    "\n"
    "\n"
    "\t(***************************)\n"
    "\t(*                         *)\n"
    "\t(*     FUNCTION BLOCKS     *)\n"
    "\t(*                         *)\n"
    "\t(***************************)\n"
    "\n"
    "FUNCTION_BLOCK fb_name\n"
    "\n"
    "{ DE:\"testing fb\" }\n"
    "\n"
    "\tVAR_IN_OUT\n"
    "\tinout1 : DINT; { DE:\"inout1 descr\" }\n"
    "\tinout2 : LREAL; { DE:\"inout2 descr\" }\n"
    "\tEND_VAR\n"
    "\n"
    "\tVAR_INPUT\n"
    "\tin1 : DINT; { DE:\"in1 descr\" }\n"
    "\tin2 : LREAL; { DE:\"in2 descr\" }\n"
    "\tEND_VAR\n"
    "\n"
    "\tVAR_OUTPUT\n"
    "\tout1 : DINT; { DE:\"out1 descr\" }\n"
    "\tout2 : LREAL; { DE:\"out2 descr\" }\n"
    "\tEND_VAR\n"
    "\n"
    "\tVAR_EXTERNAL\n"
    "\text1 : DINT; { DE:\"ext1 descr\" }\n"
    "\text2 : STRING[ 80 ]; { DE:\"ext2 descr\" }\n"
    "\tEND_VAR\n"
    "\n"
    "\tVAR\n"
    "\tloc1 : DINT; { DE:\"loc1 descr\" }\n"
    "\tloc2 : LREAL; { DE:\"loc2 descr\" }\n"
    "\tEND_VAR\n"
    "\n"
    "\tVAR CONSTANT\n"
    "\tconst1 : DINT := 42; { DE:\"const1 descr\" }\n"
    "\tconst2 : LREAL := 1.5; { DE:\"const2 descr\" }\n"
    "\tEND_VAR\n"
    "\n"
    "\t{ CODE:ST }body\n"
    "END_FUNCTION_BLOCK\n"
    "\n"
    "\n"
    "\n"
    "\t(********************)\n"
    "\t(*                  *)\n"
    "\t(*     PROGRAMS     *)\n"
    "\t(*                  *)\n"
    "\t(********************)\n"
    "\n"
    "PROGRAM prg_name1\n"
    "\n"
    "{ DE:\"testing prg 1\" }\n"
    "\n"
    "\tVAR\n"
    "\tloc1 : DINT; { DE:\"loc1 descr\" }\n"
    "\tloc2 : LREAL; { DE:\"loc2 descr\" }\n"
    "\tEND_VAR\n"
    "\n"
    "\t{ CODE:ST }body\n"
    "END_PROGRAM\n"
    "\n"
    "PROGRAM prg_name2\n"
    "\n"
    "{ DE:\"testing prg 2\" }\n"
    "\n"
    "\tVAR\n"
    "\tloc1 : DINT; { DE:\"loc1 descr\" }\n"
    "\tloc2 : LREAL; { DE:\"loc2 descr\" }\n"
    "\tEND_VAR\n"
    "\n"
    "\t{ CODE:ST }body\n"
    "END_PROGRAM\n"
    "\n"
    "\n"
    "\n"
    "\t(*****************)\n"
    "\t(*               *)\n"
    "\t(*     ENUMS     *)\n"
    "\t(*               *)\n"
    "\t(*****************)\n"
    "\n"
    "TYPE\n"
    "\n"
    "\tenumname: (\n"
    "\t\t{ DE:\"testing enum\" }\n"
    "\t\telm1 := 1, { DE:\"elm1 descr\" }\n"
    "\t\telm2 := 42 { DE:\"elm2 descr\" }\n"
    "\t);\n"
    "\n"
    "END_TYPE\n"
    "\n"
    "\n"
    "\n"
    "\t(********************)\n"
    "\t(*                  *)\n"
    "\t(*     TYPEDEFS     *)\n"
    "\t(*                  *)\n"
    "\t(********************)\n"
    "\n"
    "TYPE\n"
    "\n"
    "\ttypename1 : STRING[ 80 ]; { DE:\"testing typedef\" }\n"
    "\n"
    "\ttypename2 : ARRAY[ 0..9 ] OF INT; { DE:\"testing typedef 2\" }\n"
    "\n"
    "END_TYPE\n"
    "\n"
    "\n"
    "\n"
    "\t(*******************)\n"
    "\t(*                 *)\n"
    "\t(*     STRUCTS     *)\n"
    "\t(*                 *)\n"
    "\t(*******************)\n"
    "\n"
    "TYPE\n"
    "\n"
    "\tstructname : STRUCT { DE:\"testing struct\" }\n"
    "\t\tmember1 : DINT; { DE:\"member1 descr\" }\n"
    "\t\tmember2 : STRING[ 80 ]; { DE:\"member2 descr\" }\n"
    "\t\tmember3 : ARRAY[ 0..11 ] OF INT; { DE:\"array member\" }\n"
    "\tEND_STRUCT;\n"
    "\n"
    "END_TYPE\n"
    "\n"
    "\n"
    "\n"
    "\t(*********************)\n"
    "\t(*                   *)\n"
    "\t(*     SUBRANGES     *)\n"
    "\t(*                   *)\n"
    "\t(*********************)\n"
    "\n"
    "TYPE\n"
    "\n"
    "\tsubrangename : INT (1..12); { DE:\"testing subrange\" }\n"
    "\n"
    "END_TYPE\n"
    "\n"
    "\n"
    "\n"
    "\t(********************)\n"
    "\t(*                  *)\n"
    "\t(*      MACROS      *)\n"
    "\t(*                  *)\n"
    "\t(********************)\n"
    "\n"
    "MACRO macroname\n"
    "{ DE:\"testing macro\" }\n"
    "\n"
    "\tPAR_MACRO\n"
    "\tpar1; { DE:\"par1 descr\" }\n"
    "\tpar2; { DE:\"par2 descr\" }\n"
    "\tEND_PAR\n"
    "\n"
    "\t{ CODE:ST }body\n"
    "END_MACRO\n"sv;
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"writer_pll"> writer_pll_tests = []
{

ut::test("pll::write(plcb::Variable)") = []
   {
    MG::string_write out;

    ut::should("write an INT variable") = [out]() mutable
       {
        pll::write(out, plcb::make_var("vn320"sv, plcb::make_type("INT"sv), ""sv, "testing variable"sv, 'M', 'W', 400, 320));
        ut::expect( ut::that % out.str() == "\tvn320 AT %MW400.320 : INT; { DE:\"testing variable\" }\n"sv );
       };

    ut::should("write a STRING variable") = [out]() mutable
       {
        pll::write(out, plcb::make_var("va0"sv, plcb::make_type("STRING"sv,80), ""sv, "testing string", 'M', 'B', 700, 0));
        ut::expect( ut::that % out.str() == "\tva0 AT %MB700.0 : STRING[ 80 ]; { DE:\"testing string\" }\n"sv );
       };

    ut::should("write an ARRAY variable") = [out]() mutable
       {
        pll::write(out, plcb::make_var("vbMsgs"sv, plcb::make_type("BOOL"sv,0,9), ""sv, "testing array", 'M', 'B', 300, 6000));
        ut::expect( ut::that % out.str() == "\tvbMsgs AT %MB300.6000 : ARRAY[ 0..9 ] OF BOOL; { DE:\"testing array\" }\n"sv );
       };
   };


ut::test("pll::write(plcb::Pou)") = []
   {
    plcb::Pou pou;
    pou.set_name("pouname");
    pou.set_descr("testing pou");
    pou.set_return_type("INT");
    pou.inout_vars() = { plcb::make_var("inout1"sv, plcb::make_type("DINT"sv), ""sv, "inout1 descr"),
                         plcb::make_var("inout2"sv, plcb::make_type("LREAL"sv), ""sv, "inout2 descr") };
    pou.input_vars() = { plcb::make_var("in1"sv, plcb::make_type("DINT"sv), ""sv, "in1 descr"),
                         plcb::make_var("in2"sv, plcb::make_type("LREAL"sv), ""sv, "in2 descr") };
    pou.output_vars() = { plcb::make_var("out1"sv, plcb::make_type("DINT"sv), ""sv, "out1 descr"),
                          plcb::make_var("out2"sv, plcb::make_type("LREAL"sv), ""sv, "out2 descr") };
    pou.external_vars() = { plcb::make_var("ext1"sv, plcb::make_type("DINT"sv), ""sv, "ext1 descr"),
                            plcb::make_var("ext2"sv, plcb::make_type("STRING"sv,80u), ""sv, "ext2 descr") };
    pou.local_vars() = { plcb::make_var("loc1"sv, plcb::make_type("DINT"sv), ""sv, "loc1 descr"),
                         plcb::make_var("loc2"sv, plcb::make_type("LREAL"sv), ""sv, "loc2 descr") };
    pou.local_constants() = { plcb::make_var("const1"sv, plcb::make_type("DINT"sv), "42"sv, "const1 descr"),
                              plcb::make_var("const2"sv, plcb::make_type("LREAL"sv), "1.5"sv, "const2 descr") };
    pou.set_code_type("ST");
    pou.set_body("body");

    const std::string_view expected =
        "\nFUNCTION pouname : INT\n"
        "\n"
        "{ DE:\"testing pou\" }\n"
        "\n"
        "\tVAR_IN_OUT\n"
        "\tinout1 : DINT; { DE:\"inout1 descr\" }\n"
        "\tinout2 : LREAL; { DE:\"inout2 descr\" }\n"
        "\tEND_VAR\n"
        "\n"
        "\tVAR_INPUT\n"
        "\tin1 : DINT; { DE:\"in1 descr\" }\n"
        "\tin2 : LREAL; { DE:\"in2 descr\" }\n"
        "\tEND_VAR\n"
        "\n"
        "\tVAR_OUTPUT\n"
        "\tout1 : DINT; { DE:\"out1 descr\" }\n"
        "\tout2 : LREAL; { DE:\"out2 descr\" }\n"
        "\tEND_VAR\n"
        "\n"
        "\tVAR_EXTERNAL\n"
        "\text1 : DINT; { DE:\"ext1 descr\" }\n"
        "\text2 : STRING[ 80 ]; { DE:\"ext2 descr\" }\n"
        "\tEND_VAR\n"
        "\n"
        "\tVAR\n"
        "\tloc1 : DINT; { DE:\"loc1 descr\" }\n"
        "\tloc2 : LREAL; { DE:\"loc2 descr\" }\n"
        "\tEND_VAR\n"
        "\n"
        "\tVAR CONSTANT\n"
        "\tconst1 : DINT := 42; { DE:\"const1 descr\" }\n"
        "\tconst2 : LREAL := 1.5; { DE:\"const2 descr\" }\n"
        "\tEND_VAR\n"
        "\n"
        "\t{ CODE:ST }body\n"
        "END_FUNCTION\n"sv;

    MG::string_write out;
    pll::write(out, pou, "FUNCTION"sv);
    ut::expect( ut::that % out.str() == expected );
   };


ut::test("pll::write(plcb::Struct)") = []
   {
    plcb::Struct strct;
    strct.set_name("structname");
    strct.set_descr("testing struct");

    {   plcb::Struct::Member& memb = strct.members().emplace_back();
        memb.set_name("member1"sv);
        memb.type() = plcb::make_type("DINT"sv);
        memb.set_descr("member1 descr"sv);   }
    {   plcb::Struct::Member& memb = strct.members().emplace_back();
        memb.set_name("member2"sv);
        memb.type() = plcb::make_type("STRING"sv,80u);
        memb.set_descr("member2 descr"sv);   }
    {   plcb::Struct::Member& memb = strct.members().emplace_back();
        memb.set_name("member3"sv);
        memb.type() = plcb::make_type("INT"sv,0u,11u);
        memb.set_descr("array member"sv);   }

    const std::string_view expected =
        "\n\tstructname : STRUCT { DE:\"testing struct\" }\n"
        "\t\tmember1 : DINT; { DE:\"member1 descr\" }\n"
        "\t\tmember2 : STRING[ 80 ]; { DE:\"member2 descr\" }\n"
        "\t\tmember3 : ARRAY[ 0..11 ] OF INT; { DE:\"array member\" }\n"
        "\tEND_STRUCT;\n"sv;

    MG::string_write out;
    pll::write(out, strct);
    ut::expect( ut::that % out.str() == expected );
   };


ut::test("pll::write(plcb::Enum)") = []
   {
    plcb::Enum enm;
    enm.set_name("enumname");
    enm.set_descr("testing enum");
    enm.elements() = { plcb::make_enelem("elm1", "1", "elm1 descr"),
                       plcb::make_enelem("elm2", "42", "elm2 descr") };

    const std::string_view expected =
        "\n\tenumname: (\n"
        "\t\t{ DE:\"testing enum\" }\n"
        "\t\telm1 := 1, { DE:\"elm1 descr\" }\n"
        "\t\telm2 := 42 { DE:\"elm2 descr\" }\n"
        "\t);\n"sv;

    MG::string_write out;
    pll::write(out, enm);
    ut::expect( ut::that % out.str() == expected );
   };


ut::test("pll::write(plcb::TypeDef)") = []
   {
    plcb::TypeDef tdef;
    tdef.set_name("typename"sv);
    tdef.type() = plcb::make_type("STRING"sv, 80u);
    tdef.set_descr("testing typedef"sv);

    MG::string_write out;
    pll::write(out, tdef);
    ut::expect( ut::that % out.str() == "\n\ttypename : STRING[ 80 ]; { DE:\"testing typedef\" }\n"sv );
   };


ut::test("pll::write(plcb::Subrange)") = []
   {
    plcb::Subrange subrng;
    subrng.set_name("subrangename");
    subrng.set_type_name( plcb::make_type("INT") );
    subrng.set_range(1,12);
    subrng.set_descr("testing subrange");

    MG::string_write out;
    pll::write(out, subrng);
    ut::expect( ut::that % out.str() == "\n\tsubrangename : INT (1..12); { DE:\"testing subrange\" }\n"sv );
   };


ut::test("pll::write(plcb::Macro)") = []
   {
    plcb::Macro macro;
    macro.set_name("macroname");
    macro.set_descr("testing macro");
    macro.parameters() = { plcb::make_mparam("par1", "par1 descr"),
                           plcb::make_mparam("par2", "par2 descr") };
    macro.set_code_type("ST");
    macro.set_body("body");

    const std::string_view expected =
        "\nMACRO macroname\n"
        "{ DE:\"testing macro\" }\n"
        "\n"
        "\tPAR_MACRO\n"
        "\tpar1; { DE:\"par1 descr\" }\n"
        "\tpar2; { DE:\"par2 descr\" }\n"
        "\tEND_PAR\n"
        "\n"
        "\t{ CODE:ST }body\n"
        "END_MACRO\n"sv;

    MG::string_write out;
    pll::write(out, macro);
    ut::expect( ut::that % out.str() == expected );
   };


ut::test("pll::write(plcb::Library)") = []
   {
    const plcb::Library lib = plcb::make_sample_lib();

    MG::string_write out;
    pll::write_lib(out, lib, MG::keyvals{"no-timestamp"});
    ut::expect( ut::that % out.str() == sample_lib_pll );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
