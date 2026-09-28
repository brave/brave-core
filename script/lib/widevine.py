#!/usr/bin/env python3

# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from os.path import dirname, realpath, join, exists
from shutil import which

import argparse
import os
import sys

from lib.util import execute

SIGNATURE_GENERATOR_PY = \
    realpath(join(dirname(dirname(dirname(dirname(__file__)))), 'third_party',
             'widevine', 'scripts', 'signature_generator.py'))

SIGN_WIDEVINE_CERT = os.getenv('SIGN_WIDEVINE_CERT')
SIGN_WIDEVINE_KEY = os.getenv('SIGN_WIDEVINE_KEY')
SIGN_WIDEVINE_PASSPHRASE = os.getenv('SIGN_WIDEVINE_PASSPHRASE')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_file', required=True)
    parser.add_argument('--output_file', required=True)
    parser.add_argument('--flags', required=True)
    args = parser.parse_args()
    if not can_generate_sig_file():
        print(
            'Widevine signing certificate, key, passphrase or '
            'signature_generator.py is missing.',
            file=sys.stderr)
        return 1
    generate_sig_file(args.input_file, args.output_file, args.flags)
    return 0


def can_generate_sig_file():
    return exists(SIGNATURE_GENERATOR_PY) and SIGN_WIDEVINE_CERT and \
           SIGN_WIDEVINE_KEY and SIGN_WIDEVINE_PASSPHRASE


def generate_sig_file(input_file, output_file, flags):
    # signature_generator.py carries a vpython spec for its dependencies.
    # On Windows, depot_tools only provides vpython3.bat, which subprocess
    # cannot find by its bare name. which() resolves it via PATHEXT.
    execute([
        which('vpython3'), SIGNATURE_GENERATOR_PY, '--input_file', input_file,
        '--output_file', output_file, '--flags', flags, '--certificate',
        SIGN_WIDEVINE_CERT, '--private_key', SIGN_WIDEVINE_KEY,
        '--private_key_passphrase', SIGN_WIDEVINE_PASSPHRASE
    ])
    assert exists(output_file)


if __name__ == '__main__':
    sys.exit(main())
