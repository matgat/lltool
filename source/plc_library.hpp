#pragma once
//  ---------------------------------------------
//  IEC 61131-3 stuff and descriptors of PLC elements
//  ---------------------------------------------
//  #include "plc_library.hpp" // plcb::Library
//  ---------------------------------------------
#include <concepts> // std::convertible_to<>
#include <stdexcept> // std::runtime_error
#include <cstdint> // std::uint16_t
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
//                      type  typevar  index   subindex
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
    [[nodiscard]] std::size_t array_dim() const noexcept { return m_ArrayDim; }
    [[nodiscard]] std::size_t array_startidx() const noexcept { return m_ArrayFirstIdx; }
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

    [[nodiscard]] std::size_t size() const noexcept
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

    [[nodiscard]] const std::vector<Variables_Group>& groups() const noexcept { return m_Groups; }
    [[nodiscard]] std::vector<Variables_Group>& groups() noexcept { return m_Groups; }
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

        //[[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
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

    //[[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
    [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
    void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }

    [[nodiscard]] const std::vector<Member>& members() const noexcept { return m_Members; }
    [[nodiscard]] bool members_contain(const std::string_view nam) const noexcept
       {
        return std::ranges::any_of(m_Members, [nam](const Member& memb) noexcept { return memb.name()==nam; });
       }
    void add_member(Member&& memb)
       {
        if( members_contain(memb.name()) )
           {
            throw std::runtime_error{ fmt::format("Duplicate struct member \"{}\"", memb.name()) };
           }
        m_Members.push_back(std::move(memb));
       }
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

    //[[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
    [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
    void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }
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

    [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
    void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }

    [[nodiscard]] const std::vector<Element>& elements() const noexcept { return m_Elements; }
    [[nodiscard]] std::vector<Element>& elements() noexcept { return m_Elements; }
};



/////////////////////////////////////////////////////////////////////////////
// A subrange declaration
class Subrange final
{
 private:
    std::string_view m_Name;
    std::string_view m_Type;
    int m_MinVal = 0;
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

   [[nodiscard]]  std::string_view type() const noexcept { return m_Type; }
    void set_type(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error{"Empty subrange type"};
           }
        m_Type = sv;
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

        //[[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
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

    //[[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
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
        if( not global_constants().is_empty() ) s += fmt::format(", {} global constants", global_constants().size());
        if( not global_retainvars().is_empty() ) s += fmt::format(", {} global retain vars", global_retainvars().size());
        if( not global_variables().is_empty() ) s += fmt::format(", {} global vars", global_variables().size());
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

}}//:::: plc::buf :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

/////////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
