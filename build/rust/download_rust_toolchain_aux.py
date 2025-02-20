#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import contextlib
import os
from pathlib import Path
import shutil
import tarfile
import tempfile

import brave_chromium_utils
import build_rust_toolchain_aux
import deps
import deps_config


@contextlib.contextmanager
def TemporaryDirectory():
    temp_dir = tempfile.mkdtemp()
    try:
        yield temp_dir
    finally:
        shutil.rmtree(temp_dir)


def main():
    with tempfile.TemporaryFile() as temp_file:
        deps.DownloadUrl(
            deps_config.DEPS_PACKAGES_URL + '/rust-toolchain-aux/' +
            build_rust_toolchain_aux.package_name(), temp_file)

        with TemporaryDirectory() as temp_dir:
            temp_file.seek(0)
            tarfile.open(mode='r:xz',
                         fileobj=temp_file).extractall(path=temp_dir)

            with brave_chromium_utils.sys_path(
                    build_rust_toolchain_aux.TOOLS_RUST):
                import build_rust
                rust_toolchain = build_rust.RUST_TOOLCHAIN_OUT_DIR

            rust_lld_path = os.path.join(rust_toolchain, 'bin',
                                         build_rust_toolchain_aux.RUST_LLD)
            Path(rust_lld_path).unlink(missing_ok=True)
            shutil.move(
                os.path.join(temp_dir, build_rust_toolchain_aux.RUST_LLD),
                rust_lld_path)

            wasm32_unknown_unknown_path = os.path.join(
                rust_toolchain, 'lib', 'rustlib',
                build_rust_toolchain_aux.WASM32_UNKNOWN_UNKNOWN)
            shutil.rmtree(wasm32_unknown_unknown_path, ignore_errors=True)
            shutil.move(
                os.path.join(temp_dir,
                             build_rust_toolchain_aux.WASM32_UNKNOWN_UNKNOWN),
                wasm32_unknown_unknown_path)


if __name__ == '__main__':
    main()
