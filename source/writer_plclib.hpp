#pragma once
//  ---------------------------------------------
//  Writes lib to LogicLab5 'plclib' xml format
//  ---------------------------------------------
//  #include "writer_plclib.hpp" // plclib::write_lib()
//  ---------------------------------------------
#include <concepts> // std::same_as<>
#include <stdexcept> // std::runtime_error
#include <cassert>
#include <cstdint> // std::uint16_t
#include <array>

#include <fmt/format.h> // fmt::format

#include "string_conversions.hpp" // str::to_num<>()
#include "ascii_parsing_utils.hpp" // ascii::extract_pair<>
#include "keyvals.hpp" // MG::keyvals
#include "timestamp.hpp" // MG::get_human_readable_timestamp()
#include "plc_library.hpp" // plcb::*
#include "output_streamable_concept.hpp" // MG::OutputStreamable

using namespace std::string_view_literals; // Use "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace plclib //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
[[nodiscard]] inline std::string_view ind(const std::size_t lvl) noexcept
{
    static constexpr std::array indent
       {
        ""sv
       ,"\t"sv
       ,"\t\t"sv
       ,"\t\t\t"sv
       ,"\t\t\t\t"sv
       ,"\t\t\t\t\t"sv
       ,"\t\t\t\t\t\t"sv
       ,"\t\t\t\t\t\t\t"sv
       ,"\t\t\t\t\t\t\t\t"sv
       ,"\t\t\t\t\t\t\t\t\t"sv
       ,"\t\t\t\t\t\t\t\t\t\t"sv
       ,"\t\t\t\t\t\t\t\t\t\t\t"sv
       };
    return indent[lvl % indent.size()];
}


/////////////////////////////////////////////////////////////////////////////
// A class to hold schema version <major-ver>.<minor-ver>
class SchemaVersion final
{
 private:
    std::uint16_t m_major_ver = 2;
    std::uint16_t m_minor_ver = 8;

 public:
    void operator=(const std::string_view sv)
       {
        try{
            const auto [first_num, second_num, remaining] = ascii::extract_pair<std::uint16_t,std::uint16_t>(sv);
            if( not remaining.empty() )
               {
                throw std::runtime_error{ fmt::format("ignored content \"{}\"", remaining) };
               }
            m_major_ver = first_num;
            m_minor_ver = second_num;
           }
        catch(std::exception& e)
           {
            throw std::runtime_error{ fmt::format("Invalid plclib schema version: {} ({})", sv, e.what()) };
           }
       }

    [[nodiscard]] std::uint16_t major_version() const noexcept { return m_major_ver; }
    [[nodiscard]] std::uint16_t minor_version() const noexcept { return m_minor_ver; }

    [[nodiscard]] auto operator<=>(const SchemaVersion&) const noexcept = default;

