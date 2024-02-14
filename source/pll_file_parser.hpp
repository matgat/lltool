#pragma once
//  ---------------------------------------------
//  Parses a LogicLab 'pll' file
//  ---------------------------------------------
//  #include "pll_file_parser.hpp" // ll::pll_parse()
//  ---------------------------------------------
#include <cassert>
#include <cstdint> // std::uint32_t

#include "plain_parser_base.hpp" // plain::ParserBase
#include "plc_library.hpp" // plcb::*
#include "string_utilities.hpp" // str::trim_right()



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace ll
{


/////////////////////////////////////////////////////////////////////////////
class PllParser final : public plain::ParserBase<char>
{            using inherited = plain::ParserBase<char>;
 public:
    PllParser(const std::string_view buf)
      : inherited(buf)
       {}

    //-----------------------------------------------------------------------
    void check_heading_comment(plcb::Library& lib)
       {
        inherited::skip_any_space();
        if( eat_block_comment_start() )
           {// Could be my custom header
            //(*
            //    name: test
            //    descr: Libraries for strato machines (Macotec M-series machines)
            //    version: 0.5.0
            //    author: MG
            //    dependencies: Common.pll, defvar.pll, iomap.pll, messages.pll
            //*)
            // Yes, using a parser inside a parser!
            inherited parser{ get_block_comment() };
            struct keyvalue_t final
               {
                std::string_view key;
                std::string_view value;

                [[nodiscard]] bool get_next( inherited& parser )
                   {
                    while( true )
                       {
                        parser.skip_any_space();
                        key = parser.get_identifier();
                        if( not key.empty() )
                           {
                            parser.skip_blanks();
                            if( parser.got_any_of<':','='>() )
                               {
                                parser.get_next();
                                parser.skip_blanks();
                                value = parser.get_rest_of_line();
                                if( not value.empty() )
                                   {
                                    return true;
                                   }
                               }
                           }
                        else
                           {
                            return false;
                           }
                       }
                   }
               } entry;

            while( entry.get_next(parser) )
               {
                if( entry.key.starts_with("descr"sv) )
                   {
                    lib.set_descr( entry.value );
                   }
                else if( entry.key=="version"sv )
                   {
                    lib.set_version( entry.value );
                   }
               }
           }
       }


    //-----------------------------------------------------------------------
    void collect_next(plcb::Library& lib)
       {
        inherited::skip_any_space();
        if( not inherited::has_codepoint() )
           {
           }
        else if( eat_block_comment_start() )
           {
            skip_block_comment();
           }
        else if( inherited::eat_token("PROGRAM"sv) )
           {
            auto& pou = lib.programs().emplace_back();
            collect_pou(pou, "PROGRAM"sv, "END_PROGRAM"sv);
           }
        else if( inherited::eat_token("FUNCTION_BLOCK"sv) )
           {
            auto& pou = lib.function_blocks().emplace_back();
            collect_pou(pou, "FUNCTION_BLOCK"sv, "END_FUNCTION_BLOCK"sv);
           }
        else if( inherited::eat_token("FUNCTION"sv) )
           {
            auto& pou = lib.functions().emplace_back();
            collect_pou(pou, "FUNCTION"sv, "END_FUNCTION"sv, true);
           }
        else if( inherited::eat_token("MACRO"sv) )
           {
            auto& macro = lib.macros().emplace_back();
            collect_macro(macro);
           }
        else if( inherited::eat_token("TYPE"sv) )
           {// struct/typdef/enum/subrange
            collect_types(lib);
           }
        else if( inherited::eat_token("VAR_GLOBAL"sv) )
           {
            const auto [ constants ] = collect_var_block_modifiers();
            if( constants )
               {
                collect_global_constants( lib.global_constants().groups() );
               }
            else
               {
                collect_global_vars( lib.global_variables().groups() );
               }
           }
        else
           {
            throw inherited::create_parse_error( fmt::format("Unexpected content: {}", str::escape(inherited::get_rest_of_line())) );
           }
       }

#ifndef TEST_UNITS
 private:
#endif
    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat_block_comment_start() noexcept
       {
        return inherited::eat("(*"sv);
       }
    [[nodiscard]] std::string_view get_block_comment()
       {
        return inherited::get_until("*)");
       }
    void skip_block_comment()
       {
        [[maybe_unused]] auto cmt = inherited::get_until("*)");
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] bool got_directive_start() noexcept
       {// { DE:"some string" }
        return inherited::got('{');
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] plcb::Directive collect_directive()
       {// { DE:"some string" } or { CODE:ST }
        plcb::Directive dir;

        assert( inherited::got('{') );
        inherited::get_next();

        inherited::skip_blanks();
        dir.set_key( inherited::get_identifier() );

        inherited::skip_blanks();
        if( not inherited::got(':') )
           {
            throw inherited::create_parse_error( fmt::format("Missing ':' after directive {}", dir.key()) );
           }
        inherited::get_next();
        inherited::skip_blanks();

        if( inherited::got('\"') )
           {
            inherited::get_next();
            dir.set_value( inherited::get_until_and_skip(ascii::is<'\"'>, ascii::is_any_of<'<','>','\n'>) );
           }
        else
           {
            dir.set_value( inherited::get_identifier() );
           }

        inherited::skip_blanks();
        if( not inherited::got('}') )
           {
            throw inherited::create_parse_error( fmt::format("Unclosed directive {} after {}", dir.key(), dir.value()) );
           }
        inherited::get_next();

        return dir;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] plcb::Variable collect_variable()
       {//  VarName : Type := Val; { DE:"descr" }
        //  VarName AT %MB300.6000 : ARRAY[ 0..999 ] OF BOOL; { DE:"descr" }
        //  VarName AT %MB700.0 : STRING[ 80 ]; {DE:"descr"}
        plcb::Variable var;

        // [Name]
        inherited::skip_blanks();
        var.set_name( inherited::get_identifier() );
        inherited::skip_blanks();
        if( inherited::got(',') )
           {
            throw inherited::create_parse_error( fmt::format("Multiple names not supported in declaration of variable \"{}\"", var.name()) );
           }

        // [Location address]
        if( inherited::eat_token("AT"sv) )
           {// Specified a location address %<type><typevar><index>.<subindex>
            inherited::skip_blanks();
            if( not inherited::eat('%') )
               {
                throw inherited::create_parse_error( fmt::format("Expected '%' in address of variable \"{}\" address", var.name()) );
               }
            // Here expecting something like: MB300.6000
            var.address().set_zone( inherited::curr_codepoint() ); // Typically M or Q
            inherited::get_next();
            var.address().set_typevar( inherited::curr_codepoint() );
            inherited::get_next();
            var.address().set_index( inherited::extract_index<decltype(var.address().index())>() );
            if( not inherited::eat('.') )
               {
                throw inherited::create_parse_error( fmt::format("Expected '.' in variable \"{}\" address", var.name()) );
               }
            var.address().set_subindex( inherited::extract_index<decltype(var.address().subindex())>() );
            inherited::skip_blanks();
           }

        // [Name/Type separator]
        if( not inherited::eat(':') )
           {
            throw inherited::create_parse_error( fmt::format("Expected ':' before variable \"{}\" type", var.name()) );
           }
        collect_variable_data(var);
        return var;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] plcb::Type collect_type()
       {// ARRAY[ 0..999 ] OF BOOL
        // ARRAY[1..2, 1..2] OF DINT
        // STRING[ 80 ]
        // DINT
        plcb::Type type;

        // [Array data]
        inherited::skip_blanks();
        if( inherited::eat_token("ARRAY"sv) )
           {// Specifying an array ex. ARRAY[ 0..999 ] OF BOOL;
            // Get array size
            inherited::skip_blanks();
            if( not inherited::eat('[') )
               {
                throw inherited::create_parse_error("Expected '[' in array declaration");
               }

            inherited::skip_blanks();
            const auto idx_start = inherited::extract_index<decltype(type.array_startidx())>();
            inherited::skip_blanks();
            if( not inherited::eat(".."sv) )
               {
                throw inherited::create_parse_error("Expected \"..\" in array range declaration");
               }
            inherited::skip_blanks();
            const auto idx_last = inherited::extract_index<decltype(type.array_lastidx())>();
            inherited::skip_blanks();
            if( inherited::got(',') )
               {
                throw inherited::create_parse_error("Multidimensional arrays not yet supported");
               }
            // ...Support multidimensional arrays?
            if( not inherited::eat(']') )
               {
                throw inherited::create_parse_error("Expected ']' in array declaration");
               }
            inherited::skip_blanks();
            if( not inherited::eat_token("OF"sv) )
               {
                throw inherited::create_parse_error("Expected \"OF\" in array declaration");
               }
            type.set_array_range(idx_start, idx_last);
            inherited::skip_blanks();
           }

        // [Type name]
        type.set_name( inherited::get_identifier() );
        inherited::skip_blanks();
        if( inherited::eat('[') )
           {// There's a length
            inherited::skip_blanks();
            type.set_length( extract_index<decltype(type.length())>() );
            inherited::skip_blanks();
            if( not inherited::eat(']') )
               {
                throw inherited::create_parse_error("Expected ']' in type length");
               }
            inherited::skip_blanks();
           }

        return type;
       }


    //-----------------------------------------------------------------------
    void collect_variable_data(plcb::Variable& var)
       {// ... ARRAY[ 0..999 ] OF BOOL; { DE:"descr" }
        // ... ARRAY[1..2, 1..2] OF DINT := [1, 2, 3, 4]; { DE:"multidimensional array" }
        // ... STRING[ 80 ]; { DE:"descr" }
        // ... DINT; { DE:"descr" }
        var.type() = collect_type();

        // [Value]
        if( inherited::eat(':') )
           {
            if( not inherited::eat('=') )
               {
                throw inherited::create_parse_error( fmt::format("Unexpected colon in variable \"{}\" type", var.name()) );
               }
            inherited::skip_blanks();
            if( inherited::got('[') )
               {
                throw inherited::create_parse_error( fmt::format("Array initialization not yet supported in variable \"{}\"", var.name()) );
               }

            var.set_value( str::trim_right(inherited::get_until(ascii::is<';'>, ascii::is_any_of<':','=','<','>','\"','\n'>)) );
           }

        if( !inherited::eat(';') )
           {
            throw inherited::create_parse_error( fmt::format("Truncated definition of variable \"{}\"", var.name()) );
           }

        // [Description]
        inherited::skip_blanks();
        if( got_directive_start() )
           {
            const plcb::Directive dir = collect_directive();
            if( dir.key()=="DE"sv )
               {
                var.set_descr( dir.value() );
               }
            else
               {
                throw inherited::create_parse_error( fmt::format("Unexpected directive \"{}\" in variable \"{}\" declaration", dir.key(), var.name()) );
               }
           }

        // Expecting a line end now
        inherited::check_and_eat_endline();
       }


    //-----------------------------------------------------------------------
    void collect_global_constants(std::vector<plcb::Variables_Group>& vgroups)
       {
        collect_global_vars(vgroups, true);
       }
    void collect_global_vars(std::vector<plcb::Variables_Group>& vgroups, const bool value_needed =false)
       {//    VAR_GLOBAL
        //    {G:"System"}
        //    Cnc : fbCncM32; { DE:"device" }
        //    {G:"Arrays"}
        //    vbMsgs AT %MB300.6000 : ARRAY[ 0..999 ] OF BOOL; { DE:"msg array" }
        //    END_VAR

        // Since I'm appending I won't support more than one for each global groups
        if( not vgroups.empty() )
           {
            throw inherited::create_parse_error("Multiple blocks of global variables declarations not allowed");
           }

        const auto start = inherited::save_context();
        while( true )
           {
            inherited::skip_blanks();
            if( not inherited::has_codepoint() )
               {
                throw inherited::create_parse_error("VAR_GLOBAL not closed by END_VAR", start.line);
               }
            else if( inherited::got_endline() )
               {// Skip empty lines
                inherited::get_next();
               }
            else if( eat_block_comment_start() )
               {
                skip_block_comment();
               }
            else if( got_directive_start() )
               {
                const plcb::Directive dir = collect_directive();
                if( dir.key()=="G" )
                   {// A group
                    vgroups.emplace_back().set_name( dir.value() );
                   }
                else
                   {
                    throw inherited::create_parse_error( fmt::format("Unexpected directive \"{}\" in global vars", dir.key()) );
                   }
               }
            else if( inherited::eat_token("END_VAR"sv) )
               {
                break;
               }
            else
               {// Expected a variable entry
                if( vgroups.empty() )
                   {
                    vgroups.emplace_back(); // Unnamed group
                   }
                const plcb::Variable& var = vgroups.back().add_variable( collect_variable() );

                if( value_needed and !var.has_value() )
                   {
                    throw inherited::create_parse_error( fmt::format("Value not specified for \"{}\"", var.name()) );
                   }
               }
           }
       }


    //-----------------------------------------------------------------------
    struct varblock_modifiers_t final { bool constants=false; };
    varblock_modifiers_t collect_var_block_modifiers()
       {
        varblock_modifiers_t modifiers;

        while( not inherited::got_endline() and inherited::has_codepoint() )
           {
            inherited::skip_blanks();
            const std::string_view modifier = inherited::get_notspace();
            if( not modifier.empty() )
               {
                if( modifier=="CONSTANT"sv )
                   {
                    modifiers.constants = true;
                   }
                //else if( modifier=="RETAIN"sv )
                else
                   {
                    throw inherited::create_parse_error( fmt::format("Modifier `{}` not supported"sv, modifier) );
                   }
               }
           }
        inherited::get_next(); // skip endline

        return modifiers;
       }


    //-----------------------------------------------------------------------
    void collect_pou(plcb::Pou& pou, const std::string_view start_tag, const std::string_view end_tag, const bool needs_ret_type =false)
       {
        //POU NAME : RETURN_VALUE
        //{ DE:"Description" }
        //    VAR_YYY
        //    ...
        //    END_VAR
        //    { CODE:ST }
        //(* Body *)
        //END_POU

        struct local final //////////////////////////////////////////////////
           {
            //-------------------------------------------------------------------
            static void collect_constants_block(ll::PllParser& parser, std::vector<plcb::Variable>& vars)
               {
                collect_variables_block(parser, vars, true);
               }
            static void collect_variables_block(ll::PllParser& parser, std::vector<plcb::Variable>& vars, const bool value_needed =false)
               {
                struct local final
                   {
                    [[nodiscard]] static bool contains(const std::vector<plcb::Variable>& vars, const std::string_view var_name) noexcept
                       {
                        //return std::ranges::any_of(vars, [](const plcb::Variable& var) noexcept { return var.name()==var_name; });
                        for(const plcb::Variable& var : vars) if(var.name()==var_name) return true;
                        return false;
                       }

                    [[nodiscard]] static plcb::Variable& add_variable(std::vector<plcb::Variable>& vars, plcb::Variable&& var)
                       {
                        if( contains(vars, var.name()) )
                           {
                            throw std::runtime_error( fmt::format("Duplicate variable \"{}\"", var.name()) );
                           }
                        vars.push_back( std::move(var) );
                        return vars.back();
                       }
                   };

                const auto start = parser.save_context();
                while( true )
                   {
                    parser.skip_blanks();
                    if( not parser.has_codepoint() )
                       {
                        parser.restore_context( start ); // Strong guarantee
                        throw parser.create_parse_error("VAR block not closed by END_VAR", start.line);
                       }
                    else if( parser.got_endline() )
                       {
                        parser.get_next();
                       }
                    else if( parser.eat_block_comment_start() )
                       {
                        parser.skip_block_comment();
                       }
                    else if( parser.eat_token("END_VAR"sv) )
                       {
                        break;
                       }
                    else
                       {// Expected a variable entry
                        const plcb::Variable& var = local::add_variable(vars, parser.collect_variable());

                        if( value_needed and !var.has_value() )
                           {
                            throw parser.create_parse_error( fmt::format("Value not specified for \"{}\"", var.name()) );
                           }
                       }
                   }
               }

            //-------------------------------------------------------------------
            static void collect_pou_header(ll::PllParser& parser, plcb::Pou& pou, const std::string_view start_tag, const std::string_view end_tag)
               {
                const auto start = parser.save_context();
                while( true )
                   {
                    parser.skip_blanks();
                    if( not parser.has_codepoint() )
                       {
                        parser.restore_context( start ); // Strong guarantee
                        throw parser.create_parse_error( fmt::format("{} not closed by {}", start_tag, end_tag), start.line );
                       }
                    else if( parser.got_endline() )
                       {
                        parser.get_next();
                        continue;
                       }
                    else if( parser.got_directive_start() )
                       {
                        const plcb::Directive dir = parser.collect_directive();
                        if( dir.key()=="DE"sv )
                           {// Is a description
                            if( not pou.descr().empty() )
                               {
                                throw parser.create_parse_error( fmt::format("{} has already a description: {}", start_tag, pou.descr()) );
                               }
                            pou.set_descr( dir.value() );
                           }
                        else if( dir.key()=="CODE"sv )
                           {// Header finished
                            //fmt::print("\033[36m pou header ends at '{}' ({}:{})\033[0m\n", str::escape(parser.curr_codepoint()), parser.curr_line(), parser.curr_offset());
                            pou.set_code_type( dir.value() );
                            break;
                           }
                        else
                           {
                            throw parser.create_parse_error( fmt::format("Unexpected directive \"{}\" in {} {}", dir.key(), start_tag, pou.name()) );
                           }
                       }
                    else if( parser.eat_token("VAR_INPUT"sv) )
                       {
                        parser.skip_endline();
                        collect_variables_block( parser, pou.input_vars() );
                       }
                    else if( parser.eat_token("VAR_OUTPUT"sv) )
                       {
                        parser.skip_endline();
                        collect_variables_block( parser, pou.output_vars() );
                       }
                    else if( parser.eat_token("VAR_IN_OUT"sv) )
                       {
                        parser.skip_endline();
                        collect_variables_block( parser, pou.inout_vars() );
                       }
                    else if( parser.eat_token("VAR_EXTERNAL"sv) )
                       {
                        parser.skip_endline();
                        collect_variables_block( parser, pou.external_vars() );
                       }
                    else if( parser.eat_token("VAR"sv) )
                       {
                        const auto [ constants ] = parser.collect_var_block_modifiers();
                        if( constants )
                           {
                            collect_constants_block( parser, pou.local_constants() );
                           }
                        else
                           {
                            collect_variables_block( parser, pou.local_vars() );
                           }
                       }
                    else
                       {
                        throw parser.create_parse_error( fmt::format("Unexpected content in {} {} header: {}", start_tag, pou.name(), str::escape(parser.get_rest_of_line())) );
                       }
                   }
               }
           }; ///////////////////////////////////////////////////////////////

        // Get name
        inherited::skip_blanks();
        pou.set_name( inherited::get_identifier() );
        if( pou.name().empty() )
           {
            throw inherited::create_parse_error( fmt::format("No name found for {}", start_tag) );
           }

        // Get possible return type
        inherited::skip_blanks();
        if( inherited::eat(':') )
           {// Got a return type!
            inherited::skip_blanks();
            pou.set_return_type( get_alphabetic() );
            if( pou.return_type().empty() )
               {
                throw inherited::create_parse_error( fmt::format("Empty return type in {} {}", start_tag, pou.name()) );
               }
            if( not needs_ret_type )
               {
                throw inherited::create_parse_error( fmt::format("Return type specified in {} {}", start_tag, pou.name()) );
               }
            skip_endline();
           }
        else
           {// No return type
            if( needs_ret_type )
               {
                throw inherited::create_parse_error( fmt::format("Return type not specified in {} {}", start_tag, pou.name()) );
               }
           }

        local::collect_pou_header(*this, pou, start_tag, end_tag);
        pou.set_body( inherited::get_until_newline_token(end_tag) );
        skip_endline();
       }


    //-----------------------------------------------------------------------
    void collect_macro(plcb::Macro& macro)
       {
        //MACRO NAME
        //{ DE:"Macro description" }
        //    PAR_MACRO
        //    WHAT; { DE:"Parameter description" }
        //    END_PAR
        //    { CODE:ST }
        //(* Macro body *)
        //END_MACRO

        struct local final //////////////////////////////////////////////////
           {
            //-----------------------------------------------------------------------
            [[nodiscard]] static plcb::Macro::Parameter collect_macro_parameter(ll::PllParser& parser)
               {//   WHAT; {DE:"Parameter description"}
                plcb::Macro::Parameter par;

                parser.skip_blanks();
                par.set_name( parser.get_identifier() );
                parser.skip_blanks();
                if( not parser.eat(';') )
                   {
                    throw parser.create_parse_error("Missing ';' after macro parameter");
                   }

                parser.skip_blanks();
                if( parser.got_directive_start() )
                   {
                    const plcb::Directive dir = parser.collect_directive();
                    if( dir.key()=="DE"sv )
                       {
                        par.set_descr( dir.value() );
                       }
                    else
                       {
                        throw parser.create_parse_error( fmt::format("Unexpected directive \"{}\" in macro parameter", dir.key()) );
                       }
                   }

                parser.skip_endline();

                return par;
               }

            //-----------------------------------------------------------------------
            static void collect_macro_parameters(ll::PllParser& parser, std::vector<plcb::Macro::Parameter>& pars)
               {
                const auto start = parser.save_context();
                while( true )
                   {
                    parser.skip_blanks();
                    if( not parser.has_codepoint() )
                       {
                        parser.restore_context( start ); // Strong guarantee
                        throw parser.create_parse_error("PAR_MACRO not closed by END_PAR", start.line);
                       }
                    else if( parser.got_endline() )
                       {
                        parser.get_next();
                       }
                    else if( parser.eat_block_comment_start() )
                       {
                        parser.skip_block_comment();
                       }
                    else if( parser.eat_token("END_PAR"sv) )
                       {
                        break;
                       }
                    else
                       {
                        pars.push_back( collect_macro_parameter(parser) );
                       }
                   }
               }

            //-----------------------------------------------------------------------
            static void collect_macro_header(ll::PllParser& parser, plcb::Macro& macro)
               {
                const auto start = parser.save_context();
                while( true )
                   {
                    parser.skip_blanks();
                    if( not parser.has_codepoint() )
                       {
                        parser.restore_context( start ); // Strong guarantee
                        throw parser.create_parse_error("MACRO not closed by END_MACRO");
                       }
                    else if( parser.got_endline() )
                       {
                        parser.get_next();
                        continue;
                       }
                    else if( parser.got_directive_start() )
                       {
                        const plcb::Directive dir = parser.collect_directive();
                        if( dir.key()=="DE"sv )
                           {// Is a description
                            if( not macro.descr().empty() )
                               {
                                throw parser.create_parse_error( fmt::format("Macro {} has already a description: {}", macro.name(), macro.descr()) );
                               }
                            macro.set_descr( dir.value() );
                           }
                        else if( dir.key()=="CODE"sv )
                           {// Header finished
                            macro.set_code_type( dir.value() );
                            break;
                           }
                        else
                           {
                            throw parser.create_parse_error( fmt::format("Unexpected directive \"{}\" in macro {} header", dir.key(), macro.name()) );
                           }
                       }
                    else if( parser.eat_token("PAR_MACRO"sv) )
                       {
                        parser.skip_endline();
                        if( not macro.parameters().empty() )
                           {
                            throw parser.create_parse_error("Multiple groups of macro parameters are not allowed");
                           }
                        collect_macro_parameters( parser, macro.parameters() );
                       }
                    else
                       {
                        throw parser.create_parse_error( fmt::format("Unexpected content in macro {} header: {}", macro.name(), str::escape(parser.get_rest_of_line())) );
                       }
                   }
               }
           }; ///////////////////////////////////////////////////////////////

        inherited::skip_blanks();
        macro.set_name( inherited::get_identifier() );
        if( macro.name().empty() )
           {
            throw inherited::create_parse_error("No name found for MACRO");
           }

        local::collect_macro_header(*this, macro);
        macro.set_body( inherited::get_until_newline_token("END_MACRO"sv) );
        skip_endline();
       }


    //-----------------------------------------------------------------------
    void collect_types(plcb::Library&){} // ###### TEMP #####################

    //void collect_types(plcb::Library& lib)
    //   {
    //    //    TYPE
    //    //    struct_name : STRUCT { DE:"descr" } member : DINT; { DE:"member descr" } ... END_STRUCT;
    //    //    type_name : STRING[ 80 ]; { DE:"descr" }
    //    //    enum_name: ( { DE:"descr" } VAL1 := 0, { DE:"elem descr" } ... );
    //    //    subrange_name : DINT (30..100);
    //    //    END_TYPE
    //
    //    struct local final //////////////////////////////////////////////////
    //       {
    //        //-----------------------------------------------------------------------
    //        static void collect_struct_body(ll::PllParser& parser, plcb::Struct& strct)
    //           {// <name> : STRUCT { DE:"struct descr" }
    //            //    x : DINT; { DE:"member descr" }
    //            //    y : DINT; { DE:"member descr" }
    //            //END_STRUCT;
    //
    //            // Name already collected, "STRUCT" already skipped
    //            // Possible description
    //            parser.skip_blanks();
    //            if( got_directive_start() )
    //               {
    //                const plcb::Directive dir = collect_directive();
    //                if( dir.key()=="DE"sv )
    //                   {
    //                    strct.set_descr( dir.value() );
    //                   }
    //                else
    //                   {
    //                    throw parser.create_parse_error( fmt::format("Unexpected directive \"{}\" in struct \"{}\"", dir.key(), strct.name()) );
    //                   }
    //               }
    //
    //            while( i<siz )
    //               {
    //                parser.skip_any_space();
    //                if( parser.eat("END_STRUCT;"sv) )
    //                   {
    //                    break;
    //                   }
    //                else if( parser.got_endline() )
    //                   {// Nella lista membri ammetto righe vuote
    //                    parser.get_next();
    //                    continue;
    //                   }
    //                else if( eat_block_comment_start() )
    //                   {// Nella lista membri ammetto righe di commento
    //                    skip_block_comment();
    //                    continue;
    //                   }
    //
    //                strct.add_member( collect_member() );
    //
    //                if( strct.members().back().has_address() )
    //                   {
    //                    throw parser.create_parse_error( fmt::format("Struct member \"{}\" cannot have an address", strct.members().back().name()) );
    //                   }
    //               }
    //
    //            // Expecting a line end now
    //            parser.check_and_eat_endline(); // ("struct {}"sv, strct.name());
    //           }
    //
    //        //-----------------------------------------------------------------------
    //        //static [[nodiscard]] bool collect_enum_element(ll::PllParser& parser, plcb::Enum::Element& elem)
    //        //   {// VAL1 := 0, { DE:"elem descr" }
    //        //    // [Name]
    //        //    parser.skip_any_space();
    //        //    elem.set_name( parser.get_identifier() );
    //        //
    //        //    // [Value]
    //        //    parser.skip_blanks();
    //        //    if( not parser.eat(":="sv) )
    //        //       {
    //        //        throw parser.create_parse_error( fmt::format("Value not found in enum element \"{}\"", elem.name()) );
    //        //       }
    //        //    parser.skip_blanks();
    //        //    elem.set_value( collect_numeric_value() );
    //        //    parser.skip_blanks();
    //        //
    //        //    // Qui c'è una virgola se ci sono altri valori successivi
    //        //    const bool has_next = i<siz and parser.eat(',');
    //        //
    //        //    // [Description]
    //        //    parser.skip_blanks();
    //        //    if( got_directive_start() )
    //        //       {
    //        //        const plcb::Directive dir = collect_directive();
    //        //        if( dir.key()=="DE"sv )
    //        //           {
    //        //            elem.set_descr( dir.value() );
    //        //           }
    //        //        else
    //        //           {
    //        //            throw parser.create_parse_error( fmt::format("Unexpected directive \"{}\" in enum element \"{}\"", dir.key(), elem.name()) );
    //        //           }
    //        //       }
    //        //
    //        //    // Expecting a line end now
    //        //    parser.check_and_eat_endline(); // ("enum element {}"sv, elem.name());
    //        //    return has_next;
    //        //   }
    //
    //
    //        //-----------------------------------------------------------------------
    //        //static void collect_enum_body(ll::PllParser& parser, plcb::Enum& en)
    //        //   {// <name>: ( { DE:"enum descr" }
    //        //    //     VAL1 := 0, { DE:"elem descr" }
    //        //    //     VAL2 := -1 { DE:"elem desc" }
    //        //    // );
    //        //    // Name already collected, ": (" already skipped
    //        //    // Possible description
    //        //    parser.skip_blanks();
    //        //    // Possibile interruzione di linea
    //        //    if( parser.got_endline() )
    //        //       {
    //        //        parser.get_next();
    //        //        parser.skip_blanks();
    //        //       }
    //        //    if( got_directive_start() )
    //        //       {
    //        //        const plcb::Directive dir = collect_directive();
    //        //        if( dir.key()=="DE"sv )
    //        //           {
    //        //            en.set_descr( dir.value() );
    //        //           }
    //        //        else
    //        //           {
    //        //            throw parser.create_parse_error( fmt::format("Unexpected directive \"{}\" in enum \"{}\"", dir.key(), en.name()) );
    //        //           }
    //        //       }
    //        //
    //        //    // Elements
    //        //    bool has_next;
    //        //    do {
    //        //        plcb::Enum::Element elem;
    //        //        has_next = collect_enum_element(elem);
    //        //        en.elements().push_back(elem);
    //        //       }
    //        //    while( has_next );
    //        //
    //        //    // End expected
    //        //    parser.skip_blanks();
    //        //    if( not parser.eat(");"sv) )
    //        //       {
    //        //        throw parser.create_parse_error( fmt::format("Expected termination \");\" after enum \"{}\"", en.name()) );
    //        //       }
    //        //
    //        //    // Expecting a line end now
    //        //    parser.check_and_eat_endline(); // ("enum {}"sv, en.name());
    //        //   }
    //
    //
    //        //-----------------------------------------------------------------------
    //        //static void collect_subrange_body(ll::PllParser& parser, plcb::Subrange& subr)
    //        //   {// <name> : DINT (5..23); { DE:"descr" }
    //        //    // Name already collected, ':' already skipped
    //        //
    //        //    // [Type]
    //        //    parser.skip_blanks();
    //        //    subr.set_type(parser.get_identifier());
    //        //
    //        //    // [Min and Max]
    //        //    parser.skip_blanks();
    //        //    if( not parser.eat('(') )
    //        //       {
    //        //        throw parser.create_parse_error( fmt::format("Expected \"(min..max)\" in subrange \"{}\"", subr.name()) );
    //        //       }
    //        //    parser.skip_blanks();
    //        //    const auto min_val = extract_integer(); // extract_double(); // Nah, Floating point numbers seems not supported
    //        //    parser.skip_blanks();
    //        //    if( not parser.eat(".."sv) )
    //        //       {
    //        //        throw parser.create_parse_error( fmt::format("Expected \"..\" in subrange \"{}\"", subr.name()) );
    //        //       }
    //        //    parser.skip_blanks();
    //        //    const auto max_val = extract_integer();
    //        //    parser.skip_blanks();
    //        //    if( not parser.eat(')') )
    //        //       {
    //        //        throw parser.create_parse_error( fmt::format("Expected ')' in subrange \"{}\"", subr.name()) );
    //        //       }
    //        //
    //        //    parser.skip_blanks();
    //        //    if( not parser.eat(';') )
    //        //       {
    //        //        throw parser.create_parse_error( fmt::format("Expected ';' in subrange \"{}\"", subr.name()) );
    //        //       }
    //        //    subr.set_range(min_val, max_val);
    //        //
    //        //    // [Description]
    //        //    parser.skip_blanks();
    //        //    if( got_directive_start() )
    //        //       {
    //        //        const plcb::Directive dir = collect_directive();
    //        //        if(dir.key()=="DE"sv)
    //        //           {
    //        //            subr.set_descr(dir.value());
    //        //           }
    //        //        else
    //        //           {
    //        //            throw parser.create_parse_error( fmt::format("Unexpected directive \"{}\" in subrange \"{}\" declaration", dir.key(), subr.name()) );
    //        //           }
    //        //       }
    //        //
    //        //    // Expecting a line end now
    //        //    parser.check_and_eat_endline(); // ("subrange {}"sv, subr.name());
    //        //   }
    //
    //       }; ///////////////////////////////////////////////////////////////
    //
    //    const auto start = parser.save_context();
    //    while( true )
    //       {
    //        inherited::skip_blanks();
    //        if( not inherited::has_codepoint() )
    //           {
    //            parser.restore_context( start ); // Strong guarantee
    //            throw inherited::create_parse_error("TYPE not closed by END_TYPE", start.line );
    //           }
    //        else if( inherited::got_endline() )
    //           {
    //            inherited::get_next();
    //            continue;
    //           }
    //        else if( inherited::eat_token("END_TYPE"sv) )
    //           {
    //            break;
    //           }
    //        else
    //           {// Expected a type name token here
    //            const std::string_view type_name = inherited::get_identifier();
    //            if( type_name.empty() )
    //               {
    //                throw inherited::create_parse_error( fmt::format("Unexpected content in TYPE block: {}", str::escape(parser.get_rest_of_line())) );
    //               }
    //
    //            // Expected a colon after the name
    //            inherited::skip_blanks();
    //            if( not inherited::eat(':') )
    //               {
    //                throw inherited::create_parse_error( fmt::format("Missing ':' after type name \"{}\"", type_name) );
    //               }
    //
    //            // Check what it is (struct, typedef, enum, subrange)
    //            inherited::skip_blanks();
    //            if( inherited::eat_token("STRUCT"sv) )
    //               {// <name> : STRUCT
    //                plcb::Struct& strct = lib.structs().emplace_back();
    //                strct.set_name(type_name);
    //                collect_struct_body( parser, strct );
    //               }
    //            else if( inherited::eat('(') )
    //               {// <name>: ( { DE:"an enum" } ...
    //                plcb::Enum& en = lib.enums().emplace_back();
    //                en.set_name(type_name);
    //                collect_enum_body( en );
    //               }
    //            else
    //               {// Could be a typedef or a subrange
    //                // I have to peek to see which one
    //                std::size_t j = i;
    //                while(j<siz and buf[j]!=';' and buf[j]!='(' and buf[j]!='{' and buf[j]!='\n' ) ++j;
    //                if( j<siz and buf[j]=='(' )
    //                   {// <name> : <type> (<min>..<max>); { DE:"a subrange" }
    //                    plcb::Subrange& subr = lib.subranges().emplace_back();
    //                    subr.set_name(type_name);
    //                    collect_subrange_body(subr);
    //                   }
    //                else
    //                   {// <name> : <type>; { DE:"a typedef" }
    //                    plcb::TypeDef& tdef = lib.typedefs().emplace_back();
    //                    tdef.set_name(type_name);
    //                    tdef.type() = collect_type();
    //                    inherited::skip_blanks();
    //                    if( parser.got_directive_start() )
    //                       {
    //                        const plcb::Directive dir = parser.collect_directive();
    //                        if( dir.key()=="DE"sv )
    //                           {
    //                            tdef.set_descr( dir.value() );
    //                           }
    //                        else
    //                           {
    //                            throw parser.create_parse_error( fmt::format("Unexpected directive \"{}\" for typedef {}", dir.key(), tdef.name()) );
    //                           }
    //                       }
    //                   }
    //               }
    //           }
    //       }
    //   }

    //-----------------------------------------------------------------------
    void skip_endline()
       {
        inherited::skip_blanks();
        inherited::check_and_eat_endline();
       }
};



//---------------------------------------------------------------------------
// Parse pll file
void pll_parse(const std::string& file_path, const std::string_view buf, plcb::Library& lib, fnotify_t const& notify_issue)
{
    PllParser parser{buf};
    parser.set_on_notify_issue(notify_issue);
    parser.set_file_path( file_path );

    try{
        parser.check_heading_comment(lib);
        while( parser.has_codepoint() )
           {
            parser.collect_next(lib);
           }
       }
    catch( parse::error& )
       {
        throw;
       }
    catch( std::exception& e )
       {
        throw parser.create_parse_error(e.what());
       }
}


}//::::::::::::::::::::::::::::::::: ll :::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
static ut::suite<"pll_file_parser"> pll_file_parser_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("ll::PllParser::check_heading_comment()") = []
   {
    ll::PllParser parser
       {"    (*\n"
        "        name: test\n"
        "        descr: Libraries for strato machines (Macotec M-series machines)\n"
        "        version: 0.5.0\n"
        "        author: MG\n"
        "        dependencies: Common.pll, defvar.pll, iomap.pll, messages.pll\n"
        "    *)\n"sv};
    plcb::Library lib("test");
    parser.check_heading_comment(lib);
    ut::expect( ut::that % lib.descr()=="Libraries for strato machines (Macotec M-series machines)"sv );
    ut::expect( ut::that % lib.version()=="0.5.0"sv );
   };


