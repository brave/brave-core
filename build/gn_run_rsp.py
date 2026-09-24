#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Helper script for GN to run an arbitrary binary with environment variables from
a response file.

Run with:
  python3 gn_run_rsp.py <file.rsp>

Example response file:
  PATH=./ wasm-pack build
"""

import os
import subprocess
import shlex
import sys
from pathlib import Path


# Paths prefixed with abs@ will be converted to absolute paths.
def maybe_abspath(value):
    entries = value.split(os.pathsep)
    for index, entry in enumerate(entries):
        prefix, marker, path = entry.partition('abs@')
        if marker:
            entries[index] = prefix + os.path.abspath(path)

    return os.pathsep.join(entries)

def main():
    if len(sys.argv) < 2:
        print('Usage: python3 gn_run_rsp.py <file.rsp>', file=sys.stderr)
        sys.exit(1)

    response_file = sys.argv[1]
    try:
        with open(response_file, 'r') as f:
            content = f.read()
    except IOError as e:
        print(f'Error reading response file {response_file}: {e}',
              file=sys.stderr)
        sys.exit(1)

    args = shlex.split(content)

    # Resolve any abs@ paths
    for index, arg in enumerate(args):
        args[index] = maybe_abspath(arg)

    # Parse environment variables from command line arguments.
    env_vars = {}
    for arg in args:
        if '=' in arg:
            name, value = arg.split('=', 1)
            env_vars[name] = value
        else:
            break


    # Ensure the command contains a path (absolute or relative)
    cmd = args[len(env_vars)]
    if os.path.basename(cmd) == cmd:
        print(f'The command to run must have a path: {cmd}', file=sys.stderr)
        sys.exit(1)

    # The rest of the arguments are passed directly to the executable.
    args = args[len(env_vars):]

    # Always prepend PATH if set
    if env_vars.get('PATH') is not None:
        env_vars['PATH'] = env_vars['PATH'] + os.pathsep + os.getenv('PATH')

    env = os.environ.copy()
    env.update(env_vars)

    ret = subprocess.call(args, env=env)
    if ret != 0:
        if ret <= -100:
            # Windows error codes such as 0xC0000005 and 0xC0000409 are much
            # easier to recognize and differentiate in hex. In order to print
            # them as unsigned hex we need to add 4 Gig to them.
            print('%s failed with exit code 0x%08X' % (response_file, ret +
                                                       (1 << 32)))
        else:
            print('%s failed with exit code %d' % (response_file, ret))
    sys.exit(ret)


if __name__ == '__main__':
    main()
