#!/usr/bin/env vpython3
#
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# /// script
# dependencies = [
#   'lxml==4.9.3'
# ]
# ///
"""Fails if an upstream GRD/GRDP file has strings that Brave rebrands but is
not in the localization mapping. Reads pristine files from Chromium's HEAD."""

import argparse
import os
import subprocess
import sys

BRAVE_SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SRC_SOURCE_ROOT = os.path.dirname(BRAVE_SOURCE_ROOT)

sys.path.insert(0, os.path.join(BRAVE_SOURCE_ROOT, 'script'))

from lib.l10n.grd_coverage import find_unmapped  # pylint: disable=wrong-import-position


def git(*args):
    return subprocess.run(
        ['git', '-C', SRC_SOURCE_ROOT, *args],
        check=True,
        capture_output=True,
    ).stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--mapped',
        nargs='*',
        default=[],
        help='Chromium GRD/GRDP paths in the mapping, relative to src',
    )
    args = parser.parse_args()

    paths = git('ls-tree', '-r', '--name-only', 'HEAD').decode().split('\n')
    paths = [p for p in paths if p.endswith(('.grd', '.grdp'))]
    unmapped = find_unmapped(
        paths, lambda p: git('show', f'HEAD:{p}'), args.mapped
    )
    if not unmapped:
        print('All upstream GRD/GRDP files with brand names are covered.')
        return 0

    print(
        'These upstream files have strings that Brave rebrands but are not in '
        'the localization mapping (build/commands/lib/l10nUtil.js). Add them '
        'there and in build/commands/lib/branding.js, or allowlist them with '
        'a reason in script/lib/l10n/grd_coverage.py:'
    )
    for path, names in unmapped.items():
        print(f'  {path} ({len(names)} strings, e.g. {names[0]})')
    return 1


if __name__ == '__main__':
    sys.exit(main())
