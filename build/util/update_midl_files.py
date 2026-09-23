#!/usr/bin/env python3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import shutil
import hashlib

from brave_chromium_utils import wspath

# Chromium compares pre-installed midl files and generated midl files from IDL
# during the build to check integrity. Generated files during the build time and
# upstream pre-installed files are different because we use different IDL file.
# So, we should copy our pre-installed files to overwrite upstream pre-installed
# files. After checking, pre-installed files are copied to gen dir and they are
# used to compile.


def update_midl_files():
    midl_dir = wspath("//brave/win_build_output/midl")
    with os.scandir(midl_dir) as entries:
        for entry in entries:
            if not entry.is_dir():
                continue
            src_dir = entry.path
            src_dir_name = entry.name
            dst_dir = wspath(
                f"//third_party/win_build_output/midl/{src_dir_name}")
            shutil.copytree(src_dir,
                            dst_dir,
                            dirs_exist_ok=True,
                            copy_function=copy_only_if_modified)


def copy_only_if_modified(src, dst):
    """Copy file if it doesn't exist or if its hash is different."""

    def file_hash(file_path):
        with open(file_path, "rb") as f:
            return hashlib.file_digest(f, "sha256").digest()

    if not os.path.exists(dst) or file_hash(src) != file_hash(dst):
        shutil.copy2(src, dst)


if __name__ == "__main__":
    update_midl_files()
