#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Removes stale files and directories that are no longer needed."""

import argparse
import os
import shutil


def RemoveAllStaleFiles(paths):
    for path in paths:
        try:
            if os.path.isdir(path) and not os.path.islink(path):
                shutil.rmtree(path)
            elif os.path.lexists(path):
                os.remove(path)
        except OSError:
            # Another process may have touched this path.
            pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        'paths',
        nargs='*',
        metavar='PATH',
        help='File or directory to remove, if present.',
    )
    args = parser.parse_args()
    RemoveAllStaleFiles(args.paths)


if __name__ == '__main__':
    main()
