#pragma once
//  ---------------------------------------------
//  test facilities
//  This code should be already proven good
//  and shouldn't depend on tested units
//  ---------------------------------------------
//  #include "test_facilities.hpp" // test::*
//  ---------------------------------------------
#include <stdexcept>
#include <vector>
#include <cctype> // std::tolower
#include <string>
#include <string_view>
#include <ranges> // std::ranges::sort
#include <thread> // std::this_thread
#include <chrono> // std::chrono::*
#include <fstream> // std::ifstream, std::ofstream
#include <filesystem> // std::filesystem

#include <fmt/format.h> // fmt::format

namespace fs = std::filesystem;
using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace test //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//-----------------------------------------------------------------------
[[nodiscard]] bool compare_nocase(const std::string_view sv1, const std::string_view sv2) noexcept
{
    if( sv1.size()!=sv2.size() ) return false;
    for( std::size_t i=0; i<sv1.size(); ++i )
        if( std::tolower(static_cast<unsigned char>(sv1[i])) !=
            std::tolower(static_cast<unsigned char>(sv2[i])) ) return false;
    return true;
}

//-----------------------------------------------------------------------
[[nodiscard]] constexpr std::string tolower(std::string s) noexcept
{
    std::ranges::transform(s, s.begin(), [](const char ch) noexcept { return static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); });
    return s;
}


//---------------------------------------------------------------------------
template<typename T>
[[nodiscard]] bool have_same_elements(std::vector<T>&& v1, std::vector<T>&& v2)
{
    std::ranges::sort(v1);
    std::ranges::sort(v2);
    return v1 == v2;
}

//---------------------------------------------------------------------------
template<typename T>
[[nodiscard]] bool have_same_elements(std::vector<T>&& v, std::initializer_list<T> lst)
{
    return have_same_elements<T>( std::move(v), std::vector<T>{lst});
}


//---------------------------------------------------------------------------
template<typename T>
[[nodiscard]] std::vector<T> join(const std::vector<T>& v1, const std::vector<T>& v2)
{
    std::vector<T> v_tot{ v1 };
  #ifdef __cpp_lib_containers_ranges
    v_tot.append_range(v2);
  #else
    v_tot.reserve(v_tot.size() + v2.size());
    v_tot.insert(v_tot.end(), v2.cbegin(), v2.cend());
  #endif
    return v_tot;
}


//---------------------------------------------------------------------------
void sleep_for_seconds(const unsigned int t_s)
{
    std::this_thread::sleep_for( std::chrono::seconds(t_s) );
}


//---------------------------------------------------------------------------
[[nodiscard]] std::string generate_unique_timestamp(const std::string_view prefix)
{
    const auto milliseconds_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return fmt::format("{}{:0>10}.{:0>3}", prefix, milliseconds_epoch/1000, milliseconds_epoch % 1000);
}


//---------------------------------------------------------------------------
[[nodiscard]] std::string read_file_content(const std::string& file_name)
{
    std::ifstream file(file_name);
    return std::string{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

//---------------------------------------------------------------------------
void write_to_file(const std::string& file_name, const std::string_view content)
{
    std::ofstream file(file_name);
    file << content;
}

//---------------------------------------------------------------------------
void append_to_file(const std::string& file_name, const std::string_view content)
{
    std::ofstream file(file_name, std::ios::app);
    file << content;
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
        write_to_file(m_path.string(), content);
       }

    [[nodiscard]] const fs::path& path() const noexcept
       {
        return m_path;
       }

    [[nodiscard]] bool has_content(const std::string_view given_content) const
       {
        return read_file_content(m_path.string()) == given_content;
       }

    void operator<<(const std::string_view content) const
       {
        append_to_file(m_path.string(), content);
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

    [[nodiscard]] const fs::path& path() const noexcept
       {
        return m_dirpath;
       }

    [[nodiscard]] fs::path build_file_path(const std::string_view name) const
       {
        return m_dirpath / name;
       }

    [[maybe_unused]] File add_file(const std::string_view name, const std::string_view content) const
       {
        return File(build_file_path(name), content);
       }
};

}//::::::::::::::::::::::::::::::::::: test :::::::::::::::::::::::::::::::::
