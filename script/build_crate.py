# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
from pathlib import Path
import platform
import subprocess

parser = argparse.ArgumentParser(
    description='Build crates in //brave/tools/crates/config.gni')
parser.add_argument('--temp_dir_path', required=True)
parser.add_argument('--rust_path', required=True)
args, cargo_args = parser.parse_known_args()
env = os.environ.copy()
if platform.system() == "Windows":
    env['TEMP'] = env['TMP'] = args.temp_dir_path
else:
    env["TMPDIR"] = args.temp_dir_path
env["PATH"] = args.rust_path + os.pathsep + env["PATH"]
subprocess.check_call(cargo_args, env=env)
Path(f'{args.temp_dir_path}/.stamp').touch()
