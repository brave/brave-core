#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from urllib.error import HTTPError
import os
from pathlib import Path
import shutil
import tempfile

import brave_chromium_utils
import build_rust_toolchain_aux
import deps
import deps_config

with brave_chromium_utils.sys_path(build_rust_toolchain_aux.TOOLS_RUST):
    import build_rust
    RUST_TOOLCHAIN = build_rust.RUST_TOOLCHAIN_OUT_DIR

BRAVE_RUST_TOOLCHAIN_AUX = os.path.join(RUST_TOOLCHAIN,
                                        '.brave_rust_toolchain_aux')


def is_download_needed():
    if os.getenv('SKIP_DOWNLOAD_RUST_TOOLCHAIN_AUX', '0') == '1':
        return False

    package_name = ''
    try:
        with open(BRAVE_RUST_TOOLCHAIN_AUX) as file:
            package_name = file.readline().rstrip()
    except Exception:
        pass
    return package_name != build_rust_toolchain_aux.package_name()


def install_package(src_dir):
    rust_lld_path = os.path.join(RUST_TOOLCHAIN, 'bin',
                                 build_rust_toolchain_aux.RUST_LLD)
    wasm32_unknown_unknown_path = os.path.join(
        RUST_TOOLCHAIN, 'lib', 'rustlib',
        build_rust_toolchain_aux.WASM32_UNKNOWN_UNKNOWN)

    # delete existing artifacts
    Path(rust_lld_path).unlink(missing_ok=True)
    shutil.rmtree(wasm32_unknown_unknown_path, ignore_errors=True)

    # move new artifacts into their final places
    shutil.move(os.path.join(src_dir, build_rust_toolchain_aux.RUST_LLD),
                rust_lld_path)
    shutil.move(
        os.path.join(src_dir, build_rust_toolchain_aux.WASM32_UNKNOWN_UNKNOWN),
        wasm32_unknown_unknown_path)


def save_package_name():
    with open(BRAVE_RUST_TOOLCHAIN_AUX, 'w+') as file:
        file.write(f'{build_rust_toolchain_aux.package_name()}\n')


def main():
    if not is_download_needed():
        print(f'Skipping {Path(__file__).stem}')
        return

    with tempfile.TemporaryDirectory(dir=RUST_TOOLCHAIN) as temp_dir:
        try:
            deps.DownloadAndUnpack(
                f'{deps_config.DEPS_PACKAGES_URL}/rust-toolchain-aux-temp/{build_rust_toolchain_aux.package_name()}',
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
