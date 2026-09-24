#!/usr/bin/env python3
# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
import shutil
import sys

from lib.config import SOURCE_ROOT, enable_verbose_mode
from lib.util import execute_stdout, scoped_cwd

WEB_DISCOVERY_DIR = os.path.join(
    SOURCE_ROOT, 'vendor', 'web-discovery-project')


def main():
    args = parse_args()

    pnpm = shutil.which('pnpm')
    if not pnpm:
        raise RuntimeError('Unable to find pnpm in PATH')

    with scoped_cwd(WEB_DISCOVERY_DIR):
        if args.verbose:
            enable_verbose_mode()
        if args.install:
            execute_stdout([pnpm, 'install', '--frozen-lockfile', '--yes'])
        if args.build:
            env = os.environ.copy()
            env["OUTPUT_PATH"] = args.output_path
            execute_stdout([pnpm, 'run', 'build-module'], env=env)


def parse_args():
    parser = argparse.ArgumentParser(description='Web Discovery Project setup')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Prints the output of the subprocesses')
    parser.add_argument('-i', '--install',
                        action='store_true',
                        help='Install Web Discovery Project dependencies')
    parser.add_argument('--output_path')
    parser.add_argument('-b', '--build',
                        action='store_true',
                        help='Build Web Discovery Project')

    return parser.parse_args()


if __name__ == '__main__':
    sys.exit(main())
