#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# update.py vendors the deps of the Rust workspace in `wasm`.
# Run this script via npm run update_wasm_resources
# whenever you need to add a new workspace member,
# or bump the version of an existing member's deps.

import os
from pathlib import Path
import shutil
import subprocess
import sys
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

    # Backup specific files that should be preserved
    backed_up_files = {}

    # Backup vendor/.clang-format if it exists
    clang_format_path = Path('vendor/.clang-format')
    if clang_format_path.exists():
        print(f'Backing up: {clang_format_path}')
        backed_up_files[str(clang_format_path)] = clang_format_path.read_text()

    # Backup vendor/*/README.chromium files
    for readme in Path('vendor').glob('*/README.chromium'):
        if readme.exists():
            print(f'Backing up: {readme}')
            backed_up_files[str(readme)] = readme.read_text()

    # Backup candle_embedding_gemma/.cargo/config.toml
    candle_config = Path('candle_embedding_gemma/.cargo/config.toml')
    if candle_config.exists():
        print(f'Backing up: {candle_config}')
        backed_up_files[str(candle_config)] = candle_config.read_text()

    # Purge and regenerate
    shutil.rmtree('vendor', ignore_errors=True)
    for member in members:
        Path(f'{member}/.cargo/config.toml').unlink(missing_ok=True)

    subprocess.run([CARGO, 'vendor'], check=True)

    # Clean up compiled objects and DLLs from vendor directory
    for pattern in ['**/*.o', '**/*.dll']:
        for file_path in Path('vendor').glob(pattern):
            print(f'Removing: {file_path}')
            file_path.unlink()

    # Restore backed up files
    for file_path, content in backed_up_files.items():
        path = Path(file_path)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
        print(f'Restored: {file_path}')

    # Generate default config.toml for members that don't have one
    for member in members:
        config_path = Path(f'{member}/.cargo/config.toml')
        if not config_path.exists():
            config_path.parent.mkdir(parents=True, exist_ok=True)
            with open(config_path, 'w') as f:
                toml.dump(CONFIG_TOML, f)


if __name__ == '__main__':
    main()
