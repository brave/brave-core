# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
from lib.util import execute_stdout

parser = argparse.ArgumentParser(description='Compile Rust packages to WASM')
parser.add_argument('--wasm_bindgen_path', required=True)
args, wasm_pack_args = parser.parse_known_args()
env = os.environ.copy()
env['PATH'] = args.wasm_bindgen_path + os.pathsep + env['PATH']
execute_stdout(wasm_pack_args, env)