    [[nodiscard]] std::string string() const { return fmt::format("{}.{}"sv, major_version(), minor_version()); }
};



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Variable& var, const std::string_view tag, const std::size_t lvl)
{
    assert( not var.name().empty() );

    f<< ind(lvl) << '<' << tag << " name=\""sv << var.name() << "\" type=\""sv << var.type() << '\"';

    if( var.has_length() )
       {
        f<< " length=\""sv << std::to_string(var.length()) << '\"';
       }

    if( var.is_array() )
       {
        if( var.array_startidx()!=0u )
           {
            throw std::runtime_error{ fmt::format("plclib doesn't support arrays with a not null start index in variable {}", var.name()) };
           }
        f<< " dim0=\""sv << std::to_string(var.array_dim()) << '\"';
       }

    if( var.has_descr() or var.has_value() or var.has_address() )
       {// Tag contains something
        f<< ">\n"sv;

        if( var.has_descr() )
           {
            f<< ind(lvl+1) << "<descr>"sv << var.descr() << "</descr>\n"sv;
           }

        if( var.has_value() )
           {
            f<< ind(lvl+1) << "<initValue>"sv << var.value() << "</initValue>\n"sv;
           }

        if( var.has_address() )
           {
            f<< ind(lvl+1) << "<address type=\""sv << var.address().type() << "\""
             <<                       " typeVar=\""sv << var.address().typevar() << "\""
             <<                       " index=\""sv << std::to_string(var.address().index()) << "\""
             <<                       " subIndex=\""sv << std::to_string(var.address().subindex()) << "\""
             <<                       "/>\n"sv;
           }

        f<< ind(lvl) << "</"sv << tag << ">\n"sv;
       }
    else
       {// Empty tag
        f<< "/>\n"sv;
       }
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Pou& pou, const std::string_view tag, const std::size_t lvl)
{
    f<< ind(lvl) << '<' << tag << " name=\""sv << pou.name() << "\" version=\"1.0.0\" creationDate=\"0\" lastModifiedDate=\"0\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\">\n"sv;
    if( pou.has_descr() )
       {
        f<< ind(lvl+1) << "<descr>"sv << pou.descr() << "</descr>\n"sv;
       }
    if( pou.has_return_type() )
       {
        f<< ind(lvl+1) << "<returnValue>"sv << pou.return_type() << "</returnValue>\n"sv;
       }

    // [Variables]
    if( not pou.inout_vars().empty() or
        not pou.input_vars().empty() or
        not pou.output_vars().empty() or
        not pou.external_vars().empty() or
        not pou.local_vars().empty() or
        not pou.local_constants().empty() )
       {
        f<< ind(lvl+1) << "<vars>\n"sv;
        if( not pou.inout_vars().empty() )
           {
            f<< ind(lvl+2) << "<inoutVars>\n"sv;
            for( const auto& var : pou.inout_vars() )
               {
                write(f, var, "var"sv, lvl+3);
               }
            f<< ind(lvl+2) << "</inoutVars>\n"sv;
           }
        if( not pou.input_vars().empty() )
           {
            f<< ind(lvl+2) << "<inputVars>\n"sv;
            for( const auto& var : pou.input_vars() )
               {
                write(f, var, "var"sv, lvl+3);
               }
            f<< ind(lvl+2) << "</inputVars>\n"sv;
           }
        if( not pou.output_vars().empty() )
           {
            f<< ind(lvl+2) << "<outputVars>\n"sv;
            for( const auto& var : pou.output_vars() )
               {
                write(f, var, "var"sv, lvl+3);
               }
            f<< ind(lvl+2) << "</outputVars>\n"sv;
           }
        if( not pou.external_vars().empty() )
           {
            f<< ind(lvl+2) << "<externalVars>\n"sv;
            for( const auto& var : pou.external_vars() )
               {
                write(f, var, "var"sv, lvl+3);
               }
            f<< ind(lvl+2) << "</externalVars>\n"sv;
           }
        if( not pou.local_vars().empty() )
           {
            f<< ind(lvl+2) << "<localVars>\n"sv;
            for( const auto& var : pou.local_vars() )
               {
                write(f, var, "var"sv, lvl+3);
               }
            f<< ind(lvl+2) << "</localVars>\n"sv;
           }
        if( not pou.local_constants().empty() )
           {
            f<< ind(lvl+2) << "<localConsts>\n"sv;
            for( const auto& var : pou.local_constants() )
               {
                write(f, var, "const"sv, lvl+3);
               }
            f<< ind(lvl+2) << "</localConsts>\n"sv;
           }
        f<< ind(lvl+1) << "</vars>\n"sv;
       }
    else
       {
        f<< ind(lvl+1) << "<vars/>\n"sv;
       }

    f<< ind(lvl+1) << "<iecDeclaration active=\"FALSE\"/>\n"sv;
    if( tag=="functionBlock"sv )
       {
        f<< ind(lvl+1) << "<interfaces/>\n"sv
         << ind(lvl+1) << "<methods/>\n"sv;
       }

    // [Body]
    f<< ind(lvl+1) << "<sourceCode type=\""sv << pou.code_type() << "\">\n"sv
     << ind(lvl+2) << "<![CDATA["sv << pou.body() << "]]>\n"sv
     << ind(lvl+1) << "</sourceCode>\n"sv;

    f<< ind(lvl) << "</"sv << tag << ">\n"sv;
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Enum& enm, const std::size_t lvl)
{
    f<< ind(lvl) << "<enum name=\""sv << enm.name() << "\" version=\"1.0.0\">\n"sv;
    f<< ind(lvl+1) << "<descr>"sv << enm.descr() << "</descr>\n"sv;
    f<< ind(lvl+1) << "<elements>\n"sv;
    for( const auto& elem : enm.elements() )
       {
        f<< ind(lvl+2) << "<element name=\""sv << elem.name() << "\">\n"sv
         << ind(lvl+3) << "<descr>"sv << elem.descr() << "</descr>\n"sv
         << ind(lvl+3) << "<value>"sv << elem.value() << "</value>\n"sv
         << ind(lvl+2) << "</element>\n"sv;
       }
    f<< ind(lvl+1) << "</elements>\n"sv;
    f<< ind(lvl+1) << "<iecDeclaration active=\"FALSE\"/>\n"sv;
    f<< ind(lvl) << "</enum>\n"sv;
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::TypeDef& tdef, const std::size_t lvl)
{
    f<< ind(lvl) << "<typedef name=\""sv << tdef.name() << "\" type=\""sv << tdef.type() << '\"';
    if( tdef.has_length() ) f<< " length=\""sv << std::to_string(tdef.length()) << '\"';
    if( tdef.is_array() )
       {
        if( tdef.array_startidx()!=0u )
           {
            throw std::runtime_error{ fmt::format("plclib doesn't support arrays with a not null start index in typedef {}",tdef.name()) };
           }
        f<< " dim0=\""sv << std::to_string(tdef.array_dim()) << '\"';
       }
    f<< ">\n"sv;
    f<< ind(lvl+1) << "<iecDeclaration active=\"FALSE\"/>\n"sv;
    f<< ind(lvl+1) << "<descr>"sv << tdef.descr() << "</descr>\n"sv;
    f<< ind(lvl) << "</typedef>\n"sv;
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Struct& strct, const std::size_t lvl)
{
    f<< ind(lvl) << "<struct name=\""sv << strct.name() << "\" version=\"1.0.0\">\n"sv;
    f<< ind(lvl+1) << "<descr>"sv << strct.descr() << "</descr>\n"sv;
    if( strct.members().empty() )
       {
        f<< ind(lvl+1) << "<vars/>\n"sv;
       }
    else
       {
        f<< ind(lvl+1) << "<vars>\n"sv;
        for( const auto& var : strct.members() )
           {
            f<< ind(lvl+2) << "<var name=\""sv << var.name() << "\" type=\""sv << var.type() << "\">\n"sv
             << ind(lvl+3) << "<descr>"sv << var.descr() << "</descr>\n"sv
             << ind(lvl+2) << "</var>\n"sv;
           }
        f<< ind(lvl+1) << "</vars>\n"sv;
       }
    f<< ind(lvl+1) << "<iecDeclaration active=\"FALSE\"/>\n"sv;
    f<< ind(lvl) << "</struct>\n"sv;
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Subrange& subrng, const std::size_t lvl)
{
    f<< ind(lvl) << "<subrange name=\""sv << subrng.name() << "\" version=\"1.0.0\" type=\""sv << subrng.type() << "\">\n"sv;
    //f<< ind(lvl+1) << "<title>"sv << subrng.title() << "</title>\n"sv;
    f<< ind(lvl+1) << "<descr>"sv << subrng.descr() << "</descr>\n"sv;
    f<< ind(lvl+1) << "<minValue>"sv << std::to_string(subrng.min_value()) << "</minValue>\n"sv;
    f<< ind(lvl+1) << "<maxValue>"sv << std::to_string(subrng.max_value()) << "</maxValue>\n"sv;
    f<< ind(lvl+1) << "<iecDeclaration active=\"FALSE\"/>\n"sv;
    f<< ind(lvl) << "</subrange>\n"sv;
}



