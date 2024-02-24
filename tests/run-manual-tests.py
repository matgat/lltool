#!/usr/bin/env python
from pathlib import Path
from subprocess import call

def abspath(relpath):
    script_dir = Path(__file__).resolve().parent
    return (script_dir / relpath).resolve()

call(["python", abspath("run-system-tests.py"), "manual"])
