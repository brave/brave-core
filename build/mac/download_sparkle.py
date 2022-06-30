#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import os

from deps_config import DEPS_PACKAGES_URL, MAC_TOOLCHAIN_ROOT
import deps


SPARKLE_BINARIES_DIR = os.path.join(MAC_TOOLCHAIN_ROOT, 'sparkle_binaries')


def main():
    args = parse_args()
    if args.revision != get_current_revision():
        url = DEPS_PACKAGES_URL + '/sparkle/sparkle-' + args.revision + \
              '.tar.gz'
        deps.DownloadAndUnpack(url, SPARKLE_BINARIES_DIR)
        set_current_revision(args.revision)


def parse_args():
    parser = argparse.ArgumentParser(description='Download Sparkle binaries.')
    parser.add_argument('revision')
    return parser.parse_args()


def get_current_revision():
    try:
        with open(os.path.join(SPARKLE_BINARIES_DIR, '.revision')) as f:
            return f.read()
    except FileNotFoundError:
        return None


def set_current_revision(value):
    with open(os.path.join(SPARKLE_BINARIES_DIR, '.revision'), 'w') as f:
        f.write(value)


if __name__ == '__main__':
    main()
