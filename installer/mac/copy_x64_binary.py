#!/usr/bin/env python
# coding: utf-8

# Copyright (c) 2020 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
import shutil
import sys


def copy_x64(x64_src_path, x64_dest_path):
    """Copies the x64 binary from the x64 build dir."""

    if not os.path.exists(x64_src_path):
        raise Exception('Could not find x64 app (%s)' % x64_src_path)

    if os.path.exists(x64_dest_path):
        shutil.rmtree(x64_dest_path)

    # TODO check why brave_resources.pak differs between x64 and arm64
    shutil.copytree(x64_src_path, x64_dest_path, symlinks=True,
            ignore=shutil.ignore_patterns('Sparkle.framework', '*.pak'))
    # remove conflicting files
    os.remove(os.path.join(x64_dest_path, 'Contents', 'Info.plist'))


def main(args):
    parser = argparse.ArgumentParser(
        description='Copy macos x64 binary to arm64 for universalize')
    parser.add_argument('x64_src_path',
                        help='Root output dir for arm64 build')
    parser.add_argument('x64_dest_path',
                        help='The location to copy the x64 binary to in root_out_dir.')
    parsed = parser.parse_args(args)

    copy_x64(parsed.x64_src_path, parsed.x64_dest_path)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
