#!/usr/bin/env python3

# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
The main motivation for this file is to work around the following limitation of
upstream's sign_updater.py:

Upstream's sign_updater.py calls `shutil.move("UpdaterSetup", output_dir)`,
where `output_dir` is given by the `--output` parameter. This fails when the
destination file already exists. As a result, upstream's sign_updater.py cannot
be called multiple times, which prevents incremental compilation.

Furthermore, upstream's sign_updater.py places a lot of files into the
`--output` directory that we don't need. The implementation in this file also
makes sure that only the necessary file is created there.
"""

from argparse import ArgumentParser
from os.path import join
from subprocess import run
from tempfile import TemporaryDirectory

import os
import sys


def main():
    args, upstream_args = parse_args()
    with TemporaryDirectory() as tmp_dir:
        new_args = upstream_args + ['--output', tmp_dir]
        exit_code = call_upstream(args.upstream_sign_updater_py, new_args)
        if exit_code == 0:
            os.rename(join(tmp_dir, args.output_file),
                      join(args.output_dir, args.output_file))
        return exit_code


def call_upstream(upstream_sign_updater_py, args):
    # pylint: disable=subprocess-run-check
    cp = run([sys.executable, upstream_sign_updater_py] + args)
    return cp.returncode


def parse_args():
    parser = ArgumentParser()
    parser.add_argument('upstream_sign_updater_py')
    parser.add_argument('output_dir')
    parser.add_argument('output_file')
    return parser.parse_known_args()


if __name__ == '__main__':
    sys.exit(main())
