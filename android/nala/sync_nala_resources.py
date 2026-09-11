#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Copies a list of Nala resource files into a generated resource directory.

Like copy(), but also deletes anything in the destination that is not in the
list. The file list comes from Nala's generated sources.gni, so it changes on a
Nala bump without anyone editing BUILD.gn. GN and Ninja leave the outputs of a
copy() that no longer exists behind, and prepare_resources.py then fails the
build because the orphan isn't listed in the android_resources() sources:

  Error: Found files not listed in the sources list of the BUILD.gn target:
  gen/brave/android/nala/res/color/material_thin.xml

Removing the orphans here keeps an incremental build working across a bump.
"""

import argparse
import os
import shutil
import sys


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-dir',
                        required=True,
                        help='Directory in Nala to copy the resources from.')
    parser.add_argument('--output-dir',
                        required=True,
                        help='Generated resource directory to write to. Any '
                        'file in here that is not in `names` is deleted, so '
                        'this must not be shared with another target.')
    parser.add_argument('names',
                        nargs='+',
                        help='Resource file names, present in both dirs.')
    options = parser.parse_args(argv)

    os.makedirs(options.output_dir, exist_ok=True)

    expected = set(options.names)
    for orphan in sorted(set(os.listdir(options.output_dir)) - expected):
        os.remove(os.path.join(options.output_dir, orphan))

    # Copy unconditionally: skipping an unchanged file would leave its mtime
    # older than the source and make Ninja re-run this action every build.
    for name in sorted(expected):
        shutil.copyfile(os.path.join(options.source_dir, name),
                        os.path.join(options.output_dir, name))


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