ut::test("ll::PllParser::collect_directive()") = []
   {
    ll::PllParser parser{"{ DE:\"some string\" }  {bad:\" }"sv};

    const plcb::Directive dir = parser.collect_directive();
    ut::expect( ut::that % dir.key()=="DE"sv );
    ut::expect( ut::that % dir.value()=="some string"sv );

    parser.skip_any_space();
    ut::expect( ut::throws([&parser]{ [[maybe_unused]] auto bad = parser.collect_directive(); }) ) << "should complain for bad directive\n";
   };


ut::test("ll::PllParser::collect_type()") = []
   {
    ut::should("parse a scalar") = []()
       {
        ll::PllParser parser{" XYZ "sv};
        ut::expect( ut::that % plcb::to_string(parser.collect_type()) == "XYZ"sv );
       };

    ut::should("parse a length") = []()
       {
        ll::PllParser parser{" XYZ[ 123 ] "sv};
        ut::expect( ut::that % plcb::to_string(parser.collect_type()) == "XYZ[123]"sv );
       };

    ut::should("parse an array") = []()
       {
        ll::PllParser parser{" ARRAY[ 0..42 ] OF XYZ "sv};
        ut::expect( ut::that % plcb::to_string(parser.collect_type()) == "XYZ[0:42]"sv );
       };
   };


