#pragma once
//  ---------------------------------------------
//  Sipro details
//  ---------------------------------------------
//  #include "sipro.hpp" // sipro::Register
//  ---------------------------------------------
#include <cstdint> // std::uint8_t, std::uint16_t
#include <array>
#include <string_view>

#include "string_conversions.hpp" // str::to_num_or<>()


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sipro
{

//---------------------------------------------------------------------------
// Tell if a string is a supported IEC numerical type
//[[nodiscard]] bool is_supported_iec_type(const std::string_view sv)
//   {
//    static constexpr std::array supported_iec_types =
//       {
//        "BOOL"sv  // BOOLean [FALSE|TRUE]
//       ,"SINT"sv  // Short INTeger [-128 … 127]
//       ,"INT"sv   // INTeger [-32768 … +32767]
//       ,"DINT"sv  // Double INTeger [-2147483648 … 2147483647]
//       ,"USINT"sv // Unsigned Short INTeger [0 … 255]
//       ,"UINT"sv  // Unsigned INTeger [0 … 65535]
//       ,"UDINT"sv // Unsigned Double INTeger [0 … 4294967295]
//       ,"LREAL"sv // Long REAL number [±10^308]
//       ,"BYTE"sv  // 1 byte
//       ,"WORD"sv  // 2 bytes
//       ,"DWORD"sv // 4 bytes
//       };
//    return std::ranges::contains(supported_iec_types, sv);
//   }


/////////////////////////////////////////////////////////////////////////////
class Register final
{
    enum class type : std::uint8_t
       {
        none=0, // !Consecutive indexes!
        vb,
        vn,
        vq,
        vd,
        va,
        //din,
        //dout,
        size
       };

    static constexpr std::array<std::string_view, std::to_underlying(type::size)>
    reg_iec_types =
       {
        ""       // none
       ,"BOOL"   // vb
       ,"INT"    // vn
       ,"DINT"   // vq
       ,"LREAL"  // vd
       ,"STRING" // va
       //,"BOOL"   // din
       //,"BOOL"   // dout
       };

    static constexpr std::array<char, std::to_underlying(type::size)>
    plc_var_type =
       {
        '\0' // none
       ,'B'  // vb
       ,'W'  // vn
       ,'D'  // vq
       ,'L'  // vd
       ,'B'  // va
       //,'X'  // din
       //,'X'  // dout
       };

    static constexpr std::array<std::uint16_t, std::to_underlying(type::size)>
    plc_var_address =
       {
        0,   // none
        300, // vb
        400, // vn
        500, // vq
        600, // vd
        700  // va
       };

 private:
    std::uint16_t m_index = 0u;
    type m_type = type::none;

 public:
    explicit constexpr Register(std::string_view sv) noexcept
       {// From strings like "vq123"
        // Prefix
        if( sv.length()>=3 and (sv[0]=='v' or sv[0]=='V') )
           {
            // Type
                 if(sv[1]=='b' or sv[1]=='B') m_type = type::vb;
            else if(sv[1]=='n' or sv[1]=='N') m_type = type::vn;
            else if(sv[1]=='q' or sv[1]=='Q') m_type = type::vq;
            else if(sv[1]=='d' or sv[1]=='D') m_type = type::vd;
            else if(sv[1]=='a' or sv[1]=='A') m_type = type::va;

            // Index
            if( m_type!=type::none )
               {
                sv.remove_prefix(2);
                if( const auto idx = str::to_num_or<std::uint16_t>(sv) )
                   {
                    m_index = idx.value();
                   }
                else
                   {
                    m_type = type::none; // Not a valid register index, invalidate
                   }
               }
           }
       }

    [[nodiscard]] constexpr std::uint16_t index() const noexcept { return m_index; }
    [[nodiscard]] constexpr bool has_index_out_of_range() const noexcept { return m_index>9999u; }

    [[nodiscard]] friend constexpr bool are_same_type(const Register& lhs, const Register& rhs) noexcept { return lhs.m_type==rhs.m_type; }
    [[nodiscard]] constexpr bool is_valid() const noexcept { return m_type!=type::none; }
    [[nodiscard]] constexpr bool is_vb() const noexcept { return m_type==type::vb; }
    [[nodiscard]] constexpr bool is_vn() const noexcept { return m_type==type::vn; }
    [[nodiscard]] constexpr bool is_vq() const noexcept { return m_type==type::vq; }
    [[nodiscard]] constexpr bool is_vd() const noexcept { return m_type==type::vd; }
    [[nodiscard]] constexpr bool is_va() const noexcept { return m_type==type::va; }

    [[nodiscard]] constexpr std::uint16_t get_va_length() const noexcept { return 80; }

    [[nodiscard]] constexpr std::string_view iec_type() const noexcept { return reg_iec_types[std::to_underlying(m_type)]; }
    [[nodiscard]] constexpr char iec_address_type() const noexcept { return 'M'; }
    [[nodiscard]] constexpr char iec_address_vartype() const noexcept { return plc_var_type[std::to_underlying(m_type)]; }
    [[nodiscard]] constexpr std::uint16_t iec_address_index() const noexcept { return plc_var_address[std::to_underlying(m_type)]; }
};

}//::::::::::::::::::::::::::::::::: sipro ::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
static ut::suite<"sipro::"> sipro_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("sipro::Register") = []
   {
    ut::test("vq123") = []
       {
        const sipro::Register vq123{"VQ123"sv};
        ut::expect( ut::that % vq123.is_vq() and vq123.index()==123 );
       };

    ut::test("bad type") = []
       {
        const sipro::Register vx123{"vx123"sv};
        ut::expect( not vx123.is_valid() );
       };

    ut::test("large index") = []
       {
        const sipro::Register vqbadidx{"vq10000"sv};
        ut::expect( vqbadidx.is_valid() and vqbadidx.has_index_out_of_range() );
       };
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
