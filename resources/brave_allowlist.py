#!/usr/bin/env python3
# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import sys
import os


# Merges 2 allowlists (repack_allowlist + brave_allowlist = output file) to
# a new file that contains both resources. Filters duplicated resources ids.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repack_allowlist',
                        help='Path to the repack allowist.',
                        required=True,
                        metavar='FILE')
    parser.add_argument('--brave_allowlist',
                        help='Path to the brave repack allowist.',
                        required=True,
                        metavar='FILE')
    parser.add_argument('--output',
                        help='Path to the brave output allowist.',
                        required=True,
                        metavar='FILE')

    args = parser.parse_args()

    if not os.path.exists(args.repack_allowlist):
        print('Repack allowlist not found: {}'.format(args.repack_allowlist))
        return 1

    if not os.path.exists(args.brave_allowlist):
        print('Brave allowlist not found: {}'.format(args.brave_allowlist))
        return 1

    unique = set()
    with open(args.repack_allowlist, 'r') as f:
        for line in f.readlines():
            unique = unique | set(line.split()[:2])

    with open(args.brave_allowlist, 'r') as f:
        for line in f.readlines():
            unique = unique | set(line.split()[:2])

    if os.path.exists(args.output):
        os.remove(args.output)

    with open(args.output, 'w') as f:
        for num in unique:
            f.write(str(num) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
