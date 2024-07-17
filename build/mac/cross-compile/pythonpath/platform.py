# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
The purpose of this file is to replace the built-in function platform.mac_ver()
by an implementation that runs (via SSH) on a remote macOS host. The host name
is given by environment variable MACOS_HOST.
"""

# Load all members of the built-in platform module:

import sys

_this_module = sys.modules.pop('platform')
sys.path = sys.path[::-1]
from platform import *  # pylint: disable=wildcard-import,import-self,unused-wildcard-import

sys.path = sys.path[::-1]
sys.modules['platform'] = _this_module
del _this_module
del sys


# Now change the built-in function mac_ver() so it runs on MACOS_HOST:
def mac_ver():
    # pylint: disable=import-outside-toplevel
    from shlex import quote
    from subprocess import run, PIPE
    import os
    macos_host = os.environ['MACOS_HOST']
    python = 'import platform ; print(platform.mac_ver())'
    cmdline = ['ssh', macos_host, 'python3 -c ' + quote(python)]
    cp = run(cmdline, stdout=PIPE, check=True)  # pylint: disable=subprocess-run-check
    return eval(cp.stdout)
