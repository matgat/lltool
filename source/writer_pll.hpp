﻿#pragma once
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
inline void write(MG::OutputStreamable auto& f, const plcb::Enum& enm)
{
    f<< "\n\t"sv << enm.name() << ": (\n"sv;
    if( !enm.descr().empty() )
       {
        f<< "\t\t{ DE:\""sv << enm.descr() << "\" }\n"sv;
       }
    if( !enm.elements().empty() )
       {
        const auto it_last = std::prev( enm.elements().cend() );
        for( auto it=enm.elements().cbegin(); it!=it_last; ++it )
           {
            f<< "\t\t"sv << it->name() << " := "sv << it->value() << ',';
            if( !it->descr().empty() )
               {
                f<< " { DE:\""sv << it->descr() << "\" }"sv;
               }
            f<< '\n';
           }
        // Last element
        f<< "\t\t"sv << it_last->name() << " := "sv << it_last->value();
        if( !it_last->descr().empty() )
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

    if( !tdef.descr().empty() )
       {
        f<< " { DE:\""sv << tdef.descr() << "\" }"sv;
       }
    f<< '\n';
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Struct& strct)
{
    f<< '\t' << strct.name() << " : STRUCT"sv;

    if( !strct.descr().empty() )
       {
        f<< " { DE:\""sv << strct.descr() << "\" }"sv;
       }
    f<< '\n';

    for( const auto& var : strct.members() )
       {
        f<< "\t\t"sv << var.name() << " : "sv << var.type() << ';';
        if( !var.descr().empty() )
           {
            f<< " { DE:\""sv << var.descr() << "\" }"sv;
           }
        f<< '\n';
       }
    f<< "\tEND_STRUCT;\n\n"sv;
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Subrange& subrng)
{
    f<< '\t' << subrng.name() << " : "sv << subrng.type()
     << " ("sv
     << std::to_string(subrng.min_value()) << ".."sv
     << std::to_string(subrng.max_value())
     << ");"sv;

    if( !subrng.descr().empty() )
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
void write_lib(MG::OutputStreamable auto& f, const plcb::Library& lib, const MG::keyvals& options)
{
    // [Options]
    const std::string_view sects_spacer = "\n\n\n"sv; // options.value_or("pll-sects-spacer", "\n\n\n"sv);
    const std::string_view blocks_spacer = "\n\n"sv; // options.value_or("pll-blocks-spacer", "\n\n"sv);

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
        for( const auto& enm : lib.enums() )
           {
            write(f, enm);
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
            write(f, tdef);
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
            write(f, strct);
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
        for( const auto& subrng : lib.subranges() )
           {
            write(f, subrng);
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
#include "string_write.hpp" // MG::string_write
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"writer_pll"> writer_pll_tests = []
{

ut::test("pll::write(plcb::Variable)") = []
   {
    MG::string_write out;

    ut::should("write an INT variable") = [out]() mutable
       {
        pll::write(out, plcb::make_var("vn320"sv, "INT"sv, 0, ""sv, "testing variable"sv, 'M', 'W', 400, 320));
        ut::expect( ut::that % out.str() == "\tvn320 AT %MW400.320 : INT; { DE:\"testing variable\" }\n"sv );
       };

    ut::should("write a STRING variable") = [out]() mutable
       {
        pll::write(out, plcb::make_var("va0", "STRING", 80, ""sv, "testing array", 'M', 'B', 700, 0));
        ut::expect( ut::that % out.str() == "\tva0 AT %MB700.0 : STRING[ 80 ]; { DE:\"testing array\" }\n"sv );
       };
   };


ut::test("pll::write(plcb::Pou)") = []
   {
    plcb::Pou pou;
    pou.set_name("pouname");
    pou.set_descr("testing pou");
    pou.set_return_type("INT");
    pou.inout_vars() = { plcb::make_var("inout1", "DINT", 0u, ""sv, "inout1 descr"),
                         plcb::make_var("inout2", "LREAL", 0u, ""sv, "inout2 descr") };
    pou.input_vars() = { plcb::make_var("in1", "DINT", 0u, ""sv, "in1 descr"),
                         plcb::make_var("in2", "LREAL", 0u, ""sv, "in2 descr") };
    pou.output_vars() = { plcb::make_var("out1", "DINT", 0u, ""sv, "out1 descr"),
                          plcb::make_var("out2", "LREAL", 0u, ""sv, "out2 descr") };
    pou.external_vars() = { plcb::make_var("ext1", "DINT", 0u, ""sv, "ext1 descr"),
                            plcb::make_var("ext2", "STRING", 80u, ""sv, "ext2 descr") };
    pou.local_vars() = { plcb::make_var("loc1", "DINT", 0u, ""sv, "loc1 descr"),
                         plcb::make_var("loc2", "LREAL", 0u, ""sv, "loc2 descr") };
    pou.local_constants() = { plcb::make_var("const1", "DINT", 0u, "42"sv, "const1 descr"),
                              plcb::make_var("const2", "LREAL", 0u, "1.5"sv, "const2 descr") };
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
        "END_FUNCTION\n\n"sv;

    MG::string_write out;
    pll::write(out, pou, "FUNCTION"sv);
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
    plcb::TypeDef tdef{ plcb::make_var("typename", "LREAL", 0u, ""sv, "testing typedef") };

    MG::string_write out;
    pll::write(out, tdef);
    ut::expect( ut::that % out.str() == "\ttypename : LREAL; { DE:\"testing typedef\" }\n"sv );
   };


ut::test("pll::write(plcb::Struct)") = []
   {
    plcb::Struct strct;
    strct.set_name("structname");
    strct.set_descr("testing struct");
    strct.add_member( plcb::make_var("member1", "DINT", 0u, ""sv, "member1 descr")  );
    strct.add_member( plcb::make_var("member2", "LREAL", 0u, ""sv, "member2 descr")  );

    const std::string_view expected =
        "\tstructname : STRUCT { DE:\"testing struct\" }\n"
        "\t\tmember1 : DINT; { DE:\"member1 descr\" }\n"
        "\t\tmember2 : LREAL; { DE:\"member2 descr\" }\n"
        "\tEND_STRUCT;\n\n"sv;

    MG::string_write out;
    pll::write(out, strct);
    ut::expect( ut::that % out.str() == expected );
   };


ut::test("pll::write(plcb::Subrange)") = []
   {
    plcb::Subrange subrng;
    subrng.set_name("subrangename");
    subrng.set_type("INT");
    subrng.set_range(1,12);
    subrng.set_descr("testing subrange");

    MG::string_write out;
    pll::write(out, subrng);
    ut::expect( ut::that % out.str() == "\tsubrangename : INT (1..12); { DE:\"testing subrange\" }\n"sv );
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
        "END_MACRO\n\n"sv;

    MG::string_write out;
    pll::write(out, macro);
    ut::expect( ut::that % out.str() == expected );
   };


ut::test("pll::write(plcb::Library)") = []
   {
    plcb::Library lib{"testlib"sv};

   {auto& grp = lib.global_variables().groups().emplace_back();
    grp.set_name("globs");
    grp.mutable_variables() = { plcb::make_var("gvar1", "DINT", 0u, ""sv, "gvar1 descr"),
                                plcb::make_var("gvar2", "LREAL", 0u, ""sv, "gvar2 descr") };
   }
    //lib.global_constants()
    //lib.global_retainvars()

   {auto& prg = lib.programs().emplace_back();
    prg.set_name("prgname");
    prg.set_descr("testing prg");
    prg.local_vars() = { plcb::make_var("loc1", "DINT", 0u, ""sv, "loc1 descr"),
                         plcb::make_var("loc2", "LREAL", 0u, ""sv, "loc2 descr") };
    prg.set_code_type("ST");
    prg.set_body("body");
   }

   {auto& fb = lib.function_blocks().emplace_back();
    fb.set_name("fbname");
    fb.set_descr("testing fb");
    fb.inout_vars() = { plcb::make_var("inout1", "DINT", 0u, ""sv, "inout1 descr"),
                        plcb::make_var("inout2", "LREAL", 0u, ""sv, "inout2 descr") };
    fb.input_vars() = { plcb::make_var("in1", "DINT", 0u, ""sv, "in1 descr"),
                        plcb::make_var("in2", "LREAL", 0u, ""sv, "in2 descr") };
    fb.output_vars() = { plcb::make_var("out1", "DINT", 0u, ""sv, "out1 descr"),
                         plcb::make_var("out2", "LREAL", 0u, ""sv, "out2 descr") };
    fb.external_vars() = { plcb::make_var("ext1", "DINT", 0u, ""sv, "ext1 descr"),
                           plcb::make_var("ext2", "STRING", 80u, ""sv, "ext2 descr") };
    fb.local_vars() = { plcb::make_var("loc1", "DINT", 0u, ""sv, "loc1 descr"),
                        plcb::make_var("loc2", "LREAL", 0u, ""sv, "loc2 descr") };
    fb.local_constants() = { plcb::make_var("const1", "DINT", 0u, "42"sv, "const1 descr"),
                             plcb::make_var("const2", "LREAL", 0u, "1.5"sv, "const2 descr") };
    fb.set_code_type("ST");
    fb.set_body("body");
   }

    //lib.functions()
    //lib.function_blocks()
    //lib.macros()
    //lib.structs()
    //lib.typedefs()
    //lib.enums()
    //lib.subranges()

    const std::string_view expected =
        "(*\n"
        "\tname: testlib\n"
        "\tdescr: PLC library\n"
        "\tversion: 1.0.0\n"
        "\tauthor: pll::write()\n"
        "\tglobal-variables: 2\n"
        "\tfunction blocks: 1\n"
        "\tprograms: 1\n"
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
        "\n"
        "\tVAR_GLOBAL\n"
        "\t{G:\"globs\"}\n"
        "\tgvar1 : DINT; { DE:\"gvar1 descr\" }\n"
        "\tgvar2 : LREAL; { DE:\"gvar2 descr\" }\n"
        "\tEND_VAR\n"
        "\n"
        "\n"
        "\n"
        "\t(***************************)\n"
        "\t(*                         *)\n"
        "\t(*     FUNCTION BLOCKS     *)\n"
        "\t(*                         *)\n"
        "\t(***************************)\n"
        "\n"
        "\n"
        "\n"
        "FUNCTION_BLOCK fbname\n"
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
        "\n"
        "\t(********************)\n"
        "\t(*                  *)\n"
        "\t(*     PROGRAMS     *)\n"
        "\t(*                  *)\n"
        "\t(********************)\n"
        "\n"
        "\n"
        "\n"
        "PROGRAM prgname\n"
        "\n"
        "{ DE:\"testing prg\" }\n"
        "\n"
        "\tVAR\n"
        "\tloc1 : DINT; { DE:\"loc1 descr\" }\n"
        "\tloc2 : LREAL; { DE:\"loc2 descr\" }\n"
        "\tEND_VAR\n"
        "\n"
        "\t{ CODE:ST }body\n"
        "END_PROGRAM\n"
        "\n"sv;

    MG::string_write out;
    MG::keyvals opts; opts.assign("no-timestamp");
    pll::write_lib(out, lib, opts);
    ut::expect( ut::that % out.str() == expected );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////