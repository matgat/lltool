#pragma once
//  ---------------------------------------------
//  Some facilities to deal with unicode text
//  ---------------------------------------------
//  #include "unicode_text.hpp" // utxt::*
//  ---------------------------------------------
#include <cassert>
#include <cstdint> // std::uint8_t, std::uint16_t, ...
#include <utility> // std::unreachable()
#include <string>
#include <string_view>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace utxt
{

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace details
{
    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr std::uint16_t combine_chars(const char h, const char l) noexcept
       {
        return static_cast<std::uint16_t>(static_cast<unsigned char>(h) << 0x8) |
               static_cast<std::uint16_t>(static_cast<unsigned char>(l));
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr std::uint32_t combine_chars(const char hh, const char hl, const char lh, const char ll) noexcept
       {
        return static_cast<std::uint32_t>(static_cast<unsigned char>(hh) << 0x18) |
               static_cast<std::uint32_t>(static_cast<unsigned char>(hl) << 0x10) |
               static_cast<std::uint32_t>(static_cast<unsigned char>(lh) << 0x8) |
               static_cast<std::uint32_t>(static_cast<unsigned char>(ll));
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr char high_byte_of(const std::uint16_t word) noexcept
       {
        return static_cast<char>((word >> 0x8) & 0xFF);
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr char low_byte_of(const std::uint16_t word) noexcept
       {
        return static_cast<char>(word & 0xFF);
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr char hh_byte_of(const std::uint32_t dword) noexcept
       {
        return static_cast<char>((dword >> 0x18) & 0xFF);
       }

    //-------------------------------------------------------------------
    [[nodiscard]] constexpr char hl_byte_of(const std::uint32_t dword) noexcept
       {
        return static_cast<char>((dword >> 0x10) & 0xFF);
       }

    //-------------------------------------------------------------------
    [[nodiscard]] constexpr char lh_byte_of(const std::uint32_t dword) noexcept
       {
        return static_cast<char>((dword >> 0x8) & 0xFF);
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] constexpr char ll_byte_of(const std::uint32_t dword) noexcept
       {
        return static_cast<char>(dword & 0xFF);
       }

} //::::::::::::::::::::::::::::::: details :::::::::::::::::::::::::::::::::


enum class Enc : std::uint8_t
   {
    //ANSI, // Nah, time to drop codepages
    UTF8 =0,
    UTF16LE,
    UTF16BE,
    UTF32LE,
    UTF32BE
   };


namespace codepoint
{
    inline static constexpr char32_t invalid = U'�'; // replacement character '\u{FFFD}'
    inline static constexpr char32_t null = U'\0';
    //inline static constexpr char32_t bom = U'\uFEFF'; // Zero Width No-Break Space (BOM)
}



/////////////////////////////////////////////////////////////////////////////
using flags_t = std::uint8_t;
namespace flag
{
    enum : flags_t
    {
       NONE = 0x0
      ,SKIP_BOM = 0x1 // Skip the byte order mask
       //,RESERVED = 0x2 // Reserved
       //,RESERVED = 0x4 // Reserved
       //,RESERVED = 0x8 // Reserved
    };
}



//---------------------------------------------------------------------------
// auto [enc, bom_size] = utxt::detect_encoding_of(bytes);
struct bom_ret_t final { Enc enc; std::uint8_t bom_size; };
bom_ret_t constexpr detect_encoding_of(const std::string_view bytes)
   {// +-----------+-------------+
    // | Encoding  |   Bytes     |
    // |-----------|-------------|
    // | utf-8     | EF BB BF    |
    // | utf-16-be | FE FF       |
    // | utf-16-le | FF FE       |
    // | utf-32-be | 00 00 FE FF |
    // | utf-32-le | FF FE 00 00 |
    // +-----------+-------------+
    using enum Enc;
    if( bytes.size()>2 ) [[likely]]
       {
        if( bytes[0]=='\xFF' and bytes[1]=='\xFE' )
           {
            if( bytes.size()>=4 and bytes[2]=='\x00' and bytes[3]=='\x00' )
               {
                return {UTF32LE, 4};
               }
            else
               {
                return {UTF16LE, 2};
               }
           }
        else if( bytes[0]=='\xFE' and bytes[1]=='\xFF' )
           {
            return {UTF16BE, 2};
           }
        else if( bytes.size()>=4 and bytes[0]=='\x00' and bytes[1]=='\x00' and bytes[2]=='\xFE' and bytes[3]=='\xFF' )
           {
            return {UTF32BE, 4};
           }
        else if( bytes[0]=='\xEF' and bytes[1]=='\xBB' and bytes[2]=='\xBF' )
           {
            return {UTF8, 3};
           }
       }
    // Fallback
    return {UTF8, 0};
   }


//---------------------------------------------------------------------------
// Decode: Extract a codepoint according to encoding and endianness
template<Enc enc> constexpr char32_t extract_codepoint(const std::string_view bytes, std::size_t& pos) noexcept;

//---------------------------------------------------------------------------
//template<> constexpr char32_t extract_codepoint<Enc::ANSI>(const std::string_view bytes, std::size_t& pos) noexcept
//{
//    assert( pos<bytes.size() );
//
//    return bytes[pos++];
//}

//---------------------------------------------------------------------------
template<> constexpr char32_t extract_codepoint<Enc::UTF8>(const std::string_view bytes, std::size_t& pos) noexcept
{
    assert( pos<bytes.size() );

    if( (bytes[pos] & 0x80)==0 ) [[likely]]
       {
        const char32_t codepoint = static_cast<char32_t>(bytes[pos]);
        ++pos;
        return codepoint;
       }

    else if( (pos+1)<bytes.size() and (bytes[pos] & 0xE0)==0xC0 and (bytes[pos+1] & 0xC0)==0x80 )
       {
        const char32_t codepoint = static_cast<char32_t>((bytes[pos] & 0x1F) << 6) |
                                   static_cast<char32_t>(bytes[pos+1] & 0x3F);
        pos += 2;
        return codepoint;
       }

    else if( (pos+2)<bytes.size() and (bytes[pos] & 0xF0)==0xE0 and (bytes[pos+1] & 0xC0)==0x80 and (bytes[pos+2] & 0xC0)==0x80 )
       {
        const char32_t codepoint = static_cast<char32_t>((bytes[pos] & 0x0F) << 12) |
                                   static_cast<char32_t>((bytes[pos+1] & 0x3F) << 6) |
                                   static_cast<char32_t>(bytes[pos+2] & 0x3F);
        pos += 3;
        return codepoint;
       }

    else if( (pos+3)<bytes.size() and (bytes[pos] & 0xF8)==0xF0 and (bytes[pos+1] & 0xC0)==0x80 and (bytes[pos+2] & 0xC0)==0x80 and (bytes[pos+3] & 0xC0)==0x80 )
       {
        const char32_t codepoint = static_cast<char32_t>((bytes[pos] & 0x07) << 18) |
                                   static_cast<char32_t>((bytes[pos+1] & 0x3F) << 12) |
                                   static_cast<char32_t>((bytes[pos+2] & 0x3F) << 6) |
                                   static_cast<char32_t>(bytes[pos+3] & 0x3F);
        pos += 4;
        return codepoint;
       }

    // Invalid utf-8 character
    ++pos;
    return codepoint::invalid;
}

//---------------------------------------------------------------------------
template<bool LE> constexpr char32_t extract_next_codepoint_from_utf16(const std::string_view bytes, std::size_t& pos) noexcept
{
    assert( (pos+1)<bytes.size() );

    auto get_code_unit = [](const std::string_view buf, const std::size_t i) -> std::uint16_t
                           {
                            if constexpr(LE) return details::combine_chars(buf[i+1], buf[i]); // Little endian
                            else             return details::combine_chars(buf[i], buf[i+1]); // Big endian
                           };

    // If the first codeunit is in the intervals [0x0000–0xD800), [0xE000–0xFFFF]
    // then coincides with the codepoint itself (Basic Multilingual Plane)
    // Otherwise the whole codepoint is composed by two codeunits:
    // the first in the interval [0xD800-0xDC00), and the second in [0xDC00–0xE000)
    // codeunit1 = 0b110110yyyyyyyyyy // 0xD800 + yyyyyyyyyy [0xD800-0xDC00)
    // codeunit2 = 0b110111xxxxxxxxxx // 0xDC00 + xxxxxxxxxx [0xDC00–0xE000)
    // codepoint = 0x10000 + yyyyyyyyyyxxxxxxxxxx

    const std::uint16_t codeunit1 = get_code_unit(bytes, pos);
    pos += 2;

    if( codeunit1<0xD800 or codeunit1>=0xE000 ) [[likely]]
       {// Basic Multilingual Plane
        return codeunit1;
       }

    else if( codeunit1>=0xDC00 or (pos+1)>=bytes.size() )
       {// Not a first surrogate!
        return codepoint::invalid;
       }

    // Here expecting the second codeunit
    const std::uint16_t codeunit2 = get_code_unit(bytes, pos);
    if( codeunit2<0xDC00 or codeunit2>=0xE000 ) [[unlikely]]
       {// Not a second surrogate!
        return codepoint::invalid;
       }

    // Ok, I have the two valid codeunits
    pos += 2;
    return 0x10000 + static_cast<char32_t>((codeunit1 - 0xD800) << 0xA) + (codeunit2 - 0xDC00);
}
template<> constexpr char32_t extract_codepoint<Enc::UTF16LE>(const std::string_view bytes, std::size_t& pos) noexcept
{
    return extract_next_codepoint_from_utf16<true>(bytes, pos);
}
template<> constexpr char32_t extract_codepoint<Enc::UTF16BE>(const std::string_view bytes, std::size_t& pos) noexcept
{
    return extract_next_codepoint_from_utf16<false>(bytes, pos);
}

//---------------------------------------------------------------------------
template<> constexpr char32_t extract_codepoint<Enc::UTF32LE>(const std::string_view bytes, std::size_t& pos) noexcept
{
    assert( (pos+3)<bytes.size() );

    const char32_t codepoint = details::combine_chars(bytes[pos+3], bytes[pos+2], bytes[pos+1], bytes[pos]); // Little endian
    pos += 4;
    return codepoint;
}

//---------------------------------------------------------------------------
template<> constexpr char32_t extract_codepoint<Enc::UTF32BE>(const std::string_view bytes, std::size_t& pos) noexcept
{
    assert( (pos+3)<bytes.size() );

    const char32_t codepoint = details::combine_chars(bytes[pos], bytes[pos+1], bytes[pos+2], bytes[pos+3]); // Big endian
    pos += 4;
    return codepoint;
}


//---------------------------------------------------------------------------
// Encode: Write a codepoint according to encoding and endianness
template<Enc enc> constexpr void append_codepoint(const char32_t codepoint, std::string& bytes) noexcept;

//---------------------------------------------------------------------------
//template<> constexpr void append_codepoint<Enc::ANSI>(const char32_t codepoint, std::string& bytes) noexcept
//{
//    bytes += static_cast<char>(codepoint); // Narrowing!
//}

//---------------------------------------------------------------------------
template<> constexpr void append_codepoint<Enc::UTF8>(const char32_t codepoint, std::string& bytes) noexcept
{
    if( codepoint<0x80 ) [[likely]]
       {
        bytes.push_back( static_cast<char>(codepoint) );
       }
    else if( codepoint<0x800 )
       {
        bytes.push_back( static_cast<char>(0xC0 | (codepoint >> 6) ));
        bytes.push_back( static_cast<char>(0x80 | (codepoint & 0x3F)) );
       }
    else if( codepoint<0x10000 )
       {
        bytes.push_back( static_cast<char>(0xE0 | (codepoint >> 12)) );
        bytes.push_back( static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)) );
        bytes.push_back( static_cast<char>(0x80 | (codepoint & 0x3F)) );
       }
    else
       {
        bytes.push_back( static_cast<char>(0xF0 | (codepoint >> 18)) );
        bytes.push_back( static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)) );
        bytes.push_back( static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)) );
        bytes.push_back( static_cast<char>(0x80 | (codepoint & 0x3F)) );
       }
}

