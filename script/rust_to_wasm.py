# Copyright 2014 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Helper script for GN to run an arbitrary binary. See compiled_action.gni.

Run with:
  python gn_run_binary.py <binary_name> [args ...]
"""

import os
import sys
import subprocess
from lib.util import execute_stdout, scoped_cwd

# This script is designed to run binaries produced by the current build. We
# may prefix it with "./" to avoid picking up system versions that might
# also be on the path.
path = sys.argv[1]
if not os.path.isabs(path):
    path = './' + path

# The rest of the arguments are passed directly to the executable.
args = [path] + sys.argv[2:]

my_env = os.environ.copy()
my_env[
    "PATH"] = f"/Users/szilard/git/brave-browser/src/out/Component_arm64/wasm_bindgen/release:{my_env['PATH']}"
subprocess.Popen(args, env=my_env)
# execute_stdout(args, env)
