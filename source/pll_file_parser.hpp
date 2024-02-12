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
            collect_type(lib);
           }
        else if( inherited::eat_token("VAR_GLOBAL"sv) )
           {
            // Check for modifiers
            bool constants = false;
            while( not inherited::got_endline() and inherited::has_codepoint() )
               {
                inherited::skip_blanks();
                const std::string_view modifier = inherited::get_notspace();
                if( not modifier.empty() )
                   {
                    if( modifier=="CONSTANT"sv )
                       {
                        constants = true;
                       }
                    //else if( modifier=="RETAIN"sv )
                    else
                       {
                        throw create_parse_error( fmt::format("{} modifier not supported"sv, modifier) );
                       }
                   }
               }
            inherited::get_next(); // skip endline

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
            throw create_parse_error( fmt::format("Unexpected content: {}", str::escape(inherited::get_rest_of_line())) );
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
            throw create_parse_error( fmt::format("Missing ':' after directive {}", dir.key()) );
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
            throw create_parse_error( fmt::format("Unclosed directive {} after {}", dir.key(), dir.value()) );
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
            throw create_parse_error( fmt::format("Multiple names not supported in declaration of variable \"{}\"", var.name()) );
           }

        // [Location address]
        if( inherited::eat_token("AT"sv) )
           {// Specified a location address %<type><typevar><index>.<subindex>
            inherited::skip_blanks();
            if( not inherited::eat('%') )
               {
                throw create_parse_error( fmt::format("Expected '%' in address of variable \"{}\" address", var.name()) );
               }
            // Here expecting something like: MB300.6000
            var.address().set_type( inherited::curr_codepoint() ); // Typically M or Q
            inherited::get_next();
            var.address().set_typevar( inherited::curr_codepoint() );
            inherited::get_next();
            var.address().set_index( inherited::extract_index<decltype(var.address().index())>() );
            if( not inherited::eat('.') )
               {
                throw create_parse_error( fmt::format("Expected '.' in variable \"{}\" address", var.name()) );
               }
            var.address().set_subindex( inherited::extract_index<decltype(var.address().subindex())>() );
            inherited::skip_blanks();
           }

        // [Name/Type separator]
        if( not inherited::eat(':') )
           {
            throw create_parse_error( fmt::format("Expected ':' before variable \"{}\" type", var.name()) );
           }
        collect_variable_data(var);
        return var;
       }


    //-----------------------------------------------------------------------
    void collect_variable_data(plcb::Variable& var)
       {// ... ARRAY[ 0..999 ] OF BOOL; { DE:"descr" }
        // ... ARRAY[1..2, 1..2] OF DINT := [1, 2, 3, 4]; { DE:"multidimensional array" }
        // ... STRING[ 80 ]; { DE:"descr" }
        // ... DINT; { DE:"descr" }
        // [Array data]
        inherited::skip_blanks();
        if( inherited::eat_token("ARRAY"sv) )
           {// Specifying an array ex. ARRAY[ 0..999 ] OF BOOL;
            // Get array size
            inherited::skip_blanks();
            if( not inherited::eat('[') )
               {
                throw create_parse_error( fmt::format("Expected '[' in array variable \"{}\"", var.name()) );
               }

            inherited::skip_blanks();
            const auto idx_start = inherited::extract_index<decltype(var.array_startidx())>();
            inherited::skip_blanks();
            if( not inherited::eat(".."sv) )
               {
                throw create_parse_error( fmt::format("Expected \"..\" in array index of variable \"{}\"", var.name()) );
               }
            inherited::skip_blanks();
            const auto idx_last = inherited::extract_index<decltype(var.array_lastidx())>();
            inherited::skip_blanks();
            if( inherited::got(',') )
               {
                throw create_parse_error( fmt::format("Multidimensional arrays not yet supported in variable \"{}\"", var.name()) );
               }
            // TODO: Collect array dimensions (multidimensional: dim0, dim1, dim2)
            if( not inherited::eat(']') )
               {
                throw create_parse_error( fmt::format("Expected ']' in array variable \"{}\"", var.name()) );
               }
            inherited::skip_blanks();
            if( not inherited::eat_token("OF"sv) )
               {
                throw create_parse_error( fmt::format("Expected \"OF\" in array variable \"{}\"", var.name()) );
               }
            var.set_array_range(idx_start, idx_last);
            inherited::skip_blanks();
           }

        // [Type]
        var.set_type( inherited::get_identifier() );
        inherited::skip_blanks();
        if( inherited::eat('[') )
           {// There's a length
            inherited::skip_blanks();
            var.set_length( extract_index<decltype(var.length())>() );
            inherited::skip_blanks();
            if( not inherited::eat(']') )
               {
                throw create_parse_error( fmt::format("Expected ']' in variable length \"{}\"", var.name()) );
               }
            inherited::skip_blanks();
           }

        // [Value]
        if( inherited::eat(':') )
           {
            if( not inherited::eat('=') )
               {
                throw create_parse_error( fmt::format("Unexpected colon in variable \"{}\" type", var.name()) );
               }
            inherited::skip_blanks();
            if( inherited::got('[') )
               {
                throw create_parse_error( fmt::format("Array initialization not yet supported in variable \"{}\"", var.name()) );
               }

            var.set_value( str::trim_right(inherited::get_until(ascii::is<';'>, ascii::is_any_of<':','=','<','>','\"','\n'>)) );
           }

        if( !inherited::eat(';') )
           {
            throw create_parse_error( fmt::format("Truncated definition of variable \"{}\"", var.name()) );
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
                throw create_parse_error( fmt::format("Unexpected directive \"{}\" in variable \"{}\" declaration", dir.key(), var.name()) );
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
            throw create_parse_error("Multiple global variables declaration");
           }

        const auto start = inherited::save_context();
        while( true )
           {
            inherited::skip_blanks();
            if( not inherited::has_codepoint() )
               {
                throw create_parse_error("VAR_GLOBAL not closed by END_VAR", start.line);
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
                    throw create_parse_error( fmt::format("Unexpected directive \"{}\" in global vars", dir.key()) );
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
                    throw create_parse_error( fmt::format("Value not specified for \"{}\"", var.name()) );
                   }
               }
           }
       }


    //-----------------------------------------------------------------------
    void collect_pou([[maybe_unused]] plcb::Pou& pou, [[maybe_unused]] const std::string_view start_tag, [[maybe_unused]] const std::string_view end_tag, [[maybe_unused]] const bool needs_ret_type =false)
       {
        //POU NAME : RETURN_VALUE
        //{ DE:"Description" }
        //    VAR_YYY
        //    ...
        //    END_VAR
        //    { CODE:ST }
        //(* Body *)
        //END_POU

        //struct local_t final
        //   {
        //    ll::PllParser& parser;
        //    explicit local_t() constexpr( ll::PllParser& p)
        //      : parser(p)
        //       {}
        //
        //    //-------------------------------------------------------------------
        //    void collect_var_block(std::vector<plcb::Variable>& vars, const bool value_needed =false)
        //       {
        //        struct local final
        //           {
        //            [[nodiscard]] static bool contains(const std::vector<plcb::Variable>& vars, const std::string_view var_name) noexcept
        //               {
        //                //return std::ranges::any_of(vars, [](const plcb::Variable& var) noexcept { return var.name()==var_name; });
        //                for(const plcb::Variable& var : vars) if(var.name()==var_name) return true;
        //                return false;
        //               }
        //
        //            static plcb::Variable& add_variable(std::vector<plcb::Variable>& vars, plcb::Variable&& var)
        //               {
        //                if( contains(vars, var.name()) )
        //                   {
        //                    throw std::runtime_error( fmt::format("Duplicate variable \"{}\"", var.name()) );
        //                   }
        //                vars.push_back( std::move(var) );
        //                return vars.back();
        //               }
        //           };
        //
        //        const auto start = parser->save_context();
        //        while( true )
        //           {
        //            parser->skip_blanks();
        //            if( not parser->has_codepoint() )
        //               {
        //                throw create_parse_error("VAR block not closed by END_VAR", start.line);
        //               }
        //            else if( parser->got_endline() )
        //               {// Skip empty lines
        //                parser->get_next();
        //               }
        //            else if( eat_block_comment_start() )
        //               {
        //                skip_block_comment();
        //               }
        //            else if( parser->eat_token("END_VAR"sv) )
        //               {
        //                break;
        //               }
        //            else
        //               {// Expected a variable entry
        //                const plcb::Variable& var = local::add_variable(vars, collect_variable());
        //
        //                if( value_needed and !var.has_value() )
        //                   {
        //                    throw create_parse_error( fmt::format("Value not specified for \"{}\"", var.name()) );
        //                   }
        //               }
        //           }
        //       }
        //
        //
        //    //-------------------------------------------------------------------
        //    void collect_pou_header(plcb::Pou& pou, const std::string_view start_tag, const std::string_view end_tag)
        //       {
        //        while( true )
        //           {
        //            parser->skip_blanks();
        //            if( not parser->has_codepoint() )
        //               {
        //                throw create_parse_error( fmt::format("{} not closed by {}", start_tag, end_tag), start.line );
        //               }
        //            else if( parser->got_endline() )
        //               {// Sono ammesse righe vuote
        //                parser->get_next();
        //                continue;
        //               }
        //            else
        //               {
        //                if( got_directive_start() )
        //                   {
        //                    const plcb::Directive dir = collect_directive();
        //                    if( dir.key()=="DE"sv )
        //                       {// Is a description
        //                        if( not pou.descr().empty() )
        //                           {
        //                            throw create_parse_error( fmt::format("{} has already a description: {}", start_tag, pou.descr()) );
        //                           }
        //                        pou.set_descr( dir.value() );
        //                       }
        //                    else if( dir.key()=="CODE"sv )
        //                       {// Header finished
        //                        pou.set_code_type( dir.value() );
        //                        break;
        //                       }
        //                    else
        //                       {
        //                        throw create_parse_error( fmt::format("Unexpected directive \"{}\" in {} of {}", dir.key(), start_tag, pou.name()) );
        //                       }
        //                   }
        //                else if( parser->eat_token("VAR_INPUT"sv) )
        //                   {
        //                    parser->check_and_eat_endline(); // ("VAR_INPUT of {}"sv, pou.name());
        //                    collect_var_block( pou.input_vars() );
        //                   }
        //                else if( parser->eat_token("VAR_OUTPUT"sv) )
        //                   {
        //                    parser->check_and_eat_endline(); // ("VAR_OUTPUT of {}"sv, pou.name());
        //                    collect_var_block( pou.output_vars() );
        //                   }
        //                else if( parser->eat_token("VAR_IN_OUT"sv) )
        //                   {
        //                    parser->check_and_eat_endline(); // ("VAR_IN_OUT of {}"sv, pou.name());
        //                    collect_var_block( pou.inout_vars() );
        //                   }
        //                else if( parser->eat_token("VAR_EXTERNAL"sv) )
        //                   {
        //                    parser->check_and_eat_endline(); // ("VAR_EXTERNAL of {}"sv, pou.name());
        //                    collect_var_block( pou.external_vars() );
        //                   }
        //                else if( parser->eat_token("VAR"sv) )
        //                   {
        //                    // Check if there's some additional attributes
        //                    parser->skip_blanks();
        //                    if( parser->eat_token("CONSTANT"sv) )
        //                       {
        //                        parser->check_and_eat_endline(); // ("VAR CONSTANT of {}"sv, pou.name());
        //                        collect_var_block( pou.local_constants(), true );
        //                       }
        //                    //else if( parser->eat_token("RETAIN"sv) )
        //                    //   {
        //                    //    throw create_parse_error("RETAIN variables not supported");
        //                    //   }
        //                    else if( parser->got_endline() )
        //                       {
        //                        parser->get_next();
        //                        collect_var_block( pou.local_vars() );
        //                       }
        //                    else
        //                       {
        //                        throw create_parse_error( fmt::format("Unexpected content after {} of {}: {}", start_tag, pou.name(), str::escape(parser->get_rest_of_line())) );
        //                       }
        //                   }
        //                else if( parser->eat_token(end_tag) )
        //                   {
        //                    throw create_parse_error( fmt::format("Truncated {} of {}", start_tag, pou.name()) );
        //                    break;
        //                   }
        //                else
        //                   {
        //                    throw create_parse_error( fmt::format("Unexpected content in {} of {} header: {}", start_tag, pou.name(), str::escape(parser->get_rest_of_line())) );
        //                   }
        //               }
        //           }
        //       }
        //   } local(parser);
        //
        //// Get name
        //const auto start = inherited::save_context();
        //inherited::skip_blanks();
        //pou.set_name( inherited::get_identifier() );
        //if( pou.name().empty() )
        //   {
        //    throw create_parse_error( fmt::format("No name found for {}", start_tag) );
        //   }
        //
        //// Get possible return type
        //// Dopo il nome dovrebbe esserci la dichiarazione del parametro ritornato
        //inherited::skip_blanks();
        //if( i<siz and inherited::eat(':') )
        //   {// Got a return type!
        //    inherited::skip_blanks();
        //    pou.set_return_type( get_alphabetic() );
        //    if( pou.return_type().empty() )
        //       {
        //        throw create_parse_error( fmt::format("Empty return type in {} {}", start_tag, pou.name()) );
        //       }
        //    if( not needs_ret_type )
        //       {
        //        throw create_parse_error( fmt::format("Return type specified in {} {}", start_tag, pou.name()) );
        //       }
        //    inherited::check_and_eat_endline();
        //   }
        //else
        //   {// No return type
        //    if( needs_ret_type )
        //       {
        //        throw create_parse_error( fmt::format("Return type not specified in {} {}", start_tag, pou.name()) );
        //       }
        //   }
        //
        //// Collect description and variables
        //collect_pou_header(pou, start_tag, end_tag, start);
        //
        //// Collect the code body
        //if( pou.code_type().empty() )
        //   {
        //    throw create_parse_error( fmt::format("CODE not found in {} {}", start_tag, pou.name()) );
        //   }
        ////else if( pou.code_type()!="ST"sv )
        ////   {
        ////    notify_issue( fmt::format("Code type: {} for {} {}", dir.value(), start_tag, pou.name())) ;
        ////   }
        //pou.set_body( get_until_newline_token(end_tag, start) );
       }


    //-----------------------------------------------------------------------
    //[[nodiscard]] plcb::Macro::Parameter collect_macro_parameter()
    //   {//   WHAT; {DE:"Parameter description"}
    //    plcb::Macro::Parameter par;
    //    inherited::skip_blanks();
    //    par.set_name( inherited::get_identifier() );
    //    inherited::skip_blanks();
    //    if( not inherited::eat(';') )
    //       {
    //        throw create_parse_error("Missing ';' after macro parameter");
    //       }
    //    inherited::skip_blanks();
    //    if( got_directive_start() )
    //       {
    //        const plcb::Directive dir = collect_directive();
    //        if( dir.key()=="DE"sv )
    //           {
    //            par.set_descr( dir.value() );
    //           }
    //        else
    //           {
    //            throw create_parse_error( fmt::format("Unexpected directive \"{}\" in macro parameter", dir.key()) );
    //           }
    //       }
    //
    //    // Expecting a line end now
    //    inherited::check_and_eat_endline(); // ("macro parameter {}"sv, par.name());
    //
    //    return par;
    //   }


    //-----------------------------------------------------------------------
    //void collect_macro_parameters(std::vector<plcb::Macro::Parameter>& pars)
    //   {
    //    while( i<siz )
    //       {
    //        inherited::skip_blanks();
    //        if( inherited::eat_token("END_PAR"sv) )
    //           {
    //            break;
    //           }
    //        else if( inherited::got_endline() )
    //           {// Sono ammesse righe vuote
    //            inherited::get_next();
    //            continue;
    //           }
    //        else if( eat_block_comment_start() )
    //           {// Ammetto righe di commento?
    //            skip_block_comment();
    //           }
    //        else if( inherited::eat_token("END_MACRO"sv) )
    //           {
    //            throw create_parse_error("Truncated params in macro");
    //            break;
    //           }
    //        else
    //           {
    //            pars.push_back( collect_macro_parameter() );
    //            //DBGLOG("    Macro param {}: {}\n", pars.back().name(), pars.back().descr())
    //           }
    //       }
    //   }


    //-----------------------------------------------------------------------
    //void collect_macro_header(plcb::Macro& macro)
    //   {
    //    while( i<siz )
    //       {
    //        inherited::skip_blanks();
    //        if( not inherited::has_codepoint() )
    //           {
    //            throw create_parse_error("MACRO not closed by END_MACRO");
    //           }
    //        else if( inherited::got_endline() )
    //           {// Sono ammesse righe vuote
    //            inherited::get_next();
    //            continue;
    //           }
    //        else
    //           {
    //            if( got_directive_start() )
    //               {
    //                const plcb::Directive dir = collect_directive();
    //                if( dir.key()=="DE"sv )
    //                   {// Is a description
    //                    if( not macro.descr().empty() )
    //                       {
    //                        throw create_parse_error( fmt::format("Macro {} has already a description: {}", macro.name(), macro.descr()) );
    //                       }
    //                    macro.set_descr( dir.value() );
    //                    //DBGLOG("    Macro description: {}\n", dir.value())
    //                   }
    //                else if( dir.key()=="CODE"sv )
    //                   {// Header finished
    //                    macro.set_code_type( dir.value() );
    //                    break;
    //                   }
    //                else
    //                   {
    //                    throw create_parse_error( fmt::format("Unexpected directive \"{}\" in macro {} header", dir.key(), macro.name()) );
    //                   }
    //               }
    //            else if( inherited::eat_token("PAR_MACRO"sv) )
    //               {
    //                if( not macro.parameters().empty() )
    //                   {
    //                    throw create_parse_error("Multiple groups of macro parameters");
    //                   }
    //                inherited::check_and_eat_endline(); // ("PAR_MACRO of {}"sv, macro.name());
    //                collect_macro_parameters( macro.parameters() );
    //               }
    //            else if( inherited::eat_token("END_MACRO"sv) )
    //               {
    //                throw create_parse_error("Truncated macro");
    //                break;
    //               }
    //            else
    //               {
    //                throw create_parse_error( fmt::format("Unexpected content in header of macro {}: {}", macro.name(), str::escape(inherited::get_rest_of_line())) );
    //               }
    //           }
    //       }
    //   }


    //-----------------------------------------------------------------------
    void collect_macro([[maybe_unused]] plcb::Macro& macro)
       {
        //MACRO IS_MSG
        //{ DE:"Macro description" }
        //    PAR_MACRO
        //    WHAT; { DE:"Parameter description" }
        //    END_PAR
        //    { CODE:ST }
        //(* Macro body *)
        //END_MACRO

        // Get name
        //inherited::skip_blanks();
        //macro.set_name( inherited::get_identifier() );
        //if( macro.name().empty() )
        //   {
        //    throw create_parse_error("No name found for MACRO");
        //   }
        //
        //collect_macro_header(macro);
        //
        //// Collect the code body
        //if( macro.code_type().empty() )
        //   {
        //    throw create_parse_error( fmt::format("CODE not found in MACRO {}", macro.name()) );
        //   }
        ////else if( macro.code_type()!="ST"sv )
        ////   {
        ////    notify_issue( fmt::format("Code type: {} for MACRO {}", dir.value(), macro.name()) );
        ////   }
        //macro.set_body( get_until_newline_token("END_MACRO"sv) );
       }



    //-----------------------------------------------------------------------
    //void collect_rest_of_struct(plcb::Struct& strct)
    //   {// <name> : STRUCT { DE:"struct descr" }
    //    //    x : DINT; { DE:"member descr" }
    //    //    y : DINT; { DE:"member descr" }
    //    //END_STRUCT;
    //    // Name already collected, "STRUCT" already skipped
    //    // Possible description
    //    inherited::skip_blanks();
    //    if( got_directive_start() )
    //       {
    //        const plcb::Directive dir = collect_directive();
    //        if( dir.key()=="DE"sv )
    //           {
    //            strct.set_descr( dir.value() );
    //           }
    //        else
    //           {
    //            throw create_parse_error( fmt::format("Unexpected directive \"{}\" in struct \"{}\"", dir.key(), strct.name()) );
    //           }
    //       }
    //
    //    while( i<siz )
    //       {
    //        inherited::skip_any_space();
    //        if( inherited::eat("END_STRUCT;"sv) )
    //           {
    //            break;
    //           }
    //        else if( inherited::got_endline() )
    //           {// Nella lista membri ammetto righe vuote
    //            inherited::get_next();
    //            continue;
    //           }
    //        else if( eat_block_comment_start() )
    //           {// Nella lista membri ammetto righe di commento
    //            skip_block_comment();
    //            continue;
    //           }
    //
    //        strct.add_member( collect_variable() );
    //        // Some checks
    //        //if( strct.members().back().has_value() )
    //        //   {
    //        //    throw create_parse_error( fmt::format("Struct member \"{}\" cannot have a value ({})", strct.members().back().name(), strct.members().back().value()) );
    //        //   }
    //        if( strct.members().back().has_address() )
    //           {
    //            throw create_parse_error( fmt::format("Struct member \"{}\" cannot have an address", strct.members().back().name()) );
    //           }
    //       }
    //
    //    // Expecting a line end now
    //    inherited::check_and_eat_endline(); // ("struct {}"sv, strct.name());
    //    //DBGLOG("    [*] Collected struct \"{}\", {} members\n", strct.name(), strct.members().size())
    //   }

    //-----------------------------------------------------------------------
    //[[nodiscard]] bool collect_enum_element(plcb::Enum::Element& elem)
    //   {// VAL1 := 0, { DE:"elem descr" }
    //    // [Name]
    //    inherited::skip_any_space();
    //    elem.set_name( inherited::get_identifier() );
    //
    //    // [Value]
    //    inherited::skip_blanks();
    //    if( not inherited::eat(":="sv) )
    //       {
    //        throw create_parse_error( fmt::format("Value not found in enum element \"{}\"", elem.name()) );
    //       }
    //    inherited::skip_blanks();
    //    elem.set_value( collect_numeric_value() );
    //    inherited::skip_blanks();
    //
    //    // Qui c'è una virgola se ci sono altri valori successivi
    //    const bool has_next = i<siz and inherited::eat(',');
    //
    //    // [Description]
    //    inherited::skip_blanks();
    //    if( got_directive_start() )
    //       {
    //        const plcb::Directive dir = collect_directive();
    //        if( dir.key()=="DE"sv )
    //           {
    //            elem.set_descr( dir.value() );
    //           }
    //        else
    //           {
    //            throw create_parse_error( fmt::format("Unexpected directive \"{}\" in enum element \"{}\"", dir.key(), elem.name()) );
    //           }
    //       }
    //
    //    // Expecting a line end now
    //    inherited::check_and_eat_endline(); // ("enum element {}"sv, elem.name());
    //    //DBGLOG("    [*] Collected enum element: name=\"{}\" value=\"{}\" descr=\"{}\"\n", elem.name(), elem.value(), elem.descr())
    //    return has_next;
    //   }


    //-----------------------------------------------------------------------
    //void collect_rest_of_enum(plcb::Enum& en)
    //   {// <name>: ( { DE:"enum descr" }
    //    //     VAL1 := 0, { DE:"elem descr" }
    //    //     VAL2 := -1 { DE:"elem desc" }
    //    // );
    //    // Name already collected, ": (" already skipped
    //    // Possible description
    //    inherited::skip_blanks();
    //    // Possibile interruzione di linea
    //    if( inherited::got_endline() )
    //       {
    //        inherited::get_next();
    //        inherited::skip_blanks();
    //       }
    //    if( got_directive_start() )
    //       {
    //        const plcb::Directive dir = collect_directive();
    //        if( dir.key()=="DE"sv )
    //           {
    //            en.set_descr( dir.value() );
    //           }
    //        else
    //           {
    //            throw create_parse_error( fmt::format("Unexpected directive \"{}\" in enum \"{}\"", dir.key(), en.name()) );
    //           }
    //       }
    //
    //    // Elements
    //    bool has_next;
    //    do {
    //        plcb::Enum::Element elem;
    //        has_next = collect_enum_element(elem);
    //        en.elements().push_back(elem);
    //       }
    //    while( has_next );
    //
    //    // End expected
    //    inherited::skip_blanks();
    //    if( not inherited::eat(");"sv) )
    //       {
    //        throw create_parse_error( fmt::format("Expected termination \");\" after enum \"{}\"", en.name()) );
    //       }
    //
    //    // Expecting a line end now
    //    inherited::check_and_eat_endline(); // ("enum {}"sv, en.name());
    //    //DBGLOG("    [*] Collected enum \"{}\", {} elements\n", en.name(), en.elements().size())
    //   }


    //-----------------------------------------------------------------------
    //void collect_rest_of_subrange(plcb::Subrange& subr)
    //   {// <name> : DINT (5..23); { DE:"descr" }
    //    // Name already collected, ':' already skipped
    //
    //    // [Type]
    //    inherited::skip_blanks();
    //    subr.set_type(inherited::get_identifier());
    //
    //    // [Min and Max]
    //    inherited::skip_blanks();
    //    if( not inherited::eat('(') )
    //       {
    //        throw create_parse_error( fmt::format("Expected \"(min..max)\" in subrange \"{}\"", subr.name()) );
    //       }
    //    inherited::skip_blanks();
    //    const auto min_val = extract_integer(); // extract_double(); // Nah, Floating point numbers seems not supported
    //    inherited::skip_blanks();
    //    if( not inherited::eat(".."sv) )
    //       {
    //        throw create_parse_error( fmt::format("Expected \"..\" in subrange \"{}\"", subr.name()) );
    //       }
    //    inherited::skip_blanks();
    //    const auto max_val = extract_integer();
    //    inherited::skip_blanks();
    //    if( not inherited::eat(')') )
    //       {
    //        throw create_parse_error( fmt::format("Expected ')' in subrange \"{}\"", subr.name()) );
    //       }
    //
    //    inherited::skip_blanks();
    //    if( not inherited::eat(';') )
    //       {
    //        throw create_parse_error( fmt::format("Expected ';' in subrange \"{}\"", subr.name()) );
    //       }
    //    subr.set_range(min_val, max_val);
    //
    //    // [Description]
    //    inherited::skip_blanks();
    //    if( got_directive_start() )
    //       {
    //        const plcb::Directive dir = collect_directive();
    //        if(dir.key()=="DE"sv)
    //           {
    //            subr.set_descr(dir.value());
    //           }
    //        else
    //           {
    //            throw create_parse_error( fmt::format("Unexpected directive \"{}\" in subrange \"{}\" declaration", dir.key(), subr.name()) );
    //           }
    //       }
    //
    //    // Expecting a line end now
    //    inherited::check_and_eat_endline(); // ("subrange {}"sv, subr.name());
    //    //DBGLOG("    [*] Collected subrange: name=\"{}\" type=\"{}\" min=\"{}\" max=\"{}\" descr=\"{}\"\n", subr.name(), subr.type(), subr.min(), subr.max(), subr.descr())
    //   }

    //-----------------------------------------------------------------------
    void collect_type([[maybe_unused]] plcb::Library& lib)
       {
        //    TYPE
        //    str_name : STRUCT { DE:"descr" } member : DINT; { DE:"member descr" } ... END_STRUCT;
        //    typ_name : STRING[ 80 ]; { DE:"descr" }
        //    en_name: ( { DE:"descr" } VAL1 := 0, { DE:"elem descr" } ... );
        //    subr_name : DINT (30..100);
        //    END_TYPE
        //while( i<siz )
        //   {
        //    inherited::skip_blanks();
        //    if( not inherited::has_codepoint() )
        //       {
        //        throw create_parse_error("TYPE not closed by END_TYPE");
        //       }
        //    else if( inherited::got_endline() )
        //       {
        //        inherited::get_next();
        //        continue;
        //       }
        //    else if( inherited::eat_token("END_TYPE"sv) )
        //       {
        //        break;
        //       }
        //    else
        //       {// Expected a type name token here
        //        const std::string_view type_name = inherited::get_identifier();
        //        //DBGLOG("Found typename {} in line {}\n", type_name, line)
        //        if( type_name.empty() )
        //           {
        //            throw create_parse_error("type name not found");
        //            inherited::skip_line();
        //           }
        //        else
        //           {
        //            // Expected a colon after the name
        //            inherited::skip_blanks();
        //            if( not inherited::eat(':') )
        //               {
        //                throw create_parse_error( fmt::format("Missing ':' after type name \"{}\"", type_name) );
        //               }
        //            // Check what it is (struct, typedef, enum, subrange)
        //            inherited::skip_blanks();
        //            if( not inherited::has_codepoint() )
        //               {
        //                continue;
        //               }
        //            else if( inherited::eat_token("STRUCT"sv) )
        //               {// <name> : STRUCT
        //                plcb::Struct strct;
        //                strct.set_name(type_name);
        //                collect_rest_of_struct( strct );
        //                //DBGLOG("    struct {}, {} members\n", strct.name(), strct.members().size())
        //                lib.structs().push_back( std::move(strct) );
        //               }
        //            else if( inherited::eat('(') )
        //               {// <name>: ( { DE:"an enum" }
        //                plcb::Enum en;
        //                en.set_name(type_name);
        //                collect_rest_of_enum( en );
        //                //DBGLOG("    enum {}, {} constants\n", en.name(), en.elements().size())
        //                lib.enums().push_back( std::move(en) );
        //               }
        //            else
        //               {// Could be a typedef or a subrange
        //                // I have to peek to see which one
        //                std::size_t j = i;
        //                while(j<siz and buf[j]!=';' and buf[j]!='(' and buf[j]!='{' and buf[j]!='\n' ) ++j;
        //                if( j<siz and buf[j]=='(' )
        //                   {// <name> : <type> (<min>..<max>); { DE:"a subrange" }
        //                    plcb::Subrange subr;
        //                    subr.set_name(type_name);
        //                    collect_rest_of_subrange(subr);
        //                    //DBGLOG("    subrange {}: {}\n", var.name(), var.type())
        //                    lib.subranges().push_back( std::move(subr) );
        //                   }
        //                else
        //                   {// <name> : <type>; { DE:"a typedef" }
        //                    plcb::Variable var;
        //                    var.set_name(type_name);
        //                    collect_variable_data( var );
        //                    //DBGLOG("    typedef {}: {}\n", var.name(), var.type())
        //                    lib.typedefs().push_back( plcb::TypeDef(var) );
        //                   }
        //               }
        //           }
        //       }
        //   }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr std::string_view get_until_newline_token(const std::string_view tok, const inherited::context_t& start)
       {
        do {
            if( inherited::got_endline() )
               {
                inherited::get_next();
                inherited::skip_blanks();
                const std::size_t candidate_end = inherited::curr_offset();
                if( inherited::eat_token(tok) )
                   {
                    return inherited::get_view_between(start.offset, candidate_end);
                   }
               }
           }
        while( inherited::get_next() );
        inherited::restore_context( start ); // Strong guarantee
        throw create_parse_error(fmt::format("Unclosed content (\"{}\" not found)",tok), start.line);
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

ut::test("get_until_newline_token()") = []
   {
    ll::PllParser parser {  "start\n"
                            "123\n"
                            "endnot\n"
                            "  end start2\n"
                            "not end\n"
                            "end"sv };


    ut::expect( ut::throws([&parser]
       {
        const auto start = parser.save_context();
        [[maybe_unused]] auto n = parser.get_until_newline_token("xxx"sv, start);
       }) ) << "should complain for unclosed content\n";

    const auto start1 = parser.save_context();
    ut::expect( ut::that % parser.get_until_newline_token("end"sv, start1) == "start\n123\nendnot\n  "sv );
    ut::expect( ut::that % parser.curr_line()==4u );
    const auto start2 = parser.save_context();
    ut::expect( ut::that % parser.get_until_newline_token("end"sv, start2) == " start2\nnot end\n"sv );
    ut::expect( ut::that % parser.curr_line()==6u );
    ut::expect( not parser.has_codepoint() );
   };

ut::test("ll::PllParser::check_heading_comment()") = []
   {
    ll::PllParser parser{
        "    (*\n"
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


ut::test("ll::PllParser::collect_variable()") = []
   {
    ll::PllParser parser{"  VarName : Type := Val; { DE:\"Descr\" }\n"
                         "  Enabled AT %MB300.6000 : ARRAY[ 0..999 ] OF BOOL; { DE:\"an array of bools\" }\n"
                         "  bad :\n"
                         "  Title AT %MB700.0 : STRING[ 80 ]; {DE:\"a string\"}\n"sv};

    parser.skip_any_space();
    ut::expect( ut::that % plcb::to_string(parser.collect_variable()) == "VarName Type 'Descr' (=Val)"sv );

    parser.skip_any_space();
    ut::expect( ut::that % plcb::to_string(parser.collect_variable()) == "Enabled BOOL[0...999] 'an array of bools' <MB300.6000>"sv );

    parser.skip_any_space();
    ut::expect( ut::throws([&parser]{ [[maybe_unused]] auto bad = parser.collect_variable(); }) ) << "should complain for bad variable\n";
    [[maybe_unused]] const auto rest_of_bad_var = parser.get_rest_of_line();

    ut::expect( ut::that % plcb::to_string(parser.collect_variable()) == "Title STRING[80] 'a string' <MB700.0>"sv );
   };


ut::test("ll::PllParser::collect_global_vars()") = []
   {
    // Note: opened by "VAR_GLOBAL\n"
    ll::PllParser parser{"    {G:\"System\"}\n"
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
    ut::expect( ut::that % plcb::to_string(gvars_System[0]) == "Cnc fbCncM32 'Cnc device'"sv );
    ut::expect( ut::that % plcb::to_string(gvars_System[1]) == "vqDevErrors DINT[0...99] 'Emg array' <MD500.5800>"sv );

    const plcb::Variables_Group& Cnc_group = gvars_groups[1];
    ut::expect( ut::that % Cnc_group.name() == "Cnc Shared Variables"sv );
    const auto& gvars_Cnc = Cnc_group.variables();
    ut::expect( ut::fatal(gvars_Cnc.size() == 3u) );
    ut::expect( ut::that % plcb::to_string(gvars_Cnc[0]) == "sysTimer UDINT 'System timer 1 KHz' <MD0.0>"sv );
    ut::expect( ut::that % plcb::to_string(gvars_Cnc[1]) == "din BOOL[0...511] 'Digital Inputs' <IX100.0>"sv );
    ut::expect( ut::that % plcb::to_string(gvars_Cnc[2]) == "va1 STRING[80] 'a string' <MB700.1>"sv );
   };


ut::test("ll::PllParser::collect_global_constants()") = []
   {
    // Note: opened by "VAR_GLOBAL CONSTANT\n"
    ll::PllParser parser{"        GlassDensity : DINT := 2600; { DE:\"a dint constant\" }\n"
                         "        PI : LREAL := 3.14; { DE:\"[rad] π\" }\n"
                         "    END_VAR\n"sv};
    std::vector<plcb::Variables_Group> gconsts_groups;
    parser.collect_global_constants( gconsts_groups );

    ut::expect( ut::fatal(gconsts_groups.size() == 1u) );
    const plcb::Variables_Group& gconsts_group = gconsts_groups[0];
    ut::expect( ut::that % gconsts_group.name() == ""sv );

    const auto& gconsts = gconsts_group.variables();
    ut::expect( ut::fatal(gconsts.size() == 2u) );
    ut::expect( ut::that % plcb::to_string(gconsts[0]) == "GlassDensity DINT 'a dint constant' (=2600)"sv );
    ut::expect( ut::that % plcb::to_string(gconsts[1]) == "PI LREAL '[rad] π' (=3.14)"sv );
   };


ut::test("ll::pll_parse()") = []
   {
    ut::expect( true );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
