#pragma once
//  ---------------------------------------------
//  IEC 61131-3 stuff and descriptors of PLC elements
//  ---------------------------------------------
//  #include "plc_library.hpp" // plcb::Library
//  ---------------------------------------------
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

// Built in numeric types
static constexpr std::array<std::string_view,15> num_types =
   {
    "BOOL"sv,   // [1] BOOLean [FALSE|TRUE]
    "SINT"sv,   // [1] Short INTeger [-128 … 127]
    "INT"sv,    // [2] INTeger [-32768 … +32767]
    "DINT"sv,   // [4] Double INTeger [-2147483648 … 2147483647]
    "LINT"sv,   // [8] Long INTeger [-2^63 … 2^63-1]
    "USINT"sv,  // [1] Unsigned Short INTeger [0 … 255]
    "UINT"sv,   // [2] Unsigned INTeger [0 … 65535]
    "UDINT"sv,  // [4] Unsigned Double INTeger [0 … 4294967295]
    "ULINT"sv,  // [8] Unsigned Long INTeger [0 … 2^64-1]
    "REAL"sv,   // [4] REAL number [±10^38]
    "LREAL"sv,  // [8] Long REAL number [±10^308]
    "BYTE"sv,   // [1] 1 byte
    "WORD"sv,   // [2] 2 bytes
    "DWORD"sv,  // [4] 4 bytes
    "LWORD"sv   // [8] 8 bytes
   };

//---------------------------------------------------------------------------
// Tell if a string is a recognized numerical type
[[nodiscard]] constexpr bool is_num_type(const std::string_view sv)
   {
    return std::ranges::find(num_types, sv) != num_types.end();
   }



/////////////////////////////////////////////////////////////////////////////
// Variable address ex. MB700.320
//                      M     B        700  .  320
//                      ↑     ↑        ↑       ↑
//                      type  typevar  index   subindex
class VariableAddress final
{
 private:
    char m_Type = '\0'; // Typically will be 'M'
    char m_TypeVar = '\0';
    std::uint16_t m_Index = 0;
    std::uint16_t m_SubIndex = 0;

 public:
    explicit VariableAddress(const char typ,
                             const char vtyp,
                             const std::uint16_t idx,
                             const std::uint16_t sub) noexcept
      : m_Type(typ)
      , m_TypeVar(vtyp)
      , m_Index(idx)
      , m_SubIndex(sub)
       {}

    [[nodiscard]] bool is_empty() const noexcept { return m_Type=='\0'; }

    [[nodiscard]] char type() const noexcept { return m_Type; }
    void set_type(const char typ) noexcept { m_Type = typ; }

    [[nodiscard]] char typevar() const noexcept { return m_TypeVar; }
    void set_typevar(const char vtyp) noexcept { m_TypeVar = vtyp; }

    [[nodiscard]] std::uint16_t index() const noexcept { return m_Index; }
    void set_index(const std::uint16_t idx) noexcept { m_Index = idx; }

    [[nodiscard]] std::uint16_t subindex() const noexcept { return m_SubIndex; }
    void set_subindex(const std::uint16_t idx) noexcept { m_SubIndex = idx; }
};


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace buf //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{// Content that refers to an external buffer


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
            throw std::runtime_error("Empty directive key");
           }
        m_Key = sv;
       }

    [[nodiscard]] std::string_view value() const noexcept { return m_Value; }
    void set_value(const std::string_view sv) noexcept { m_Value = sv; }
};



/////////////////////////////////////////////////////////////////////////////
// A variable declaration
class Variable final
{
 private:
    std::string_view m_Name;
    VariableAddress m_Address;
    std::string_view m_Type;
    std::size_t m_Length = 0;
    std::size_t m_ArrayFirstIdx = 0;
    std::size_t m_ArrayDim = 0;
    std::string_view m_Value;
    std::string_view m_Descr;

 public:
    [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
    void set_name(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error("Empty variable name");
           }
        m_Name = sv;
       }

    [[nodiscard]] bool has_address() const noexcept { return not m_Address.is_empty(); }
    [[nodiscard]] VariableAddress& address() noexcept { return m_Address; }
    [[nodiscard]] const VariableAddress& address() const noexcept { return m_Address; }

