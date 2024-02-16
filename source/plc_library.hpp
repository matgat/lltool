#pragma once
//  ---------------------------------------------
//  IEC 61131-3 stuff and descriptors of PLC elements
//  ---------------------------------------------
//  #include "plc_library.hpp" // plcb::Library
//  ---------------------------------------------
#include <concepts> // std::convertible_to<>
#include <stdexcept> // std::runtime_error
#include <cstdint> // std::uint16_t
#include <ranges> // std::views::take
#include <algorithm> // std::ranges::sort, std::ranges::find
#include <string>
#include <string_view>
#include <array>
#include <vector>

#include <fmt/core.h> // fmt::format

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace plc //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
// Tell if a string is a recognized IEC numerical type
//[[nodiscard]] bool is_iec_num_type(const std::string_view sv)
//   {
//    static constexpr std::array iec_num_types =
//       {
//        "BOOL"sv  // [1] BOOLean [FALSE|TRUE]
//       ,"SINT"sv  // [1] Short INTeger [-128 … 127]
//       ,"INT"sv   // [2] INTeger [-32768 … +32767]
//       ,"DINT"sv  // [4] Double INTeger [-2147483648 … 2147483647]
//       ,"LINT"sv  // [8] Long INTeger [-2^63 … 2^63-1]
//       ,"USINT"sv // [1] Unsigned Short INTeger [0 … 255]
//       ,"UINT"sv  // [2] Unsigned INTeger [0 … 65535]
//       ,"UDINT"sv // [4] Unsigned Double INTeger [0 … 4294967295]
//       ,"ULINT"sv // [8] Unsigned Long INTeger [0 … 2^64-1]
//       ,"REAL"sv  // [4] REAL number [±10^38]
//       ,"LREAL"sv // [8] Long REAL number [±10^308]
//       ,"BYTE"sv  // [1] 1 byte
//       ,"WORD"sv  // [2] 2 bytes
//       ,"DWORD"sv // [4] 4 bytes
//       ,"LWORD"sv // [8] 8 bytes
//       };
//    return std::ranges::contains(iec_num_types, sv);
//   }



/////////////////////////////////////////////////////////////////////////////
// Variable address ex. MB700.320
//                      M     B        700  .  320
//                      ↑     ↑        ↑       ↑
//                      zone  typevar  index   subindex
class Address final
{
 private:
    char m_Zone = '\0'; // 'M', 'Q', ...
    char m_TypeVar = '\0'; // 'B', ...
    std::uint16_t m_Index = 0;
    std::uint16_t m_SubIndex = 0;

 public:
    Address() noexcept = default;
    Address(const char zn,
            const char tp,
            const std::uint16_t idx,
            const std::uint16_t sub) noexcept
      : m_Zone(zn)
      , m_TypeVar(tp)
      , m_Index(idx)
      , m_SubIndex(sub)
       {}

    [[nodiscard]] bool is_empty() const noexcept { return m_Zone=='\0'; }

    [[nodiscard]] char zone() const noexcept { return m_Zone; }
    void set_zone(const char zn) noexcept { m_Zone = zn; }

    [[nodiscard]] char typevar() const noexcept { return m_TypeVar; }
    void set_typevar(const char tp) noexcept { m_TypeVar = tp; }

    [[nodiscard]] std::uint16_t index() const noexcept { return m_Index; }
    void set_index(const std::uint16_t idx) noexcept { m_Index = idx; }

    [[nodiscard]] std::uint16_t subindex() const noexcept { return m_SubIndex; }
    void set_subindex(const std::uint16_t idx) noexcept { m_SubIndex = idx; }
};



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace buf //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{// Content that refers to an external buffer


//---------------------------------------------------------------------------
template<typename T>
concept ClassWithName = requires(T o) { { o.name() } -> std::convertible_to<std::string_view>; };
template<ClassWithName T>
[[nodiscard]] bool less_by_name(const T& a, const T& b) noexcept { return a.name()<b.name(); }



/////////////////////////////////////////////////////////////////////////////
// A type
class Type final
{
 private:
    std::string_view m_Name;
    std::size_t m_Length = 0u;
    std::size_t m_ArrayFirstIdx = 0u;
    std::size_t m_ArrayDim = 0u;

 public:
    [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
    void set_name(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error{"Empty type name"};
           }
        m_Name = sv;
       }

    [[nodiscard]] bool has_length() const noexcept { return m_Length>0; }
    [[nodiscard]] std::size_t length() const noexcept { return m_Length; }
    void set_length(const std::size_t len)
       {
        if( len<=1u )
           {
            throw std::runtime_error{ fmt::format("Invalid type length: {}", len) };
           }
        m_Length = len;
       }

    [[nodiscard]] bool is_array() const noexcept { return m_ArrayDim>0; }
    [[nodiscard]] std::size_t array_startidx() const noexcept { return m_ArrayFirstIdx; }
    [[nodiscard]] std::size_t array_dim() const noexcept { return m_ArrayDim; }
    [[nodiscard]] std::size_t array_lastidx() const noexcept { return m_ArrayFirstIdx + m_ArrayDim - 1u; }