//---------------------------------------------------------------------------
// Encode a codepoint outside Basic Multilingual Plane
constexpr std::pair<std::uint16_t,std::uint16_t> encode_as_utf16(uint32_t codepoint) noexcept
{
    assert( codepoint>=0x10000 );
    codepoint -= 0x10000;
    return { static_cast<std::uint16_t>((codepoint >> 10) + 0xD800),
             static_cast<std::uint16_t>((codepoint & 0x3FF) + 0xDC00) };
}

//---------------------------------------------------------------------------
template<> constexpr void append_codepoint<Enc::UTF16LE>(const char32_t codepoint, std::string& bytes) noexcept
{
    if( codepoint<0x10000 ) [[likely]]
       {
        const std::uint16_t codeunit = static_cast<std::uint16_t>(codepoint);
        bytes.push_back( details::low_byte_of( codeunit) );
        bytes.push_back( details::high_byte_of( codeunit ) );
       }
    else
       {
        const auto codeunits = encode_as_utf16(codepoint);
        bytes.push_back( details::low_byte_of( codeunits.first ) );
        bytes.push_back( details::high_byte_of( codeunits.first ) );
        bytes.push_back( details::low_byte_of( codeunits.second ) );
        bytes.push_back( details::high_byte_of( codeunits.second ) );
       }
}