    [[nodiscard]] std::string_view type() const noexcept { return m_Type; }
    void set_type(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error("Empty variable type");
           }
        m_Type = sv;
       }

    [[nodiscard]] bool has_length() const noexcept { return m_Length>0; }
    [[nodiscard]] std::size_t length() const noexcept { return m_Length; }
    void set_length(const std::size_t n) { m_Length = n; }

    [[nodiscard]] bool is_array() const noexcept { return m_ArrayDim>0; }
    [[nodiscard]] std::size_t array_dim() const noexcept { return m_ArrayDim; }
    [[nodiscard]] std::size_t array_startidx() const noexcept { return m_ArrayFirstIdx; }
    [[nodiscard]] std::size_t array_lastidx() const noexcept { return m_ArrayFirstIdx + m_ArrayDim - 1u; }
    void set_array_range(const std::size_t idx_start, const std::size_t idx_last)
       {
        if( idx_start>=idx_last )
           {
            throw std::runtime_error(fmt::format("Invalid array range {}..{} of variable \"{}\"", idx_start, idx_last, name()));
           }
        //if( idx_start!=0u ) throw std::runtime_error(fmt::format("Invalid array start index {} of variable \"{}\"", start_idx, name()));
        m_ArrayFirstIdx = idx_start;
        m_ArrayDim = idx_last - idx_start + 1u;
       }

    [[nodiscard]] bool has_value() const noexcept { return not m_Value.empty(); }
    [[nodiscard]] std::string_view value() const noexcept { return m_Value; }
    void set_value(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error("Empty variable initialization value");
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
        return std::ranges::any_of(m_Variables, [var_name](const Variable& var){ return var.name()==var_name; });
       }
    void add_variable(Variable&& var)
       {
        if( contains(var.name()) )
           {
            throw std::runtime_error(fmt::format("Duplicate variable \"{}\" in group \"{}\"", var.name(), name()));
           }
        m_Variables.push_back(std::move(var));
       }

    void sort()
       {
        std::ranges::sort(m_Variables, [](const Variable& a, const Variable& b) noexcept -> bool { return a.name()<b.name(); });
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
        std::ranges::sort(groups(), [](const Variables_Group& a, const Variables_Group& b) noexcept -> bool { return a.name()<b.name(); });
       }

    [[nodiscard]] const std::vector<Variables_Group>& groups() const noexcept { return m_Groups; }
    [[nodiscard]] std::vector<Variables_Group>& groups() noexcept { return m_Groups; }
};



/////////////////////////////////////////////////////////////////////////////
// A struct
class Struct final
{
 private:
    std::string_view m_Name;
    std::string_view m_Descr;
    std::vector<Variable> m_Members;

 public:
    [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
    void set_name(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error("Empty struct name");
           }
        m_Name = sv;
       }

    //[[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
    [[nodiscard]] std::string_view descr() const noexcept { return m_Descr; }
    void set_descr(const std::string_view sv) noexcept { m_Descr = sv; }

    [[nodiscard]] const std::vector<Variable>& members() const noexcept { return m_Members; }
    [[nodiscard]] bool members_contain(const std::string_view var_name) const noexcept
       {
        return std::ranges::any_of(m_Members, [var_name](const Variable& var){ return var.name()==var_name; });
       }
    void add_member(Variable&& var)
       {
        if( members_contain(var.name()) )
           {
            throw std::runtime_error(fmt::format("Duplicate struct member \"{}\"", var.name()));
           }
        m_Members.push_back(std::move(var));
       }
};



/////////////////////////////////////////////////////////////////////////////
// A type declaration
class TypeDef final
{
 private:
    std::string_view m_Name;
    std::string_view m_Type;
    std::size_t m_Length = 0;
    std::size_t m_ArrayFirstIdx = 0;
    std::size_t m_ArrayDim = 0;
    std::string_view m_Descr;

 public:
    explicit TypeDef(const Variable& var)
      : m_Name(var.name())
      , m_Type(var.type())
      , m_Length(var.length())
      , m_ArrayFirstIdx(var.array_startidx())
      , m_ArrayDim(var.array_dim())
      , m_Descr(var.descr())
       {
        if( var.has_value() )
           {
            throw std::runtime_error(fmt::format("Typedef \"{}\" cannot have a value ({})", var.name(), var.value()));
           }
        if( var.has_address() )
           {
            throw std::runtime_error(fmt::format("Typedef \"{}\" cannot have an address", var.name()));
           }
        //if(m_Length>0) --m_Length; // WTF Ad un certo punto Axel ha deciso che nei typedef la dimensione è meno uno??
       }

    [[nodiscard]] std::string_view name() const noexcept { return m_Name; }
    //void set_name(const std::string_view sv)
    //   {
    //    if( sv.empty() ) throw std::runtime_error("Empty typedef name");
    //    m_Name = sv;
    //   }

    [[nodiscard]] std::string_view type() const noexcept { return m_Type; }
    //void set_type(const std::string_view sv)
    //   {
    //    if( sv.empty() ) throw std::runtime_error("Empty typedef type");
    //    m_Type = sv;
    //   }

    [[nodiscard]] bool has_length() const noexcept { return m_Length>0; }
    [[nodiscard]] std::size_t length() const noexcept { return m_Length; }

