#!/usr/bin/env python
import os, sys
import ctypes
import subprocess
import time
import shutil

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
def launch(command_and_args):
    return ctypes.c_int32( subprocess.call(command_and_args) ).value

#----------------------------------------------------------------------------
def pause():
    input(f'{YELLOW}Press <ENTER> to continue{END}')

#----------------------------------------------------------------------------
#def is_key_pressed():
#    if os.name=='nt':
#        import msvcrt
#        return msvcrt.kbhit()
#    else:
#        import termios
#        #import tty
#        fd = sys.stdin.fileno()
#        old_settings = termios.tcgetattr(fd)
#        try:
#            #tty.setcbreak(fd)
#            return bool(sys.stdin.read(1))
#        finally:
#            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)

#----------------------------------------------------------------------------
#def wait_no_key_pressed():
#    while is_key_pressed():
#        time.sleep(0.1)

#----------------------------------------------------------------------------
def main():
    set_title(__file__)
    os.chdir(script_dir)

    print(f"{BLUE}Building {CYAN}{projectname}{END}")
    if (build_ret:=launch(build_cmd))!=0:
        print(f"\n{RED}Build error: {YELLOW}{build_ret}{END}")
        pause()
        return build_ret

    if not os.path.isfile(exe):
        print(f"\n{RED}{exe} not generated!{END}")
        pause()
        return 1

    print(f"\n{GREEN}Build of {projectname} ok{END}")

    #if f"{configuration}|{platform}"=="Release|x64" and os.name=='nt' and os.path.isdir(dst_path:=os.path.expandvars('%UserProfile%/Bin')):
    #    print(f"{GRAY}Copying {END}{exe}{GRAY} to {END}{dst_path}")
    #    shutil.copy(exe, dst_path)

    print("Closing...")
    time.sleep(3)
    #wait_no_key_pressed()
    return 0

#----------------------------------------------------------------------------
if __name__ == '__main__':
    sys.exit(main())
