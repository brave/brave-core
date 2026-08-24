# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Base code to for rendering gn args"""

from __future__ import annotations

import argparse
import functools
import json
import sys

import gen_paths

_CHROMIUM_SRC_DIR = gen_paths.BOTS_DIR.parents[2]
sys.path.insert(0, str(_CHROMIUM_SRC_DIR / 'build'))
import gn_helpers


class BotsError(Exception):
    """Raised for a user-facing `bots.py` failure, e.g. an unknown builder."""


def list_generated_builders() -> list[str]:
    """Every builder with a generated `gn-args.json`, sorted by name."""
    if not gen_paths.BUILDERS_OUTPUT_DIR.is_dir():
        return []
    return sorted(p.name for p in gen_paths.BUILDERS_OUTPUT_DIR.iterdir()
                  if (p / 'gn-args.json').is_file())


class OutputGenerator:
    """Resolves and renders one builder's generated `gn-args.json`.

    This class produces the rendered `args.gn` that is used by `bots gen` and
    `bots lookup`.
    """

    def __init__(self, builder_name: str) -> None:
        self.builder_name = builder_name

    def read_generated_gn_args(self) -> dict:
        """Reads this builder's generated `gn-args.json`.

        Raises:
            BotsError: this builder has no generated `gn-args.json`.
        """
        path = (gen_paths.BUILDERS_OUTPUT_DIR / self.builder_name /
                'gn-args.json')
        if not path.is_file():
            available = list_generated_builders()
            hint = (' available builders: %s.' %
                    ', '.join(available) if available else '')
            raise BotsError(
                'builder %r not found under %s; run `bots.py snapshot` '
                'first, or check the name.%s' %
                (self.builder_name, gen_paths.BUILDERS_OUTPUT_DIR, hint))
        return json.loads(path.read_bytes().decode('utf-8'))

    def secrets_import_path(self) -> str:
        """The `//`-prefixed GN label `render_args_gn()` imports secrets from.
        """
        return '//out/%s/secrets.gni' % self.builder_name

    def render_args_gn(self, resolved: dict) -> str:
        """Renders a resolved `gn-args.json` payload as `args.gn` text."""
        lines = []
        if resolved.get('secrets'):
            lines.append('import("%s")' % self.secrets_import_path())
        if resolved.get('args_file'):
            lines.append('import("%s")' % resolved['args_file'])
        lines.append(gn_helpers.ToGNString(resolved['gn_args']))
        return '\n'.join(lines)


def _error_with_available_builders(parser: argparse.ArgumentParser,
                                   message: str) -> None:
    """A `parser.error()` replacement that also lists generated builders.

    This is similar to what you would get with argparser errors, with the only
    addition that it lists the available builders if no builder was provided.
    """
    parser.print_usage(sys.stderr)
    print('%s: error: %s' % (parser.prog, message), file=sys.stderr)
    builders = list_generated_builders()
    if builders:
        print('\navailable builders:', file=sys.stderr)
        for name in builders:
            print('  %s' % name, file=sys.stderr)
    parser.exit(2)


def add_builder_argument(subparser: argparse.ArgumentParser, *,
                         help_text: str) -> None:
    """Adds the `builder` positional shared by every subcommand that reads
    one builder's generated output, wiring its usage errors to list the
    builders actually available under `generated/builders/`.
    """
    subparser.add_argument('builder', help=help_text)
    subparser.error = functools.partial(_error_with_available_builders,
                                        subparser)