    [[nodiscard]] bool is_array() const noexcept { return m_ArrayDim>0; }
    [[nodiscard]] std::size_t array_dim() const noexcept { return m_ArrayDim; }
    [[nodiscard]] std::size_t array_startidx() const noexcept { return m_ArrayFirstIdx; }
    [[nodiscard]] std::size_t array_lastidx() const noexcept { return m_ArrayFirstIdx + m_ArrayDim - 1u; }

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
                throw std::runtime_error("Empty enum constant name");
               }
             m_Name = sv;
            }

        [[nodiscard]] std::string_view value() const noexcept { return m_Value; }
        void set_value(const std::string_view sv)
            {
             if( sv.empty() )
               {
                throw std::runtime_error(fmt::format("Enum constant {} must have a value",name()));
               }
             m_Value = sv;
            }

        //[[nodiscard]] bool has_descr() const noexcept { return not m_Descr.empty(); }
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
            throw std::runtime_error("Empty enum name");
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
            throw std::runtime_error("Empty subrange name");
           }
        m_Name = sv;
       }

   [[nodiscard]]  std::string_view type() const noexcept { return m_Type; }
    void set_type(const std::string_view sv)
       {
        if( sv.empty() )
           {
            throw std::runtime_error("Empty subrange type");
           }
        m_Type = sv;
       }

    [[nodiscard]] int min_value() const noexcept { return m_MinVal; }
    [[nodiscard]] int max_value() const noexcept { return m_MaxVal; }
    void set_range(const int min_val, const int max_val)
       {
        if( max_val<min_val )
           {
            throw std::runtime_error(fmt::format("Invalid range {}..{} of subrange \"{}\"", min_val, max_val, name()));
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
            throw std::runtime_error("Empty POU name");
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
        std::ranges::sort(inout_vars(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name()<b.name(); });
        std::ranges::sort(input_vars(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name()<b.name(); });
        std::ranges::sort(output_vars(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name()<b.name(); });
        std::ranges::sort(external_vars(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name()<b.name(); });
        std::ranges::sort(local_vars(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name()<b.name(); });
        std::ranges::sort(local_constants(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name()<b.name(); });
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
                throw std::runtime_error("Empty parameter name");
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
            throw std::runtime_error("Empty macro name");
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
    explicit Library(const std::string& nam) noexcept
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
        return     global_constants().size()==0
               and global_retainvars().size()==0
               and global_variables().size()==0
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

    void check() const
       {
        // Global constants must have a value (already checked in parsing)
        for( const auto& consts_grp : global_constants().groups() )
           {
            if( const auto ivar=std::ranges::find_if(consts_grp.variables(), [](const Variable& var){ return not var.has_value(); });
                ivar!=consts_grp.variables().end() )
               {
                throw std::runtime_error(fmt::format("Global constant \"{}\" has no value", ivar->name()));
               }
           }

        // Functions must have a return type and cannot have certain variables type
        for( const auto& funct : functions() )
           {
            if( not funct.has_return_type() )
               {
                throw std::runtime_error(fmt::format("Function \"{}\" has no return type",funct.name()));
               }
            if( not funct.output_vars().empty() )
               {
                throw std::runtime_error(fmt::format("Function \"{}\" cannot have output variables",funct.name()));
               }
            if( not funct.inout_vars().empty() )
               {
                throw std::runtime_error(fmt::format("Function \"{}\" cannot have in-out variables",funct.name()));
               }
            if( not funct.external_vars().empty() )
               {
                throw std::runtime_error(fmt::format("Function \"{}\" cannot have external variables",funct.name()));
               }
           }

        // Programs cannot have a return type and cannot have certain variables type
        for( const auto& prog : programs() )
           {
            if( prog.has_return_type() )
               {
                throw std::runtime_error(fmt::format("Program \"{}\" cannot have a return type",prog.name()));
               }
            if( not prog.input_vars().empty() )
               {
                throw std::runtime_error(fmt::format("Program \"{}\" cannot have input variables",prog.name()));
               }
            if( not prog.output_vars().empty() )
               {
                throw std::runtime_error(fmt::format("Program \"{}\" cannot have output variables",prog.name()));
               }
            if( not prog.inout_vars().empty() )
               {
                throw std::runtime_error(fmt::format("Program \"{}\" cannot have in-out variables",prog.name()));
               }
            if( not prog.external_vars().empty() )
               {
                throw std::runtime_error(fmt::format("Program \"{}\" cannot have external variables",prog.name()));
               }
           }
       }

    void sort()
       {
        global_constants().sort();
        global_retainvars().sort();
        global_variables().sort();

        std::ranges::sort(programs(), [](const Pou& a, const Pou& b) noexcept -> bool { return a.name()<b.name(); });
        std::ranges::sort(function_blocks(), [](const Pou& a, const Pou& b) noexcept -> bool { return a.name()<b.name(); });
        std::ranges::sort(functions(), [](const Pou& a, const Pou& b) noexcept -> bool { return a.name()<b.name(); });
        std::ranges::sort(macros(), [](const Macro& a, const Macro& b) noexcept -> bool { return a.name()<b.name(); });
        std::ranges::sort(typedefs(), [](const TypeDef& a, const TypeDef& b) noexcept -> bool { return a.name()<b.name(); });
        std::ranges::sort(enums(), [](const Enum& a, const Enum& b) noexcept -> bool { return a.name()<b.name(); });
        std::ranges::sort(subranges(), [](const Subrange& a, const Subrange& b) noexcept -> bool { return a.name()<b.name(); });
        //std::ranges::sort(interfaces(), [](const Interface& a, const Interface& b) noexcept -> bool { return a.name()<b.name(); });

        //for( auto& pou : programs() )        pou.sort_variables();
        //for( auto& pou : function_blocks() ) pou.sort_variables();
        //for( auto& pou : functions() )       pou.sort_variables();
       }

    [[nodiscard]] std::string to_str() const noexcept
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

    //---------------------------------------------------------------------------
    //void write_full_summary(const sys::file_write& f, const std::string_view ind) const
    //   {
    //    if( not global_constants().is_empty() )
    //       {
    //        f << '\n' << ind << "constants:\n"sv;
    //        for( const auto& group : global_constants().groups() )
    //           {
    //            f << ind << ind << group.name() << '\n';
    //            for( const auto& var : group.variables() )
    //               {
    //                f << ind << ind << ind << var.name() << '\n';
    //               }
    //           }
    //       }
    //
    //    if( not global_variables().is_empty() or not global_retainvars().is_empty() )
    //       {
    //        f << '\n' << ind << "global vars:\n"sv;
    //        for( const auto& group : global_variables().groups() )
    //           {
    //            f << ind << ind << group.name() << '\n';
    //            for( const auto& var : group.variables() )
    //               {
    //                f << ind << ind << ind << var.name() << '\n';
    //               }
    //           }
    //        for( const auto& group : global_retainvars().groups() )
    //           {
    //            f << ind << ind << group.name() << '\n';
    //            for( const auto& var : group.variables() )
    //               {
    //                f << ind << ind << ind << var.name() << '\n';
    //               }
    //           }
    //       }
    //
    //    if( not functions().empty() )
    //       {
    //        f << '\n' << ind << "functions:\n"sv;
    //        for( const auto& elem : functions() )
    //           {
    //            f << ind << ind << elem.name() << ':' << pou.return_type() << '\n';
    //           }
    //       }
    //
    //    if( not function_blocks().empty() )
    //       {
    //        f << '\n' << ind << "function blocks:\n"sv;
    //        for( const auto& elem : function_blocks() )
    //           {
    //            f << ind << ind << elem.name() << '\n';
    //           }
    //       }
    //
    //    if( not programs().empty() )
    //       {
    //        f << '\n' << ind << "programs:\n"sv;
    //        for( const auto& elem : programs() )
    //           {
    //            f << ind << ind << elem.name() << '\n';
    //           }
    //       }
    //
    //    if( not macros().empty() )
    //       {
    //        f << '\n' << ind << "macros:\n"sv;
    //        for( const auto& elem : macros() )
    //           {
    //            f << ind << ind << elem.name() << '\n';
    //           }
    //       }
    //
    //    if( not typedefs().empty() )
    //       {
    //        f << '\n' << ind << "typedefs:\n"sv;
    //        for( const auto& elem : typedefs() )
    //           {
    //            f << ind << ind << elem.name() << '\n';
    //           }
    //       }
    //
    //    if( not enums().empty() )
    //       {
    //        f << '\n' << ind << "enums:\n"sv;
    //        for( const auto& elem : enums() )
    //           {
    //            f << ind << ind << elem.name() << '\n';
    //           }
    //       }
    //
    //    if( not subranges().empty() )
    //       {
    //        f << '\n' << ind << "subranges:\n"sv;
    //        for( const auto& elem : subranges() )
    //           {
    //            f << ind << ind << elem.name() << '\n';
    //           }
    //       }
    //
    //    //if( not interfaces().empty() )
    //    //   {
    //    //    f << '\n' << ind << "interfaces:\n"sv;
    //    //    for( const auto& elem : interfaces() )
    //    //       {
    //    //        f << ind << ind << elem.name() << '\n';
    //    //       }
    //    //   }
    //   }
};

}//:::: buf :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

}//:::: plc :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

namespace plcb = plc::buf; // Objects that refer to an external buffer
