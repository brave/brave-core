#!/usr/bin/env python3
# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import glob
import os
import sys
import shutil

import deps

UNPACK_DIR = os.path.join(os.path.dirname(__file__), '_unpack')
CCACHE_DIR = os.path.join(os.path.dirname(__file__), 'bin')


def main():
    args = parse_args()
    if args.version == get_current_version():
        return

    if os.path.exists(CCACHE_DIR):
        shutil.rmtree(CCACHE_DIR)

    url = ('https://github.com/ccache/ccache/releases/download/'
           f'v{args.version}/ccache-{args.version}-')
    if sys.platform == 'linux':
        url += 'linux-x86_64.tar.xz'
    elif sys.platform == 'darwin':
        url += 'darwin.tar.gz'
    else:
        url += 'windows-x86_64.zip'
    deps.DownloadAndUnpack(url, UNPACK_DIR)
    ccache_extracted_dir = glob.glob(f'{UNPACK_DIR}/ccache-*')
    if not ccache_extracted_dir:
        raise RuntimeError(f"Can't find extracted ccache at {UNPACK_DIR}")
    ccache_extracted_dir = ccache_extracted_dir[0]
    os.rename(ccache_extracted_dir, CCACHE_DIR)
    shutil.rmtree(UNPACK_DIR)
    set_current_version(args.version)


def parse_args():
    parser = argparse.ArgumentParser(description='Download ccache binaries.')
    parser.add_argument('version')
    return parser.parse_args()


def get_current_version():
    try:
        with open(os.path.join(CCACHE_DIR, '.version')) as f:
            return f.read()
    except FileNotFoundError:
        return None


def set_current_version(value):
    with open(os.path.join(CCACHE_DIR, '.version'), 'w') as f:
        f.write(value)


if __name__ == '__main__':
    main()
