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


def copy_other_arch(src_path, dest_path):
    """Copies the other architecture's app into the output dir for universalize.

    The universal build is driven from one architecture (the primary, built
    locally) and merged with the other architecture's prebuilt app. This copies
    that prebuilt app into place so the universalizer can lipo-merge it.
    """

    # copytree() fails if the destination already exists, so clear it first to
    # keep this action re-runnable.
    if os.path.exists(dest_path):
        shutil.rmtree(dest_path)

    # Sparkle.framework is already a universal (fat) binary in every build, so a
    # single copy suffices and merging two would fail. The .pak files cannot be
    # lipo-merged either (they are not Mach-O). As of this writing, the .pak
    # differences between the two builds are not architectural: they come from
    # non-deterministic web UI bundling (styled-components class hashes,
    # minifier variable names, embedded build timestamps) that varies between
    # any two independent builds (even of the same architecture).
    # TODO(https://github.com/brave/brave-browser/issues/57254): Make .pak file
    # builds reproducible and remove the *.pak ignore pattern below.
    shutil.copytree(src_path,
                    dest_path,
                    symlinks=True,
                    ignore=shutil.ignore_patterns('Sparkle.framework',
                                                  '*.pak'))


def main(args):
    parser = argparse.ArgumentParser(
        description='Copy the other arch macos binary into root_out_dir for '
        'universalize')
    parser.add_argument('src_path',
                        help='The other architecture build output app path.')
    parser.add_argument(
        'dest_path', help='The location to copy the app to in root_out_dir.')
    parsed = parser.parse_args(args)

    copy_other_arch(parsed.src_path, parsed.dest_path)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