ut::test("ll::PllParser::collect_variable()") = []
   {
    ll::PllParser parser{"  VarName : Type := Val; { DE:\"Descr\" }\n"
                         "  Enabled AT %MB300.6000 : ARRAY[ 0..999 ] OF BOOL; { DE:\"an array of bools\" }\n"
                         "  bad :\n"
                         "  Title AT %MB700.0 : STRING[ 80 ]; {DE:\"a string\"}\n"sv};

    parser.skip_any_space();
    ut::expect( ut::that % plcb::to_string(parser.collect_variable()) == "VarName Type 'Descr' (=Val)"sv );

    parser.skip_any_space();
    ut::expect( ut::that % plcb::to_string(parser.collect_variable()) == "Enabled BOOL[0:999] 'an array of bools' <MB300.6000>"sv );

    parser.skip_any_space();
    ut::expect( ut::throws([&parser]{ [[maybe_unused]] auto bad = parser.collect_variable(); }) ) << "should complain for bad variable\n";
    [[maybe_unused]] const auto rest_of_bad_var = parser.get_rest_of_line();

    ut::expect( ut::that % plcb::to_string(parser.collect_variable()) == "Title STRING[80] 'a string' <MB700.0>"sv );
   };


ut::test("ll::PllParser::collect_global_vars()") = []
   {
    ll::PllParser parser
       {//"  VAR_GLOBAL\n"
        "    {G:\"System\"}\n"
        "        Cnc : fbCncM32; { DE:\"Cnc device\" }\n"
        "        vqDevErrors AT %MD500.5800 : ARRAY[ 0..99 ] OF DINT; { DE:\"Emg array\" }\n"
        "    {G:\"Cnc Shared Variables\"}\n"
        "        sysTimer AT %MD0.0 : UDINT;  { DE:\"System timer 1 KHz\"}\n"
        "        din      AT %IX100.0 : ARRAY[ 0..511 ] OF BOOL; {DE:\"Digital Inputs\"}\n"
        "        va1      AT %MB700.1 : STRING[ 80 ]; {DE:\"a string\"}\n"
        "    END_VAR\n"sv};
    std::vector<plcb::Variables_Group> gvars_groups;
    parser.collect_global_vars( gvars_groups );

    ut::expect( ut::fatal(gvars_groups.size() == 2u) );

    const plcb::Variables_Group& System_group = gvars_groups[0];
    ut::expect( ut::that % System_group.name() == "System"sv );
    const auto& gvars_System = System_group.variables();
    ut::expect( ut::fatal(gvars_System.size() == 2u) );
    ut::expect( ut::that % plcb::to_string(gvars_System.at(0)) == "Cnc fbCncM32 'Cnc device'"sv );
    ut::expect( ut::that % plcb::to_string(gvars_System.at(1)) == "vqDevErrors DINT[0:99] 'Emg array' <MD500.5800>"sv );

    const plcb::Variables_Group& Cnc_group = gvars_groups[1];
    ut::expect( ut::that % Cnc_group.name() == "Cnc Shared Variables"sv );
    const auto& gvars_Cnc = Cnc_group.variables();
    ut::expect( ut::fatal(gvars_Cnc.size() == 3u) );
    ut::expect( ut::that % plcb::to_string(gvars_Cnc.at(0)) == "sysTimer UDINT 'System timer 1 KHz' <MD0.0>"sv );
    ut::expect( ut::that % plcb::to_string(gvars_Cnc.at(1)) == "din BOOL[0:511] 'Digital Inputs' <IX100.0>"sv );
    ut::expect( ut::that % plcb::to_string(gvars_Cnc.at(2)) == "va1 STRING[80] 'a string' <MB700.1>"sv );
   };