//---------------------------------------------------------------------------
template<> constexpr void append_codepoint<Enc::UTF16BE>(const char32_t codepoint, std::string& bytes) noexcept
{
    if( codepoint<0x10000 ) [[likely]]
       {
        const std::uint16_t codeunit = static_cast<std::uint16_t>(codepoint);
        bytes.push_back( details::high_byte_of( codeunit ) );
        bytes.push_back( details::low_byte_of( codeunit) );
       }
    else
       {
        const auto codeunits = encode_as_utf16(codepoint);
        bytes.push_back( details::high_byte_of( codeunits.first ) );
        bytes.push_back( details::low_byte_of( codeunits.first ) );
        bytes.push_back( details::high_byte_of( codeunits.second ) );
        bytes.push_back( details::low_byte_of( codeunits.second ) );
       }
}

//---------------------------------------------------------------------------
template<> constexpr void append_codepoint<Enc::UTF32LE>(const char32_t codepoint, std::string& bytes) noexcept
{
    bytes.push_back( details::ll_byte_of( codepoint ) );
    bytes.push_back( details::lh_byte_of( codepoint ) );
    bytes.push_back( details::hl_byte_of( codepoint ) );
    bytes.push_back( details::hh_byte_of( codepoint ) );
}

//---------------------------------------------------------------------------
template<> constexpr void append_codepoint<Enc::UTF32BE>(const char32_t codepoint, std::string& bytes) noexcept
{
    bytes.push_back( details::hh_byte_of( codepoint ) );
    bytes.push_back( details::hl_byte_of( codepoint ) );
    bytes.push_back( details::lh_byte_of( codepoint ) );
    bytes.push_back( details::ll_byte_of( codepoint ) );
}



