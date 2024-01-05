#!/usr/bin/env vpython3

# Copyright (c) 2018 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
import sys

from lib.config import PLATFORM, enable_verbose_mode, is_verbose_mode
from lib.util import execute_stdout, scoped_cwd

NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
    NPM += '.cmd'

def main():
    args = parse_args()
    if not args.yes and PLATFORM != 'win32':
        check_root()
    if args.verbose:
        enable_verbose_mode()

    update_node_modules('.')


def parse_args():
    parser = argparse.ArgumentParser(description='Bootstrap this project')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Prints the output of the subprocesses')
    parser.add_argument('-y', '--yes', '--assume-yes',
                        action='store_true',
                        help='Run non-interactively by assuming "yes" to all '
                        'prompts.')
    return parser.parse_args()

def check_root():
    if os.geteuid() == 0:  # pylint: disable=no-member
        print("We suggest not running this as root, unless you're really sure.")
        choice = input("Do you want to continue? [y/N]: ")
        if choice not in ('y', 'Y'):
            sys.exit(0)

def update_node_modules(dirname, env=None):
    if env is None:
        env = os.environ.copy()
    with scoped_cwd(dirname):
        args = [NPM, 'install', '--no-save', '--yes']
        if is_verbose_mode():
            args += ['--verbose']
        execute_stdout(args, env)


if __name__ == '__main__':
    sys.exit(main())
