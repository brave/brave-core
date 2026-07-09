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


def copy_app_bundle(src_path, dest_path):
    """Copy an app bundle before universalize, stripping non-lipo artifacts.

    Used for both x64 and arm64 per-arch inputs (signed or unsigned) after
    extraction from prebuilt app zips. Signature metadata and Widevine .sig files are
    not Mach-O; lipo cannot merge them. The package step signs the universal app
    when signing is enabled.
    """

    if not os.path.exists(src_path):
        raise Exception('Could not find app bundle (%s)' % src_path)

    if os.path.exists(dest_path):
        shutil.rmtree(dest_path)

    # TODO check why brave_resources.pak differs between x64 and arm64
    shutil.copytree(src_path, dest_path, symlinks=True,
                    ignore=shutil.ignore_patterns('Sparkle.framework', '*.pak',
                                                  '_CodeSignature',
                                                  'CodeResources', '*.sig'))
    # remove conflicting files
    os.remove(os.path.join(dest_path, 'Contents', 'Info.plist'))


def main(args):
    parser = argparse.ArgumentParser(
        description='Copy a macOS app bundle for universalize')
    parser.add_argument('src_path',
                        help='Source path to the app bundle to copy from.')
    parser.add_argument('dest_path',
                        help='Destination path for the app bundle.')
    parsed = parser.parse_args(args)

    copy_app_bundle(parsed.src_path, parsed.dest_path)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
