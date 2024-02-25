#!/usr/bin/env python
import os, sys, psutil
import subprocess
import ctypes
import time
import re

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
def is_temp_console():
    parent_process = psutil.Process(os.getpid()).parent().name()
    temp_parents = re.compile(r"(?i)^(explorer.*|.*terminal)$")
    return temp_parents.match(parent_process)

#----------------------------------------------------------------------------
def closing_bad(msg):
    print(f"\n{RED}{msg}{END}")
    if is_temp_console():
        input(f'{YELLOW}Press <ENTER> to exit{END}')

#----------------------------------------------------------------------------
def closing_ok(msg):
    print(f"\n{GREEN}{msg}{END}")
    if is_temp_console():
        print(f'{GRAY}Closing...{END}')
        time.sleep(3)

#----------------------------------------------------------------------------
def launch(command_and_args):
    return ctypes.c_int32( subprocess.call(command_and_args) ).value


#----------------------------------------------------------------------------
def main():
    set_title(__file__)
    os.chdir(script_dir)

    print(f"{BLUE}Building {CYAN}{testprojectname}{END}")
    if (build_ret:=launch(build_cmd))!=0:
        closing_bad(f"Build returned {build_ret}")
        return build_ret

    if not os.path.isfile(test_exe):
        closing_bad(f"{test_exe} not generated!")
        return 1

    print(f"{GRAY}Launching {test_exe}{END}")
    if (tests_ret:=launch([test_exe]))!=0:
        closing_bad(f"Test returned {tests_ret}")
        return tests_ret

    closing_ok(f'{testprojectname}: all tests ok')
    return 0

#----------------------------------------------------------------------------
if __name__ == '__main__':
    sys.exit(main())
