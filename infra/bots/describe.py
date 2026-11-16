# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Print a builder's generated configuration.

A builder carries GN args, its spec, the `.gclient` lists it syncs, and the
targets it compiles and tests. Use `--json` to produce JSON for tooling.
"""

from __future__ import annotations

import argparse
import dataclasses
import json

import gen_paths
import generated_output


def describe_builder(builder_name: str) -> dict:
    """One builder's generated output.

    Raises:
        BotsError: the builder's generated output cannot be read.
    """
    return dataclasses.asdict(
        generated_output.GeneratedBuilder(builder_name).read()
    )


def _print_text(name: str, description: dict) -> None:
    """Prints one builder's description for a person to read."""
    gn_args = description['gn_args']['gn_args']
    chromium_config = description['builder_spec']['chromium_config']
    sync = description['sync']
    targets = description['targets']

    print('%s:' % name)
    print('  target_os:  %s' % gn_args['target_os'])
    print('  target_cpu: %s' % gn_args['target_cpu'])
    if 'build_config' in chromium_config:
        print('  build_config: %s' % chromium_config['build_config'])
    # What the checkout syncs, besides the host (always synced).
    print('  sync target_os:  %s' % (', '.join(sync['target_os']) or '(host)'))
    print('  sync target_cpu: %s' % (', '.join(sync['target_cpu']) or '(host)'))

    compile_targets = targets['compile']
    print('  compile: %s' % (', '.join(compile_targets) or '(none)'))

    tests = targets['tests']
    print('  tests (%d): %s' % (len(tests), ', '.join(tests) or '(none)'))

    secrets = description['gn_args'].get('secrets')
    if secrets:
        # Names only - these are environment variable names, never values.
        print('  secrets: %s' % ', '.join(sorted(secrets)))

    print(
        '  gn_args (%d): %s'
        % (len(gn_args), ', '.join(sorted(gn_args)) or '(none)')
    )


def cmd_describe(args: argparse.Namespace) -> int:
    if args.builder:
        names = [args.builder]
    else:
        names = generated_output.list_generated_builders()
        if not names:
            raise generated_output.BotsError(
                'no generated builders found under %s; run `bots.py '
                'snapshot` first.' % gen_paths.BUILDERS_OUTPUT_DIR
            )

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
        "every builder's when none is given.",
    )
    # Unlike `lookup`/`gen`'s builder argument (generated_output.
    # add_builder_argument), this one is optional: omitting it means
    # "describe everything", not a usage error, so it does not go through
    # that helper's required-positional wiring.
    describe_parser.add_argument(
        'builder',
        nargs='?',
        help='A builder name, e.g. "linux-x64-asan-brave". Omit to '
        'describe every generated builder.',
    )
    describe_parser.add_argument(
        '--json',
        action='store_true',
        help='Print JSON instead of a human-readable report.',
    )
    describe_parser.set_defaults(func=cmd_describe)
    return describe_parser
