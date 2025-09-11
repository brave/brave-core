#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
from pathlib import Path
import platform
import subprocess
import sys
import tarfile

import brave_chromium_utils

CONFIG_TOML_TEMPLATE = 'config.toml.template'
TOOLS_RUST = '//tools/rust'

LLD = 'lld' + ('.exe' if sys.platform == 'win32' else '')
RUST_LLD = f'rust-{LLD}'
WASM32_UNKNOWN_UNKNOWN = 'wasm32-unknown-unknown'

def restore_config_toml_template():
    args = [
        'git', '-C',
        brave_chromium_utils.get_src_dir(), 'checkout', '--',
        os.path.join(brave_chromium_utils.wspath(TOOLS_RUST),
                     CONFIG_TOML_TEMPLATE)
    ]
    subprocess.check_call(args)


# Disable the profiler for the wasm32-unknown-unknown target.
def edit_config_toml_template():
    with open(CONFIG_TOML_TEMPLATE, 'a') as file:
        file.write('\n[target.wasm32-unknown-unknown]\nprofiler = false\n')


def prepare_run_xpy():
    args = ['python3', 'build_rust.py', '--prepare-run-xpy']
    subprocess.check_call(args)


def run_xpy():
    with brave_chromium_utils.sys_path(TOOLS_RUST):
        import build_rust
        target_triple = build_rust.RustTargetTriple()

    args = [
        'python3', 'build_rust.py', '--run-xpy', '--', 'build', '--build',
        target_triple, '--target', f'{target_triple},wasm32-unknown-unknown',
        '--stage', '1'
    ]
    subprocess.check_call(args)


def package_name():
    if sys.platform == 'darwin':
        if platform.machine() == 'arm64':
            platform_prefix = 'mac-arm64'
        else:
            platform_prefix = 'mac'
    elif sys.platform == 'win32':
        platform_prefix = 'win'
    else:
        platform_prefix = 'linux-x64'

    with brave_chromium_utils.sys_path(TOOLS_RUST):
        import package_rust
        return f'{platform_prefix}-{package_rust.RUST_TOOLCHAIN_PACKAGE_NAME}'


def create_archive():
    with brave_chromium_utils.sys_path(TOOLS_RUST):
        import build_rust
        target_triple = build_rust.RustTargetTriple()
        stage1_output_path = os.path.join(build_rust.RUST_BUILD_DIR,
                                          target_triple, 'stage1', 'lib',
                                          'rustlib')

        with tarfile.open(package_name(), 'w:xz') as tar:
            tar.add(os.path.join(build_rust.RUST_HOST_LLVM_INSTALL_DIR, 'bin',
                                 LLD),
                    arcname=RUST_LLD)
            tar.add(os.path.join(stage1_output_path, WASM32_UNKNOWN_UNKNOWN),
                    arcname=WASM32_UNKNOWN_UNKNOWN)


def main():
    parser = argparse.ArgumentParser(
        description='Build and package rust-lld and wasm32-unknown-unknown')
    parser.add_argument('--out-dir',
                        default=os.path.dirname(os.path.realpath(__file__)))
    args = parser.parse_args()
    Path(args.out_dir).mkdir(parents=True, exist_ok=True)

    os.chdir(brave_chromium_utils.wspath(TOOLS_RUST))
    restore_config_toml_template()
    edit_config_toml_template()
    prepare_run_xpy()
    run_xpy()
    restore_config_toml_template()

    os.chdir(args.out_dir)
    create_archive()


if __name__ == '__main__':
    sys.exit(main())
