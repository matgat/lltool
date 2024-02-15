#pragma once
//  ---------------------------------------------
//  Parses a LogicLab 'pll' file
//  ---------------------------------------------
//  #include "pll_file_parser.hpp" // ll::pll_parse()
//  ---------------------------------------------
#include <cassert>
#include <optional>

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
    [[nodiscard]] std::optional<std::string_view> collect_possible_description_and_endline()
       {// { DE:"some string" }
        std::optional<std::string_view> descr;

        inherited::skip_blanks();
        if( got_directive_start() )
           {
            const plcb::Directive dir = collect_directive();
            if( dir.key()=="DE"sv )
               {
                descr = dir.value();
               }
            else
               {
                throw inherited::create_parse_error( fmt::format("Unexpected directive \"{}\"", dir.key()) );
               }
            inherited::skip_blanks();
           }

        inherited::check_and_eat_endline();

        return descr;
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
                throw inherited::create_parse_error( fmt::format("Missing '%' in address of variable \"{}\" address", var.name()) );
               }
            // Here expecting something like: MB300.6000
            var.address().set_zone( inherited::curr_codepoint() ); // Typically M or Q
            inherited::get_next();
            var.address().set_typevar( inherited::curr_codepoint() );
            inherited::get_next();
            var.address().set_index( inherited::extract_index<decltype(var.address().index())>() );
            if( not inherited::eat('.') )
               {
                throw inherited::create_parse_error( fmt::format("Missing '.' in variable \"{}\" address", var.name()) );
               }
            var.address().set_subindex( inherited::extract_index<decltype(var.address().subindex())>() );
            inherited::skip_blanks();
           }

        // [Name/Type separator]
        if( not inherited::eat(':') )
           {
            throw inherited::create_parse_error( fmt::format("Missing ':' before variable \"{}\" type", var.name()) );
           }
        collect_variable_data(var);
        return var;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] plcb::Type collect_type()
       {// ARRAY[ 0..999 ] OF BOOL
        // ARRAY[0..2, 0..2] OF DINT
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
                throw inherited::create_parse_error("Missing '[' in array declaration");
               }

            inherited::skip_blanks();
            const auto idx_start = inherited::extract_index<decltype(type.array_startidx())>();
            inherited::skip_blanks();
            if( not inherited::eat(".."sv) )
               {
                throw inherited::create_parse_error("Missing \"..\" in array range declaration");
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
                throw inherited::create_parse_error("Missing ']' in array declaration");
               }
            inherited::skip_blanks();
            if( not inherited::eat_token("OF"sv) )
               {
                throw inherited::create_parse_error("Missing \"OF\" in array declaration");
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
                throw inherited::create_parse_error("Missing ']' in type length");
               }
           }

        return type;
       }


    //-----------------------------------------------------------------------
    void collect_variable_data(plcb::Variable& var)
       {// ... ARRAY[ 0..999 ] OF BOOL; { DE:"descr" }
        // ... ARRAY[0..1, 0..1] OF INT := [1, 2, 3, 4]; { DE:"multidimensional array" }
        // ... STRING[ 80 ]; { DE:"descr" }
        // ... DINT; { DE:"descr" }
        var.type() = collect_type();

        // [Value]
        inherited::skip_blanks();
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
            throw inherited::create_parse_error( fmt::format("Missing ';' after variable \"{}\" definition", var.name()) );
           }

        // [Description]
        if( const auto descr = collect_possible_description_and_endline() )
           {
            var.set_descr( descr.value() );
           }
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
                            throw std::runtime_error{ fmt::format("Duplicate variable \"{}\"", var.name()) };
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
                        /*const*/ plcb::Variable& var = local::add_variable(vars, parser.collect_variable());

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
                       }
                    else if( parser.got_directive_start() )
                       {
                        const plcb::Directive dir = parser.collect_directive();
                        if( dir.key()=="DE"sv )
                           {// Is a description
                            if( pou.has_descr() )
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

                if( const auto descr = parser.collect_possible_description_and_endline() )
                   {
                    par.set_descr( descr.value() );
                   }

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
                       }
                    else if( parser.got_directive_start() )
                       {
                        const plcb::Directive dir = parser.collect_directive();
                        if( dir.key()=="DE"sv )
                           {// Is a description
                            if( macro.has_descr() )
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
    void collect_types(plcb::Library& lib)
       {
        //    TYPE
        //    struct_name : STRUCT { DE:"descr" } member : DINT; { DE:"member descr" } ... END_STRUCT;
        //    typedef_name : STRING[ 80 ]; { DE:"descr" }
        //    enum_name: ( { DE:"descr" } VAL1 := 0, { DE:"elem descr" } ... );
        //    subrange_name : DINT (30..100);
        //    END_TYPE

        struct local final //////////////////////////////////////////////////
           {
            //---------------------------------------------------------------
            static [[nodiscard]] void collect_struct_member(ll::PllParser& parser, plcb::Struct::Member& memb)
               {// member : DINT; { DE:"member descr" }
                // [Name]
                parser.skip_any_space();
                memb.set_name( parser.get_identifier() );

                // Expected a colon after the name
                parser.skip_blanks();
                if( not parser.eat(':') )
                   {
                    throw parser.create_parse_error( fmt::format("Missing ':' after member name \"{}\"", memb.name()) );
                   }

                // [Type]
                parser.skip_blanks();
                memb.type() = parser.collect_type();
                parser.skip_blanks();
                if( !parser.eat(';') )
                   {
                    throw parser.create_parse_error( fmt::format("Missing ';' after member \"{}\" definition", memb.name()) );
                   }

                // [Description]
                if( const auto descr = parser.collect_possible_description_and_endline() )
                   {
                    memb.set_descr( descr.value() );
                   }
               }

            //---------------------------------------------------------------
            static void collect_struct_body(ll::PllParser& parser, plcb::Struct& strct)
               {// <name> : STRUCT { DE:"struct descr" }
                //    x : DINT; { DE:"member descr" }
                //    y : DINT; { DE:"member descr" }
                //END_STRUCT;

                // Name already collected, "STRUCT" already skipped
                if( const auto descr = parser.collect_possible_description_and_endline() )
                   {
                    strct.set_descr( descr.value() );
                   }

                const auto start = parser.save_context();
                while( true )
                   {
                    parser.skip_any_space();
                    if( not parser.has_codepoint() )
                       {
                        parser.restore_context( start ); // Strong guarantee
                        throw parser.create_parse_error("STRUCT not closed by END_STRUCT", start.line);
                       }
                    else if( parser.got_endline() )
                       {
                        parser.get_next();
                       }
                    else if( parser.eat_block_comment_start() )
                       {
                        parser.skip_block_comment();
                       }
                    else if( parser.eat_token("END_STRUCT"sv) )
                       {
                        parser.skip_blanks();
                        if( !parser.eat(';') )
                           {
                            throw parser.create_parse_error("Missing ';' after END_STRUCT");
                           }
                        break;
                       }
                    else
                       {// Expected a struct member here
                        plcb::Struct::Member& memb = strct.members().emplace_back();
                        collect_struct_member(parser, memb);
                        if( strct.is_last_member_name_not_unique() )
                           {
                            throw parser.create_parse_error( fmt::format("Duplicate struct member \"{}\"", memb.name()) );
                           }
                       }
                   }

                parser.skip_line();
               }

            //---------------------------------------------------------------
            static [[nodiscard]] bool collect_enum_element(ll::PllParser& parser, plcb::Enum::Element& elem)
               {// VAL1 := 0, { DE:"elem descr" }
                // [Name]
                parser.skip_any_space();
                elem.set_name( parser.get_identifier() );

                // [Value]
                parser.skip_blanks();
                if( not parser.eat(":="sv) )
                   {
                    throw parser.create_parse_error( fmt::format("Value not found in enum element \"{}\"", elem.name()) );
                   }
                parser.skip_blanks();
                elem.set_value( parser.get_float() );

                // Qui c'è una virgola se ci sono altri valori successivi
                parser.skip_blanks();
                const bool has_next = parser.eat(',');

                // [Description]
                if( const auto descr = parser.collect_possible_description_and_endline() )
                   {
                    elem.set_descr( descr.value() );
                   }

                return has_next;
               }

            //---------------------------------------------------------------
            static void collect_enum_body(ll::PllParser& parser, plcb::Enum& enm)
               {// <name>: ( { DE:"enum descr" }
                //     VAL1 := 0, { DE:"elem descr" }
                //     VAL2 := -1 { DE:"elem desc" }
                // );
                // Name already collected, '(' already skipped
                if( const auto descr = parser.collect_possible_description_and_endline() )
                   {
                    enm.set_descr( descr.value() );
                   }

                // [Elements]
                while( true )
                   {
                    plcb::Enum::Element& elem = enm.elements().emplace_back();
                    if( not collect_enum_element(parser, elem) )
                       {// No next item
                        break;
                       }
                   }

                parser.skip_blanks();
                if( not parser.eat(");"sv) )
                   {
                    throw parser.create_parse_error( fmt::format("Expected termination \");\" after enum \"{}\"", enm.name()) );
                   }

                parser.skip_line();
               }

            //---------------------------------------------------------------
            static void collect_typedef_body(ll::PllParser& parser, plcb::TypeDef& tdef)
               {// <name> : <type>; { DE:"a typedef" }
                // Name and type already collected, ';' already skipped
                if( const auto descr = parser.collect_possible_description_and_endline() )
                   {
                    tdef.set_descr( descr.value() );
                   }
               }

            //---------------------------------------------------------------
            static void collect_subrange_body(ll::PllParser& parser, plcb::Subrange& subrng)
               {// <name> : DINT (5..23); { DE:"descr" }
                // Name and type already collected, '(' already skipped

                // [Min and Max]
                parser.skip_blanks();
                const auto min_val = parser.extract_integer<decltype(subrng.min_value())>();
                parser.skip_blanks();
                if( not parser.eat(".."sv) )
                   {
                    throw parser.create_parse_error( fmt::format("Missing \"..\" in subrange \"{}\"", subrng.name()) );
                   }
                parser.skip_blanks();
                const auto max_val = parser.extract_integer<decltype(subrng.max_value())>();
                parser.skip_blanks();
                if( not parser.eat(')') )
                   {
                    throw parser.create_parse_error( fmt::format("Missing closing ')' in subrange \"{}\" definition", subrng.name()) );
                   }
                subrng.set_range(min_val, max_val);

                parser.skip_blanks();
                if( not parser.eat(';') )
                   {
                    throw parser.create_parse_error( fmt::format("Missing ';' after subrange \"{}\" definition", subrng.name()) );
                   }

                // [Description]
                if( const auto descr = parser.collect_possible_description_and_endline() )
                   {
                    subrng.set_descr( descr.value() );
                   }
               }

           }; ///////////////////////////////////////////////////////////////

        const auto start = save_context();
        while( true )
           {
            inherited::skip_any_space();
            if( not inherited::has_codepoint() )
               {
                restore_context( start ); // Strong guarantee
                throw inherited::create_parse_error("TYPE not closed by END_TYPE", start.line );
               }
            else if( inherited::eat_token("END_TYPE"sv) )
               {
                break;
               }
            else
               {// Expected a type name token here
                const std::string_view type_name = inherited::get_identifier();
                if( type_name.empty() )
                   {
                    throw inherited::create_parse_error( fmt::format("Unexpected content in TYPE block: {}", str::escape(inherited::get_rest_of_line())) );
                   }

                // Expected a colon after the name
                inherited::skip_blanks();
                if( not inherited::eat(':') )
                   {
                    throw inherited::create_parse_error( fmt::format("Missing ':' after type name \"{}\"", type_name) );
                   }

                // Check what it is (struct, typedef, enum, subrange)
                inherited::skip_blanks();
                if( inherited::eat_token("STRUCT"sv) )
                   {// <name> : STRUCT ...
                    plcb::Struct& strct = lib.structs().emplace_back();
                    strct.set_name(type_name);
                    local::collect_struct_body(*this, strct);
                   }
                else if( inherited::eat('(') )
                   {// <name> : ( ...
                    plcb::Enum& enm = lib.enums().emplace_back();
                    enm.set_name(type_name);
                    local::collect_enum_body(*this, enm);
                   }
                else
                   {// Could be a typedef or a subrange:
                    // <name> : <type>; { DE:"a typedef" }
                    // <name> : <type> (<min>..<max>); { DE:"a subrange" }
                    // Either way I expect a type
                    const plcb::Type type = collect_type();

                    inherited::skip_blanks();
                    if( inherited::eat(';') )
                       {// <name> : <type>; { DE:"a typedef" }
                        plcb::TypeDef& tdef = lib.typedefs().emplace_back();
                        tdef.set_name(type_name);
                        tdef.type() = type;
                        local::collect_typedef_body(*this, tdef);
                       }
                    else if( inherited::eat('(') )
                       {// <name> : <type> (<min>..<max>); { DE:"a subrange" }
                        plcb::Subrange& subrng = lib.subranges().emplace_back();
                        subrng.set_name(type_name);
                        subrng.set_type_name(type);
                        local::collect_subrange_body(*this, subrng);
                       }
                    else
                       {
                        throw inherited::create_parse_error("Invalid type definition (not struct, enum, typedef or subrange)");
                       }
                   }
               }
           }
       }

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

    try{
        parser.collect_global_vars( gvars_groups );
       }
    catch( std::exception& e )
       {
        ut::log << "\033[95m" "Exception: " "\033[31m" << e.what() << "\033[0m" "(line " << parser.curr_line() << ':' << parser.curr_offset() << ")\n";
        throw;
       }

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

    try{
        parser.collect_global_constants( gconsts_groups );
       }
    catch( std::exception& e )
       {
        ut::log << "\033[95m" "Exception: " "\033[31m" << e.what() << "\033[0m" "(line " << parser.curr_line() << ':' << parser.curr_offset() << ")\n";
        throw;
       }

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
        try{
            parser.collect_pou(fn, "FUNCTION"sv, "END_FUNCTION"sv, true);
           }
        catch( std::exception& e )
           {
            ut::log << "\033[95m" "Exception: " "\033[31m" << e.what() << "\033[0m" "(line " << parser.curr_line() << ':' << parser.curr_offset() << ")\n";
            throw;
           }

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
        try{
            parser.collect_pou(fb, "FUNCTION_BLOCK"sv, "END_FUNCTION_BLOCK"sv);
           }
        catch( std::exception& e )
           {
            ut::log << "\033[95m" "Exception: " "\033[31m" << e.what() << "\033[0m" "(line " << parser.curr_line() << ':' << parser.curr_offset() << ")\n";
            throw;
           }

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
    try{
        parser.collect_macro(macro);
       }
    catch( std::exception& e )
       {
        ut::log << "\033[95m" "Exception: " "\033[31m" << e.what() << "\033[0m" "(line " << parser.curr_line() << ':' << parser.curr_offset() << ")\n";
        throw;
       }

    ut::expect( ut::that % macro.name() == "MACRO_NAME"sv );
    ut::expect( ut::that % macro.descr() == "Macro description"sv );
    ut::expect( ut::that % macro.code_type() == "ST"sv );
    ut::expect( ut::that % macro.body() == "\n"
                                           "(* Macro body *)\n"sv );
    ut::expect( ut::that % macro.parameters().size() == 2u );
    {   const plcb::Macro::Parameter& par = macro.parameters().at(0);
        ut::expect( ut::that % par.name() == "WHAT1"sv );
        ut::expect( ut::that % par.descr() == "Parameter 1"sv );
    }
    {   const plcb::Macro::Parameter& par = macro.parameters().at(1);
        ut::expect( ut::that % par.name() == "WHAT2"sv );
        ut::expect( ut::that % par.descr() == ""sv );
    }
   };


ut::test("ll::PllParser::collect_types()") = []
   {
    ll::PllParser parser
       {//"TYPE\n"
        "\n"
        "    ST_SHEET : STRUCT { DE:\"Sheet structure\" }\n"
        "        Width : DINT; { DE:\"X dimension [um]\" }\n"
        "        Height : DINT; { DE:\"Y dimension [um]\" }\n"
        "        Ids : ARRAY[ 0..99 ] OF INT; { DE:\"Identificative numbers\" }\n"
        "    END_STRUCT;\n"
        "\n"
        "    EN_CMD: (    { DE:\"Commands\" }\n"
        "        CMD_STOP := 0,    { DE:\"Abort operation\" }\n"
        "        CMD_MOVE := 2,    { DE:\"Start movement\" }\n"
        "        CMD_TAKE := 5    { DE:\"Deliver piece\" }\n"
        "    );\n"
        "\n"
        "    VASTR : STRING[ 80 ]; { DE:\"Sipro string (va)\" }\n"
        "\n"
        "    INCLINATION : INT (15..85); { DE:\"Tilting angle [deg]\" }\n"
        "\n"
        "END_TYPE\n"sv };

    plcb::Library lib("types");
    ut::expect( lib.structs().empty() and
                lib.enums().empty() and
                lib.typedefs().empty() and
                lib.subranges().empty() );

    try{
        parser.collect_types(lib);
       }
    catch( std::exception& e )
       {
        ut::log << "\033[95m" "Exception: " "\033[31m" << e.what() << "\033[0m" "(line " << parser.curr_line() << ':' << parser.curr_offset() << ")\n";
        throw;
       }

    ut::expect( ut::that % lib.structs().size() == 1u );
    const plcb::Struct& strct = lib.structs().at(0);
    ut::expect( ut::that % strct.name() == "ST_SHEET"sv );
    ut::expect( ut::that % strct.descr() == "Sheet structure"sv );
    ut::expect( ut::that % strct.members().size() == 3u );
    {   const plcb::Struct::Member& memb = strct.members().at(0);
        ut::expect( ut::that % memb.name() == "Width"sv );
        ut::expect( ut::that % plcb::to_string(memb.type()) == "DINT"sv );
        ut::expect( ut::that % memb.descr() == "X dimension [um]"sv );
    }
    {   const plcb::Struct::Member& memb = strct.members().at(1);
        ut::expect( ut::that % memb.name() == "Height"sv );
        ut::expect( ut::that % plcb::to_string(memb.type()) == "DINT"sv );
        ut::expect( ut::that % memb.descr() == "Y dimension [um]"sv );
    }
    {   const plcb::Struct::Member& memb = strct.members().at(2);
        ut::expect( ut::that % memb.name() == "Ids"sv );
        ut::expect( ut::that % plcb::to_string(memb.type()) == "INT[0:99]"sv );
        ut::expect( ut::that % memb.descr() == "Identificative numbers"sv );
    }

    ut::expect( ut::that % lib.enums().size() == 1u );
    const plcb::Enum& enm = lib.enums().at(0);
    ut::expect( ut::that % enm.name() == "EN_CMD"sv );
    ut::expect( ut::that % enm.descr() == "Commands"sv );
    ut::expect( ut::that % enm.elements().size() == 3u );
    {   const plcb::Enum::Element& elem = enm.elements().at(0);
        ut::expect( ut::that % elem.name() == "CMD_STOP"sv );
        ut::expect( ut::that % elem.value() == "0"sv );
        ut::expect( ut::that % elem.descr() == "Abort operation"sv );
    }
    {   const plcb::Enum::Element& elem = enm.elements().at(1);
        ut::expect( ut::that % elem.name() == "CMD_MOVE"sv );
        ut::expect( ut::that % elem.value() == "2"sv );
        ut::expect( ut::that % elem.descr() == "Start movement"sv );
    }
    {   const plcb::Enum::Element& elem = enm.elements().at(2);
        ut::expect( ut::that % elem.name() == "CMD_TAKE"sv );
        ut::expect( ut::that % elem.value() == "5"sv );
        ut::expect( ut::that % elem.descr() == "Deliver piece"sv );
    }

    ut::expect( ut::that % lib.typedefs().size() == 1u );
    const plcb::TypeDef& tdef = lib.typedefs().at(0);
    ut::expect( ut::that % tdef.name() == "VASTR"sv );
    ut::expect( ut::that % plcb::to_string(tdef.type()) == "STRING[80]"sv );
    ut::expect( ut::that % tdef.descr() == "Sipro string (va)"sv );

    ut::expect( ut::that % lib.subranges().size() == 1u );
    const plcb::Subrange& subrng = lib.subranges().at(0);
    ut::expect( ut::that % subrng.name() == "INCLINATION"sv );
    ut::expect( ut::that % subrng.type_name() == "INT"sv );
    ut::expect( ut::that % subrng.min_value() == 15 );
    ut::expect( ut::that % subrng.max_value() == 85 );
    ut::expect( ut::that % subrng.descr() == "Tilting angle [deg]"sv );
   };


ut::test("ll::pll_parse()") = []
   {
    const std::string_view buf =
        "(*\n"
        "	descr: test lib\n"
        "	version: 1.2.3\n"
        "	author: llconv pll::write()\n"
        "	date: 2024-02-15 10:05:33\n"
        "\n"
        "	global-variables: 1770\n"
        "	global-constants: 925\n"
        "*)\n"
        "\n"
        "\n"
        "	VAR_GLOBAL\n"
        "	{G:\"Header_Variables\"}\n"
        "	vbHeartBeat AT %MB300.2 : BOOL; { DE:\"Battito di vita ogni secondo\" }\n"
        "	vdExpDate AT %ML600.255 : LREAL; { DE:\"Expiration date of scheduled maintenance\" }\n"
        "	END_VAR\n"
        "\n"
        "	VAR_GLOBAL CONSTANT\n"
        "	{G:\"Header_Constants\"}\n"
        "	SW_VER : LREAL := 34.4; { DE:\"Versione del software\" }\n"
        "	END_VAR\n"
        "\n"
        "	(*** PROGRAMS ***)\n"
        "PROGRAM Init\n"
        "\n"
        "	{ DE:\"Startup operations\" }\n"
        "\n"
        "	{ CODE:ST }(*    Machine initialization\n"
        "      ----------------------------------------------\n"
        "\n"
        "      OVERVIEW\n"
        "      ----------------------------------------------\n"
        "      Executed once on PLC boot\n"
        "*)\n"
        "InitTask();\n"
        "\n"
        "END_PROGRAM\n"
        "\n"
        "	(*** FUNCTIONS ***)\n"
        "FUNCTION ExecPlcFun : DINT\n"
        "\n"
        "    VAR_INPUT\n"
        "        FunCode : DINT;\n"
        "        Par1    : INT;\n"
        "        Par2    : INT;\n"
        "    END_VAR\n"
        "\n"
        "    { CODE:EMBEDDED }\n"
        "END_FUNCTION\n"
        "\n"
        "FUNCTION fnOppSign : BOOL\n"
        "\n"
        "	VAR_INPUT\n"
        "	Value1 : DINT; { DE:\"Value1\" }\n"
        "	Value2 : DINT; { DE:\"Value2\" }\n"
        "	END_VAR\n"
        "\n"
        "	{ CODE:ST }(*   Tells if two values have opposite sign\n"
        "      ----------------------------------------------\n"
        "\n"
        "      DETAILS\n"
        "      ----------------------------------------------\n"
        "      Zero is neutral\n"
        "\n"
        "      EXAMPLE\n"
        "      ----------------------------------------------\n"
        "      IF fnOppSign(vq[1], Xr.CurrPos) THEN ...\n"
        "*)\n"
        "fnOppSign := (Value1>0 AND Value2<0) OR (Value1<0 AND Value2>0);\n"
        "\n"
        "END_FUNCTION\n"
        "\n"
        "\n"
        "	(*** FUNCTION BLOCKS ***)\n"
        "\n"
        "FUNCTION_BLOCK fbEdges\n"
        "\n"
        "{ DE:\"Detect signal edges\" }\n"
        "\n"
        "	VAR_INPUT\n"
        "	in : BOOL; { DE:\"Boolean input\" }\n"
        "	END_VAR\n"
        "\n"
        "	VAR_OUTPUT\n"
        "	rise : BOOL; { DE:\"Input is rising in this PLC cycle\" }\n"
        "	fall : BOOL; { DE:\"Input is falling in this PLC cycle\" }\n"
        "	END_VAR\n"
        "\n"
        "	VAR\n"
        "	in_prev : BOOL; { DE:\"Input previous value\" }\n"
        "	END_VAR\n"
        "\n"
        "	{ CODE:ST }(*    fbEdges (M-series)\n"
        "      ----------------------------------------------\n"
        "\n"
        "      OVERVIEW\n"
        "      ----------------------------------------------\n"
        "      Rileva cambiamento di stato di un booleano\n"
        "\n"
        "      SIGNALS\n"
        "      ----------------------------------------------\n"
        "        in ____‾‾‾‾‾‾‾‾‾‾‾‾___|____‾‾‾‾‾‾‾‾_____\n"
        "      rise ___|_______________|___|_____________\n"
        "      fall _______________|___|___________|_____\n"
        "\n"
        "      EXAMPLE OF USAGE\n"
        "      ----------------------------------------------\n"
        "      edgSignal : fbEdges; { DE:\"Signal changes\" }\n"
        "\n"
        "      edgSignal( in:=Signal );\n"
        "      IF edgSignal.rise THEN\n"
        "          MESSAGE('Signal is just risen');\n"
        "      ELSIF edgSignal.fall THEN\n"
        "          MESSAGE('Signal is just fallen');\n"
        "      ELSE\n"
        "          MESSAGE('Signal stationary');\n"
        "      END_IF;\n"
        "*)\n"
        "rise := in AND NOT in_prev;\n"
        "fall := NOT in AND in_prev;\n"
        "(* changed := rise OR fall; *)\n"
        "in_prev := in;\n"
        "\n"
        "END_FUNCTION_BLOCK\n"
        "\n"
        "	(*** MACROS ***)\n"
        "\n"
        "MACRO ASSERT\n"
        "\n"
        "	PAR_MACRO\n"
        "	COND; { DE:\"Condition to be validated (BOOL)\" }\n"
        "	MSG; { DE:\"Condition not met message (STRING)\" }\n"
        "	END_PAR\n"
        "\n"
        "	{ CODE:ST }\n"
        "(* IF NOT (COND) THEN Log(txt:=MSG, lvl:=1); END_IF; *)\n"
        "END_MACRO\n"
        "\n"
        "	(*** STRUCTURES ***)\n"
        "\n"
        "TYPE\n"
        "\n"
        "	ST_RECT : STRUCT { DE:\"Descrittore rettangolo\" }\n"
        "		Left : DINT; { DE:\"Ascissa minore [um]\" }\n"
        "		Right : DINT; { DE:\"Ascissa maggiore [um]\" }\n"
        "		Bottom : DINT; { DE:\"Ordinata minore [um]\" }\n"
        "		Top : DINT; { DE:\"Ordinata maggiore [um]\" }\n"
        "	END_STRUCT;\n"
        "\n"
        "	ST_SHEET : STRUCT { DE:\"Descrittore sottolastra\" }\n"
        "		Width : DINT; { DE:\"Larghezza [um]\" }\n"
        "		Height : DINT; { DE:\"Altezza [um]\" }\n"
        "		Id : INT; { DE:\"pos:piece 0:scrap neg:cuts\" }\n"
        "	END_STRUCT;\n"
        "\n"
        "END_TYPE\n"
        "\n"
        "	(*** ENUMS ***)\n"
        "\n"
        "TYPE\n"
        "\n"
        "	EN_AXIS_MV_CMD: (	{ DE:\"Enum for MoveTo axis movements local service\" }\n"
        "		MV_ERROR := -1,	{ DE:\"Mov Svc: errore\" }\n"
        "		MV_DONE := 0,	{ DE:\"Mov Svc: movim completato\" }\n"
        "		MV_WAITSTOP := 1,	{ DE:\"Mov Svc: attendi stop asse\" }\n"
        "		MV_FREEMOVE := 2,	{ DE:\"Mov Svc: prepara mov libero\" }\n"
        "		MV_TOOLMOVE := 3,	{ DE:\"Mov Svc: prepara mov con utensile\" }\n"
        "		MV_LOCKMOVE := 4,	{ DE:\"Mov Svc: prepara agganciamento\" }\n"
        "		MV_LOCKED := 5,		{ DE:\"Mov Svc: stato agganciato\" }\n"
        "		MV_SUSPENDED := 6,	{ DE:\"Mov Svc: movimento sospeso\" }\n"
        "		MV_WAITMOVE := 7	{ DE:\"Mov Svc: attendi movim\" }\n"
        "	);\n"
        "\n"
        "	EN_OUTZONE_CMD: (	{ DE:\"Enum for fbOutZone.NewPiece commands\" }\n"
        "		OZ_NONE := 0,	{ DE:\"No command\" }\n"
        "		OZ_NOTIFY := 1,	{ DE:\"Just notify that a piece is going to be delivered\" }\n"
        "		OZ_UPDATE := 2,	{ DE:\"A piece released somewhere, just update the busy zone\" }\n"
        "		OZ_TAKE := 3	{ DE:\"Take care of the delivered piece\" }\n"
        "	);\n"
        "\n"
        "END_TYPE\n"
        "\n"
        "	(*** TYPEDEFS ***)\n"
        "TYPE\n"
        "	VASTR : STRING[ 80 ]; { DE:\"Sipro string (va)\" }\n"
        "END_TYPE\n"sv;

    plcb::Library lib("test");

    try{
        struct issues_t final { int num=0; void operator()(std::string&&) noexcept {++num;}; } issues;
        ll::pll_parse(lib.name(), buf, lib, std::ref(issues));
        ut::expect( ut::that % issues.num==0 ) << "no issues expected\n";
       }
    catch( parse::error& e )
       {
        ut::log << "\033[95m" "Exception: " "\033[31m" << e.what() << "\033[0m" "(line " << e.line() << ")\n";
        throw;
       }

    ut::expect( ut::that % lib.version() == "1.2.3"sv );
    ut::expect( ut::that % lib.descr() == "test lib"sv );

    ut::expect( ut::that % lib.global_constants().vars_count() == 1u );
    ut::expect( ut::that % lib.global_retainvars().vars_count() == 0u );
    ut::expect( ut::that % lib.global_variables().vars_count() == 2u );
    ut::expect( ut::that % lib.functions().size() == 2u );
    ut::expect( ut::that % lib.function_blocks().size() == 1u );
    ut::expect( ut::that % lib.programs().size() == 1u );
    ut::expect( ut::that % lib.macros().size() == 1u );
    ut::expect( ut::that % lib.structs().size() == 2u );
    ut::expect( ut::that % lib.typedefs().size() == 1u );
    ut::expect( ut::that % lib.enums().size() == 2u );
    ut::expect( ut::that % lib.subranges().size() == 0u );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
