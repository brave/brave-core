#!/usr/bin/env python3
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import subprocess
import sys
import os

NODE_MODULES = os.path.join(os.path.dirname(__file__), '..', 'node_modules')


def PathInNodeModules(*args):
    return os.path.join(NODE_MODULES, *args)


def RunNodeRaw(cmd_parts):
    cmd = ['node'] + cmd_parts
    process = subprocess.Popen(cmd,
                               cwd=os.getcwd(),
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE,
                               universal_newlines=True)
    stdout, stderr = process.communicate()
    return process.returncode, stdout, stderr


def RunNode(cmd_parts, include_command_in_error=True):
    returncode, stdout, stderr = RunNodeRaw(cmd_parts)
    if returncode != 0:
        err = stderr if len(stderr) > 0 else stdout
        raise RuntimeError(
            f"Command '{' '.join(['node'] + cmd_parts)}' failed\n{err}"
            if include_command_in_error else err)

    return stdout


if __name__ == '__main__':
    RunNode(sys.argv[1:])
