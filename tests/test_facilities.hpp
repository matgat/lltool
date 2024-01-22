#pragma once
//  ---------------------------------------------
//  test facilities
//  #include "test_facilities.hpp" // test::*
//  ---------------------------------------------
#include <stdexcept>
#include <string_view>
#include <cstdlib> // std::system()
#include <thread>
#include <chrono>
#include <filesystem> // std::filesystem

#include <fmt/format.h> // fmt::format

#include "string_utilities.hpp" // str::*
//#include "system_process.hpp" // sys::*
#include "file_write.hpp" // sys::file_write()
#include "memory_mapped_file.hpp" // sys::memory_mapped_file

namespace fs = std::filesystem;
using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace test //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
void sleep_for_seconds(const unsigned int t_s)
{
    std::this_thread::sleep_for( std::chrono::seconds(t_s) );
}


//---------------------------------------------------------------------------
[[maybe_unused]] int execute_wait(const char* const cmd)
{
    return std::system( cmd );
}

//---------------------------------------------------------------------------
[[maybe_unused]] int execute_wait(const fs::path& exe, std::initializer_list<std::string_view> args ={})
{
    return execute_wait( fmt::format("{} {}", exe.string(), str::join_left(' ', args)).c_str() );
}


//---------------------------------------------------------------------------
std::string generate_unique_timestamp(const std::string_view prefix)
{
    const auto milliseconds_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return fmt::format("{}{:0>10}.{:0>3}", prefix, milliseconds_epoch/1000, milliseconds_epoch % 1000);
}


//---------------------------------------------------------------------------
[[nodiscard]] bool have_same_content(const fs::path& path1, const fs::path& path2)
{
    const sys::memory_mapped_file content1{ path1.string().c_str() };
    const sys::memory_mapped_file content2{ path2.string().c_str() };
    return content1.as_string_view() == content2.as_string_view();
}



/////////////////////////////////////////////////////////////////////////////
class File
{
 private:
    fs::path m_path;

 public:
    explicit File(fs::path&& pth)
      : m_path( std::move(pth) )
       {
        if( fs::exists(m_path) )
           {
            throw std::runtime_error( fmt::format("Temporary file {} already existing!", m_path.string()) );
           }
       }

    explicit File(fs::path&& pth, const std::string_view content)
      : File(std::move(pth))
       {
        sys::file_write fw(m_path.string().c_str());
        fw << content;
       }

    [[nodiscard]] const fs::path& path() const noexcept
       {
        return m_path;
       }

    [[nodiscard]] bool has_content(const std::string_view given_content) const
       {
        const sys::memory_mapped_file file_content{ m_path.string().c_str() };
        return file_content.as_string_view() == given_content;
       }

    void operator<<(const std::string_view content) const
       {
        sys::file_write fw(m_path.string().c_str(), sys::file_write::APPEND);
        fw << content;
       }

    void remove() noexcept
       {
        if( fs::exists(m_path) )
           {
            std::error_code ec;
            fs::remove(m_path, ec);
           }
        m_path.clear();
       }
};


/////////////////////////////////////////////////////////////////////////////
class TemporaryFile final : public File
{
 public:
    explicit TemporaryFile(const std::string_view name ="~file.tmp")
      : File(get_directory() / name)
       {
        if( fs::exists(path()) )
           {
            throw std::runtime_error( fmt::format("Temporary file {} already existing!", path().string()) );
           }
       }

    explicit TemporaryFile(const std::string_view name, const std::string_view content)
      : TemporaryFile(name)
       {
        operator<<(content);
       }

    ~TemporaryFile() noexcept
       {
        remove();
       }

 private:
    static fs::path get_directory() noexcept
       {
        std::error_code ec;
        fs::path dir_path = fs::temp_directory_path(ec);
        if(ec) dir_path = fs::current_path();
        return dir_path;
       }
};


/////////////////////////////////////////////////////////////////////////////
class TemporaryDirectory final
{
 private:
    fs::path m_dirpath;

 public:
    explicit TemporaryDirectory()
     : m_dirpath( fs::temp_directory_path() / generate_unique_timestamp("~tmp_"sv) )
       {
        if( fs::exists(m_dirpath) )
           {
            throw std::runtime_error( fmt::format("Temporary directory {} already existing!", m_dirpath.string()) );
           }
        fs::create_directory(m_dirpath);
       }

    ~TemporaryDirectory() noexcept
       {
        std::error_code ec;
        fs::remove_all(m_dirpath, ec);
       }

    TemporaryDirectory(const TemporaryDirectory&) = delete; // Prevent copy
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;


    [[nodiscard]] fs::path build_file_path(const std::string_view name) const
       {
        return m_dirpath / name;
       }

    [[maybe_unused]] File add_file(const std::string_view name, const std::string_view content) const
       {
        return File(build_file_path(name), content);
       }
};



}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
