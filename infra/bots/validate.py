# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""bots validate: sanity-check `infra/config/generated/builders/`.

This is an extra validation for the values found in the snapshot, run during
the PRESUBMIT stage. Each builder's own output is checked as it is read (see
`generated_output.GeneratedBuilder`); this adds the checks across builders
and against the machine's secrets.
"""

from __future__ import annotations

import argparse
import dataclasses
import json

import dotenv
import gen_paths
import generated_output

# The placeholder value an unset secret carries.
_UNSET_SECRET_PLACEHOLDER = 'dummy'


def _leaked_secrets(builder_name: str, gn_args_json: dict) -> list[str]:
    """Where a declared secret's real value made it into `gn_args`.

    The real values are those of the secrets `.env` file, by GN arg name.
    """
    secrets = gn_args_json.get('secrets') or {}
    if not secrets:
        return []
    secret_values = dotenv.read()
    problems = []
    for gn_arg_name in secrets:
        value = secret_values.get(gn_arg_name)
        if not value or value == _UNSET_SECRET_PLACEHOLDER:
            continue
        for other_name, other_value in gn_args_json['gn_args'].items():
            if isinstance(other_value, str) and value in other_value:
                problems.append(
                    '%s: gn_args[%r] appears to contain the value of secret '
                    '%r (from %s). Secret values MUST NEVER be checked in'
                    % (
                        gen_paths.BUILDERS_OUTPUT_DIR
                        / builder_name
                        / 'gn-args.json',
                        other_name,
                        gn_arg_name,
                        dotenv.DEFAULT_PATH,
                    )
                )
    return problems


def _duplicates(
    outputs: dict[str, generated_output.GeneratedOutput],
) -> list[str]:
    """The builders whose generated output is the same as another's."""
    seen: dict[str, str] = {}
    problems = []
    for name, output in outputs.items():
        key = json.dumps(dataclasses.asdict(output), sort_keys=True)
        first_seen_as = seen.setdefault(key, name)
        if first_seen_as != name:
            problems.append(
                'builders %r and %r are exact duplicates (identical generated '
                'output); consolidate them into one' % (first_seen_as, name)
            )
    return problems


def find_problems() -> list[str]:
    """Validates every generated builder.

    Returns a list of human-readable problems. Empty means success.
    """
    names = generated_output.list_generated_builders()
    if not names:
        return [
            'no generated builders found under %s; run `bots.py snapshot` '
            'first.' % gen_paths.BUILDERS_OUTPUT_DIR
        ]
    problems = []
    outputs = {}
    for name in names:
        builder = generated_output.GeneratedBuilder(name)
        builder_problems = builder.problems()
        if builder_problems:
            problems.extend(builder_problems)
            continue
        output = builder.read()
        problems.extend(_leaked_secrets(name, output.gn_args))
        outputs[name] = output
    problems.extend(_duplicates(outputs))
    return problems


def cmd_validate(args: argparse.Namespace) -> int:
    problems = find_problems()
    if problems:
        raise generated_output.BotsError(
            'infra/config/generated/builders/ has problems:\n  '
            + '\n  '.join(problems)
        )
    if not args.quiet:
        print(
            'infra/config/generated/builders/ looks ok (%d builder(s)).'
            % len(generated_output.list_generated_builders())
        )
    return 0


def add_subparser(subparsers) -> argparse.ArgumentParser:
    """Registers the `validate` subcommand onto `bots.py`'s subparsers."""
    validate_parser = subparsers.add_parser(
        'validate', description='Sanity-check infra/config/generated/builders/.'
    )
    validate_parser.add_argument(
        '-q',
        '--quiet',
        action='store_true',
        help="Don't print anything on success.",
    )
    validate_parser.set_defaults(func=cmd_validate)
    return validate_parser
