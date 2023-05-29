#!/usr/bin/env python3

# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import sys

from shutil import which
from subprocess import run

USAGE = "Usage: run.py ENV=value ENV=value ... -- executable [args]"

def main(args):
    env, executable, executable_args = parse_args(args)

    child_env = dict(os.environ)
    child_env.update(env)

    # On Windows, run(...) does not search PATH for the executable. So do it
    # here:
    executable_path = which(executable)

    # pylint: disable=subprocess-run-check
    sys.exit(run([executable_path] + executable_args, env=child_env).returncode)

def parse_args(args):
    env = {}
    for i, arg in enumerate(args):
        if arg == '--':
            break
        try:
            env_var, value = arg.split('=', 1)
        except ValueError:
            print_usage_and_exit()
        env[env_var] = value
    else:
        print_usage_and_exit()
    try:
        executable = args[i + 1]
    except IndexError:
        print_usage_and_exit()
    executable_args = args[i + 2:]
    return env, executable, executable_args

def print_usage_and_exit():
    print(USAGE)
    sys.exit(1)

if __name__ == '__main__':
    main(sys.argv[1:])
