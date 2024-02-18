#include <stdexcept> // std::runtime_error
#include <string>
#include <string_view>
#include <ranges> // std::ranges::any_of
#include <filesystem> // std::filesystem
namespace fs = std::filesystem;
using namespace std::literals; // "..."sv

#include <fmt/format.h> // fmt::*

#include "args_extractor.hpp" // MG::args_extractor
#include "issues_collector.hpp" // MG::issues
#include "keyvals.hpp" // MG::keyvals
#include "edit_text_file.hpp" // sys::edit_text_file()
#include "file_globbing.hpp" // MG::file_glob()
#include "has_duplicate_basenames.hpp" // MG::find_duplicate_basename()

#include "project_updater.hpp" // ll::update_project_libraries()
#include "libraries_converter.hpp" // ll::convert_libraries()

constexpr std::string_view app_name = "lltool"sv;
constexpr std::string_view app_descr = "A tool capable to manipulate LogicLab files"sv;


/////////////////////////////////////////////////////////////////////////////
class Arguments final
{
    class task_t final
    {
        enum en_task_t : char { NONE, UPDATE, CONVERT } m_value = NONE;

     public:
        void set_as_update() noexcept { m_value=UPDATE; }
        void set_as_convert() noexcept { m_value=CONVERT; }

        [[nodiscard]] bool is_update() const noexcept { return m_value==UPDATE; }
        [[nodiscard]] bool is_convert() const noexcept { return m_value==CONVERT; }
    };

 private:
    fs::path m_prj_path;
    std::vector<fs::path> m_input_files;
    fs::path m_out_path;
    MG::keyvals m_options;
    task_t m_task;
    bool m_verbose = false; // More info to stdout
    bool m_quiet = false; // No user interaction
    bool m_force = false; // Overwrite or clear existing output files

 public:
    [[nodiscard]] const auto& prj_path() const noexcept { return m_prj_path; }
    [[nodiscard]] const auto& input_files() const noexcept { return m_input_files; }
    [[nodiscard]] const auto& out_path() const noexcept { return m_out_path; }
    [[nodiscard]] const auto& options() const noexcept { return m_options; }
    [[nodiscard]] const auto& task() const noexcept { return m_task; }
    [[nodiscard]] bool verbose() const noexcept { return m_verbose; }
    [[nodiscard]] bool quiet() const noexcept { return m_quiet; }
    [[nodiscard]] bool overwrite_existing() const noexcept { return m_force; }

 public:
    //-----------------------------------------------------------------------
    void parse(const int argc, const char* const argv[])
       {
        MG::args_extractor args(argc, argv);
        args.apply_switch_by_name_or_char = [this](const std::string_view full_name, const char brief_name) { apply_switch(full_name,brief_name); };

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
            else if( arg=="help"sv )
               {
                print_help_and_exit();
               }
            else if( args.is_switch(arg) )
               {
                args.apply_switch(arg);
               }
            else
               {
                throw std::invalid_argument{ fmt::format("Unrecognized task: {}", arg) };
               }
            args.next();

            while( args.has_current() )
               {
                arg = args.current();
                if( args.is_switch(arg) )
                   {
                    if( arg=="--to"sv or arg=="--out"sv or arg=="-o"sv )
                       {
                        const std::string_view str = args.get_next_value_of(arg);
                        if( not m_out_path.empty() )
                           {
                            throw std::invalid_argument{ fmt::format("Output was already set to {}", m_out_path.string()) };
                           }
                        m_out_path = str;
                       }
                    else if( arg=="--options"sv or arg=="-p"sv )
                       {
                        m_options.assign( args.get_next_value_of(arg) );
                       }
                    else
                       {
                        args.apply_switch(arg);
                       }
                   }
                else
                   {// Expecting input path
                    if( task().is_update() )
                       {// Must be the project path
                        if( not m_prj_path.empty() )
                           {// This must be the project path
                            throw std::invalid_argument{ fmt::format("Project file was already set to {}", m_prj_path.string()) };
                           }

                        m_prj_path = arg;
                        if( not fs::exists(m_prj_path) )
                           {
                            throw std::invalid_argument{ fmt::format("Project file not found: {}", m_prj_path.string()) };
                           }
                       }
                    else if( task().is_convert() )
                       {// Must be the input file(s)
                      #ifdef __cpp_lib_containers_ranges
                        m_input_files.append_range( MG::file_glob( fs::path(arg) ) );
                      #else
                        auto input_paths = MG::file_glob( fs::path(arg) );
                        m_input_files.reserve(m_input_files.size() + input_paths.size());
                        m_input_files.insert(m_input_files.end(), input_paths.cbegin(), input_paths.cend());
                      #endif
                       }
                   }
                args.next();
               }
           }

