#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Generate Brave's build-defaults matrix from a declarative configs.pyl.

This is a small, standalone analogue of Chromium's //tools/mb/mb.py. It reads
//brave/tools/bots/configs.pyl, which catalogs the GN values Brave always
overrides in Chromium as reusable `mixins`, named `configs` that compose those
mixins, and a `builder_groups` matrix of build-default targets. For every
target it flattens the referenced config into a single GN args dict and writes
it to:

  //brave/build/defaults/generated/<target>/gn-args.json

Unlike upstream mb_config.pyl (which expresses `gn_args` as a space-separated
string), Brave's configs.pyl uses a typed dict for `gn_args` so values are
real Python literals (bool/str/int/list).

Usage:
  bots.py gen [--check]   Generate (or, with --check, verify up-to-date) files.
  bots.py validate        Parse and flatten everything without writing.
"""

import argparse
import ast
import json
import os
import sys

# //brave/tools/bots/bots.py -> up to //brave -> up to // (src).
_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_BRAVE_DIR = os.path.dirname(os.path.dirname(_THIS_DIR))
_SRC_DIR = os.path.dirname(_BRAVE_DIR)

_DEFAULT_CONFIG = os.path.join(_THIS_DIR, 'configs.pyl')

# Generated build-defaults matrix lives here, one gn-args.json per target.
_OUTPUT_SUBDIR = os.path.join('brave', 'build', 'defaults', 'generated')


class BotsError(Exception):
    """A configuration or generation error with a user-facing message."""


def load_config(config_path):
    """Parses a configs.pyl file into a validated dict.

  The file is a Python literal (with '#' comments), identical in spirit to
  Chromium's .pyl files. We use ast.literal_eval so arbitrary code cannot run.
  """
    with open(config_path, encoding='utf-8') as f:
        try:
            contents = ast.literal_eval(f.read())
        except (ValueError, SyntaxError) as e:
            raise BotsError(f'Failed to parse {config_path}: {e}') from e

    if not isinstance(contents, dict):
        raise BotsError(f'{config_path} must contain a dict at the top level')
    for key in ('mixins', 'configs', 'builder_groups'):
        contents.setdefault(key, {})
    return contents


def flatten_config(configs, mixins, config_name):
    """Flattens a named config into a single ordered GN args dict."""
    if config_name not in configs:
        raise BotsError(f'Unknown config "{config_name}"')
    vals = {}
    _flatten_mixins(mixins, configs[config_name], vals, set())
    return vals


def _flatten_mixins(mixins, names, vals, visiting):
    """Applies `names` in order, recursing into nested mixins.

  Mirrors mb.py's FlattenMixins: a mixin's own `gn_args` is applied first, then
  its nested `mixins` are applied (so nested values override the parent's own).
  Across a list, later entries win on duplicate keys.
  """
    for name in names:
        if name not in mixins:
            raise BotsError(f'Unknown mixin "{name}"')
        if name in visiting:
            raise BotsError(f'Cyclic mixin reference involving "{name}"')
        visiting.add(name)

        mixin = mixins[name]
        gn_args = mixin.get('gn_args', {})
        if not isinstance(gn_args, dict):
            raise BotsError(f'Mixin "{name}" gn_args must be a dict')
        vals.update(gn_args)

        _flatten_mixins(mixins, mixin.get('mixins', []), vals, visiting)
        visiting.discard(name)


def iter_builders(contents):
    """Yields (builder_group, bot, config_name), erroring on duplicate bots."""
    seen = {}
    for group, builders in contents['builder_groups'].items():
        for bot, config_name in builders.items():
            if bot in seen:
                raise BotsError(
                    f'Bot "{bot}" defined in both "{seen[bot]}" and "{group}"')
            seen[bot] = group
            yield group, bot, config_name


def render(gn_args):
    """Serializes a flattened gn_args dict to gn-args.json file contents."""
    return json.dumps({'gn_args': gn_args}, sort_keys=True, indent=2) + '\n'


def generate(contents):
    """Returns {absolute_output_path: file_contents} for all bots."""
    configs = contents['configs']
    mixins = contents['mixins']
    result = {}
    for _group, bot, config_name in iter_builders(contents):
        gn_args = flatten_config(configs, mixins, config_name)
        out_path = os.path.join(_SRC_DIR, _OUTPUT_SUBDIR, bot, 'gn-args.json')
        result[out_path] = render(gn_args)
    return result


def cmd_gen(args):
    contents = load_config(args.config)
    files = generate(contents)

    if args.check:
        stale = []
        for path, expected in sorted(files.items()):
            actual = None
            if os.path.exists(path):
                with open(path, encoding='utf-8') as f:
                    actual = f.read()
            if actual != expected:
                stale.append(path)
        if stale:
            rel = [os.path.relpath(p, _SRC_DIR) for p in stale]
            print('gn-args.json files are out of date; run `bots.py gen`:',
                  file=sys.stderr)
            for p in rel:
                print(f'  {p}', file=sys.stderr)
            return 1
        print(f'{len(files)} gn-args.json file(s) up to date.')
        return 0

    for path, expected in sorted(files.items()):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(expected)
    print(f'Wrote {len(files)} gn-args.json file(s) under '
          f'{_OUTPUT_SUBDIR}{os.sep}.')
    return 0


def cmd_validate(args):
    contents = load_config(args.config)
    count = 0
    for _group, _bot, config_name in iter_builders(contents):
        flatten_config(contents['configs'], contents['mixins'], config_name)
        count += 1
    print(f'OK: {count} builder(s) validated.')
    return 0


def main(argv):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--config',
                        default=_DEFAULT_CONFIG,
                        help='Path to configs.pyl (default: sibling file).')
    subparsers = parser.add_subparsers(dest='command')

    gen = subparsers.add_parser('gen', help='Generate gn-args.json files.')
    gen.add_argument(
        '--check',
        action='store_true',
        help='Verify files are up to date instead of writing them.')
    gen.set_defaults(func=cmd_gen)

    validate = subparsers.add_parser(
        'validate', help='Parse and flatten every config without writing.')
    validate.set_defaults(func=cmd_validate)

    args = parser.parse_args(argv)
    if not getattr(args, 'func', None):
        args.func = cmd_gen
        args.check = False

    try:
        return args.func(args)
    except BotsError as e:
        print(f'Error: {e}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
