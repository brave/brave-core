#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import toml

import brave_chromium_utils

with brave_chromium_utils.sys_path('//tools/rust'):
    import update_rust
    CARGO = os.path.join(update_rust.RUST_TOOLCHAIN_OUT_DIR, 'bin',
                         'cargo' + ('.exe' if sys.platform == 'win32' else ''))

CONFIG_TOML = {
    'source': {
        'crates-io': {
            'replace-with': 'vendored-sources'
        },
        'vendored-sources': {
            'directory': '../vendor'
        }
    }
}


def main():
    os.chdir(os.path.dirname(os.path.realpath(__file__)))
    members = toml.load('Cargo.toml')['workspace']['members']

    shutil.rmtree('vendor', ignore_errors=True)
    for member in members:
        Path(f'{member}/.cargo/config.toml').unlink(missing_ok=True)

    subprocess.run([CARGO, 'vendor'], check=True)
    for member in members:
        Path(f'{member}/.cargo').mkdir(exist_ok=True)
        with open(Path(f'{member}/.cargo/config.toml'), 'w') as f:
            toml.dump(CONFIG_TOML, f)

    with tempfile.TemporaryFile(dir='.') as f:
        subprocess.run([CARGO, 'metadata', '--format-version=1'],
                       stdout=f,
                       check=True)
        f.seek(0)
        packages = json.load(f)['packages']

    for member in members:
        deps = toml.load(Path(f'{member}/Cargo.toml'))['dependencies']
        for package in filter(lambda package: package['name'] in deps,
                              packages):
            name = package['name']
            with open(Path(f'vendor/{name}/README.chromium'), 'w') as f:
                f.write(f"Name: {name}\n"
                        f"URL: https://crates.io/crates/{name}\n"
                        f"Version: {package['version']}\n"
                        f"License: {package['license']}\n"
                        f"Description: {package['description']}\n")


if __name__ == '__main__':
    main()
