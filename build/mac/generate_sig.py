#!/usr/bin/env python3

# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import subprocess
import sys


def Main():
    parser = argparse.ArgumentParser(usage='%(prog)s [options]')
    parser.add_argument('--sign-update', dest='sign_update_path', action='store',
        help='The path of sign_update binary', required=True)
    parser.add_argument('--sign-key-file', dest='sign_key_file', action='store',
        help='The private key to sign patch file', required=True)
    parser.add_argument('--target', dest='target', action='store',
        help='Target file path for signing.', required=True)
    parser.add_argument('--output', dest='output', action='store',
        help='The path of the output.', required=True)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--dsa", action="store_true", help='Use DSA')
    group.add_argument("--eddsa", action="store_true", help='Use EdDSA')
    args = parser.parse_args()

    # sign file with the specified algorithm
    with open(args.output, 'w') as file:
        if args.dsa:
            command = [args.sign_update_path, args.target, args.sign_key_file]
        else:
            # EdDSA
            command = [args.sign_update_path, '-s', args.sign_key, args.target]
        try:
            subprocess.check_call(command, stdout=file)
        except subprocess.CalledProcessError as e:
            print(e.output)
            raise e

    return 0


if __name__ == '__main__':
    sys.exit(Main())
