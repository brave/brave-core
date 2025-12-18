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

    # Parse environment variables from command line arguments.
    env_vars = {}
    for arg in args:
        if '=' in arg:
            name, value = arg.split('=', 1)
            env_vars[name] = value
        else:
            break

    # This script is designed to run binaries produced by the current build. We
    # may prefix it with "./" to avoid picking up system versions that might
    # also be on the path.
    path = args[len(env_vars)]
    if not os.path.isabs(path):
        path = './' + path

    # The rest of the arguments are passed directly to the executable.
    args = [path] + args[len(env_vars) + 1:]

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
