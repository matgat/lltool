#pragma once
//  ---------------------------------------------
//  Facility to write to file
//  #include "file_write.hpp" // sys::file_write()
//  ---------------------------------------------
#include <cassert>
#include <stdexcept>
#include <string_view>
#include <cstdio> // std::fopen, ::fopen_s (Microsoft)

#include <fmt/format.h> // fmt::format
#include "os-detect.hpp" // MS_WINDOWS, POSIX



//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
// (Over)Write a file
class file_write final
{
 private:
    std::FILE* m_fstream = nullptr;

 public:
    enum flags_t : char
    {
       NONE = 0x0
      ,APPEND = 0x1
    };

    explicit file_write(const char* const pth_cstr, const flags_t flags =NONE)
      : m_fstream( file_open(pth_cstr, (flags & file_write::APPEND) ? "ab" : "wb") )
       {
        if( not m_fstream )
           {
            throw std::runtime_error{ fmt::format("Cannot write to file {}", pth_cstr) };
           }
       }

    ~file_write() noexcept
       {
        if( m_fstream )
           {
            std::fclose(m_fstream);
           }
       }

    file_write(const file_write&) = delete; // Prevent copy
    file_write& operator=(const file_write&) = delete;

    file_write(file_write&& other) noexcept
      : m_fstream(other.m_fstream)
       {
        other.m_fstream = nullptr;
       }

    file_write& operator=(file_write&& other) noexcept
       {
        std::swap(m_fstream, other.m_fstream);
        return *this;
       }


    void set_buffer_size(const std::size_t siz =BUFSIZ) noexcept
       {
        std::setvbuf(m_fstream, nullptr, siz>0 ? _IOFBF : _IONBF, siz);
       }


    const file_write& operator<<(const char c) const noexcept
       {
        assert(m_fstream!=nullptr);
        std::fputc(c, m_fstream);
        return *this;
       }

    const file_write& operator<<(const std::string_view sv) const noexcept
       {
        assert(m_fstream!=nullptr);
        std::fwrite(sv.data(), sizeof(std::string_view::value_type), sv.length(), m_fstream);
        return *this;
       }

 private:
    [[nodiscard]] static inline std::FILE* file_open( const char* const filename, const char* const mode ) noexcept
       {
      #if defined(MS_WINDOWS)
        std::FILE* f;
        const errno_t err = ::fopen_s(&f, filename, mode);
        if(err) f = nullptr;
      #elif defined(POSIX)
        std::FILE* const f = std::fopen(filename, mode);
      #endif
        return f;
       }
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"sys::file_write"> file_write_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("write and verify") = []
   {
    test::TemporaryFile file("~file_write.tmp");
    const std::string_view content = "123456"sv;

       {sys::file_write out{ file.path().string().c_str() };
        out << content; }

    ut::expect( file.has_content( content) );
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
