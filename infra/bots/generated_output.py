# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A builder's generated output: the files `bots.py snapshot` writes for it
under `infra/config/generated/builders/<builder>/`.

`render()` makes them from a builder's spec, and `GeneratedBuilder` reads them
back, checking that each carries what its consumers read, so that no other
module needs to know their names or shapes.
"""

from __future__ import annotations

import argparse
import dataclasses
import functools
import json
import sys
from typing import Any

import gen_paths


class BotsError(Exception):
    """Raised for a user-facing `bots.py` failure, e.g. an unknown builder."""


def list_generated_builders() -> list[str]:
    """Every generated builder, by name, sorted."""
    output_dir = gen_paths.BUILDERS_OUTPUT_DIR
    if not output_dir.is_dir():
        return []
    return sorted(p.name for p in output_dir.iterdir() if p.is_dir())


def _dump(data: Any) -> str:
    return json.dumps(data, indent=2, sort_keys=True) + '\n'


def _builder_id(builder) -> dict:
    """A builder's ID, as upstream's `builder_config.star` writes one, less
    the buildbucket project and bucket we have no equivalent of."""
    return {'builder': builder.name}


def _entry(builder) -> dict:
    """A builder's entry in the builder database, as upstream's
    `builder_config.star` writes one.

    Upstream's spec names recipe config items (`legacy_chromium_config`,
    `legacy_gclient_config`); ours carries what they describe, as upstream's
    TODOs ask for. Unset `chromium_config` fields are left out, as upstream
    does.
    """
    chromium_config = builder.builder_spec.chromium_config
    gclient_config = builder.builder_spec.gclient_config
    fields = (
        ('build_config', chromium_config.build_config),
        ('host_platform', chromium_config.host_platform),
        ('target_arch', chromium_config.target_arch),
        ('target_bits', chromium_config.target_bits),
        ('target_platform', chromium_config.target_platform),
    )
    return {
        'builder_id': _builder_id(builder),
        'builder_spec': {
            'builder_group': builder.builder_group,
            'chromium_config': {k: v for k, v in fields if v is not None},
            'gclient_config': {
                'target_os': list(gclient_config.target_os),
                'target_cpu': list(gclient_config.target_cpu),
            },
        },
    }


def _properties(builder) -> dict:
    """A builder's `properties.json`: the input properties of its builds.

    As lucicfg generates them upstream, with the builder's config under the
    `$brave_tests_builder_config` recipe module's properties (upstream's
    `$build/chromium_tests_builder_config`), so that consumers learn what it
    builds and syncs for from here, never from its GN args.
    """
    properties = {
        '$brave_tests_builder_config': {
            'builder_config': {
                'builder_db': {'entries': [_entry(builder)]},
                'builder_ids': [_builder_id(builder)],
            },
        },
        'builder_group': builder.builder_group,
    }
    if builder.recipe:
        properties['recipe'] = builder.recipe
    return properties


def render(builder, gn_args_registry) -> dict[str, str]:
    """The generated output of *builder*, a `lib.config` builder, by file name.

    Raises:
        ConfigError: the builder has no group, or its spec disagrees with its
            GN args.
    """
    # Only importable once `bots.py snapshot` has loaded `infra/config`.
    from lib.config import ConfigError, check_builder_spec

    if not builder.builder_group:
        raise ConfigError(
            'builder %r: builder_group must be set to use '
            'brave_tests_builder_config' % builder.name
        )
    resolved = gn_args_registry.resolve(builder.name)
    check_builder_spec(builder, resolved['gn_args'])
    gclient_config = builder.builder_spec.gclient_config
    return {
        'gn-args.json': _dump(resolved),
        'properties.json': _dump(_properties(builder)),
        'sync.json': _dump(
            {
                'target_os': list(gclient_config.target_os),
                'target_cpu': list(gclient_config.target_cpu),
            }
        ),
        'targets.json': _dump(
            {
                'compile': list(builder.targets.compile),
                'tests': list(builder.targets.tests),
            }
        ),
    }


@dataclasses.dataclass(frozen=True)
class GeneratedOutput:
    """A builder's generated output, as `GeneratedBuilder.read()` reads it."""

    # `gn-args.json`: the resolved `gn_args`, with the `secrets` and
    # `args_file` they declare.
    gn_args: dict
    # The builder's spec, out of its `properties.json`.
    builder_spec: dict
    # `sync.json`: what `init`/`sync` write into `.gclient`.
    sync: dict
    # `targets.json`: what the builder compiles and tests.
    targets: dict


def _is_list_of_strings(value: Any) -> bool:
    return isinstance(value, list) and all(isinstance(v, str) for v in value)


