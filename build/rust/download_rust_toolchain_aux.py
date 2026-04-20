#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from urllib.error import HTTPError
import os
from pathlib import Path
import platform
import shutil
import sys
import tempfile

import brave_chromium_utils
import deps
import deps_config

TOOLS_RUST = '//tools/rust'
LLD = 'lld' + ('.exe' if sys.platform == 'win32' else '')
RUST_LLD = f'rust-{LLD}'
WASM32_UNKNOWN_UNKNOWN = 'wasm32-unknown-unknown'

with brave_chromium_utils.sys_path(TOOLS_RUST):
    import build_rust
    import package_rust
    RUST_TOOLCHAIN_OUT_DIR = Path(build_rust.RUST_TOOLCHAIN_OUT_DIR)

BRAVE_RUST_TOOLCHAIN_AUX = RUST_TOOLCHAIN_OUT_DIR / '.brave_rust_toolchain_aux'


def package_name() -> str:
    """Return the platform-specific rust-toolchain-aux package filename."""
    if sys.platform == 'darwin':
        if platform.machine() == 'arm64':
            platform_prefix = 'mac-arm64'
        else:
            platform_prefix = 'mac'
    elif sys.platform == 'win32':
        platform_prefix = 'win'
    else:
        platform_prefix = 'linux-x64'

    return f'{platform_prefix}-{package_rust.RUST_TOOLCHAIN_PACKAGE_NAME}'


def is_download_needed() -> bool:
    """Return whether the currently installed aux package is stale or missing.
    """
    current_package_name = ''
    try:
        with open(BRAVE_RUST_TOOLCHAIN_AUX, encoding='utf-8') as file:
            current_package_name = file.readline().rstrip()
    except Exception:
        pass
    return current_package_name != package_name()


def install_package(src_dir):
    """Install downloaded rust-lld and wasm32 artifacts into the toolchain."""
    src_dir = Path(src_dir)
    rust_lld_path = RUST_TOOLCHAIN_OUT_DIR / 'bin' / RUST_LLD
    wasm32_unknown_unknown_path = (RUST_TOOLCHAIN_OUT_DIR / 'lib' / 'rustlib' /
                                   WASM32_UNKNOWN_UNKNOWN)

    # delete existing artifacts
    rust_lld_path.unlink(missing_ok=True)
    shutil.rmtree(wasm32_unknown_unknown_path, ignore_errors=True)

    # move new artifacts into their final places
    shutil.move(src_dir / RUST_LLD, rust_lld_path)
    shutil.move(src_dir / WASM32_UNKNOWN_UNKNOWN, wasm32_unknown_unknown_path)


def save_package_name():
    """Persist the installed aux package name for subsequent freshness checks.
    """
    with open(BRAVE_RUST_TOOLCHAIN_AUX, 'w+', encoding='utf-8',
              newline='') as file:
        file.write(f'{package_name()}\n')


def main():
    """Download and install rust-toolchain auxiliary artifacts when needed."""
    if not is_download_needed():
        print(f'Skipping {Path(__file__).stem}')
        return

    with tempfile.TemporaryDirectory(dir=RUST_TOOLCHAIN_OUT_DIR) as temp_dir:
        try:
            deps.DownloadAndUnpack(
                f'{deps_config.DEPS_PACKAGES_URL}/rust-toolchain-aux/{package_name()}',
                temp_dir)
            install_package(temp_dir)
            save_package_name()
        except HTTPError as error:
            if error.code == 403:
                print("If you see this error message, "
                      "it means you're likely on the rebase team "
                      "and doing a Chromium bump - please visit "
                      "https://ci.brave.com/view/rust and use your branch "
                      "to build the required Rust auxiliary package.")
            raise


if __name__ == '__main__':
    main()
