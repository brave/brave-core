#!/usr/bin/env python3

# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import os
import subprocess
import sys

cert = os.environ.get('CERT')
cert_hash = os.environ.get('AUTHENTICODE_HASH')
signtool_args = (os.environ.get('SIGNTOOL_ARGS') or
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


def run_cmd(cmd):
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    for line in p.stdout:
        print(line)
    p.wait()
    assert p.returncode == 0, "Error signing"


def sign_binaries(base_dir, endswidth=('.exe', '.dll')):
    matches = []
    for root, _, filenames in os.walk(base_dir):
        for filename in filenames:
            if filename.endswith(endswidth):
                matches.append(os.path.join(root, filename))

    for binary in matches:
        sign_binary(binary)


def sign_binary(binary):
    cmd = get_sign_cmd(binary)
    run_cmd(cmd)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-b', '--build_dir', required=True,
        help='Build directory. The paths in input_file are relative to this.')
    args = parser.parse_args()

    args.build_dir = os.path.normpath(args.build_dir)

    sign_binaries(args.build_dir, ('brave.exe', 'chrome.dll'))


if __name__ == '__main__':
    sys.exit(main())
