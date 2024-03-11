#!/usr/bin/env python3

# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import os
import shutil
import sys

from os.path import abspath, dirname
from lib.util import execute

cert = os.environ.get('CERT')
cert_hash = os.environ.get('AUTHENTICODE_HASH')
signtool_args = (
    os.environ.get('SIGNTOOL_ARGS') or
    # We use a http:// URL because at least our current version of signtool
    # (10.0.22621.0, March 2024) does not support https://. See
    # https://github.com/brave/brave-browser/issues/165#issuecomment-1983445659
    # for more information.
    'sign /t http://timestamp.digicert.com /sm '
    '/fd sha256')


assert cert or cert_hash or signtool_args, \
    'At least one of AUTHENTICODE_HASH, CERT and SIGNTOOL_ARGS must be set.\n'\
    'The preferred parameter is AUTHENTICODE_HASH. Its value can be obtained '\
    'via the command `Get-ChildItem -path cert:\\LocalMachine\\My`.\n' \
    'CERT is a part of the name in the //CurrentUser/My Windows Certificate ' \
    'Store. It is ambiguous and will likely be deprecated in the future.'


def get_sign_cmd(file):
    # https://docs.microsoft.com/en-us/dotnet/framework/tools/signtool-exe
    # signtool should be in the path if it was set up correctly by gn through
    # src/build/vs_toolchain.py
    cmd = 'signtool {}'.format(signtool_args)
    if cert:
        cmd = cmd + ' /n "' + cert + '"'
    if cert_hash:
        cmd = cmd + ' /sha1 "' + cert_hash + '"'
    return cmd + ' "' + file + '"'


def sign_binaries(base_dir, endswidth=('.exe', '.dll')):
    matches = []
    for root, _, filenames in os.walk(base_dir):
        for filename in filenames:
            if filename.endswith(endswidth):
                matches.append(os.path.join(root, filename))

    for binary in matches:
        sign_binary(binary)


def sign_binary(binary, out_file=None):
    if out_file:
        os.makedirs(dirname(abspath(out_file)), exist_ok=True)
        shutil.copy(binary, out_file)
        binary = out_file
    cmd = get_sign_cmd(binary)
    execute(cmd)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('file', help='the file to sign.')
    parser.add_argument('--out_file',
                        help=('where to place the signed file. By default, the '
                              'file is signed in-place.'))
    args = parser.parse_args()
    sign_binary(args.file, args.out_file)


if __name__ == '__main__':
    sys.exit(main())