ut::test("ll::PllParser::collect_global_constants()") = []
   {
    ll::PllParser parser
       {//"  VAR_GLOBAL CONSTANT\n"
        "        GlassDensity : DINT := 2600; { DE:\"a dint constant\" }\n"
        "        PI : LREAL := 3.14; { DE:\"[rad] π\" }\n"
        "    END_VAR\n"sv};
    std::vector<plcb::Variables_Group> gconsts_groups;
    parser.collect_global_constants( gconsts_groups );

    ut::expect( ut::fatal(gconsts_groups.size() == 1u) );
    const plcb::Variables_Group& gconsts_group = gconsts_groups.at(0);
    ut::expect( ut::that % gconsts_group.name() == ""sv );

    const auto& gconsts = gconsts_group.variables();
    ut::expect( ut::fatal(gconsts.size() == 2u) );
    ut::expect( ut::that % plcb::to_string(gconsts.at(0)) == "GlassDensity DINT 'a dint constant' (=2600)"sv );
    ut::expect( ut::that % plcb::to_string(gconsts.at(1)) == "PI LREAL '[rad] π' (=3.14)"sv );
   };


ut::test("ll::PllParser::collect_pou()") = []
   {
    ut::test("FUNCTION") = []
       {
        ll::PllParser parser
           {//"FUNCTION"
            " fnDist : LREAL\n"
            "\n"
            "	{ DE:\"Applies the Pythagorean theorem\" }\n"
            "\n"
            "	VAR_INPUT\n"
            "	a : LREAL; { DE:\"Measure a\" }\n"
            "	b : LREAL; { DE:\"Measure b\" }\n"
            "	END_VAR\n"
            "\n"
            "	{ CODE:ST }(* Square root of the sum of squares *)\n"
            "fnDist := SQRT( POW(a,2.0) + POW(b,2.0) );\n"
            "\n"
            "END_FUNCTION\n"sv};

        plcb::Pou fn;
        parser.collect_pou(fn, "FUNCTION"sv, "END_FUNCTION"sv, true);

        ut::expect( ut::that % fn.name() == "fnDist"sv );
        ut::expect( ut::that % fn.descr() == "Applies the Pythagorean theorem"sv );
        ut::expect( ut::that % fn.return_type() == "LREAL"sv );
        ut::expect( ut::that % fn.code_type() == "ST"sv );
        ut::expect( ut::that % fn.body() == "(* Square root of the sum of squares *)\n"
                                            "fnDist := SQRT( POW(a,2.0) + POW(b,2.0) );\n"
                                            "\n"sv );
        ut::expect( fn.inout_vars().empty() );
        ut::expect( ut::that % fn.input_vars().size() == 2u );
        ut::expect( ut::that % plcb::to_string(fn.input_vars().at(0)) == "a LREAL 'Measure a'"sv );
        ut::expect( ut::that % plcb::to_string(fn.input_vars().at(1)) == "b LREAL 'Measure b'"sv );
        ut::expect( fn.output_vars().empty() );
        ut::expect( fn.external_vars().empty() );
        ut::expect( fn.local_vars().empty() );
        ut::expect( fn.local_constants().empty() );
       };

    ut::test("FUNCTION_BLOCK") = []
       {
        ll::PllParser parser
           {//"FUNCTION_BLOCK"
            " fbSheetStack\n"
            "\n"
            "{ DE:\"A stack data container of glass sheets\" }\n"
            "\n"
            "	VAR_IN_OUT\n"
            "	Push : BOOL;\n"
            "	Pop : BOOL;\n"
            "	END_VAR\n"
            "\n"
            "	VAR_OUTPUT\n"
            "	Size : INT := 0; { DE:\"Current size\" }\n"
            "	END_VAR\n"
            "\n"
            "	VAR_EXTERNAL\n"
            "	dlog : fbLog; { DE:\"Logging facility\" }\n"
            "	END_VAR\n"
            "\n"
            "	VAR\n"
            "	i_Sheets : ARRAY[ 0..9 ] OF ST_SHEET; { DE:\"Data container\" }\n"
            "	END_VAR\n"
            "\n"
            "	VAR CONSTANT\n"
            "	MaxSize : INT := 10; { DE:\"array size\" }\n"
            "	END_VAR\n"
            "\n"
            "	{ CODE:ST }(* fbSheetStack *)\n"
            "\n"
            "IF Push THEN\n"
            "ELSIF Pop THEN\n"
            "END_IF;\n"
            "END_FUNCTION_BLOCK  \n"sv};

        plcb::Pou fb;
        parser.collect_pou(fb, "FUNCTION_BLOCK"sv, "END_FUNCTION_BLOCK"sv);

        ut::expect( ut::that % fb.name() == "fbSheetStack"sv );
        ut::expect( ut::that % fb.descr() == "A stack data container of glass sheets"sv );
        ut::expect( ut::that % fb.return_type() == ""sv );
        ut::expect( ut::that % fb.code_type() == "ST"sv );
        ut::expect( ut::that % fb.body() == "(* fbSheetStack *)\n"
                                            "\n"
                                            "IF Push THEN\n"
                                            "ELSIF Pop THEN\n"
                                            "END_IF;\n"sv );

        ut::expect( ut::that % fb.inout_vars().size() == 2u );
        ut::expect( ut::that % plcb::to_string(fb.inout_vars().at(0)) == "Push BOOL"sv );
        ut::expect( ut::that % plcb::to_string(fb.inout_vars().at(1)) == "Pop BOOL"sv );

        ut::expect( fb.input_vars().empty() );

        ut::expect( ut::that % fb.output_vars().size() == 1u );
        ut::expect( ut::that % plcb::to_string(fb.output_vars().at(0)) == "Size INT 'Current size' (=0)"sv );

        ut::expect( ut::that % fb.external_vars().size() == 1u );
        ut::expect( ut::that % plcb::to_string(fb.external_vars().at(0)) == "dlog fbLog 'Logging facility'"sv );

        ut::expect( ut::that % fb.local_vars().size() == 1u );
        ut::expect( ut::that % plcb::to_string(fb.local_vars().at(0)) == "i_Sheets ST_SHEET[0:9] 'Data container'"sv );

        ut::expect( ut::that % fb.local_constants().size() == 1u );
        ut::expect( ut::that % plcb::to_string(fb.local_constants().at(0)) == "MaxSize INT 'array size' (=10)"sv );
       };
   };


