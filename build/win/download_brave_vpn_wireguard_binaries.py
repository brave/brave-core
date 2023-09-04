#!/usr/bin/env vpython3

# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os

from deps_config import DEPS_PACKAGES_URL
import deps


def get_download_url(version, library):
    return DEPS_PACKAGES_URL + \
           '/brave-vpn-wireguard-dlls/' + library + '-' + \
            version + '.zip'


def get_library_path(library):
    return os.path.join(os.getcwd(), *['third_party', library])


def get_library_revision_path(library):
    return os.path.join(get_library_path(library), '.revision')


def main():
    args = parse_args()
    if args.revision != get_current_revision(args.library):
        url = get_download_url(args.revision, args.library)
        target_path = get_library_path(args.library)
        print('url:{} to {}'.format(url, target_path))
        deps.DownloadAndUnpack(url, target_path)
        set_current_revision(args.revision, args.library)


def parse_args():
    parser = argparse.ArgumentParser(description='Download Wireguard binaries.')
    parser.add_argument('revision')
    parser.add_argument('library')
    return parser.parse_args()


def get_current_revision(library):
    try:
        with open(get_library_revision_path(library)) as f:
            return f.read()
    except FileNotFoundError:
        return None


def set_current_revision(value, library):
    with open(get_library_revision_path(library), 'w') as f:
        f.write(value)


if __name__ == '__main__':
    main()