/////////////////////////////////////////////////////////////////////////////
template<Enc ENC> class bytes_buffer_t final
{
 public:
    struct context_t final
       {
        std::size_t current_byte_offset;
       };
    constexpr context_t save_context() const noexcept
       {
        return { m_current_byte_offset };
       }
    constexpr void restore_context(const context_t& context) noexcept
       {
        m_current_byte_offset = context.current_byte_offset;
       }

 private:
    std::string_view m_byte_buf;
    std::size_t m_current_byte_offset = 0; // Index of currently pointed byte

 public:
    explicit constexpr bytes_buffer_t(const std::string_view bytes) noexcept
      : m_byte_buf{bytes}
       {}

    [[nodiscard]] constexpr std::string_view get_current_view() const noexcept
       {
        return m_byte_buf.substr(m_current_byte_offset);
       }

    [[nodiscard]] constexpr std::string_view get_view_between(const std::size_t from_byte_pos, const std::size_t to_byte_pos) const noexcept
       {
        assert( from_byte_pos<=to_byte_pos );
        return m_byte_buf.substr(from_byte_pos, to_byte_pos-from_byte_pos);
       }

    [[nodiscard]] constexpr std::size_t byte_pos() const noexcept { return m_current_byte_offset; }
    constexpr void advance_of(const std::size_t bytes_num) noexcept { m_current_byte_offset += bytes_num; }
    constexpr void set_as_depleted() noexcept { m_current_byte_offset = m_byte_buf.size(); }

    [[nodiscard]] constexpr bool has_bytes() const noexcept
       {
        return m_current_byte_offset < m_byte_buf.size();
       }

    [[nodiscard]] constexpr bool has_codepoint() const noexcept
       {
        if constexpr(ENC==Enc::UTF16LE or ENC==Enc::UTF16BE)
           {
            return (m_current_byte_offset+1) < m_byte_buf.size();
           }
        else if constexpr(ENC==Enc::UTF32LE or ENC==Enc::UTF32BE)
           {
            return (m_current_byte_offset+3) < m_byte_buf.size();
           }
        else
           {
            return m_current_byte_offset < m_byte_buf.size();
           }
       }

    [[nodiscard]] constexpr char32_t extract_codepoint() noexcept
       {
        assert( has_codepoint() );
        const char32_t next_codepoint = utxt::extract_codepoint<ENC>(m_byte_buf, m_current_byte_offset);
        assert( m_current_byte_offset<=m_byte_buf.size() );
        return next_codepoint;
       }
};


// To instantiate the right template at runtime, so ugly...
#define TEXT_DISPATCH_TO_ENC(E,L,...)\
    switch(E)\
       {using enum utxt::Enc;\
        case UTF8: return L UTF8 __VA_ARGS__;\
        case UTF16LE: return L UTF16LE __VA_ARGS__;\
        case UTF16BE: return L UTF16BE __VA_ARGS__;\
        case UTF32LE: return L UTF32LE __VA_ARGS__;\
        case UTF32BE: return L UTF32BE __VA_ARGS__;\
       }\
    std::unreachable();
    //throw std::runtime_error{ fmt::format("Unhandled encoding: {}"sv, std::to_underlying(E)) };

#define TEXT_ATTACH_TO_ENC(E,L)\
    switch(E)\
       {using enum utxt::Enc;\
        case UTF8: return L##UTF8;\
        case UTF16LE: return L##UTF16LE;\
        case UTF16BE: return L##UTF16BE;\
        case UTF32LE: return L##UTF32LE;\
        case UTF32BE: return L##UTF32BE;\
       }\
    std::unreachable();



/// Re-encode bytes

//---------------------------------------------------------------------------
// Re-encode a byte buffer from INENC to OUTENC
// const std::string out_bytes = utxt::reencode<UTF16LE,UTF8>(in_bytes);
template<utxt::Enc INENC,utxt::Enc OUTENC>
constexpr std::string reencode(const std::string_view in_bytes)
{
    std::string out_bytes;

    // utf-32 occupa in genere più bytes
    using enum utxt::Enc;
    if constexpr( INENC==UTF8 and (OUTENC==UTF32BE or OUTENC==UTF32LE) ) // cppcheck-suppress redundantCondition
       {
        out_bytes.reserve( 4 * in_bytes.size() );
       }
    else if constexpr( (INENC==UTF16BE or INENC==UTF16LE) and (OUTENC==UTF32BE or OUTENC==UTF32LE) )
       {
        out_bytes.reserve( 2 * in_bytes.size() );
       }
    else if constexpr( INENC==UTF8 and (OUTENC==UTF16BE or OUTENC==UTF16LE) ) // cppcheck-suppress redundantCondition
       {
        out_bytes.reserve( 2 * in_bytes.size() );
       }
    else
       {
        out_bytes.reserve( in_bytes.size() );
       }

    utxt::bytes_buffer_t<INENC> bytes_buf(in_bytes);
    while( bytes_buf.has_codepoint() )
       {
        append_codepoint<OUTENC>(bytes_buf.extract_codepoint(), out_bytes);
       }

    // Detect truncated
    if( bytes_buf.has_bytes() )
       {// Truncated codepoint!
        append_codepoint<OUTENC>(codepoint::invalid, out_bytes);
       }

    return out_bytes;
}

//---------------------------------------------------------------------------
// const std::string out_bytes = utxt::encode_as<utxt::Enc::UTF8>(in_bytes);
template<utxt::Enc OUTENC>
[[nodiscard]] constexpr std::string encode_as(std::string_view in_bytes, const flags_t flags =flag::NONE)
{
    const auto [in_enc, bom_size] = detect_encoding_of(in_bytes);
    if( flags & flag::SKIP_BOM )
       {
        in_bytes.remove_prefix(bom_size);
       }
    TEXT_DISPATCH_TO_ENC(in_enc, reencode<, ,OUTENC>(in_bytes))
}

