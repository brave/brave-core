#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""bots: generates and consumes `brave/infra/config`'s per-builder output.

To generate the builders' output, run:

    python3 infra/bots/bots.py snapshot
"""

from __future__ import annotations

import argparse
import sys

import describe
import gen
import generated_output
import lookup
import snapshot
import validate


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest='command', required=True)

    snapshot.add_subparser(subparsers)
    lookup.add_subparser(subparsers)
    gen.add_subparser(subparsers)
    validate.add_subparser(subparsers)
    describe.add_subparser(subparsers)

    args = parser.parse_args(argv)
    try:
        return args.func(args)
    except generated_output.BotsError as e:
        print(e, file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
