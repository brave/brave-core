# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Description:
# rust_to_wasm.py serves as a proxy script that allows you to
# specify wasm-bindgen-cli's path (via --wasm_bindgen_cli_path)
# and wasm-opt's path (via --wasm_opt_path) for wasm-pack,
# while forwarding the rest of the command line arguments to wasm-pack.
# See more on that in components/common/rust_to_wasm.gni.
# This is required since wasm-bindgen-cli and wasm-opt are in root_build_dir
# (normally, we would add them to the $PATH in build/commands/lib/config.js).

# Example usage:
# python3 rust_to_wasm.py --wasm_bindgen_cli_path /path/to/wasm-bindgen-cli --wasm_opt_path /path/to/wasm-opt /path/to/wasm-pack [wasm-pack command line arguments...]

import argparse
import os
import subprocess
import shutil
import sys

parser = argparse.ArgumentParser(description='Compile Rust packages to WASM')
parser.add_argument('--wasm_bindgen_cli_path', required=True)
parser.add_argument('--wasm_opt_path', required=True)
args, wasm_pack_args = parser.parse_known_args()
env = os.environ.copy()
env['PATH'] = args.wasm_bindgen_cli_path + os.pathsep + args.wasm_opt_path + os.pathsep + env[
    'PATH']

rustc_path = shutil.which('rustc', path=env['PATH'])
if rustc_path:
    print(f'rust_to_wasm.py is using rustc: {rustc_path}')
    subprocess.check_call([rustc_path, '--version'], env=env)
else:
    print('rustc not found on PATH', file=sys.stderr)

subprocess.check_call(wasm_pack_args, env=env)
