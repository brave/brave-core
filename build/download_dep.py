##!/usr/bin/env python3

# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from argparse import ArgumentParser
from brave_chromium_utils import wspath
from deps_config import DEPS_PACKAGES_URL
from os.path import join

import deps


def main():
    args = parse_args()
    dep_url = DEPS_PACKAGES_URL + '/' + args.dep_path
    dest_dir = wspath(args.dest_dir)
    if dep_url != get_url(dest_dir):
        deps.DownloadAndUnpack(dep_url, dest_dir)
        set_url(dest_dir, dep_url)


def parse_args():
    parser = ArgumentParser(description='Download and extract an archive.')
    parser.add_argument('dep_path')
    parser.add_argument('dest_dir')
    return parser.parse_args()


def get_url(dest_dir):
    try:
        with open(join(dest_dir, '.url')) as f:
            return f.read()
    except FileNotFoundError:
        return None


def set_url(dest_dir, value):
    with open(join(dest_dir, '.url'), 'w') as f:
        f.write(value)


if __name__ == '__main__':
    main()
