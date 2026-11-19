# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A builder's generated output: the files `bots.py snapshot` writes for it
under `infra/config/generated/builders/<builder>/`.

`render()` makes them from a builder's spec, and `GeneratedBuilder` reads them
back, checking that each carries what its consumers read, so that no other
module needs to know their names or shapes. `render_defaults()` makes the
`.gni` defaults every Brave build imports, under
`infra/config/generated/defaults/`.
"""

from __future__ import annotations

import argparse
import dataclasses
import functools
import json
import shutil
import subprocess
import sys
import textwrap
from typing import Any

import gen_paths

if str(gen_paths.CHROMIUM_SRC_DIR / 'build') not in sys.path:
    sys.path.insert(0, str(gen_paths.CHROMIUM_SRC_DIR / 'build'))
import gn_helpers  # pylint: disable=wrong-import-position


class BotsError(Exception):
    """Raised for a user-facing `bots.py` failure, e.g. an unknown builder."""


def list_generated_builders() -> list[str]:
    """Every generated builder, by name, sorted.

    A directory without files is no builder: `bots.py snapshot` leaves a
    removed builder's behind, and git never checks one out.
    """
    output_dir = gen_paths.BUILDERS_OUTPUT_DIR
    if not output_dir.is_dir():
        return []
    return sorted(
        p.name
        for p in output_dir.iterdir()
        if p.is_dir() and any(f.is_file() for f in p.rglob('*'))
    )


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
    brave_config = builder.builder_spec.brave_config
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
                'custom_deps': dict(gclient_config.custom_deps),
                'custom_vars': dict(gclient_config.custom_vars),
                'global_vars': dict(gclient_config.global_vars),
            },
            'brave_config': {
                'is_brave_release_build': brave_config.is_brave_release_build,
                'ignore_patch_version_number': (
                    brave_config.ignore_patch_version_number
                ),
                'use_brave_hermetic_toolchain': (
                    brave_config.use_brave_hermetic_toolchain
                ),
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
    # Upstream's `$build/siso`, with the fields Brave's builders set.
    siso = {
        key: value
        for key, value in (
            ('remote_jobs', builder.siso_remote_jobs),
            ('reapi_priority', builder.siso_reapi_priority),
        )
        if value is not None
    }
    if siso:
        properties['$brave_siso'] = siso
    if builder.archive_isolates:
        properties['$isolate'] = {'enabled': True}
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
    return {
        'gn-args.json': _dump(resolved),
        'properties.json': _dump(_properties(builder)),
        f'targets/{builder.builder_group}.json': _dump(
            {builder.name: _targets_spec(builder.targets, resolved['gn_args'])}
        ),
    }


def render_isolate_map(targets_registry) -> str:
    """`gn_isolate_map.pyl`: every declared binary's GN label and type, which
    `runtest.py` reads to run a windowed suite under Xvfb."""
    isolate_map = {
        binary.name: {'label': binary.label, 'type': binary.type}
        for binary in targets_registry.all_binaries()
    }
    return (
        '# Generated by `bots.py snapshot` from infra/config/targets/; do not '
        'edit.\n\n' + json.dumps(isolate_map, indent=2, sort_keys=True) + '\n'
    )


def _comment(text: str) -> list[str]:
    """`text` as `#` lines, each of its lines wrapped on its own."""
    lines = []
    for line in text.split('\n'):
        wrapped = textwrap.wrap(
            line, width=78, break_long_words=False, break_on_hyphens=False
        )
        lines.extend('# ' + part for part in wrapped or [''])
    return [line.rstrip() for line in lines]


def _render_defaults_file(config) -> str:
    """One config's `defaults_file`: what it is generated from, its
    description, then its args."""
    # Only importable once `bots.py snapshot` has loaded `infra/config`.
    from lib.config import thaw

    args = thaw(config.gn_args)
    blocks = [
        _comment(
            f'Generated by `bots.py snapshot` from the `{config.name}` config '
            'in infra/config/gn_args.py; do not edit.'
        )
    ]
    if config.description:
        blocks.extend(
            _comment(paragraph)
            for paragraph in config.description.split('\n\n')
        )
    blocks.append(
        [
            f'{arg} = {gn_helpers.ToGNString(value)}'
            for arg, value in args.items()
        ]
    )
    return '\n\n'.join('\n'.join(block) for block in blocks if block) + '\n'


def render_defaults(gn_args_registry) -> dict[str, str]:
    """`{defaults_file: content}` for every config with a `defaults_file`,
    before `gn format`."""
    return {
        config.defaults_file: _render_defaults_file(config)
        for config in gn_args_registry.defaults_files()
    }


def gn_format(text: str) -> str:
    """`text`, formatted as `gn format` formats a `.gni`.

    Raises:
        BotsError: `gn` is not on `PATH`, or cannot format `text`.
    """
    gn = shutil.which('gn')
    if gn is None:
        raise BotsError('gn is not on PATH; it comes with depot_tools.')
    result = subprocess.run(
        [gn, 'format', '--stdin'],
        input=text,
        capture_output=True,
        text=True,
        cwd=gen_paths.CHROMIUM_SRC_DIR,
        check=False,
    )
    if result.returncode != 0:
        raise BotsError(f'gn format failed:\n{result.stderr or result.stdout}')
    return result.stdout


def _targets_spec(targets, gn_args: dict) -> dict:
    """A builder's entry in its group's targets spec,
    `targets/<builder_group>.json`.

    Each test's arguments end with the filter files that apply to it.
    """
    spec = {}
    if targets.additional_compile_targets:
        spec['additional_compile_targets'] = list(
            targets.additional_compile_targets
        )
    spec['gtest_tests'] = []
    for test in targets.gtest_tests:
        test = dict(test)
        filter_files = _filter_files(test['test'], gn_args)
        if filter_files:
            test['args'] = [
                *test.get('args', []),
                '--test-launcher-filter-file=' + ';'.join(filter_files),
            ]
        spec['gtest_tests'].append(test)
    return spec


# Suites that run another suite's tests, and so take its filters too.
_ADDITIONAL_SUITE_FILTERS = {'updater_tests_system': ['updater_tests']}


def _filter_files(suite: str, gn_args: dict) -> list[str]:
    """The filter files that apply to `suite` on a build with `gn_args`, as
    `pnpm run test` picks them (`getApplicableFilters()` in testUtils.ts):
    Brave's by suite, OS, CPU and sanitiser, and upstream's ASan filter.

    The paths are relative to a build directory under `out/`, as a test
    launcher reads them.
    """
    src_dir = gen_paths.CHROMIUM_SRC_DIR
    target_os = gn_args['target_os']
    target_cpu = gn_args['target_cpu']
    filter_dirs = (
        src_dir / 'brave' / 'test' / 'filters',
        src_dir / 'brave' / 'test' / 'filters' / 'generated',
    )
    files = []
    for name in [*_ADDITIONAL_SUITE_FILTERS.get(suite, []), suite]:
        names = [
            name,
            f'{name}-{target_os}',
            f'{name}-{target_os}-{target_cpu}',
        ]
        for sanitiser in ('ubsan', 'asan', 'msan'):
            if gn_args.get(f'is_{sanitiser}'):
                names.append(f'{name}-{target_os}-{sanitiser}')
        candidates = [
            directory / f'{filter_name}.filter'
            for filter_name in names
            for directory in filter_dirs
        ]
        if gn_args.get('is_asan'):
            candidates.append(
                src_dir
                / 'testing'
                / 'buildbot'
                / 'filters'
                / f'{target_os}.asan.{name}.filter'
            )
        files.extend(
            '../../' + path.relative_to(src_dir).as_posix()
            for path in candidates
            if path.is_file()
        )
    return files


@dataclasses.dataclass(frozen=True)
class GeneratedOutput:
    """A builder's generated output, as `GeneratedBuilder.read()` reads it."""

    # `gn-args.json`: the resolved `gn_args`, with the `secrets` and
    # `args_file` they declare.
    gn_args: dict
    # The builder's spec, out of its `properties.json`.
    builder_spec: dict
    # Its entry in `targets/<builder_group>.json`: its tests, with their
    # arguments.
    targets_spec: dict


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
            targets_spec=self._read_targets_spec(),
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
            self._read_targets_spec,
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
            problems.extend(
                '"builder_spec.gclient_config.%s" is not an object' % field
                for field in ('custom_deps', 'custom_vars', 'global_vars')
                if not isinstance(gclient_config.get(field), dict)
            )
            siso = properties.get('$brave_siso', {})
            if not isinstance(siso, dict):
                problems.append('"$brave_siso" is not an object')
            else:
                problems.extend(
                    '"$brave_siso.%s" is not a positive integer' % field
                    for field, value in sorted(siso.items())
                    if field not in ('remote_jobs', 'reapi_priority')
                    or not isinstance(value, int)
                    or isinstance(value, bool)
                    or value < 1
                )
            brave_config = builder_spec.get('brave_config')
            if not isinstance(brave_config, dict):
                problems.append('"builder_spec.brave_config" is not an object')
            else:
                problems.extend(
                    '"builder_spec.brave_config.%s" is not a bool' % field
                    for field in (
                        'is_brave_release_build',
                        'ignore_patch_version_number',
                        'use_brave_hermetic_toolchain',
                    )
                    if not isinstance(brave_config.get(field), bool)
                )
            return problems

        return find(self._read_json('properties.json', check))

    def _read_targets_spec(self) -> dict:
        targets_dir = gen_paths.BUILDERS_OUTPUT_DIR / self.name / 'targets'
        files = sorted(targets_dir.glob('*.json'))
        if len(files) != 1:
            raise BotsError(
                '%s: expected one <builder_group>.json, found %d'
                % (targets_dir, len(files))
            )

        def check(spec_file: dict) -> list[str]:
            spec = spec_file.get(self.name)
            if not isinstance(spec, dict):
                return ['has no entry for %r' % self.name]
            problems = []
            if 'additional_compile_targets' in spec and not _is_list_of_strings(
                spec['additional_compile_targets']
            ):
                problems.append(
                    '"additional_compile_targets" is not a list of strings'
                )
            tests = spec.get('gtest_tests')
            if not isinstance(tests, list):
                return problems + ['"gtest_tests" is not a list']
            for index, test in enumerate(tests):
                if not (
                    isinstance(test, dict)
                    and isinstance(test.get('name'), str)
                    and isinstance(test.get('test'), str)
                    and _is_list_of_strings(test.get('args', []))
                ):
                    problems.append(
                        '"gtest_tests"[%d] needs a "name" and a "test", and '
                        'its "args" must be strings' % index
                    )
            return problems

        return self._read_json('targets/' + files[0].name, check)[self.name]


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
