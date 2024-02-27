#include <stdexcept> // std::exception, std::invalid_argument
#include <print>

#include "arguments.hpp" // lltool::Arguments
#include "issues_collector.hpp" // MG::issues
#include "edit_text_file.hpp" // sys::edit_text_file()

#include "project_updater.hpp" // ll::update_project_libraries()
#include "libraries_converter.hpp" // ll::convert_libraries()


//---------------------------------------------------------------------------
int main( const int argc, const char* const argv[] )
{
    lltool::Arguments args;
    try{
        args.parse(argc, argv);
        if( args.verbose() )
           {
            std::print("---- {} (build " __DATE__ ") ----\n", lltool::app_name);
           }

        MG::issues issues;
        if( args.task().is_update() )
           {
            if( args.verbose() )
               {
                std::print("Updating project {}\n", args.prj_path().string());
               }
            ll::update_project_libraries(args.prj_path(), args.out_path(), std::ref(issues));
           }
        else if( args.task().is_convert() )
           {
            if( args.input_files().size()>1 )
               {
                ll::prepare_output_dir(args.out_path(), args.overwrite_existing(), std::ref(issues));
               }

            for( const auto& input_file_path : args.input_files() )
               {
                if( args.verbose() )
                   {
                    std::print("Converting {}\n", input_file_path.string());
                   }
                ll::convert_library(input_file_path, args.out_path(), args.overwrite_existing(), args.options(), std::ref(issues));
               }
           }

        if( issues.size()>0 )
           {
            for( const auto& issue : issues )
               {
                std::print("! {}\n", issue);
               }
            return 1;
           }

        return 0;
       }

    catch( std::invalid_argument& e )
       {
        std::print("!! {}\n", e.what());
        if( not args.quiet() )
           {
            args.print_usage();
           }
       }

    catch( parse::error& e)
       {
        std::print("!! [{}:{}] {}\n", e.file(), e.line(), e.what());
        if( not args.quiet() )
           {
            sys::edit_text_file( e.file(), e.line() );
           }
       }

    catch( std::exception& e )
       {
        std::print("!! {}\n", e.what());
       }

    return 2;
}