//---------------------------------------------------------------------------
inline void write(MG::OutputStreamable auto& f, const plcb::Macro& macro, const std::size_t lvl)
{
    f<< ind(lvl) << "<macro name=\""sv << macro.name() << "\">\n"sv;
    if( macro.has_descr() )
       {
        f<< ind(lvl+1) << "<descr>"sv << macro.descr() << "</descr>\n"sv;
       }

    // [Body]
    f<< ind(lvl+1) << "<sourceCode type=\""sv << macro.code_type() << "\">\n"sv
     << ind(lvl+2) << "<![CDATA["sv << macro.body() << "]]>\n"sv
     << ind(lvl+1) << "</sourceCode>\n"sv;

    // [Parameters]
    if( macro.parameters().empty() )
       {
        f<< ind(lvl+1) << "<parameters/>\n"sv;
       }
    else
       {
        f<< ind(lvl+1) << "<parameters>\n"sv;
        for( const auto& par : macro.parameters() )
           {
            f<< ind(lvl+2) << "<parameter name=\""sv << par.name() << "\">\n"sv
             << ind(lvl+3) << "<descr>"sv << par.descr() << "</descr>\n"sv
             << ind(lvl+2) << "</parameter>\n"sv;
           }
        f<< ind(lvl+1) << "</parameters>\n"sv;
       }
    f<< ind(lvl) << "</macro>\n"sv;
}



