#!/usr/bin/env python
import os, sys
import subprocess
import time

# 🧬 Settings
script_dir = sys.path[0] # os.path.dirname(os.path.realpath(__file__))
projectname = os.path.basename(os.path.dirname(script_dir))
testprojectname = projectname + "-test"
build_dir = "../build";
if os.name=='nt':
    configuration = "Release"
    platform = "x64"

    build_cmd = ["msbuild", f"{build_dir}/{testprojectname}.vcxproj", "-t:rebuild", f"-p:Configuration={configuration}", f"-p:Platform={platform}"]
    bin_dir = f"{build_dir}/bin/win-{platform}-{configuration}"
    test_exe = f"{bin_dir}/{testprojectname}.exe"
    prog_exe = f"{bin_dir}/{projectname}.exe"
else:
    build_cmd = ["make", f"--directory={build_dir}", "test"]
    bin_dir = f"{build_dir}/bin"
    test_exe = f"{bin_dir}/{testprojectname}"
    prog_exe = f"{bin_dir}/{projectname}"

#----------------------------------------------------------------------------
GRAY = '\033[90m';
RED = '\033[91m';
GREEN = '\033[92m';
YELLOW = '\033[93m';
BLUE = '\033[94m';
MAGENTA = '\033[95m';
CYAN = '\033[96m';
END = '\033[0m';

#----------------------------------------------------------------------------
def set_title(title):
    if os.name=='nt':
        os.system(f"title {title}")
    else:
        sys.stdout.write(f"\x1b]2;{title}\x07")

#----------------------------------------------------------------------------
def launch(command_and_args):
    return subprocess.call(command_and_args)

#----------------------------------------------------------------------------
def pause():
    input(f'{YELLOW}Press <ENTER> to continue{END}')


#----------------------------------------------------------------------------
def main():
    set_title(__file__)
    os.chdir(script_dir)

    print(f"{BLUE}Building {CYAN}{testprojectname}{END}")
    if (build_ret:=launch(build_cmd))!=0:
        print(f"\n{RED}Build error: {YELLOW}{build_ret}{END}")
        pause()
        return build_ret

    if not os.path.isfile(test_exe):
        print(f"\n{RED}{test_exe} not generated!{END}")
        pause()
        return 1

    print(f"{GRAY}Launching {test_exe}{END}")
    #if os.path.isfile(prog_exe):
    #    tests_ret = launch([test_exe, f'\"{prog_exe}\"'])
    if (tests_ret:=launch([test_exe]))!=0:
        print(f"{MAGENTA}Test returned: {YELLOW}{tests_ret}{END}")
        pause()
        return tests_ret

    print(f"\n{GREEN}{testprojectname} all tests ok{END}")

    print("Closing...")
    time.sleep(3)
    return 0

#----------------------------------------------------------------------------
if __name__ == '__main__':
    sys.exit(main())
