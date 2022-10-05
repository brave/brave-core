#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

# Invoke another Python file after adding a directory to the PATH.

import os
import subprocess
import sys

dir_path = sys.argv[1]
args = sys.argv[2:]

new_env = dict(os.environ)
new_env['PATH'] = dir_path + os.pathsep + new_env['PATH']

subprocess.check_call([sys.executable] + args, env=new_env)
