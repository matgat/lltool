#include <stdexcept> // std::runtime_error
#include <string>
#include <string_view>
#include <fmt/format.h> // fmt::*
#include <filesystem> // std::filesystem
namespace fs = std::filesystem;
using namespace std::literals; // "..."sv

#include "args_extractor.hpp" // MG::args_extractor
#include "issues_collector.hpp" // MG::issues
#include "edit_text_file.hpp" // sys::edit_text_file()

#include "project_updater.hpp" // ll::update_project_libraries()

constexpr std::string_view app_name = "lltool"sv;
constexpr std::string_view app_descr = "A tool capable to manipulate LogicLab files"sv;


/////////////////////////////////////////////////////////////////////////////
class Arguments final
{
    class task_t final
    {
        enum en_task_t : char { NONE, UPDATE, CONVERT} m_value = NONE;

     public:
        void set_as_update() noexcept { m_value=UPDATE; }
        void set_as_convert() noexcept { m_value=CONVERT; }

        [[nodiscard]] bool is_update() const noexcept { return m_value==UPDATE; }
        [[nodiscard]] bool is_convert() const noexcept { return m_value==CONVERT; }
    };

 private:
    fs::path m_prj_path;
    fs::path m_out_path;
    task_t m_task;
    bool m_verbose = false;
    bool m_quiet = false;

 public:
    [[nodiscard]] const fs::path& prj_path() const noexcept { return m_prj_path; }
    [[nodiscard]] const fs::path& out_path() const noexcept { return m_out_path; }
    [[nodiscard]] const task_t& task() const noexcept { return m_task; }
    [[nodiscard]] bool verbose() const noexcept { return m_verbose; }
    [[nodiscard]] bool quiet() const noexcept { return m_quiet; }

 public:
    void parse(const int argc, const char* const argv[])
       {
        MG::args_extractor args(argc, argv);

        if( args.has_current() )
           {// As first argument I expect a task
            std::string_view arg = args.current();
            if( arg=="update"sv )
               {
                m_task.set_as_update();
               }
            else if( arg=="convert"sv )
               {
                m_task.set_as_convert();
               }
            else if( args.is_switch(arg) )
               {
                recognize_switch( args.extract_switch(arg) );
               }
            else
               {
                throw std::invalid_argument( fmt::format("Unrecognized task: {}", arg) );
               }
            args.next();

            while( args.has_current() )
               {
                arg = args.current();
                if( args.is_switch(arg) )
                   {
                    const std::string_view swtch = args.extract_switch(arg);
                    if( swtch=="out"sv || swtch=="o"sv )
                       {
                        args.next(); // Expecting an output path next
                        if( !args.has_current() )
                           {
                            throw std::invalid_argument("Missing output path");
                           }
                        if( not m_out_path.empty() )
                           {
                            throw std::invalid_argument( fmt::format("Output was already set to {}", m_out_path.string()) );
                           }
                        const std::string_view should_be_out_path = args.current();
                        if( args.is_switch(should_be_out_path) )
                           {
                            throw std::invalid_argument( fmt::format("Missing output path before {}", should_be_out_path) );
                           }
                        m_out_path = should_be_out_path;
                       }
                    else
                       {
                        recognize_switch(swtch);
                       }
                   }
                else
                   {// Expecting input path
                    if( task().is_update() )
                       {// Must be the project path
                        if( not m_prj_path.empty() )
                           {// This must be the project path
                            throw std::invalid_argument( fmt::format("Project file was already set to {}", m_prj_path.string()) );
                           }

                        m_prj_path = arg;
                        if( not fs::exists(m_prj_path) )
                           {
                            throw std::invalid_argument( fmt::format("Project file not found: {}", m_prj_path.string()) );
                           }
                       }
                   }
                args.next();
               }
           }

        check_if_ok();
       }

    void check_if_ok()
       {
        if( task().is_update() )
           {
            if( m_prj_path.empty() )
               {
                throw std::invalid_argument("Project file not given");
               }
            if( fs::exists(m_out_path) && fs::equivalent(m_prj_path, m_out_path) )
               {
                throw std::invalid_argument( fmt::format("Output file \"{}\" collides with project file, if your intent is overwrite, just don't specify it", m_out_path.string()) );
               }
           }
        else if( task().is_convert() )
           {
            throw std::invalid_argument("\"convert\" not yet ready");
           }
        else
           {
            throw std::invalid_argument("No task selected");
           }
       }

    static void print_help() noexcept
       {
        fmt::print( "\n{} (ver. " __DATE__ ")\n"
                    "{}\n"
                    "\n", app_name, app_descr );
       }

    static void print_usage() noexcept
       {
        fmt::print( "\nUsage:\n"
                    "   {0} update path/to/project.ppjs\n"
                    "   {0} convert -o path/to/dir path/to/*.h path/to/*.pll\n"
                    "       --out/-o (Specify output file/directory)\n"
                    "       --verbose/-v (Print more info on stdout)\n"
                    "       --quiet/-q (No user interaction)\n"
                    "       --help/-h (Print this help)\n"
                    "\n", app_name );
       }
 private:
    void recognize_switch(const std::string_view swtch)
       {
        if( swtch=="verbose"sv || swtch=="v"sv )
           {
            m_verbose = true;
           }
        else if( swtch=="quiet"sv || swtch=="q"sv )
           {
            m_quiet = true;
           }
        else if( swtch=="help"sv || swtch=="h"sv )
           {
            print_help();
            throw std::invalid_argument("Exiting after printing help");
           }
        else
           {
            throw std::invalid_argument( fmt::format("Unknown command switch: {}", swtch) );
           }
       }
};



//---------------------------------------------------------------------------
int main( const int argc, const char* const argv[] )
{
    Arguments args;
    try{
        args.parse(argc, argv);
        if( args.verbose() )
           {
            fmt::print( "---- {} (ver. " __DATE__ ") ----\n", app_name );
            fmt::print( "Updating project {}\n", args.prj_path().string() );
           }

        MG::issues issues;
        if( args.task().is_update() )
           {
            ll::update_project_libraries(args.prj_path(), args.out_path(), std::ref(issues));
           }
        else if( args.task().is_convert() )
           {
           }

        if( issues.size()>0 )
           {
            if( args.verbose() or not args.quiet() )
               {
                fmt::print("[!] {} issues found\n", issues.size());
                for( const auto& issue : issues )
                   {
                    fmt::print("    {}\n", issue);
                   }
               }
            return 1;
           }

        return 0;
       }

    catch( std::invalid_argument& e )
       {
        fmt::print("!! {}\n", e.what());
        if( not args.quiet() )
           {
            Arguments::print_usage();
           }
       }

    catch( text::parse_error& e)
       {
        fmt::print("!! {} ({}:{})\n", e.what(), e.file(), e.line());
        if( not args.quiet() )
           {
            sys::edit_text_file( e.file(), e.line() );
           }
       }

    catch( std::exception& e )
       {
        fmt::print("!! {}\n", e.what());
       }

    return 2;
}