//---------------------------------------------------------------------------
void write_preamble(MG::OutputStreamable auto& f, const plcb::Library& lib, const std::size_t lvl)
{
    //-----------------------------------------------------------------------
    const auto write_vargroups_var_list = [&f](const plcb::Variables_Groups& var_groups, const std::string_view tag, const std::string_view var_tag, const std::size_t slvl)
       {
        f<< ind(slvl) << "<" << tag;
        if( not var_groups.is_empty() )
           {
            f<< ">\n"sv;
            for( const auto& group : var_groups.groups() )
               {
                f<< ind(slvl+1) << "<group name=\""sv << group.name() << "\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\" version=\"1.0.0\">\n"sv;
                for( const auto& var : group.variables() )
                   {
                    write(f, var, var_tag, slvl+2);
                   }
                f<< ind(slvl+1) << "</group>\n"sv;
               }
            f<< ind(slvl) << "</"sv << tag <<  ">\n"sv;
           }
        else
           {
            f<< "/>\n"sv;
           }
       };

    //-----------------------------------------------------------------------
    const auto write_vargroups_list = [&f](const plcb::Variables_Groups& var_groups, const std::size_t slvl)
       {
        for( const auto& group : var_groups.groups() )
           {
            if( group.has_name() )
               {
                f<< ind(slvl) << "<group name=\""sv << group.name() << "\">\n"sv
                 << ind(slvl+1) << "<iecDeclaration active=\"FALSE\"/>\n"sv
                 << ind(slvl) << "</group>\n"sv;
               }
           }
       };

    //-----------------------------------------------------------------------
    constexpr auto trivial_hash = [](const std::string_view sv) constexpr noexcept -> std::size_t
       {
        std::size_t val = 0;
        for(std::size_t i=0; i<sv.length(); ++i)
           {
            val += (sv.length() - i) * static_cast<std::size_t>(sv[i]);
           }
        return val;
       };

    // [Workspace]
    f<< ind(lvl) << "<libWorkspace>\n"sv;
    f<< ind(lvl+1) << "<folder name=\""sv << lib.name() << "\" id=\""sv << std::to_string(trivial_hash(lib.name())) << "\">\n"sv;
    // [Global var groups list]
    for( const auto& grp : lib.global_constants().groups() ) if( grp.has_name() )  f<< ind(lvl+2) << "<GlobalVars name=\""sv << grp.name() << "\"/>\n"sv;
    for( const auto& grp : lib.global_retainvars().groups() ) if( grp.has_name() ) f<< ind(lvl+2) << "<GlobalVars name=\""sv << grp.name() << "\"/>\n"sv;
    for( const auto& grp : lib.global_variables().groups() ) if( grp.has_name() )  f<< ind(lvl+2) << "<GlobalVars name=\""sv << grp.name() << "\"/>\n"sv;
    // [Pou list]
    for( const auto& pou : lib.function_blocks() ) f<< ind(lvl+2) << "<Pou name=\""sv << pou.name() << "\"/>\n"sv;
    for( const auto& pou : lib.functions() )       f<< ind(lvl+2) << "<Pou name=\""sv << pou.name() << "\"/>\n"sv;
    for( const auto& pou : lib.programs() )        f<< ind(lvl+2) << "<Pou name=\""sv << pou.name() << "\"/>\n"sv;
    // [Definitions list]
    for( const auto& def : lib.macros() )     f<< ind(lvl+2) << "<Definition name=\""sv << def.name() << "\"/>\n"sv;
    for( const auto& def : lib.structs() )    f<< ind(lvl+2) << "<Definition name=\""sv << def.name() << "\"/>\n"sv;
    for( const auto& def : lib.typedefs() )   f<< ind(lvl+2) << "<Definition name=\""sv << def.name() << "\"/>\n"sv;
    for( const auto& def : lib.enums() )      f<< ind(lvl+2) << "<Definition name=\""sv << def.name() << "\"/>\n"sv;
    for( const auto& def : lib.subranges() )  f<< ind(lvl+2) << "<Definition name=\""sv << def.name() << "\"/>\n"sv;
    //for( const auto& def : lib.interfaces() ) f<< ind(lvl+2) << "<Definition name=\""sv << def.name() << "\"/>\n"sv;
    f<< ind(lvl+1) << "</folder>\n"sv;
    f<< ind(lvl) << "</libWorkspace>\n"sv;

    // [Global vars detailed list]
    write_vargroups_var_list(lib.global_variables(), "globalVars"sv, "var"sv, lvl);
    write_vargroups_var_list(lib.global_retainvars(), "retainVars"sv,"var"sv, lvl);
    write_vargroups_var_list(lib.global_constants(), "constantVars"sv, "const"sv, lvl);

    // [Global variables groups list]
    if( lib.global_constants().has_nonempty_named_group() or
        lib.global_retainvars().has_nonempty_named_group() or
        lib.global_variables().has_nonempty_named_group() )
       {
        f<< ind(lvl) << "<iecVarsDeclaration>\n"sv;

        write_vargroups_list(lib.global_constants(), lvl+1);
        write_vargroups_list(lib.global_variables(), lvl+1);
        write_vargroups_list(lib.global_retainvars(), lvl+1);

        f<< ind(lvl) << "</iecVarsDeclaration>\n"sv;
       }
    //else
    //   {
    //    f<< ind(lvl) << "<iecVarsDeclaration/>\n"sv;
    //   }
}



