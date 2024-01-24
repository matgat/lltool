#!/usr/bin/env python
from os.path import join, dirname, abspath
from subprocess import call
call(["python", join(dirname(abspath(__file__)), "run-system-tests.py"), "manual"])