//---------------------------------------------------------------------------
// const std::string out_bytes = utxt::encode_as(utxt::Enc::UTF8,in_bytes);
[[nodiscard]] constexpr std::string encode_as(const utxt::Enc out_enc, std::string_view in_bytes, const flags_t flags =flag::NONE)
{
    TEXT_DISPATCH_TO_ENC(out_enc, encode_as<, >(in_bytes,flags))
}



/// Re-encode bytes if necessary

//---------------------------------------------------------------------------
// Re-encode a byte buffer from INENC to OUTENC only if are different
template<utxt::Enc INENC,utxt::Enc OUTENC>
constexpr std::string_view reencode_if_necessary(const std::string_view in_bytes, std::string& reencoded_buf)
{
    if constexpr( INENC==OUTENC )
       {
        return in_bytes;
       }
    else
       {
        reencoded_buf = reencode<INENC,OUTENC>(in_bytes);
        return reencoded_buf;
       }
}

//---------------------------------------------------------------------------
// std::string reencoded_buf;
// const std::string_view out_bytes = utxt::encode_if_necessary_as<utxt::Enc::UTF8>(in_bytes, reencoded_buf);
template<utxt::Enc OUTENC>
[[nodiscard]] constexpr std::string_view encode_if_necessary_as(std::string_view in_bytes, std::string& reencoded_buf, const flags_t flags =flag::NONE)
{
    const auto [in_enc, bom_size] = detect_encoding_of(in_bytes);
    if( flags & flag::SKIP_BOM )
       {
        in_bytes.remove_prefix(bom_size);
       }
    TEXT_DISPATCH_TO_ENC(in_enc, reencode_if_necessary<, ,OUTENC>(in_bytes, reencoded_buf))
}

//---------------------------------------------------------------------------
// std::string reencoded_buf;
// const std::string_view out_bytes = utxt::encode_if_necessary_as(utxt::Enc::UTF8, in_bytes, reencoded_buf);
[[nodiscard]] constexpr std::string_view encode_if_necessary_as(const utxt::Enc out_enc, std::string_view in_bytes, std::string& reencoded_buf, const flags_t flags =flag::NONE)
{
    TEXT_DISPATCH_TO_ENC(out_enc, encode_if_necessary_as<, >(in_bytes, reencoded_buf, flags))
}



/// [Decode bytes to utf-32 string]

//-----------------------------------------------------------------------
template<utxt::Enc INENC>
[[nodiscard]] constexpr std::u32string to_utf32(const std::string_view bytes)
{
    std::u32string u32str;
    u32str.reserve( bytes.size() );

    utxt::bytes_buffer_t<INENC> bytes_buf(bytes);
    while( bytes_buf.has_codepoint() )
       {
        u32str.push_back( bytes_buf.extract_codepoint() );
       }

    // Detect truncated
    if( bytes_buf.has_bytes() )
       {// Truncated codepoint!
        u32str.push_back(codepoint::invalid);
       }

    return u32str;
}

[[nodiscard]] /*constexpr*/ std::u32string to_utf32(const std::u8string_view utf8str)
{
    return to_utf32<utxt::Enc::UTF8>( std::string_view(reinterpret_cast<const char*>(utf8str.data()), utf8str.size()) );
}



/// [Encode utf-32 to bytes]

//---------------------------------------------------------------------------
// Encode a char32_t sequence to OUTENC
// const std::string out_bytes = utxt::encode_as<UTF16LE>(U"abc");
//-----------------------------------------------------------------------
template<utxt::Enc OUTENC>
[[nodiscard]] constexpr std::string encode_as(const std::u32string_view u32str)
{
    std::string out_bytes;

    // Assuming the worst case: four bytes per codepoint
    out_bytes.reserve( 4 * u32str.size() );

    for( const char32_t codepoint : u32str )
       {
        append_codepoint<OUTENC>(codepoint, out_bytes);
       }

    return out_bytes;
}

//-----------------------------------------------------------------------
template<utxt::Enc OUTENC>
[[nodiscard]] constexpr std::string encode_as(const char32_t codepoint)
{
    std::string out_bytes;
    out_bytes.reserve( 4 );
    append_codepoint<OUTENC>(codepoint, out_bytes);
    return out_bytes;
}

//---------------------------------------------------------------------------
// const std::string out_bytes = utxt::encode_as(enc,U"str");
//template<typename T>
//concept a_basic_string = std::same_as<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>;
[[nodiscard]] constexpr std::string encode_as(const utxt::Enc enc, const std::u32string_view u32str)
{
    TEXT_DISPATCH_TO_ENC(enc, encode_as<, >(u32str))
}
//---------------------------------------------------------------------------
[[nodiscard]] constexpr std::string encode_as(const utxt::Enc enc, const char32_t codepoint)
{
    TEXT_DISPATCH_TO_ENC(enc, encode_as<, >(codepoint))
}



/// [Encode utf-32 to utf-8 bytes]