        check_if_ok();
       }

    //-----------------------------------------------------------------------
    void check_if_ok()
       {
        if( task().is_update() )
           {
            if( prj_path().empty() )
               {
                throw std::invalid_argument{"Project file not given"};
               }

            if( fs::exists(out_path()) )
               {// Specified an existing output path
                if( fs::is_directory(out_path()) )
                   {
                    m_out_path /= prj_path().filename();
                   }
                // I'll ensure to not overwrite an existing file
                if( fs::equivalent(prj_path(), out_path()) )
                   {
                    throw std::invalid_argument{ fmt::format("Output file \"{}\" collides with project file, if your intent is overwrite don't specify output", out_path().string()) };
                   }
                else if( not overwrite_existing() )
                   {
                    throw std::invalid_argument{ fmt::format("Won't overwrite existing file \"{}\" unless you explicitly tell me to", out_path().string()) };
                   }
               }
           }
        else if( task().is_convert() )
           {
            if( input_files().empty() )
               {
                throw std::invalid_argument{"No input files given"};
               }

            else if( input_files().size()>1  )
               {// If converting multiple files...
                //...An output directory must be specified
                if( out_path().empty() )
                   {
                    throw std::invalid_argument{"Output directory not given"};
                   }
                else if( fs::exists(out_path()) and fs::is_directory(out_path()) )
                   {//...And can't be the directory of an input file
                    if( std::ranges::any_of(input_files(), [this](const fs::path& input_file_path){return fs::equivalent(out_path(), input_file_path.parent_path());}) )
                       {
                        throw std::runtime_error{ fmt::format("Output directory \"{}\" can't contain input files", out_path().string()) };
                       }
                   }
                // Detect input files name clashes
                if( const auto dup = MG::find_duplicate_basename(input_files()); dup.has_value() )
                   {
                    throw std::runtime_error{ fmt::format("Two or more input files named \"{}\"", dup.value()) };
                   }
               }
           }
        else
           {
            throw std::invalid_argument{"No task selected"};
           }
       }

    //-----------------------------------------------------------------------
    static void print_help_and_exit()
       {
        fmt::print( "\n{} (ver. " __DATE__ ")\n"
                    "{}\n"
                    "\n", app_name, app_descr );
        // The following triggers print_usage()
        throw std::invalid_argument{"Exiting after printing help"};
       }

    //-----------------------------------------------------------------------
    static void print_usage()
       {
        fmt::print( "\nUsage:\n"
                    "   {0} [convert|update|help] [switches] [path(s)]\n"
                    "   {0} convert path/to/*.h --force -to path/to/dir\n"
                    "   {0} update --verbose path/to/project.ppjs\n"
                    "       --to/--out/-o (Specify output file/directory)\n"
                    "       --options/-p (Specify comma separated key:value)\n"
                    "       --force/-F (Overwrite/clear output files)\n"
                    "       --verbose/-v (Print more info on stdout)\n"
                    "       --quiet/-q (No user interaction)\n"
                    "\n", app_name );
       }

 private:
    //-----------------------------------------------------------------------
    void apply_switch(const std::string_view full_name, const char brief_name)
       {
        if( full_name=="force"sv or brief_name=='F' )
           {
            m_force = true;
           }
        else if( full_name=="verbose"sv or brief_name=='v' )
           {
            m_verbose = true;
           }
        else if( full_name=="quiet"sv or brief_name=='q' )
           {
            m_quiet = true;
           }
        else if( full_name=="help"sv or brief_name=='h' )
           {
            print_help_and_exit();
           }
        else
           {
            if( full_name.empty() ) throw std::invalid_argument{ fmt::format("Unknown switch: '{}'", brief_name) };
            else                    throw std::invalid_argument{ fmt::format("Unknown switch: \"{}\"", full_name) };
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
            fmt::print("---- {} (ver. " __DATE__ ") ----\n", app_name);
           }

        MG::issues issues;
        if( args.task().is_update() )
           {
            if( args.verbose() )
               {
                fmt::print("\nUpdating project {}\n", args.prj_path().string());
               }
            ll::update_project_libraries(args.prj_path(), args.out_path(), std::ref(issues));
           }
        else if( args.task().is_convert() )
           {
            if( args.input_files().size()>1 )
               {
                ll::prepare_output_dir(args.out_path(), args.overwrite_existing(), std::ref(issues));
               }

            for( const fs::path& input_file_path : args.input_files() )
               {
                if( args.verbose() )
                   {
                    fmt::print("\nConverting {}\n", input_file_path.string());
                   }
                ll::convert_library(input_file_path, args.out_path(), args.overwrite_existing(), args.options(), std::ref(issues));
               }
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

    catch( parse::error& e)
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
