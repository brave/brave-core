#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Substitute placeholders in a cargo config template.

Mirrors the upstream tools/rust/build_rust.py pattern: read a checked-in
.template file, apply string.Template substitution from --var KEY=VALUE
arguments, write the result to --out.
"""

import argparse
import string
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--template',
                        required=True,
                        type=Path,
                        help='Path to the .template input file.')
    parser.add_argument('--out',
                        required=True,
                        type=Path,
                        help='Path to write the substituted output to.')
    parser.add_argument(
        '--var',
        action='append',
        default=[],
        metavar='KEY=VALUE',
        help='Substitution applied to $KEY placeholders.  May be repeated.')
    args = parser.parse_args()

    subs: dict[str, str] = {}
    for entry in args.var:
        key, sep, value = entry.partition('=')
        if not key or not sep:
            parser.error(f'--var must be KEY=VALUE, got {entry!r}')
        subs[key] = value

    template = string.Template(args.template.read_text())
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(template.substitute(subs))
    return 0


if __name__ == '__main__':
    sys.exit(main())
