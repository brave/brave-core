# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Print a builder's generated configuration.

Brave builder carries gn args, sync config, and compile/test targets. Use
`--json` to produce JSON for tooling.
"""

from __future__ import annotations

import argparse
import json

import gen_paths
import generated_output


def describe_builder(builder_name: str) -> dict:
    """Reads and merges one builder's generated files.

    Raises:
        BotsError: this builder has no generated `gn-args.json`, same as
            `lookup`/`gen` (including the "available builders" hint).
    """
    gn_args_json = generated_output.OutputGenerator(
        builder_name).read_generated_gn_args()

    result = {'gn_args': gn_args_json}
    builder_dir = gen_paths.BUILDERS_OUTPUT_DIR / builder_name
    for key, filename in (('sync', 'sync.json'), ('targets', 'targets.json')):
        path = builder_dir / filename
        if path.is_file():
            result[key] = json.loads(path.read_bytes().decode('utf-8'))
    return result


def _print_text(name: str, description: dict) -> None:
    """Prints one builder's description for a person to read.
    """
    gn_args = description['gn_args'].get('gn_args', {})
    sync = description.get('sync', {})
    targets = description.get('targets', {})

    print('%s:' % name)
    print('  target_os:  %s' %
          sync.get('target_os', gn_args.get('target_os', '?')))
    print('  target_cpu: %s' %
          sync.get('target_cpu', gn_args.get('target_cpu', '?')))

    overrides = sync.get('gclient_overrides')
    if overrides:
        print('  gclient overrides: %s' %
              ', '.join('%s=%r' % kv for kv in sorted(overrides.items())))

    compile_targets = targets.get('compile', ())
    print('  compile: %s' % (', '.join(compile_targets) or '(none)'))

    tests = targets.get('tests', ())
    print('  tests (%d): %s' % (len(tests), ', '.join(tests) or '(none)'))

    secrets = description['gn_args'].get('secrets')
    if secrets:
        # Names only - these are environment variable names, never values.
        print('  secrets: %s' % ', '.join(sorted(secrets)))

    print('  gn_args (%d): %s' %
          (len(gn_args), ', '.join(sorted(gn_args)) or '(none)'))


def cmd_describe(args: argparse.Namespace) -> int:
    if args.builder:
        names = [args.builder]
    else:
        names = generated_output.list_generated_builders()
        if not names:
            raise generated_output.BotsError(
                'no generated builders found under %s; run `bots.py '
                'snapshot` first.' % gen_paths.BUILDERS_OUTPUT_DIR)

    descriptions = {name: describe_builder(name) for name in names}

    if args.json:
        print(json.dumps(descriptions, indent=2, sort_keys=True))
    else:
        for name in names:
            _print_text(name, descriptions[name])

    return 0


def add_subparser(subparsers) -> argparse.ArgumentParser:
    """Registers the `describe` subcommand onto `bots.py`'s subparsers."""
    describe_parser = subparsers.add_parser(
        'describe',
        description="Describe one builder's generated configuration, or "
        "every builder's when none is given.")
    # Unlike `lookup`/`gen`'s builder argument (generated_output.
    # add_builder_argument), this one is optional: omitting it means
    # "describe everything", not a usage error, so it does not go through
    # that helper's required-positional wiring.
    describe_parser.add_argument(
        'builder',
        nargs='?',
        help='A builder name, e.g. "linux-x64-asan-brave". Omit to '
        'describe every generated builder.')
    describe_parser.add_argument(
        '--json',
        action='store_true',
        help='Print JSON instead of a human-readable report.')
    describe_parser.set_defaults(func=cmd_describe)
    return describe_parser
