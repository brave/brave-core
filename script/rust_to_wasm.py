# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Description:
# rust_to_wasm.py serves as a proxy script that allows you to
# specify wasm-bindgen's path for wasm-pack (via --wasm_bindgen_path),
# while forwarding the rest of the command line arguments to wasm-pack.
# See more on that in components/common/rust_to_wasm.gni.
# This is required since wasm-bindgen is in root_build_dir
# (normally, we would add it to the $PATH in build/commands/lib/config.js).

# Example usage:
# python3 rust_to_wasm.py --wasm_bindgen_path /path/to/wasm-bindgen /path/to/wasm-pack [wasm-pack command line arguments...]

import argparse
import os
import subprocess

parser = argparse.ArgumentParser(description='Compile Rust packages to WASM')
parser.add_argument('--wasm_bindgen_path', required=True)
args, wasm_pack_args = parser.parse_known_args()
env = os.environ.copy()
env['PATH'] = args.wasm_bindgen_path + os.pathsep + env['PATH']
subprocess.check_call(wasm_pack_args, env=env)