ut::test("ll::PllParser::collect_macro()") = []
   {
    ll::PllParser parser
       {//"MACRO"
        " MACRO_NAME\n"
        "{ DE:\"Macro description\" }\n"
        "    PAR_MACRO\n"
        "    WHAT1; { DE:\"Parameter 1\" }\n"
        "    WHAT2;\n"
        "    END_PAR\n"
        "    { CODE:ST }\n"
        "(* Macro body *)\n"
        "END_MACRO\n"sv};

    plcb::Macro macro;
    parser.collect_macro(macro);

    ut::expect( ut::that % macro.name() == "MACRO_NAME"sv );
    ut::expect( ut::that % macro.descr() == "Macro description"sv );
    ut::expect( ut::that % macro.code_type() == "ST"sv );
    ut::expect( ut::that % macro.body() == "\n"
                                           "(* Macro body *)\n"sv );
    ut::expect( ut::that % macro.parameters().size() == 2u );
    const plcb::Macro::Parameter& p1 = macro.parameters().at(0);
    ut::expect( ut::that % p1.name() == "WHAT1"sv );
    ut::expect( ut::that % p1.descr() == "Parameter 1"sv );
    const plcb::Macro::Parameter& p2 = macro.parameters().at(1);
    ut::expect( ut::that % p2.name() == "WHAT2"sv );
    ut::expect( ut::that % p2.descr() == ""sv );
   };


ut::test("ll::PllParser::collect_types()") = []
   {
    //const std::string_view expected =
    //    "\tstructname : STRUCT { DE:\"testing struct\" }\n"
    //    "\t\tmember1 : DINT; { DE:\"member1 descr\" }\n"
    //    "\t\tmember2 : STRING[ 80 ]; { DE:\"member2 descr\" }\n"
    //    "\t\tmember3 : ARRAY[ 0..11 ] OF INT; { DE:\"array member\" }\n"
    //    "\tEND_STRUCT;\n"sv;

    //"\ttypename : STRING[ 80 ]; { DE:\"testing typedef\" }\n"sv
    //"\tsubrangename : INT (1..12); { DE:\"testing subrange\" }\n"sv

    //const std::string_view expected =
    //    "\n\tenumname: (\n"
    //    "\t\t{ DE:\"testing enum\" }\n"
    //    "\t\telm1 := 1, { DE:\"elm1 descr\" }\n"
    //    "\t\telm2 := 42 { DE:\"elm2 descr\" }\n"
    //    "\t);\n"sv;
   };


ut::test("ll::pll_parse()") = []
   {
    ut::expect( true );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