//---------------------------------------------------------------------------
void write_lib(MG::OutputStreamable auto& f, const plcb::Library& lib, const MG::keyvals& options)
{
    //-----------------------------------------------------------------------
    const auto get_indent = [](const MG::keyvals& opts) -> std::size_t
       {
        const std::string_view sv = opts.value_or("plclib-indent", "2"sv);
        if( const auto num=str::to_num_or<std::size_t>(sv) )
           {
            return num.value();
           }
        throw std::runtime_error( fmt::format("invalid plclib-indent:{}", sv) );
       };

    //-----------------------------------------------------------------------
    const auto write_pous = [&f](std::vector<plcb::Pou> const& pous, const std::string_view tag, const std::string_view pou_tag, const std::size_t lvl)
       {
        f<< ind(lvl) << '<' << tag;
        if( not pous.empty() )
           {
            f<< ">\n"sv;
            for( const auto& pou : pous )
               {
                write(f, pou, pou_tag, lvl+1);
               }
            f<< ind(lvl) << "</"sv << tag << ">\n"sv;
           }
        else
           {
            f<< "/>\n"sv;
           }
       };

    //-----------------------------------------------------------------------
    const auto write_elements = [&f]<typename T>(const std::vector<T>& elements, const std::string_view tag, const std::size_t lvl)
       {
        f<< ind(lvl) << '<' << tag;
        if( not elements.empty() )
           {
            f<< ">\n"sv;
            for( const auto& element : elements )
               {
                write(f, element, lvl+1);
               }
            f<< ind(lvl) << "</"sv << tag <<  ">\n"sv;
           }
        else
           {
            f<< "/>\n"sv;
           }
       };

    // [Options]
    // Get possible schema version
    SchemaVersion schema_ver;
    if( const auto schema_ver_str=options.value_of("plclib-schemaver"); schema_ver_str.has_value() )
       {
        schema_ver = schema_ver_str.value();
       }
    const std::size_t lvl = get_indent(options);

    // [Heading]
    f << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"sv
      << "<plcLibrary schemaVersion=\""sv << schema_ver.string() << "\">\n"sv
      << ind(1) << "<lib version=\""sv << lib.version() << "\" name=\""sv << lib.name() << "\" fullXml=\"true\">\n"sv;
    // Comment
    f << ind(lvl) << "<!-- author=\"plclib::write()\""sv;
    if( not options.contains("no-timestamp") )
       {
        f<< " date=\""sv << MG::get_human_readable_timestamp() << "\""sv;
       }
    f << " -->\n"sv;
    // Desc tag
    f << ind(lvl) << "<descr>"sv << lib.descr() << "</descr>\n"sv;
    // Content summary
    f << ind(lvl) << "<!--\n"sv;
    if( not lib.global_variables().is_empty() )  f<< ind(lvl+1) << "global-variables: "sv << std::to_string(lib.global_variables().size())  << '\n';
    if( not lib.global_constants().is_empty() )  f<< ind(lvl+1) << "global-constants: "sv << std::to_string(lib.global_constants().size())  << '\n';
    if( not lib.global_retainvars().is_empty() ) f<< ind(lvl+1) << "global-retain-vars: "sv << std::to_string(lib.global_retainvars().size())  << '\n';
    if( not lib.functions().empty() )            f<< ind(lvl+1) << "functions: "sv << std::to_string(lib.functions().size())  << '\n';
    if( not lib.function_blocks().empty() )      f<< ind(lvl+1) << "function blocks: "sv << std::to_string(lib.function_blocks().size())  << '\n';
    if( not lib.programs().empty() )             f<< ind(lvl+1) << "programs: "sv << std::to_string(lib.programs().size())  << '\n';
    if( not lib.macros().empty() )               f<< ind(lvl+1) << "macros: "sv << std::to_string(lib.macros().size())  << '\n';
    if( not lib.structs().empty() )              f<< ind(lvl+1) << "structs: "sv << std::to_string(lib.structs().size())  << '\n';
    if( not lib.typedefs().empty() )             f<< ind(lvl+1) << "typedefs: "sv << std::to_string(lib.typedefs().size())  << '\n';
    if( not lib.enums().empty() )                f<< ind(lvl+1) << "enums: "sv << std::to_string(lib.enums().size())  << '\n';
    if( not lib.subranges().empty() )            f<< ind(lvl+1) << "subranges: "sv << std::to_string(lib.subranges().size())  << '\n';
    //if( not lib.interfaces().empty() )           f<< ind(lvl+1) << "interfaces: "sv << std::to_string(lib.interfaces().size())  << '\n';
    f << ind(lvl) << "-->\n"sv;

    write_preamble(f, lib, lvl);

    write_pous(lib.functions(), "functions"sv, "function"sv, lvl );
    write_pous(lib.function_blocks(), "functionBlocks"sv, "functionBlock"sv, lvl );
    write_pous(lib.programs(), "programs"sv, "program"sv, lvl );

    write_elements(lib.macros(), "macros"sv, lvl);
    write_elements(lib.structs(), "structs"sv, lvl);
    write_elements(lib.typedefs(), "typedefs"sv, lvl);
    write_elements(lib.enums(), "enums"sv, lvl);
    write_elements(lib.subranges(), "subranges"sv, lvl);
    //write_elements(lib.interfaces(), "interfaces"sv, lvl);

    f<< ind(1) << "</lib>\n"sv
     << "</plcLibrary>\n"sv;
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
#include "string_write.hpp" // MG::string_write
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"writer_plclib"> writer_plclib_tests = []
{

ut::test("plclib:::write(plcb::Variable)") = []
   {
    MG::string_write out;

    ut::should("write an INT variable") = [out]() mutable
       {
        plclib::write(out, plcb::make_var("vn320"sv, "INT"sv, 0, ""sv, "testing variable"sv, 'M', 'W', 400, 320), "var"sv, 0);
        const std::string_view expected =
            "<var name=\"vn320\" type=\"INT\">\n"
            "\t<descr>testing variable</descr>\n"
            "\t<address type=\"M\" typeVar=\"W\" index=\"400\" subIndex=\"320\"/>\n"
            "</var>\n"sv;
        ut::expect( ut::that % out.str() == expected );
       };


    ut::should("write a STRING variable") = [out]() mutable
       {
        plclib::write(out, plcb::make_var("va0", "STRING", 80, ""sv, "testing array", 'M', 'B', 700, 0), "var"sv, 0);
        const std::string_view expected =
            "<var name=\"va0\" type=\"STRING\" length=\"80\">\n"
            "\t<descr>testing array</descr>\n"
            "\t<address type=\"M\" typeVar=\"B\" index=\"700\" subIndex=\"0\"/>\n"
            "</var>\n"sv;
        ut::expect( ut::that % out.str() == expected );
       };
   };


ut::test("plclib::write(plcb::Pou)") = []
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
        "<function name=\"pouname\" version=\"1.0.0\" creationDate=\"0\" lastModifiedDate=\"0\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\">\n"
        "\t<descr>testing pou</descr>\n"
        "\t<returnValue>INT</returnValue>\n"
        "\t<vars>\n"
        "\t\t<inoutVars>\n"
        "\t\t\t<var name=\"inout1\" type=\"DINT\">\n"
        "\t\t\t\t<descr>inout1 descr</descr>\n"
        "\t\t\t</var>\n"
        "\t\t\t<var name=\"inout2\" type=\"LREAL\">\n"
        "\t\t\t\t<descr>inout2 descr</descr>\n"
        "\t\t\t</var>\n"
        "\t\t</inoutVars>\n"
        "\t\t<inputVars>\n"
        "\t\t\t<var name=\"in1\" type=\"DINT\">\n"
        "\t\t\t\t<descr>in1 descr</descr>\n"
        "\t\t\t</var>\n"
        "\t\t\t<var name=\"in2\" type=\"LREAL\">\n"
        "\t\t\t\t<descr>in2 descr</descr>\n"
        "\t\t\t</var>\n"
        "\t\t</inputVars>\n"
        "\t\t<outputVars>\n"
        "\t\t\t<var name=\"out1\" type=\"DINT\">\n"
        "\t\t\t\t<descr>out1 descr</descr>\n"
        "\t\t\t</var>\n"
        "\t\t\t<var name=\"out2\" type=\"LREAL\">\n"
        "\t\t\t\t<descr>out2 descr</descr>\n"
        "\t\t\t</var>\n"
        "\t\t</outputVars>\n"
        "\t\t<externalVars>\n"
        "\t\t\t<var name=\"ext1\" type=\"DINT\">\n"
        "\t\t\t\t<descr>ext1 descr</descr>\n"
        "\t\t\t</var>\n"
        "\t\t\t<var name=\"ext2\" type=\"STRING\" length=\"80\">\n"
        "\t\t\t\t<descr>ext2 descr</descr>\n"
        "\t\t\t</var>\n"
        "\t\t</externalVars>\n"
        "\t\t<localVars>\n"
        "\t\t\t<var name=\"loc1\" type=\"DINT\">\n"
        "\t\t\t\t<descr>loc1 descr</descr>\n"
        "\t\t\t</var>\n"
        "\t\t\t<var name=\"loc2\" type=\"LREAL\">\n"
        "\t\t\t\t<descr>loc2 descr</descr>\n"
        "\t\t\t</var>\n"
        "\t\t</localVars>\n"
        "\t\t<localConsts>\n"
        "\t\t\t<const name=\"const1\" type=\"DINT\">\n"
        "\t\t\t\t<descr>const1 descr</descr>\n"
        "\t\t\t\t<initValue>42</initValue>\n"
        "\t\t\t</const>\n"
        "\t\t\t<const name=\"const2\" type=\"LREAL\">\n"
        "\t\t\t\t<descr>const2 descr</descr>\n"
        "\t\t\t\t<initValue>1.5</initValue>\n"
        "\t\t\t</const>\n"
        "\t\t</localConsts>\n"
        "\t</vars>\n"
        "\t<iecDeclaration active=\"FALSE\"/>\n"
        "\t<sourceCode type=\"ST\">\n"
        "\t\t<![CDATA[body]]>\n"
        "\t</sourceCode>\n"
        "</function>\n"sv;

    MG::string_write out;
    plclib::write(out, pou, "function"sv, 0);
    ut::expect( ut::that % out.str() == expected );
   };


ut::test("plclib::write(plcb::Enum)") = []
   {
    plcb::Enum enm;
    enm.set_name("enumname");
    enm.set_descr("testing enum");
    enm.elements() = { plcb::make_enelem("elm1", "1", "elm1 descr"),
                       plcb::make_enelem("elm2", "42", "elm2 descr") };

    const std::string_view expected =
        "<enum name=\"enumname\" version=\"1.0.0\">\n"
        "\t<descr>testing enum</descr>\n"
        "\t<elements>\n"
        "\t\t<element name=\"elm1\">\n"
        "\t\t\t<descr>elm1 descr</descr>\n"
        "\t\t\t<value>1</value>\n"
        "\t\t</element>\n"
        "\t\t<element name=\"elm2\">\n"
        "\t\t\t<descr>elm2 descr</descr>\n"
        "\t\t\t<value>42</value>\n"
        "\t\t</element>\n"
        "\t</elements>\n"
        "\t<iecDeclaration active=\"FALSE\"/>\n"
        "</enum>\n"sv;

    MG::string_write out;
    plclib::write(out, enm, 0);
    ut::expect( ut::that % out.str() == expected );
   };


ut::test("plclib::write(plcb::TypeDef)") = []
   {
    plcb::TypeDef tdef{ plcb::make_var("typename", "LREAL", 0u, ""sv, "testing typedef") };
    const std::string_view expected =
        "<typedef name=\"typename\" type=\"LREAL\">\n"
        "\t<iecDeclaration active=\"FALSE\"/>\n"
        "\t<descr>testing typedef</descr>\n"
        "</typedef>\n"sv;

    MG::string_write out;
    plclib::write(out, tdef, 0);
    ut::expect( ut::that % out.str() == expected );
   };


ut::test("plclib::write(plcb::Struct)") = []
   {
    plcb::Struct strct;
    strct.set_name("structname");
    strct.set_descr("testing struct");
    strct.add_member( plcb::make_var("member1", "DINT", 0u, ""sv, "member1 descr")  );
    strct.add_member( plcb::make_var("member2", "LREAL", 0u, ""sv, "member2 descr")  );

    const std::string_view expected =
        "<struct name=\"structname\" version=\"1.0.0\">\n"
        "\t<descr>testing struct</descr>\n"
        "\t<vars>\n"
        "\t\t<var name=\"member1\" type=\"DINT\">\n"
        "\t\t\t<descr>member1 descr</descr>\n"
        "\t\t</var>\n"
        "\t\t<var name=\"member2\" type=\"LREAL\">\n"
        "\t\t\t<descr>member2 descr</descr>\n"
        "\t\t</var>\n"
        "\t</vars>\n"
        "\t<iecDeclaration active=\"FALSE\"/>\n"
        "</struct>\n"sv;

    MG::string_write out;
    plclib::write(out, strct, 0);
    ut::expect( ut::that % out.str() == expected );
   };


ut::test("plclib::write(plcb::Subrange)") = []
   {
    plcb::Subrange subrng;
    subrng.set_name("subrangename");
    subrng.set_type("INT");
    subrng.set_range(1,12);
    subrng.set_descr("testing subrange");

    const std::string_view expected =
        "<subrange name=\"subrangename\" version=\"1.0.0\" type=\"INT\">\n"
        "\t<descr>testing subrange</descr>\n"
        "\t<minValue>1</minValue>\n"
        "\t<maxValue>12</maxValue>\n"
        "\t<iecDeclaration active=\"FALSE\"/>\n"
        "</subrange>\n"sv;

    MG::string_write out;
    plclib::write(out, subrng, 0);
    ut::expect( ut::that % out.str() == expected );
   };


ut::test("plclib::write(plcb::Macro)") = []
   {
    plcb::Macro macro;
    macro.set_name("macroname");
    macro.set_descr("testing macro");
    macro.parameters() = { plcb::make_mparam("par1", "par1 descr"),
                           plcb::make_mparam("par2", "par2 descr") };
    macro.set_code_type("ST");
    macro.set_body("body");

    const std::string_view expected =
        "<macro name=\"macroname\">\n"
        "\t<descr>testing macro</descr>\n"
        "\t<sourceCode type=\"ST\">\n"
        "\t\t<![CDATA[body]]>\n"
        "\t</sourceCode>\n"
        "\t<parameters>\n"
        "\t\t<parameter name=\"par1\">\n"
        "\t\t\t<descr>par1 descr</descr>\n"
        "\t\t</parameter>\n"
        "\t\t<parameter name=\"par2\">\n"
        "\t\t\t<descr>par2 descr</descr>\n"
        "\t\t</parameter>\n"
        "\t</parameters>\n"
        "</macro>\n"sv;

    MG::string_write out;
    plclib::write(out, macro, 0);
    ut::expect( ut::that % out.str() == expected );
   };


ut::test("plclib::write(plcb::Library)") = []
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
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
        "<plcLibrary schemaVersion=\"2.8\">\n"
        "\t<lib version=\"1.0.0\" name=\"testlib\" fullXml=\"true\">\n"
        "\t\t<!-- author=\"plclib::write()\" -->\n"
        "\t\t<descr>PLC library</descr>\n"
        "\t\t<!--\n"
        "\t\t\tglobal-variables: 2\n"
        "\t\t\tfunction blocks: 1\n"
        "\t\t\tprograms: 1\n"
        "\t\t-->\n"
        "\t\t<libWorkspace>\n"
        "\t\t\t<folder name=\"testlib\" id=\"3089\">\n"
        "\t\t\t\t<GlobalVars name=\"globs\"/>\n"
        "\t\t\t\t<Pou name=\"fbname\"/>\n"
        "\t\t\t\t<Pou name=\"prgname\"/>\n"
        "\t\t\t</folder>\n"
        "\t\t</libWorkspace>\n"
        "\t\t<globalVars>\n"
        "\t\t\t<group name=\"globs\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\" version=\"1.0.0\">\n"
        "\t\t\t\t<var name=\"gvar1\" type=\"DINT\">\n"
        "\t\t\t\t\t<descr>gvar1 descr</descr>\n"
        "\t\t\t\t</var>\n"
        "\t\t\t\t<var name=\"gvar2\" type=\"LREAL\">\n"
        "\t\t\t\t\t<descr>gvar2 descr</descr>\n"
        "\t\t\t\t</var>\n"
        "\t\t\t</group>\n"
        "\t\t</globalVars>\n"
        "\t\t<retainVars/>\n"
        "\t\t<constantVars/>\n"
        "\t\t<iecVarsDeclaration>\n"
        "\t\t\t<group name=\"globs\">\n"
        "\t\t\t\t<iecDeclaration active=\"FALSE\"/>\n"
        "\t\t\t</group>\n"
        "\t\t</iecVarsDeclaration>\n"
        "\t\t<functions/>\n"
        "\t\t<functionBlocks>\n"
        "\t\t\t<functionBlock name=\"fbname\" version=\"1.0.0\" creationDate=\"0\" lastModifiedDate=\"0\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\">\n"
        "\t\t\t\t<descr>testing fb</descr>\n"
        "\t\t\t\t<vars>\n"
        "\t\t\t\t\t<inoutVars>\n"
        "\t\t\t\t\t\t<var name=\"inout1\" type=\"DINT\">\n"
        "\t\t\t\t\t\t\t<descr>inout1 descr</descr>\n"
        "\t\t\t\t\t\t</var>\n"
        "\t\t\t\t\t\t<var name=\"inout2\" type=\"LREAL\">\n"
        "\t\t\t\t\t\t\t<descr>inout2 descr</descr>\n"
        "\t\t\t\t\t\t</var>\n"
        "\t\t\t\t\t</inoutVars>\n"
        "\t\t\t\t\t<inputVars>\n"
        "\t\t\t\t\t\t<var name=\"in1\" type=\"DINT\">\n"
        "\t\t\t\t\t\t\t<descr>in1 descr</descr>\n"
        "\t\t\t\t\t\t</var>\n"
        "\t\t\t\t\t\t<var name=\"in2\" type=\"LREAL\">\n"
        "\t\t\t\t\t\t\t<descr>in2 descr</descr>\n"
        "\t\t\t\t\t\t</var>\n"
        "\t\t\t\t\t</inputVars>\n"
        "\t\t\t\t\t<outputVars>\n"
        "\t\t\t\t\t\t<var name=\"out1\" type=\"DINT\">\n"
        "\t\t\t\t\t\t\t<descr>out1 descr</descr>\n"
        "\t\t\t\t\t\t</var>\n"
        "\t\t\t\t\t\t<var name=\"out2\" type=\"LREAL\">\n"
        "\t\t\t\t\t\t\t<descr>out2 descr</descr>\n"
        "\t\t\t\t\t\t</var>\n"
        "\t\t\t\t\t</outputVars>\n"
        "\t\t\t\t\t<externalVars>\n"
        "\t\t\t\t\t\t<var name=\"ext1\" type=\"DINT\">\n"
        "\t\t\t\t\t\t\t<descr>ext1 descr</descr>\n"
        "\t\t\t\t\t\t</var>\n"
        "\t\t\t\t\t\t<var name=\"ext2\" type=\"STRING\" length=\"80\">\n"
        "\t\t\t\t\t\t\t<descr>ext2 descr</descr>\n"
        "\t\t\t\t\t\t</var>\n"
        "\t\t\t\t\t</externalVars>\n"
        "\t\t\t\t\t<localVars>\n"
        "\t\t\t\t\t\t<var name=\"loc1\" type=\"DINT\">\n"
        "\t\t\t\t\t\t\t<descr>loc1 descr</descr>\n"
        "\t\t\t\t\t\t</var>\n"
        "\t\t\t\t\t\t<var name=\"loc2\" type=\"LREAL\">\n"
        "\t\t\t\t\t\t\t<descr>loc2 descr</descr>\n"
        "\t\t\t\t\t\t</var>\n"
        "\t\t\t\t\t</localVars>\n"
        "\t\t\t\t\t<localConsts>\n"
        "\t\t\t\t\t\t<const name=\"const1\" type=\"DINT\">\n"
        "\t\t\t\t\t\t\t<descr>const1 descr</descr>\n"
        "\t\t\t\t\t\t\t<initValue>42</initValue>\n"
        "\t\t\t\t\t\t</const>\n"
        "\t\t\t\t\t\t<const name=\"const2\" type=\"LREAL\">\n"
        "\t\t\t\t\t\t\t<descr>const2 descr</descr>\n"
        "\t\t\t\t\t\t\t<initValue>1.5</initValue>\n"
        "\t\t\t\t\t\t</const>\n"
        "\t\t\t\t\t</localConsts>\n"
        "\t\t\t\t</vars>\n"
        "\t\t\t\t<iecDeclaration active=\"FALSE\"/>\n"
        "\t\t\t\t<interfaces/>\n"
        "\t\t\t\t<methods/>\n"
        "\t\t\t\t<sourceCode type=\"ST\">\n"
        "\t\t\t\t\t<![CDATA[body]]>\n"
        "\t\t\t\t</sourceCode>\n"
        "\t\t\t</functionBlock>\n"
        "\t\t</functionBlocks>\n"
        "\t\t<programs>\n"
        "\t\t\t<program name=\"prgname\" version=\"1.0.0\" creationDate=\"0\" lastModifiedDate=\"0\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\">\n"
        "\t\t\t\t<descr>testing prg</descr>\n"
        "\t\t\t\t<vars>\n"
        "\t\t\t\t\t<localVars>\n"
        "\t\t\t\t\t\t<var name=\"loc1\" type=\"DINT\">\n"
        "\t\t\t\t\t\t\t<descr>loc1 descr</descr>\n"
        "\t\t\t\t\t\t</var>\n"
        "\t\t\t\t\t\t<var name=\"loc2\" type=\"LREAL\">\n"
        "\t\t\t\t\t\t\t<descr>loc2 descr</descr>\n"
        "\t\t\t\t\t\t</var>\n"
        "\t\t\t\t\t</localVars>\n"
        "\t\t\t\t</vars>\n"
        "\t\t\t\t<iecDeclaration active=\"FALSE\"/>\n"
        "\t\t\t\t<sourceCode type=\"ST\">\n"
        "\t\t\t\t\t<![CDATA[body]]>\n"
        "\t\t\t\t</sourceCode>\n"
        "\t\t\t</program>\n"
        "\t\t</programs>\n"
        "\t\t<macros/>\n"
        "\t\t<structs/>\n"
        "\t\t<typedefs/>\n"
        "\t\t<enums/>\n"
        "\t\t<subranges/>\n"
        "\t</lib>\n"
        "</plcLibrary>\n"sv;

    MG::string_write out;
    MG::keyvals opts; opts.assign("no-timestamp,plclib-indent:2");
    plclib::write_lib(out, lib, opts);
    ut::expect( ut::that % out.str() == expected );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
