# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""bots lookup: resolve and print the args.gn a builder would produce.
"""

from __future__ import annotations

import argparse

import generated_output


def cmd_lookup(args: argparse.Namespace) -> int:
    generator = generated_output.OutputGenerator(args.builder)
    resolved = generator.read_generated_gn_args()
    args_gn = generator.render_args_gn(resolved)
    if args.quiet:
        print(args_gn, end='')
    else:
        print('\nWriting """\\\n%s""" to out/%s/args.gn.\n' %
              (args_gn, args.builder))
    return 0


def add_subparser(subparsers) -> argparse.ArgumentParser:
    """Registers the `lookup` subcommand onto `bots.py`'s subparsers."""
    lookup_parser = subparsers.add_parser(
        'lookup',
        description='Look up the args.gn a builder would resolve to.')
    generated_output.add_builder_argument(lookup_parser,
                                          help_text='A builder name, e.g. '
                                          '"linux-x64-asan-brave".')
    lookup_parser.add_argument(
        '--quiet',
        action='store_true',
        help='Print just the args.gn contents (like `mb.py lookup --quiet`), '
        'instead of the human-readable "would write" preamble.')
    lookup_parser.set_defaults(func=cmd_lookup)
    return lookup_parser