class GeneratedBuilder:
    """Reads one builder's generated output."""

    def __init__(self, name: str) -> None:
        self.name = name

    def read(self) -> GeneratedOutput:
        """Reads the builder's generated output.

        Raises:
            BotsError: the builder is not generated, or a file of its output
                does not carry what its consumers read.
        """
        self._check_generated()
        return GeneratedOutput(
            gn_args=self._read_gn_args(),
            builder_spec=self._read_builder_spec(),
            sync=self._read_sync(),
            targets=self._read_targets(),
        )

    def problems(self) -> list[str]:
        """Everything `read()` would fail on, one entry per file."""
        try:
            self._check_generated()
        except BotsError as e:
            return [str(e)]
        problems = []
        for read in (
            self._read_gn_args,
            self._read_builder_spec,
            self._read_sync,
            self._read_targets,
        ):
            try:
                read()
            except BotsError as e:
                problems.append(str(e))
        return problems

    def _check_generated(self) -> None:
        if (gen_paths.BUILDERS_OUTPUT_DIR / self.name).is_dir():
            return
        available = list_generated_builders()
        hint = (
            ' available builders: %s.' % ', '.join(available)
            if available
            else ''
        )
        raise BotsError(
            'builder %r not found under %s; run `bots.py snapshot` first, or '
            'check the name.%s'
            % (self.name, gen_paths.BUILDERS_OUTPUT_DIR, hint)
        )

    def _read_json(self, filename: str, check) -> dict:
        """Reads *filename*, raising with what `check` finds wrong in it."""
        path = gen_paths.BUILDERS_OUTPUT_DIR / self.name / filename
        if not path.is_file():
            raise BotsError('%s: missing' % path)
        try:
            data = json.loads(path.read_bytes().decode('utf-8'))
        except ValueError as e:
            raise BotsError('%s: invalid JSON (%s)' % (path, e)) from e
        if not isinstance(data, dict):
            raise BotsError(
                '%s: expected a JSON object, got %s'
                % (path, type(data).__name__)
            )
        problems = check(data)
        if problems:
            raise BotsError('%s: %s' % (path, '; '.join(problems)))
        return data

    def _read_gn_args(self) -> dict:
        def check(gn_args_json: dict) -> list[str]:
            gn_args = gn_args_json.get('gn_args')
            if not isinstance(gn_args, dict):
                return ['"gn_args" is missing or not an object']
            problems = [
                '"gn_args" is missing %r' % required
                for required in ('target_os', 'target_cpu')
                if required not in gn_args
            ]
            args_file = gn_args_json.get('args_file')
            if args_file and not args_file.startswith('//'):
                problems.append(
                    'args_file %r is not a source-absolute ("//...") path'
                    % args_file
                )
            elif (
                args_file
                and not (gen_paths.CHROMIUM_SRC_DIR / args_file[2:]).is_file()
            ):
                problems.append('args_file %r does not exist' % args_file)
            return problems

        return self._read_json('gn-args.json', check)

    def _read_builder_spec(self) -> dict:
        builder_id = {'builder': self.name}

        def find(properties: dict) -> Any:
            builder_config = properties.get(
                '$brave_tests_builder_config', {}
            ).get('builder_config', {})
            for entry in builder_config.get('builder_db', {}).get(
                'entries', ()
            ):
                if entry.get('builder_id') == builder_id:
                    return entry.get('builder_spec')
            return None

        def check(properties: dict) -> list[str]:
            builder_ids = (
                properties.get('$brave_tests_builder_config', {})
                .get('builder_config', {})
                .get('builder_ids', ())
            )
            if builder_id not in builder_ids:
                return ['"builder_ids" does not have %r' % self.name]
            builder_spec = find(properties)
            if not isinstance(builder_spec, dict):
                return ['"builder_db" has no spec for %r' % self.name]
            problems = []
            chromium_config = builder_spec.get('chromium_config')
            if not isinstance(chromium_config, dict) or not isinstance(
                chromium_config.get('target_platform'), str
            ):
                problems.append(
                    '"builder_spec.chromium_config" has no target_platform'
                )
            gclient_config = builder_spec.get('gclient_config')
            if not isinstance(gclient_config, dict):
                problems.append(
                    '"builder_spec.gclient_config" is not an object'
                )
                return problems
            problems.extend(
                '"builder_spec.gclient_config.%s" is not a list of strings'
                % field
                for field in ('target_os', 'target_cpu')
                if not _is_list_of_strings(gclient_config.get(field))
            )
            return problems

        return find(self._read_json('properties.json', check))

    def _read_sync(self) -> dict:
        def check(sync: dict) -> list[str]:
            return [
                '%r is not a list of strings' % field
                for field in ('target_os', 'target_cpu')
                if not _is_list_of_strings(sync.get(field))
            ]

        return self._read_json('sync.json', check)

    def _read_targets(self) -> dict:
        def check(targets: dict) -> list[str]:
            return [
                '%r is not a list of strings' % field
                for field in ('compile', 'tests')
                if not _is_list_of_strings(targets.get(field))
            ]

        return self._read_json('targets.json', check)


def _error_with_available_builders(
    parser: argparse.ArgumentParser, message: str
) -> None:
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


def add_builder_argument(
    subparser: argparse.ArgumentParser, *, help_text: str
) -> None:
    """Adds the `builder` positional shared by every subcommand that reads
    one builder's generated output, wiring its usage errors to list the
    builders actually available under `generated/builders/`.
    """
    subparser.add_argument('builder', help=help_text)
    subparser.error = functools.partial(
        _error_with_available_builders, subparser
    )
