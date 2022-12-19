#!/usr/bin/env python3
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Converts Chrome-generated breakpad symbols to Backtrace archive format."""

import argparse
import errno
import glob
import os
import re
import shutil
import sys


def mkdir_p(path):
    """Simulates mkdir -p."""
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--symbols-dir',
                        required=True,
                        help='The directory where to write the symbols file.')
    parser.add_argument(
        '--input-breakpad-files-glob',
        required=True,
        help='The glob to search for generated breakpad files.')
    parser.add_argument(
        '--clear',
        default=False,
        action='store_true',
        help='Clear the symbols directory before writing new symbols.')

    options = parser.parse_args()

    if options.clear:
        try:
            shutil.rmtree(options.symbols_dir)
        except:  # pylint: disable=bare-except
            pass

    input_breakpad_files = glob.glob(options.input_breakpad_files_glob)
    if not input_breakpad_files:
        raise FileNotFoundError(
            f"Cannot find breakpad files: {options.input_breakpad_files_glob}")

    for input_breakpad_file in input_breakpad_files:
        with open(input_breakpad_file, mode="r", encoding="utf-8") as f:
            first_line = f.readline()

        module_line = re.match("MODULE [^ ]+ [^ ]+ ([0-9A-F]+) (.+)",
                               first_line)
        if not module_line:
            raise RuntimeError(
                f"Unexpected breakpad MODULE line: {first_line}")
        output_path = os.path.join(options.symbols_dir, module_line.group(2),
                                   module_line.group(1))
        output_breakpad_file = os.path.join(output_path,
                                            f"{module_line.group(2)}.sym")

        mkdir_p(output_path)
        shutil.copyfile(input_breakpad_file, output_breakpad_file)

    return 0


if __name__ == '__main__':
    sys.exit(main())