    void set_array_range(const std::size_t idx_start, const std::size_t idx_last)
       {
        if( idx_start>=idx_last )
           {
            throw std::runtime_error{ fmt::format("Invalid array range {}..{}", idx_start, idx_last) };
           }
        m_ArrayFirstIdx = idx_start;
        m_ArrayDim = idx_last - idx_start + 1u;
       }
};



/////////////////////////////////////////////////////////////////////////////
// A variable declaration
class Variable final
{
 private:
    std::string_view m_Name;
    Type m_Type;
    Address m_Address;
    std::string_view m_Value;
    std::string_view m_Descr;

 public:
    [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
    void set_name(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error{"Empty variable name"};
           }
        m_Name = sv;
       }

    [[nodiscard]] Type& type() noexcept { return m_Type; }
    [[nodiscard]] const Type& type() const noexcept { return m_Type; }

    [[nodiscard]] bool has_address() const noexcept { return not m_Address.is_empty(); }
    [[nodiscard]] Address& address() noexcept { return m_Address; }
    [[nodiscard]] const Address& address() const noexcept { return m_Address; }

    [[nodiscard]] bool has_value() const noexcept { return not m_Value.empty(); }
    [[nodiscard]] std::string_view value() const noexcept { return m_Value; }
    void set_value(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error{"Setting a variable initialization value as empty"};
           }
        m_Value = sv;
       }

    [[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
    [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
    void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }
};



/////////////////////////////////////////////////////////////////////////////
// A named group of variables
class Variables_Group final
{
 private:
    std::string_view m_Name;
    std::vector<Variable> m_Variables;

 public:
    [[nodiscard]] bool has_name() const noexcept { return not m_Name.empty(); }
    [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
    void set_name(const std::string_view sv) noexcept { m_Name = sv; }

    [[nodiscard]] bool is_empty() const noexcept { return m_Variables.empty(); }

    [[nodiscard]] const std::vector<Variable>& variables() const noexcept { return m_Variables; }
    [[nodiscard]] std::vector<Variable>& mutable_variables() noexcept { return m_Variables; }
    [[nodiscard]] bool contains(const std::string_view var_name) const noexcept
       {
        return std::ranges::any_of(m_Variables, [var_name](const Variable& var) noexcept { return var.name()==var_name; });
       }
    Variable& add_variable(Variable&& var)
       {
        if( contains(var.name()) )
           {
            throw std::runtime_error{ fmt::format("Duplicate variable \"{}\" in group \"{}\"", var.name(), name()) };
           }
        m_Variables.push_back( std::move(var) );
        return m_Variables.back();
       }

    void sort()
       {
        std::ranges::sort(m_Variables, less_by_name<decltype(m_Variables)::value_type>);
       }
};



/////////////////////////////////////////////////////////////////////////////
// A series of groups of variables
class Variables_Groups final
{
 private:
    std::vector<Variables_Group> m_Groups;

 public:
    [[nodiscard]] const std::vector<Variables_Group>& groups() const noexcept { return m_Groups; }
    [[nodiscard]] std::vector<Variables_Group>& groups() noexcept { return m_Groups; }

    [[nodiscard]] bool is_empty() const noexcept
       {
        //return groups().empty(); // Nah
        // cppcheck-suppress useStlAlgorithm
        for( const auto& group : groups() )
           {
            if(not group.is_empty()) return false;
           }
        return true;
       }

    [[nodiscard]] std::size_t vars_count() const noexcept
       {
        std::size_t tot_siz = 0;
        // cppcheck-suppress useStlAlgorithm
        for( const auto& group : groups() )
           {
            tot_siz += group.variables().size();
           }
        return tot_siz;
       }

    [[nodiscard]] bool has_nonempty_named_group() const noexcept
       {
        // cppcheck-suppress useStlAlgorithm
        for( const auto& group : groups() )
           {
            if(group.has_name() and not group.is_empty()) return true;
           }
        return false;
       }

    void sort()
       {
        std::ranges::sort(m_Groups, less_by_name<decltype(m_Groups)::value_type>);
       }
};



/////////////////////////////////////////////////////////////////////////////
// A struct
class Struct final
{
 public:
    /////////////////////////////////////////////////////////////////////////
    class Member final
    {
     private:
        std::string_view m_Name;
        Type m_Type;
        std::string_view m_Descr;

     public:
        [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
        void set_name(const std::string_view sv)
           {
            if( sv.empty() )
               {
                throw std::runtime_error{"Empty parameter name"};
               }
            m_Name = sv;
           }

        [[nodiscard]] Type& type() noexcept { return m_Type; }
        [[nodiscard]] const Type& type() const noexcept { return m_Type; }

        [[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
        [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
        void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }
    };

 private:
    std::string_view m_Name;
    std::string_view m_Descr;
    std::vector<Member> m_Members;

 public:
    [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
    void set_name(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error{"Empty struct name"};
           }
        m_Name = sv;
       }

    [[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
    [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
    void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }

    [[nodiscard]] const std::vector<Member>& members() const noexcept { return m_Members; }
    [[nodiscard]] std::vector<Member>& members() noexcept { return m_Members; }
    //[[nodiscard]] bool members_contain(const std::string_view nam) const noexcept
    //   {
    //    return std::ranges::any_of(members(), [nam](const Member& memb) noexcept { return memb.name()==nam; });
    //   }
    [[nodiscard]] bool is_last_member_name_not_unique() const noexcept
       {
        if( members().size()<=1 )
           {
            return false;
           }
        const std::string_view last_name = members().back().name();
        return std::ranges::any_of(members() | std::views::take(members().size()-1u), [last_name](const Member& memb) noexcept { return memb.name()==last_name; });
        //return std::ranges::contains(members() | std::views::take(members().size()-1u), last_name, [](const Member& memb) noexcept {return memb.name();});
       }
};



/////////////////////////////////////////////////////////////////////////////
// Enumeration definition
class Enum final
{
 public:
    /////////////////////////////////////////////////////////////////////////
    class Element final
    {
     private:
        std::string_view m_Name;
        std::string_view m_Value;
        std::string_view m_Descr;

     public:
        [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
        void set_name(const std::string_view sv)
            {
             if( sv.empty() )
               {
                throw std::runtime_error{"Empty enum constant name"};
               }
             m_Name = sv;
            }

        [[nodiscard]] std::string_view value() const noexcept { return m_Value; }
        void set_value(const std::string_view sv)
            {
             if( sv.empty() )
               {
                throw std::runtime_error{ fmt::format("Enum constant {} must have a value",name()) };
               }
             m_Value = sv;
            }

        [[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
        [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
        void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }
    };

 private:
    std::string_view m_Name;
    std::string_view m_Descr;
    std::vector<Element> m_Elements;

 public:
    [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
    void set_name(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error{"Empty enum name"};
           }
        m_Name = sv;
       }

    [[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
    [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
    void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }

    [[nodiscard]] const std::vector<Element>& elements() const noexcept { return m_Elements; }
    [[nodiscard]] std::vector<Element>& elements() noexcept { return m_Elements; }
};



/////////////////////////////////////////////////////////////////////////////
// A type declaration
class TypeDef final
{
 private:
    std::string_view m_Name;
    Type m_Type;
    std::string_view m_Descr;

 public:
    [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
    void set_name(const std::string_view sv) noexcept { m_Name = sv; }

    [[nodiscard]] Type& type() noexcept { return m_Type; }
    [[nodiscard]] const Type& type() const noexcept { return m_Type; }

    [[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
    [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
    void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }
};



/////////////////////////////////////////////////////////////////////////////
// A subrange declaration
class Subrange final
{
 private:
    std::string_view m_Name;
    std::string_view m_TypeName;
    int m_MinVal = 0; // Floating point numbers seems not supported
    int m_MaxVal = 0;
    std::string_view m_Descr;

 public:
    [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
    void set_name(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error{"Empty subrange name"};
           }
        m_Name = sv;
       }

   [[nodiscard]] std::string_view type_name() const noexcept { return m_TypeName; }
    void set_type_name(const Type& type)
       {
        if( type.has_length() or type.is_array() )
           {
            throw std::runtime_error{"Cannot define a subrange with an array type"};
           }
        m_TypeName = type.name();
       }

    [[nodiscard]] int min_value() const noexcept { return m_MinVal; }
    [[nodiscard]] int max_value() const noexcept { return m_MaxVal; }
    void set_range(const int min_val, const int max_val)
       {
        if( max_val<min_val )
           {
            throw std::runtime_error{ fmt::format("Invalid range {}..{} of subrange \"{}\"", min_val, max_val, name()) };
           }
        m_MinVal = min_val;
        m_MaxVal = max_val;
       }

    [[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
    [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
    void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }
};



/////////////////////////////////////////////////////////////////////////////
// Generic Program Organization Unit (program, function_block, function)
class Pou final
{
 private:
    std::string_view m_Name;
    std::string_view m_Descr;
    std::string_view m_ReturnType;

    std::vector<Variable> m_InOutVars;
    std::vector<Variable> m_InputVars;
    std::vector<Variable> m_OutputVars;
    std::vector<Variable> m_ExternalVars;
    std::vector<Variable> m_LocalVars;
    std::vector<Variable> m_LocalConsts;

    std::string_view m_CodeType;
    std::string_view m_Body;

 public:
    [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
    void set_name(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error{"Empty POU name"};
           }
        m_Name = sv;
       }

    [[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
    [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
    void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }

    [[nodiscard]] bool has_return_type() const noexcept { return not m_ReturnType.empty(); }
    [[nodiscard]] std::string_view return_type() const noexcept { return m_ReturnType; }
    void set_return_type(const std::string_view sv) noexcept { m_ReturnType = sv; }

    [[nodiscard]] const std::vector<Variable>& inout_vars() const noexcept { return m_InOutVars; }
    [[nodiscard]] std::vector<Variable>& inout_vars() noexcept { return m_InOutVars; }

    [[nodiscard]] const std::vector<Variable>& input_vars() const noexcept { return m_InputVars; }
    [[nodiscard]] std::vector<Variable>& input_vars() noexcept { return m_InputVars; }

    [[nodiscard]] const std::vector<Variable>& output_vars() const noexcept { return m_OutputVars; }
    [[nodiscard]] std::vector<Variable>& output_vars() noexcept { return m_OutputVars; }

    [[nodiscard]] const std::vector<Variable>& external_vars() const noexcept { return m_ExternalVars; }
    [[nodiscard]] std::vector<Variable>& external_vars() noexcept { return m_ExternalVars; }

    [[nodiscard]] const std::vector<Variable>& local_vars() const noexcept { return m_LocalVars; }
    [[nodiscard]] std::vector<Variable>& local_vars() noexcept { return m_LocalVars; }

    [[nodiscard]] const std::vector<Variable>& local_constants() const noexcept { return m_LocalConsts; }
    [[nodiscard]] std::vector<Variable>& local_constants() noexcept { return m_LocalConsts; }

    [[nodiscard]] std::string_view code_type() const noexcept { return m_CodeType; }
    void set_code_type(const std::string_view sv) noexcept { m_CodeType = sv; }

    [[nodiscard]] std::string_view body() const noexcept { return m_Body; }
    void set_body(const std::string_view sv) noexcept { m_Body = sv; }

    void sort_variables()
       {
        std::ranges::sort(m_InOutVars, less_by_name<decltype(m_InOutVars)::value_type>);
        std::ranges::sort(m_InputVars, less_by_name<decltype(m_InputVars)::value_type>);
        std::ranges::sort(m_OutputVars, less_by_name<decltype(m_OutputVars)::value_type>);
        std::ranges::sort(m_ExternalVars, less_by_name<decltype(m_ExternalVars)::value_type>);
        std::ranges::sort(m_LocalVars, less_by_name<decltype(m_LocalVars)::value_type>);
        std::ranges::sort(m_LocalConsts, less_by_name<decltype(m_LocalConsts)::value_type>);
       }
};



/////////////////////////////////////////////////////////////////////////////
// A macro expansion
class Macro final
{
 public:
    /////////////////////////////////////////////////////////////////////////
    class Parameter final
    {
     private:
        std::string_view m_Name;
        std::string_view m_Descr;

     public:
        [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
        void set_name(const std::string_view sv)
           {
            if( sv.empty() )
               {
                throw std::runtime_error{"Empty parameter name"};
               }
            m_Name = sv;
           }

        [[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
        [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
        void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }
    };

 private:
    std::string_view m_Name;
    std::string_view m_Descr;
    std::vector<Parameter> m_Parameters;
    std::string_view m_CodeType;
    std::string_view m_Body;

 public:
    [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
    void set_name(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error{"Empty macro name"};
           }
        m_Name = sv;
       }

    [[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
    [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
    void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }

    [[nodiscard]] const std::vector<Parameter>& parameters() const noexcept { return m_Parameters; }
    [[nodiscard]] std::vector<Parameter>& parameters() noexcept { return m_Parameters; }

    [[nodiscard]] std::string_view code_type() const noexcept { return m_CodeType; }
    void set_code_type(const std::string_view sv) noexcept { m_CodeType = sv; }

    [[nodiscard]] std::string_view body() const noexcept { return m_Body; }
    void set_body(const std::string_view sv) noexcept { m_Body = sv; }
};



/////////////////////////////////////////////////////////////////////////////
// The whole PLC library data aggregate
class Library final
{
 private:
    std::string m_Name;
    std::string m_Version = "1.0.0";
    std::string m_Description = "PLC library";

    Variables_Groups m_GlobalConst;
    Variables_Groups m_GlobalRetainVars;
    Variables_Groups m_GlobalVars;
    std::vector<Pou> m_Programs;
    std::vector<Pou> m_FunctionBlocks;
    std::vector<Pou> m_Functions;
    std::vector<Macro> m_Macros;
    std::vector<Struct> m_Structs;
    std::vector<TypeDef> m_TypeDefs;
    std::vector<Enum> m_Enums;
    std::vector<Subrange> m_Subranges;
    //std::vector<Interface> m_Interfaces;

 public:
    explicit Library(const std::string_view nam) noexcept
      : m_Name(nam)
       {}

    [[nodiscard]] const std::string& name() const noexcept { return m_Name; }

    [[nodiscard]] const std::string& version() const noexcept { return m_Version; }
    void set_version(const std::string_view sv) noexcept { m_Version = sv; }

    [[nodiscard]] bool has_descr() const noexcept { return not m_Description.empty(); }
    [[nodiscard]] const std::string& descr() const noexcept { return m_Description; }
    void set_descr(const std::string_view sv) noexcept { m_Description = sv; }

    [[nodiscard]] const Variables_Groups& global_constants() const noexcept { return m_GlobalConst; }
    [[nodiscard]] Variables_Groups& global_constants() noexcept { return m_GlobalConst; }

    [[nodiscard]] const Variables_Groups& global_retainvars() const noexcept { return m_GlobalRetainVars; }
    [[nodiscard]] Variables_Groups& global_retainvars() noexcept { return m_GlobalRetainVars; }

    [[nodiscard]] const Variables_Groups& global_variables() const noexcept { return m_GlobalVars; }
    [[nodiscard]] Variables_Groups& global_variables() noexcept { return m_GlobalVars; }

    [[nodiscard]] const std::vector<Pou>& programs() const noexcept { return m_Programs; }
    [[nodiscard]] std::vector<Pou>& programs() noexcept { return m_Programs; }

    [[nodiscard]] const std::vector<Pou>& function_blocks() const noexcept { return m_FunctionBlocks; }
    [[nodiscard]] std::vector<Pou>& function_blocks() noexcept { return m_FunctionBlocks; }

    [[nodiscard]] const std::vector<Pou>& functions() const noexcept { return m_Functions; }
    [[nodiscard]] std::vector<Pou>& functions() noexcept { return m_Functions; }

    [[nodiscard]] const std::vector<Macro>& macros() const noexcept { return m_Macros; }
    [[nodiscard]] std::vector<Macro>& macros() noexcept { return m_Macros; }

    [[nodiscard]] const std::vector<Struct>& structs() const noexcept { return m_Structs; }
    [[nodiscard]] std::vector<Struct>& structs() noexcept { return m_Structs; }

    [[nodiscard]] const std::vector<TypeDef>& typedefs() const noexcept { return m_TypeDefs; }
    [[nodiscard]] std::vector<TypeDef>& typedefs() noexcept { return m_TypeDefs; }

    [[nodiscard]] const std::vector<Enum>& enums() const noexcept { return m_Enums; }
    [[nodiscard]] std::vector<Enum>& enums() noexcept { return m_Enums; }

    [[nodiscard]] const std::vector<Subrange>& subranges() const noexcept { return m_Subranges; }
    [[nodiscard]] std::vector<Subrange>& subranges() noexcept { return m_Subranges; }

    //[[nodiscard]] const std::vector<Interface>& interfaces() const noexcept { return m_Interfaces; }
    //[[nodiscard]] std::vector<Interface>& interfaces() noexcept { return m_Interfaces; }

    [[nodiscard]] bool is_empty() const noexcept
       {
        return     global_constants().is_empty()
               and global_retainvars().is_empty()
               and global_variables().is_empty()
               and programs().empty()
               and function_blocks().empty()
               and functions().empty()
               and macros().empty()
               and structs().empty()
               and typedefs().empty()
               and enums().empty()
               and subranges().empty();
               //and interfaces().empty();
       }

    void throw_if_incoherent() const
       {
        // Global constants must have a value (already checked in parsing)
        for( const auto& consts_grp : global_constants().groups() )
           {
            if( const auto ivar=std::ranges::find_if(consts_grp.variables(), [](const Variable& var) noexcept { return not var.has_value(); });
                ivar!=consts_grp.variables().end() )
               {
                throw std::runtime_error{ fmt::format("Global constant \"{}\" has no value", ivar->name()) };
               }
           }

        // Functions must have a return type and cannot have certain variables type
        for( const auto& funct : functions() )
           {
            if( not funct.has_return_type() )
               {
                throw std::runtime_error{ fmt::format("Function \"{}\" has no return type",funct.name()) };
               }
            if( not funct.output_vars().empty() )
               {
                throw std::runtime_error{ fmt::format("Function \"{}\" cannot have output variables",funct.name()) };
               }
            if( not funct.inout_vars().empty() )
               {
                throw std::runtime_error{ fmt::format("Function \"{}\" cannot have in-out variables",funct.name()) };
               }
            if( not funct.external_vars().empty() )
               {
                throw std::runtime_error{ fmt::format("Function \"{}\" cannot have external variables",funct.name()) };
               }
           }

        // Programs cannot have a return type and cannot have certain variables type
        for( const auto& prog : programs() )
           {
            if( prog.has_return_type() )
               {
                throw std::runtime_error{ fmt::format("Program \"{}\" cannot have a return type",prog.name()) };
               }
            if( not prog.input_vars().empty() )
               {
                throw std::runtime_error{ fmt::format("Program \"{}\" cannot have input variables",prog.name()) };
               }
            if( not prog.output_vars().empty() )
               {
                throw std::runtime_error{ fmt::format("Program \"{}\" cannot have output variables",prog.name()) };
               }
            if( not prog.inout_vars().empty() )
               {
                throw std::runtime_error{ fmt::format("Program \"{}\" cannot have in-out variables",prog.name()) };
               }
            if( not prog.external_vars().empty() )
               {
                throw std::runtime_error{ fmt::format("Program \"{}\" cannot have external variables",prog.name()) };
               }
           }
       }

    void sort()
       {
        global_constants().sort();
        global_retainvars().sort();
        global_variables().sort();

        std::ranges::sort(m_Programs, less_by_name<decltype(m_Programs)::value_type>);
        std::ranges::sort(m_FunctionBlocks, less_by_name<decltype(m_FunctionBlocks)::value_type>);
        std::ranges::sort(m_Functions, less_by_name<decltype(m_Functions)::value_type>);
        std::ranges::sort(m_Macros, less_by_name<decltype(m_Macros)::value_type>);
        std::ranges::sort(m_Structs, less_by_name<decltype(m_Structs)::value_type>);
        std::ranges::sort(m_TypeDefs, less_by_name<decltype(m_TypeDefs)::value_type>);
        std::ranges::sort(m_Enums, less_by_name<decltype(m_Enums)::value_type>);
        std::ranges::sort(m_Subranges, less_by_name<decltype(m_Subranges)::value_type>);
        //std::ranges::sort(m_Interfaces, less_by_name<decltype(m_Interfaces)::value_type>);

        //for( auto& pou : programs() )        pou.sort_variables();
        //for( auto& pou : function_blocks() ) pou.sort_variables();
        //for( auto& pou : functions() )       pou.sort_variables();
       }

    [[nodiscard]] std::string get_summary() const noexcept
       {
        std::string s;
        s.reserve(512);
        s += fmt::format("Library \"{}\"", name());
        if( not global_constants().is_empty() ) s += fmt::format(", {} global constants", global_constants().vars_count());
        if( not global_retainvars().is_empty() ) s += fmt::format(", {} global retain vars", global_retainvars().vars_count());
        if( not global_variables().is_empty() ) s += fmt::format(", {} global vars", global_variables().vars_count());
        if( not functions().empty() ) s += fmt::format(", {} functions", functions().size());
        if( not function_blocks().empty() ) s += fmt::format(", {} function blocks", function_blocks().size());
        if( not programs().empty() ) s += fmt::format(", {} programs", programs().size());
        if( not macros().empty() ) s += fmt::format(", {} macros", macros().size());
        if( not structs().empty() ) s += fmt::format(", {} structs", structs().size());
        if( not typedefs().empty() ) s += fmt::format(", {} typedefs", typedefs().size());
        if( not enums().empty() ) s += fmt::format(", {} enums", enums().size());
        if( not subranges().empty() ) s += fmt::format(", {} subranges", subranges().size());
        //if( not interfaces().empty() ) s += fmt::format(", {} interfaces", interfaces().size());
        return s;
       }
};



/////////////////////////////////////////////////////////////////////////////
// A specific vendor directive
class Directive final
{
 private:
    std::string_view m_Key;
    std::string_view m_Value;

 public:
    [[nodiscard]] std::string_view key() const noexcept { return m_Key; }
    void set_key(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error{"Empty directive key"};
           }
        m_Key = sv;
       }

    [[nodiscard]] std::string_view value() const noexcept { return m_Value; }
    void set_value(const std::string_view sv) noexcept { m_Value = sv; }
};


}//:::: buf :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

}//:::: plc :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

namespace plcb = plc::buf; // Objects that refer to an external buffer




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Test convenience functions
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace plc{ namespace buf{

//---------------------------------------------------------------------------
std::string to_string(const plcb::Type& type)
{
    std::string s{ type.name() };

    if( type.has_length() )
       {
        s += fmt::format("[{}]"sv, type.length());
       }

    if( type.is_array() )
       {
        s += fmt::format("[{}:{}]"sv, type.array_startidx(), type.array_lastidx());
       }

    return s;
}

//---------------------------------------------------------------------------
std::string to_string(const plcb::Variable& var)
{
    std::string s = fmt::format("{} {}"sv, var.name(), to_string(var.type()));

    if( var.has_descr() )
       {
        s += fmt::format(" '{}'"sv, var.descr() );
       }

    if( var.has_value() )
       {
        s += fmt::format(" (={})"sv , var.value() );
       }

    if( var.has_address() )
       {
        s += fmt::format(" <{}{}{}.{}>"sv
                        , var.address().zone()
                        , var.address().typevar()
                        , var.address().index()
                        , var.address().subindex() );
       }

    return s;
}

//---------------------------------------------------------------------------
[[nodiscard]] Type make_type(const std::string_view nam,
                             const std::size_t len_or_startidx =0u,
                             const std::size_t endidx =0u )
{
    Type typ;

    typ.set_name( nam );
    if( endidx>0u )
       {
        typ.set_array_range(len_or_startidx, endidx);
       }
    else if( len_or_startidx>0u )
       {
        typ.set_length(len_or_startidx);
       }

    return typ;
}

//---------------------------------------------------------------------------
[[nodiscard]] Variable make_var(const std::string_view nam,
                                Type&& typ,
                                const std::string_view val,
                                const std::string_view descr,
                                const char addr_type ='\0',
                                const char addr_typevar ='\0',
                                const std::uint16_t addr_index =0,
                                const std::uint16_t addr_subindex =0 )
{
    Variable var;

    var.set_name( nam );
    var.type() = std::move(typ);
    if( not val.empty() ) var.set_value( val );
    var.set_descr( descr );

    var.address().set_zone( addr_type );
    var.address().set_typevar( addr_typevar );
    var.address().set_index( addr_index );
    var.address().set_subindex( addr_subindex );

    return var;
}

//---------------------------------------------------------------------------
[[nodiscard]] Enum::Element make_enelem(const std::string_view nam,
                                        const std::string_view val,
                                        const std::string_view descr )
{
    Enum::Element elm;

    elm.set_name( nam );
    elm.set_value( val );
    elm.set_descr( descr );

    return elm;
}

//---------------------------------------------------------------------------
[[nodiscard]] Macro::Parameter make_mparam(const std::string_view nam,
                                           const std::string_view descr )
{
    Macro::Parameter par;

    par.set_name( nam );
    par.set_descr( descr );

    return par;
}

//---------------------------------------------------------------------------
[[nodiscard]] Library make_sample_lib()
{
    Library lib{"sample-lib"sv};

    lib.set_version("1.0.2");
    lib.set_descr("sample library");

   {auto& grp = lib.global_variables().groups().emplace_back();
    grp.set_name("globs");
    grp.mutable_variables() = { make_var("gvar1"sv, make_type("DINT"sv), ""sv, "gvar1 descr"),
                                make_var("gvar2"sv, make_type("INT"sv,0,99), ""sv, "gvar2 descr") };
   }

   {auto& grp = lib.global_constants().groups().emplace_back();
    //grp.set_name("constants");
    grp.mutable_variables() = { make_var("const1"sv, make_type("INT"sv), "42"sv, "gvar1 descr"),
                                make_var("const2"sv, make_type("LREAL"sv), "3.14"sv, "gvar2 descr") };
   }

    //lib.global_retainvars()

   {auto& prg = lib.programs().emplace_back();
    prg.set_name("prg_name1");
    prg.set_descr("testing prg 1");
    prg.local_vars() = { make_var("loc1"sv, make_type("DINT"sv), ""sv, "loc1 descr"),
                         make_var("loc2"sv, make_type("LREAL"sv), ""sv, "loc2 descr") };
    prg.set_code_type("ST");
    prg.set_body("body");
   }

   {auto& prg = lib.programs().emplace_back();
    prg.set_name("prg_name2");
    prg.set_descr("testing prg 2");
    prg.local_vars() = { make_var("loc1"sv, make_type("DINT"sv), ""sv, "loc1 descr"),
                         make_var("loc2"sv, make_type("LREAL"sv), ""sv, "loc2 descr") };
    prg.set_code_type("ST");
    prg.set_body("body");
   }

   {auto& fb = lib.function_blocks().emplace_back();
    fb.set_name("fb_name");
    fb.set_descr("testing fb");
    fb.inout_vars() = { make_var("inout1"sv, make_type("DINT"sv), ""sv, "inout1 descr"),
                        make_var("inout2"sv, make_type("LREAL"sv), ""sv, "inout2 descr") };
    fb.input_vars() = { make_var("in1"sv, make_type("DINT"sv), ""sv, "in1 descr"),
                        make_var("in2"sv, make_type("LREAL"sv), ""sv, "in2 descr") };
    fb.output_vars() = { make_var("out1"sv, make_type("DINT"sv), ""sv, "out1 descr"),
                         make_var("out2"sv, make_type("LREAL"sv), ""sv, "out2 descr") };
    fb.external_vars() = { make_var("ext1"sv, make_type("DINT"sv), ""sv, "ext1 descr"),
                           make_var("ext2"sv, make_type("STRING"sv,80u), ""sv, "ext2 descr") };
    fb.local_vars() = { make_var("loc1"sv, make_type("DINT"sv), ""sv, "loc1 descr"),
                        make_var("loc2"sv, make_type("LREAL"sv), ""sv, "loc2 descr") };
    fb.local_constants() = { make_var("const1"sv, make_type("DINT"sv), "42"sv, "const1 descr"),
                             make_var("const2"sv, make_type("LREAL"sv), "1.5"sv, "const2 descr") };
    fb.set_code_type("ST");
    fb.set_body("body");
   }

   {auto& fn = lib.functions().emplace_back();
    fn.set_name("fn_name");
    fn.set_descr("testing fn");
    fn.set_return_type("INT");
    fn.input_vars() = { make_var("in1"sv, make_type("DINT"sv), ""sv, "in1 descr"),
                        make_var("in2"sv, make_type("LREAL"sv), ""sv, "in2 descr") };
    fn.local_constants() = { make_var("const1"sv, make_type("DINT"sv), "42"sv, "const1 descr") };
    fn.set_code_type("ST");
    fn.set_body("body");
   }

   {auto& macro = lib.macros().emplace_back();
    macro.set_name("macroname");
    macro.set_descr("testing macro");
    macro.parameters() = { plcb::make_mparam("par1", "par1 descr"),
                           plcb::make_mparam("par2", "par2 descr") };
    macro.set_code_type("ST");
    macro.set_body("body");
   }

   {auto& strct = lib.structs().emplace_back();
    strct.set_name("structname");
    strct.set_descr("testing struct");
       {auto& memb = strct.members().emplace_back();
        memb.set_name("member1"sv);
        memb.type() = plcb::make_type("DINT"sv);
        memb.set_descr("member1 descr"sv);
       }
       {auto& memb = strct.members().emplace_back();
        memb.set_name("member2"sv);
        memb.type() = plcb::make_type("STRING"sv,80u);
        memb.set_descr("member2 descr"sv);
       }
       {auto& memb = strct.members().emplace_back();
        memb.set_name("member3"sv);
        memb.type() = plcb::make_type("INT"sv,0u,11u);
        memb.set_descr("array member"sv);
       }
   }

   {auto& enm = lib.enums().emplace_back();
    enm.set_name("enumname");
    enm.set_descr("testing enum");
    enm.elements() = { plcb::make_enelem("elm1", "1", "elm1 descr"),
                       plcb::make_enelem("elm2", "42", "elm2 descr") };
   }

   {auto& subrng = lib.subranges().emplace_back();
    subrng.set_name("subrangename");
    subrng.set_type_name( plcb::make_type("INT") );
    subrng.set_range(1,12);
    subrng.set_descr("testing subrange");
   }

   {auto& tdef = lib.typedefs().emplace_back();
    tdef.set_name("typename1"sv);
    tdef.type() = plcb::make_type("STRING"sv, 80u);
    tdef.set_descr("testing typedef"sv);
   }

   {auto& tdef = lib.typedefs().emplace_back();
    tdef.set_name("typename2"sv);
    tdef.type() = plcb::make_type("INT"sv, 0u, 9u);
    tdef.set_descr("testing typedef 2"sv);
   }

    return lib;
}


//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Address& addr1, const Address& addr2) noexcept
{
    return addr1.zone()     == addr2.zone()
       and addr1.typevar()  == addr2.typevar()
       and addr1.index()    == addr2.index()
       and addr1.subindex() == addr2.subindex();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Type& typ1, const Type& typ2) noexcept
{
    return typ1.name()           == typ2.name()
       and typ1.length()         == typ2.length()
       and typ1.array_startidx() == typ2.array_startidx()
       and typ1.array_dim()      == typ2.array_dim();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Variable& var1, const Variable& var2) noexcept
{
    return var1.name()    == var2.name()
       and var1.type()    == var2.type()
       and var1.address() == var2.address()
       and var1.value()   == var2.value()
       and var1.descr()   == var2.descr();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Variables_Group& grp1, const Variables_Group& grp2) noexcept
{
    return grp1.name()      == grp2.name()
       and grp1.variables() == grp2.variables();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Variables_Groups& grps1, const Variables_Groups& grps2) noexcept
{
    return grps1.groups() == grps2.groups();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Struct::Member& memb1, const Struct::Member& memb2) noexcept
{
    return memb1.name()  == memb2.name()
       and memb1.type()  == memb2.type()
       and memb1.descr() == memb2.descr();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Struct& strct1, const Struct& strct2) noexcept
{
    return strct1.name()    == strct2.name()
       and strct1.descr()   == strct2.descr()
       and strct1.members() == strct2.members();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Enum::Element& elem1, const Enum::Element& elem2) noexcept
{
    return elem1.name()  == elem2.name()
       and elem1.value() == elem2.value()
       and elem1.descr() == elem2.descr();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Enum& enm1, const Enum& enm2) noexcept
{
    return enm1.name()     == enm2.name()
       and enm1.descr()    == enm2.descr()
       and enm1.elements() == enm2.elements();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const TypeDef& tdef1, const TypeDef& tdef2) noexcept
{
    return tdef1.name()  == tdef2.name()
       and tdef1.type()  == tdef2.type()
       and tdef1.descr() == tdef2.descr();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Subrange& subrng1, const Subrange& subrng2) noexcept
{
    return subrng1.name()      == subrng2.name()
       and subrng1.type_name() == subrng2.type_name()
       and subrng1.min_value() == subrng2.min_value()
       and subrng1.max_value() == subrng2.max_value()
       and subrng1.descr()     == subrng2.descr();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Pou& pou1, const Pou& pou2) noexcept
{
    return pou1.name()            == pou2.name()
       and pou1.descr()           == pou2.descr()
       and pou1.return_type()     == pou2.return_type()
       and pou1.inout_vars()      == pou2.inout_vars()
       and pou1.input_vars()      == pou2.input_vars()
       and pou1.output_vars()     == pou2.output_vars()
       and pou1.external_vars()   == pou2.external_vars()
       and pou1.local_vars()      == pou2.local_vars()
       and pou1.local_constants() == pou2.local_constants()
       and pou1.code_type()       == pou2.code_type()
       and pou1.body()            == pou2.body();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Macro::Parameter& param1, const Macro::Parameter& param2) noexcept
{
    return param1.name()  == param2.name()
       and param1.descr() == param2.descr();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Macro& macro1, const Macro& macro2) noexcept
{
    return macro1.name()       == macro2.name()
       and macro1.descr()      == macro2.descr()
       and macro1.parameters() == macro2.parameters()
       and macro1.code_type()  == macro2.code_type()
       and macro1.body()       == macro2.body();
}

//---------------------------------------------------------------------------
[[nodiscard]] bool operator==(const Library& lib1, const Library& lib2) noexcept
{
    return //lib1.name()             == lib2.name()
            lib1.version()           == lib2.version()
        and lib1.descr()             == lib2.descr()
        and lib1.global_constants()  == lib2.global_constants()
        and lib1.global_retainvars() == lib2.global_retainvars()
        and lib1.global_variables()  == lib2.global_variables()
        and lib1.programs()          == lib2.programs()
        and lib1.function_blocks()   == lib2.function_blocks()
        and lib1.functions()         == lib2.functions()
        and lib1.macros()            == lib2.macros()
        and lib1.structs()           == lib2.structs()
        and lib1.typedefs()          == lib2.typedefs()
        and lib1.enums()             == lib2.enums()
        and lib1.subranges()         == lib2.subranges();
        //and lib1.interfaces()        == lib2.interfaces();
}

}}//:::: plc::buf :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

/////////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
