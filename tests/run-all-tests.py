#!/usr/bin/env python
import sys
from pathlib import Path
from subprocess import call

script_dir = Path(__file__).resolve().parent

def abspath(relpath):
    return (script_dir / relpath).resolve()

script_list = ["run-unit-tests.py", "../build/build.py", "run-system-tests.py"]

for script in script_list:
    if call(["python", abspath(script)])!=0:
        print("n\033[91m Error\033[0m")
        sys.exit(return_code)

print("n\033[92m All ok\033[0m")
