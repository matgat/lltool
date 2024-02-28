#!/usr/bin/env python
import os, sys, psutil, shutil
import subprocess
import ctypes
import time
import re

# 🧬 Settings
script_dir = sys.path[0] # os.path.dirname(os.path.realpath(__file__))
projectname = os.path.basename(os.path.dirname(script_dir))
if os.name=='nt':
    configuration = "Release"
    platform = "x64"

    build_cmd = ["msbuild", f"{projectname}.vcxproj", "-t:rebuild", f"-p:Configuration={configuration}", f"-p:Platform={platform}"]
    exe = f"bin/win-{platform}-{configuration}/{projectname}.exe"
else:
    build_cmd = ["make"]
    exe = f"bin/{projectname}"


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
    temp_parents = re.compile(r"(?i)^(?:py|python|explorer)\.exe|.*terminal$")
    #print(f"{parent_process} {'is' if temp_parents.match(parent_process) else 'is not'} temp")
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
    start_time_s = time.perf_counter()
    return_code = ctypes.c_int32( subprocess.call(command_and_args) ).value
    exec_time_s = time.perf_counter() - start_time_s
    if exec_time_s>0.5:
        exec_time_str = f"{CYAN}{exec_time_s:.2f}{END}s"
    else:
        exec_time_str = f"{CYAN}{1000.0 * exec_time_s:.2f}{END}ms"
    print(f"{END}{command_and_args[0]} returned: {GREEN if return_code==0 else RED}{return_code}{END} after {exec_time_str}")
    return return_code


#----------------------------------------------------------------------------
def main():
    set_title(__file__)
    os.chdir(script_dir)

    print(f"\n{BLUE}Building {CYAN}{projectname}{END}")
    if (build_ret:=launch(build_cmd))!=0:
        closing_bad(f"Build error")
        return build_ret

    if not os.path.isfile(exe):
        closing_bad(f"{exe} not generated!")
        return 1

    if f"{configuration}|{platform}"=="Release|x64" and os.name=='nt' and os.path.isdir(dst_path:=os.path.expandvars('%UserProfile%/Bin')):
        print(f"{GRAY}Copying {END}{exe}{GRAY} to {END}{dst_path}")
        try: shutil.copy(exe, dst_path)
        except Exception as e: print(f"{RED}{e}{END}")

    closing_ok(f"Build of {projectname} ok")
    return 0

#----------------------------------------------------------------------------
if __name__ == '__main__':
    sys.exit(main())
