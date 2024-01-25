#pragma once
//  ---------------------------------------------
//  Handle glob patterns
//  ---------------------------------------------
//  #include "file_globbing.hpp" // MG::file_glob()
//  ---------------------------------------------
#include <stdexcept>
#include <vector>
#include <filesystem> // std::filesystem

#include "globbing.hpp" // MG::glob_match()

namespace fs = std::filesystem;


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace MG //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
[[nodiscard]] std::vector<fs::path> file_glob(const fs::path& globbed_path)
{
    std::vector<fs::path> unglobbed_paths;

    fs::path parent_folder = globbed_path.parent_path();
    if( parent_folder.empty() )
       {
        parent_folder = fs::current_path();
       }
    if( contains_wildcards(parent_folder.string()) )
       {
        throw std::runtime_error("MG::file_glob(): Wildcards in directories not supported");
       }

    const std::string globbed_fname = globbed_path.filename().string();
    if( contains_wildcards(globbed_fname) and fs::exists(parent_folder) )
       {
        unglobbed_paths.reserve(16); // Guess to minimize initial allocations
        for( const fs::directory_entry& ientry : fs::directory_iterator(parent_folder, fs::directory_options::follow_directory_symlink |
                                                                                       fs::directory_options::skip_permission_denied) )
           {// Collect if matches
            if( ientry.is_regular_file() and glob_match(ientry.path().filename().string(), globbed_fname, '/') )
               {
                unglobbed_paths.push_back( ientry.path() );
               }
           }
       }
    else
       {// Nothing to glob
        unglobbed_paths.reserve(1);
        unglobbed_paths.push_back(globbed_path);
       }

    return unglobbed_paths;
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
static ut::suite<"file_globbing"> file_globbing_tests = []
{////////////////////////////////////////////////////////////////////////////

    ut::test("MG::file_glob()") = []
       {
        test::TemporaryDirectory dir;
        const fs::path abcd = dir.add_file("abcd", "_").path();
        const fs::path a = dir.add_file("a", "_").path();
        const fs::path abcd_txt = dir.add_file("abcd.txt", "_").path();
        const fs::path abcdef_txt = dir.add_file("abcdef.txt", "_").path();
        const fs::path a_txt = dir.add_file("a.txt", "_").path();
        const fs::path b_txt = dir.add_file("b.txt", "_").path();
        const fs::path aa_txt = dir.add_file("aa.txt", "_").path();
        const fs::path aa_cfg = dir.add_file("aa.cfg", "_").path();

        ut::should("match all txt") = [&]
           {
            ut::expect( test::have_same_elements(MG::file_glob(dir.path() / "*.txt"), {abcd_txt, abcdef_txt, a_txt, b_txt, aa_txt}) );
           };

        ut::should("match all t?t") = [&]
           {
            ut::expect( test::have_same_elements(MG::file_glob(dir.path() / "*.t?t"), {abcd_txt, abcdef_txt, a_txt, b_txt, aa_txt}) );
           };

        ut::should("match txt starting with a") = [&]
           {
            ut::expect( test::have_same_elements(MG::file_glob(dir.path() / "a*.txt"), {abcd_txt, abcdef_txt, a_txt, aa_txt}) );
           };

        ut::should("match containing a") = [&]
           {
            ut::expect( test::have_same_elements(MG::file_glob(dir.path() / "*a*.*"), {abcd_txt, abcdef_txt, a_txt, aa_txt, aa_cfg}) );
           };

        ut::should("match starting with a followed by a char") = [&]
           {
            ut::expect( test::have_same_elements(MG::file_glob(dir.path() / "a?.*"), {aa_txt, aa_cfg}) );
           };

        ut::should("match txt containing b") = [&]
           {
            ut::expect( test::have_same_elements(MG::file_glob(dir.path() / "*b*.txt"), {abcd_txt, abcdef_txt, b_txt}) );
           };

        ut::should("match txt containing bc followed by just a char") = [&]
           {
            ut::expect( test::have_same_elements(MG::file_glob(dir.path() / "*bc?.txt"), {abcd_txt}) );
           };
       };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