//-----------------------------------------------------------------------
[[nodiscard]] constexpr std::string to_utf8(const std::u32string_view u32str)
{
    return encode_as<Enc::UTF8>(u32str);
}
//-----------------------------------------------------------------------
[[nodiscard]] constexpr std::string to_utf8(const char32_t codepoint)
{
    return encode_as<Enc::UTF8>(codepoint);
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::





/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
#include <array>
using namespace std::literals; // "..."sv
static ut::suite<"utxt::"> text_tests = []
{////////////////////////////////////////////////////////////////////////////
using ut::expect;
using ut::that;
using enum utxt::Enc;


ut::test("bitwise utilities") = []
   {
    using namespace utxt::details;

    const char OxCA = '\xCA';
    const char OxFE = '\xFE';
    const std::uint16_t OxCAFE = 0xCAFE;

    expect( that % combine_chars(OxCA,OxFE) == OxCAFE );
    expect( that % high_byte_of(OxCAFE) == OxCA );
    expect( that % low_byte_of(OxCAFE) == OxFE );

    expect( that % combine_chars('\x01','\x02') == 0x0102 );
    expect( that % high_byte_of(0x0102) == '\x01' );
    expect( that % low_byte_of(0x0102) == '\x02' );

    const char OxFA = '\xFA';
    const char OxDE = '\xDE';
    const char OxBA = '\xBA';
    const char OxDA = '\xDA';
    const std::uint32_t OxFADEBADA = 0xFADEBADA;

    expect( that % combine_chars(OxFA,OxDE,OxBA,OxDA) == OxFADEBADA );
    expect( that % hh_byte_of(OxFADEBADA) == OxFA );
    expect( that % hl_byte_of(OxFADEBADA) == OxDE );
    expect( that % lh_byte_of(OxFADEBADA) == OxBA );
    expect( that % ll_byte_of(OxFADEBADA) == OxDA );

    expect( that % combine_chars('\x01','\x02','\x03','\x04') == 0x01020304u );
    expect( that % hh_byte_of(0x01020304) == '\x01' );
    expect( that % hl_byte_of(0x01020304) == '\x02' );
    expect( that % lh_byte_of(0x01020304) == '\x03' );
    expect( that % ll_byte_of(0x01020304) == '\x04' );
   };


ut::test("detect_encoding_of") = []
   {
    auto test_enc_of = [](const std::string_view bytes, const utxt::bom_ret_t expected, const char* const msg) noexcept -> void
       {
        const utxt::bom_ret_t retrieved = utxt::detect_encoding_of(bytes);
        expect( retrieved.enc==expected.enc and retrieved.bom_size==expected.bom_size ) << msg;
       };

    test_enc_of("\xEF\xBB\xBF blah"sv, {UTF8,3}, "Full utf-8 BOM should be detected\n");
    test_enc_of("blah blah"sv, {UTF8,0}, "No BOM found should imply utf-8\n");
    test_enc_of("\xEF\xBB"sv, {UTF8,0}, "Incomplete utf-8 BOM should fall back to utf-8\n");
    test_enc_of(""sv, {UTF8,0}, "Empty buffer should fall back to utf-8\n");

    test_enc_of("\xFF\xFE blah"sv, {UTF16LE,2}, "Full utf-16-le BOM should be detected\n");
    test_enc_of("\xFF blah"sv, {UTF8,0}, "Incomplete utf-16-le BOM should fall back to utf-8\n");

    test_enc_of("\xFE\xFF blah"sv, {UTF16BE,2}, "Full utf-16-be BOM should be detected\n");
    test_enc_of("\xFE blah"sv, {UTF8,0}, "Incomplete utf-16-be BOM should fall back to utf-8\n");

    test_enc_of("\xFF\xFE\0\0 blah"sv, {UTF32LE,4}, "Full utf-32-le BOM should be detected\n");
    test_enc_of("\xFF\xFE\0 blah"sv, {UTF16LE,2}, "Incomplete utf-32-le BOM should be interpreted as utf-16-le\n");

    test_enc_of("\0\0\xFE\xFF blah"sv, {UTF32BE,4}, "Full utf-32-be BOM should be detected\n");
    test_enc_of("\0\0\xFE blah"sv, {UTF8,0}, "Incomplete utf-32-be BOM should fall back to utf-8\n");
    test_enc_of("\0\xFE\xFF blah"sv, {UTF8,0}, "Invalid utf-32-be BOM should fall back to utf-8\n");
   };

ut::test("codepoints decode and encode") = []
   {
    static_assert( U'🍌'==0x1F34C );

    struct test_case_t final
       {
        const char32_t code_point;
        const std::string_view encoded_as_UTF8;
        const std::string_view encoded_as_UTF16LE;
        const std::string_view encoded_as_UTF16BE;
        const std::string_view encoded_as_UTF32LE;
        const std::string_view encoded_as_UTF32BE;

        constexpr std::string_view encoded_as(const utxt::Enc enc) const noexcept
           {
            TEXT_ATTACH_TO_ENC(enc, encoded_as_)
           }
       };
    constexpr std::array<test_case_t,4> test_cases =
       {{
         { U'a', "\x61"sv, "\x61\0"sv, "\0\x61"sv, "\x61\0\0\0"sv, "\0\0\0\x61"sv }
        ,{ U'à', "\xC3\xA0"sv, "\xE0\0"sv, "\0\xE0"sv, "\xE0\0\0\0"sv, "\0\0\0\xE0"sv }
        ,{ U'⟶', "\xE2\x9F\xB6"sv, "\xF6\x27"sv, "\x27\xF6"sv, "\xF6\x27\0\0"sv, "\0\0\x27\xF6"sv }
        ,{ U'🍌', "\xF0\x9F\x8D\x8C"sv, "\x3C\xD8\x4C\xDF"sv, "\xD8\x3C\xDF\x4C"sv, "\x4C\xF3\x01\0"sv, "\0\x01\xF3\x4C"sv }
       }};

    auto test_codepoint = []<utxt::Enc ENC>(const test_case_t& test_case) constexpr -> void
       {
        //ut::log << "Testing " << test_case.encoded_as_UTF8 << " with " << static_cast<int>(std::to_underlying(ENC));
        std::size_t pos{};
        expect( utxt::extract_codepoint<ENC>(test_case.encoded_as(ENC), pos) == test_case.code_point );
        std::string bytes;
        utxt::append_codepoint<ENC>(test_case.code_point, bytes);
        expect( that % test_case.encoded_as(ENC) == bytes );
       };

    for(const auto& test_case : test_cases)
       {
        ut::test(test_case.encoded_as_UTF8) = [&test_case, &test_codepoint]
           {
            test_codepoint.template operator()<UTF8>(test_case);
            test_codepoint.template operator()<UTF16LE>(test_case);
            test_codepoint.template operator()<UTF16BE>(test_case);
            test_codepoint.template operator()<UTF32LE>(test_case);
            test_codepoint.template operator()<UTF32BE>(test_case);
           };
       }
   };

ut::test("utxt::bytes_buffer_t") = []
   {
    utxt::bytes_buffer_t<utxt::Enc::UTF16LE> buf("\x61\x00\x62\x00\x63\x00"sv); // u"abc"
    expect( buf.has_bytes() and buf.has_codepoint() and buf.byte_pos()==0 and buf.extract_codepoint()==U'a' );
    expect( buf.has_bytes() and buf.has_codepoint() and buf.byte_pos()==2 and buf.extract_codepoint()==U'b' );
    expect( buf.has_bytes() and buf.has_codepoint() and buf.byte_pos()==4 and buf.extract_codepoint()==U'c' );
    expect( not buf.has_bytes() and not buf.has_codepoint() and buf.byte_pos()==6 );
   };

ut::test("utxt::encode_as<>(bytes)") = []
   {
    expect( utxt::encode_as<UTF8>(""sv) == ""sv) << "Implicit utf-8 empty string should be empty\n";

    struct test_case_t final
       {
        const std::string_view str;
        const std::string_view encoded_as_UTF8;
        const std::string_view encoded_as_UTF16LE;
        const std::string_view encoded_as_UTF16BE;
        const std::string_view encoded_as_UTF32LE;
        const std::string_view encoded_as_UTF32BE;

        constexpr std::string_view encoded_as(const utxt::Enc enc) const noexcept
           {
            TEXT_ATTACH_TO_ENC(enc, encoded_as_)
           }
       };
    constexpr std::array<test_case_t,2> test_cases =
       {{
         { " ab"sv,
           "\xEF\xBB\xBF ab"sv,
           "\xFF\xFE\x20\x00\x61\x00\x62\x00"sv,
           "\xFE\xFF\x00\x20\x00\x61\x00\x62"sv,
           "\xFF\xFE\x00\x00\x20\x00\x00\x00\x61\x00\x00\x00\x62\x00\x00\x00"sv,
           "\x00\x00\xFE\xFF\x00\x00\x00\x20\x00\x00\x00\x61\x00\x00\x00\x62"sv }
        ,{ "è una ⛵ ┌─┐"sv,
           "\xEF\xBB\xBF\xC3\xA8\x20\x75\x6E\x61\x20\xE2\x9B\xB5\x20\xE2\x94\x8C\xE2\x94\x80\xE2\x94\x90"sv,
           "\xFF\xFE\xE8\x00\x20\x00\x75\x00\x6E\x00\x61\x00\x20\x00\xF5\x26\x20\x00\x0C\x25\x00\x25\x10\x25"sv,
           "\xFE\xFF\x00\xE8\x00\x20\x00\x75\x00\x6E\x00\x61\x00\x20\x26\xF5\x00\x20\x25\x0C\x25\x00\x25\x10"sv,
           "\xFF\xFE\x00\x00\xE8\x00\x00\x00\x20\x00\x00\x00\x75\x00\x00\x00\x6E\x00\x00\x00\x61\x00\x00\x00\x20\x00\x00\x00\xF5\x26\x00\x00\x20\x00\x00\x00\x0C\x25\x00\x00\x00\x25\x00\x00\x10\x25\x00\x00"sv, "\x00\x00\xFE\xFF\x00\x00\x00\xE8\x00\x00\x00\x20\x00\x00\x00\x75\x00\x00\x00\x6E\x00\x00\x00\x61\x00\x00\x00\x20\x00\x00\x26\xF5\x00\x00\x00\x20\x00\x00\x25\x0C\x00\x00\x25\x00\x00\x00\x25\x10"sv }
       }};

    for(const auto& ex : test_cases)
       {
        ut::test(ex.encoded_as_UTF8) = [&ex]
           {
            expect( utxt::reencode<UTF8,UTF8>(ex.encoded_as(UTF8))==ex.encoded_as(UTF8) ) << "utf-8 to utf-8\n";
            expect( utxt::reencode<UTF16LE,UTF16LE>(ex.encoded_as(UTF16LE))==ex.encoded_as(UTF16LE) ) << "utf-16le to utf-16le\n";

            expect( utxt::encode_as<UTF8>(ex.encoded_as(UTF8))==ex.encoded_as(UTF8) ) << "utf-8 to utf-8\n";
            expect( utxt::encode_as<UTF16LE>(ex.encoded_as(UTF8))==ex.encoded_as(UTF16LE) ) << "utf-8 to utf-16le\n";
            expect( utxt::encode_as<UTF16BE>(ex.encoded_as(UTF8))==ex.encoded_as(UTF16BE) ) << "utf-8 to utf-16be\n";
            expect( utxt::encode_as<UTF32LE>(ex.encoded_as(UTF8))==ex.encoded_as(UTF32LE) ) << "utf-8 to utf-32le\n";
            expect( utxt::encode_as<UTF32BE>(ex.encoded_as(UTF8))==ex.encoded_as(UTF32BE) ) << "utf-8 to utf-32be\n";

            expect( utxt::encode_as<UTF8>(ex.encoded_as(UTF16LE))==ex.encoded_as(UTF8) ) << "utf-16le to utf-8\n";
            expect( utxt::encode_as<UTF16LE>(ex.encoded_as(UTF16LE))==ex.encoded_as(UTF16LE) ) << "utf-16le to utf-16le\n";
            expect( utxt::encode_as<UTF16BE>(ex.encoded_as(UTF16LE))==ex.encoded_as(UTF16BE) ) << "utf-16le to utf-16be\n";
            expect( utxt::encode_as<UTF32LE>(ex.encoded_as(UTF16LE))==ex.encoded_as(UTF32LE) ) << "utf-16le to utf-32le\n";
            expect( utxt::encode_as<UTF32BE>(ex.encoded_as(UTF16LE))==ex.encoded_as(UTF32BE) ) << "utf-16le to utf-32be\n";

            expect( utxt::encode_as<UTF8>(ex.encoded_as(UTF16BE))==ex.encoded_as(UTF8) ) << "utf-16be to utf-8\n";
            expect( utxt::encode_as<UTF16LE>(ex.encoded_as(UTF16BE))==ex.encoded_as(UTF16LE) ) << "utf-16be to utf-16le\n";
            expect( utxt::encode_as<UTF16BE>(ex.encoded_as(UTF16BE))==ex.encoded_as(UTF16BE) ) << "utf-16be to utf-16be\n";
            expect( utxt::encode_as<UTF32LE>(ex.encoded_as(UTF16BE))==ex.encoded_as(UTF32LE) ) << "utf-16be to utf-32le\n";
            expect( utxt::encode_as<UTF32BE>(ex.encoded_as(UTF16BE))==ex.encoded_as(UTF32BE) ) << "utf-16be to utf-32be\n";

            expect( utxt::encode_as<UTF8>(ex.encoded_as(UTF32LE))==ex.encoded_as(UTF8) ) << "utf-32le to utf-8\n";
            expect( utxt::encode_as<UTF16LE>(ex.encoded_as(UTF32LE))==ex.encoded_as(UTF16LE) ) << "utf-32le to utf-16le\n";
            expect( utxt::encode_as<UTF16BE>(ex.encoded_as(UTF32LE))==ex.encoded_as(UTF16BE) ) << "utf-32le to utf-16be\n";
            expect( utxt::encode_as<UTF32LE>(ex.encoded_as(UTF32LE))==ex.encoded_as(UTF32LE) ) << "utf-32le to utf-32le\n";
            expect( utxt::encode_as<UTF32BE>(ex.encoded_as(UTF32LE))==ex.encoded_as(UTF32BE) ) << "utf-32le to utf-32be\n";

            expect( utxt::encode_as<UTF8>(ex.encoded_as(UTF32BE))==ex.encoded_as(UTF8) ) << "utf-32be to utf-8\n";
            expect( utxt::encode_as<UTF16LE>(ex.encoded_as(UTF32BE))==ex.encoded_as(UTF16LE) ) << "utf-32be to utf-16le\n";
            expect( utxt::encode_as<UTF16BE>(ex.encoded_as(UTF32BE))==ex.encoded_as(UTF16BE) ) << "utf-32be to utf-16be\n";
            expect( utxt::encode_as<UTF32LE>(ex.encoded_as(UTF32BE))==ex.encoded_as(UTF32LE) ) << "utf-32be to utf-32le\n";
            expect( utxt::encode_as<UTF32BE>(ex.encoded_as(UTF32BE))==ex.encoded_as(UTF32BE) ) << "utf-32be to utf-32be\n";
           };
       }
   };

ut::test("utxt::to_utf8") = []
   {
    expect( utxt::to_utf8(U""sv)==""sv );
    expect( utxt::to_utf8(U"aà⟶♥♫"sv)=="aà⟶♥♫"sv );
    expect( utxt::to_utf8(U'⛵')=="⛵"sv );
   };

ut::test("utxt::to_utf32") = []
   {
    expect( utxt::to_utf32(u8""sv)==U""sv );
    expect( utxt::to_utf32(u8"aà⟶♥♫"sv)==U"aà⟶♥♫"sv );
   };

ut::test("utxt::encode_as(enc,...)") = []
   {
    expect( utxt::encode_as(UTF8,U""sv)==""sv );
    expect( utxt::encode_as(UTF8,U"aà⟶♥♫"sv)=="aà⟶♥♫"sv );
    expect( utxt::encode_as(UTF8,U'⛵')=="⛵"sv );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
