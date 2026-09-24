# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""bots validate: sanity-check `infra/config/generated/builders/`.

This is an extra validation for the values found in the snapshot, run during
the PRESUBMIT stage.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import gen_paths
import generated_output
import dotenv

_CHROMIUM_SRC_DIR = gen_paths.BOTS_DIR.parents[2]

# The three files `bots.py snapshot` writes for every builder.
_REQUIRED_FILES = ('gn-args.json', 'sync.json', 'targets.json')

# The placeholder value an unset secret carries.
_UNSET_SECRET_PLACEHOLDER = 'dummy'


def _list_builder_dirs() -> list[Path]:
    """Every immediate subdirectory of `generated/builders/`, sorted by
    name."""
    if not gen_paths.BUILDERS_OUTPUT_DIR.is_dir():
        return []
    return sorted(
        (p for p in gen_paths.BUILDERS_OUTPUT_DIR.iterdir() if p.is_dir()),
        key=lambda p: p.name)


class _Validator:
    """Runs every check over one snapshot of `generated/builders/`.

    A plain function per check would need `errs` and, for the duplicate
    check, every builder's raw file text threaded through as parameters;
    both are just accumulated state, so they live on `self` instead and each
    check appends to `self._errs` directly.
    """

    def __init__(self) -> None:
        self._errs: list[str] = []
        # Builder dir -> {filename: raw text}, for the duplicate-builder
        # check to compare against without re-reading anything from disk.
        self._raw_by_dir: dict[Path, dict[str, str]] = {}

    def run(self) -> list[str]:
        """Validates every generated builder.

        Returns a list of human-readable errors. Empty means success.
        """
        builder_dirs = _list_builder_dirs()
        if not builder_dirs:
            self._errs.append(
                'no generated builders found under %s; run `bots.py '
                'snapshot` first.' % gen_paths.BUILDERS_OUTPUT_DIR)
            return self._errs

        for builder_dir in builder_dirs:
            self._validate_builder(builder_dir)
        self._check_no_duplicate_builders(builder_dirs)
        return self._errs

    def _validate_builder(self, builder_dir: Path) -> None:
        raw = self._read_raw_files(builder_dir)
        self._raw_by_dir[builder_dir] = raw

        gn_args_path = builder_dir / 'gn-args.json'
        gn_args_json = (self._parse_json(gn_args_path, raw['gn-args.json'])
                        if 'gn-args.json' in raw else None)
        if gn_args_json is not None:
            self._check_gn_args_schema(gn_args_path, gn_args_json)
            self._check_args_file_exists(gn_args_path, gn_args_json)
            self._check_secrets_not_leaked(gn_args_path, gn_args_json)

        sync_path = builder_dir / 'sync.json'
        sync_json = (self._parse_json(sync_path, raw['sync.json'])
                     if 'sync.json' in raw else None)
        if sync_json is not None:
            self._check_sync_schema(sync_path, sync_json)

        targets_path = builder_dir / 'targets.json'
        targets_json = (self._parse_json(targets_path, raw['targets.json'])
                        if 'targets.json' in raw else None)
        if targets_json is not None:
            self._check_targets_schema(targets_path, targets_json)

    def _read_raw_files(self, builder_dir: Path) -> dict[str, str]:
        """Reads each of `_REQUIRED_FILES` present under `builder_dir` as
        text.

        A missing file is reported and simply absent from the result, so
        callers can tell "present" from "missing" by dict membership.
        """
        raw = {}
        for filename in _REQUIRED_FILES:
            path = builder_dir / filename
            if not path.is_file():
                self._errs.append('%s: missing' % path)
                continue
            raw[filename] = path.read_bytes().decode('utf-8')
        return raw

    def _parse_json(self, path: Path, text: str) -> dict | None:
        try:
            parsed = json.loads(text)
        except json.JSONDecodeError as e:
            self._errs.append('%s: invalid JSON (%s)' % (path, e))
            return None
        if not isinstance(parsed, dict):
            self._errs.append('%s: expected a JSON object, got %s' %
                              (path, type(parsed).__name__))
            return None
        return parsed

    def _check_gn_args_schema(self, gn_args_path: Path,
                              gn_args_json: dict) -> None:
        """Checks `gn-args.json`'s `gn_args` carries what `gn gen`
        requires."""
        gn_args = gn_args_json.get('gn_args')
        if not isinstance(gn_args, dict):
            self._errs.append('%s: "gn_args" is missing or not an object' %
                              gn_args_path)
            return
        for required in ('target_os', 'target_cpu'):
            if required not in gn_args:
                self._errs.append('%s: "gn_args" is missing %r' %
                                  (gn_args_path, required))

    def _check_args_file_exists(self, gn_args_path: Path,
                                gn_args_json: dict) -> None:
        """Checks a declared `args_file` is a source-absolute path that
        exists.
        """
        args_file = gn_args_json.get('args_file')
        if not args_file:
            return
        if not args_file.startswith('//'):
            self._errs.append(
                '%s: args_file %r is not a source-absolute ("//...") path' %
                (gn_args_path, args_file))
            return
        if not (_CHROMIUM_SRC_DIR / args_file[2:]).is_file():
            self._errs.append('%s: args_file %r does not exist' %
                              (gn_args_path, args_file))

    def _check_secrets_not_leaked(self, gn_args_path: Path,
                                  gn_args_json: dict) -> None:
        """Checks a declared secret's real value never made it into
        `gn_args`.

        The secrets `.env` file (`dotenv.py`) is a list of `gn` values.
        """
        secrets = gn_args_json.get('secrets') or {}
        gn_args = gn_args_json.get('gn_args') or {}
        if not secrets:
            return
        secret_values = dotenv.read()
        for gn_arg_name in secrets:
            value = secret_values.get(gn_arg_name)
            if not value or value == _UNSET_SECRET_PLACEHOLDER:
                continue
            for other_name, other_value in gn_args.items():
                if isinstance(other_value, str) and value in other_value:
                    self._errs.append(
                        '%s: gn_args[%r] appears to contain the value of '
                        'secret %r (from %s). Secret values MUST NEVER be '
                        'checked in' % (gn_args_path, other_name, gn_arg_name,
                                        dotenv.DEFAULT_PATH))

    def _check_sync_schema(self, sync_path: Path, sync_json: dict) -> None:
        for required in ('target_os', 'target_cpu', 'gclient_overrides'):
            if required not in sync_json:
                self._errs.append('%s: missing %r' % (sync_path, required))

    def _check_targets_schema(self, targets_path: Path,
                              targets_json: dict) -> None:
        for required in ('compile', 'tests'):
            value = targets_json.get(required)
            if required not in targets_json:
                self._errs.append('%s: missing %r' % (targets_path, required))
            elif not isinstance(value, list):
                self._errs.append('%s: %r is not a list' %
                                  (targets_path, required))

    def _check_no_duplicate_builders(self, builder_dirs: list[Path]) -> None:
        """Flags duplicate builders that are identical."""
        seen: dict[tuple[str, ...], str] = {}
        for builder_dir in builder_dirs:
            raw = self._raw_by_dir[builder_dir]
            if len(raw) != len(_REQUIRED_FILES):
                continue  # Already reported as missing/unreadable above.
            key = tuple(raw[name] for name in _REQUIRED_FILES)
            first_seen_as = seen.get(key)
            if first_seen_as is None:
                seen[key] = builder_dir.name
            else:
                self._errs.append(
                    'builders %r and %r are exact duplicates (identical '
                    'gn-args.json, sync.json and targets.json); consolidate '
                    'them into one' % (first_seen_as, builder_dir.name))


def cmd_validate(args: argparse.Namespace) -> int:
    errs = _Validator().run()
    if errs:
        raise generated_output.BotsError(
            'infra/config/generated/builders/ has problems:\n  ' +
            '\n  '.join(errs))
    if not args.quiet:
        print('infra/config/generated/builders/ looks ok (%d builder(s)).' %
              len(_list_builder_dirs()))
    return 0


def add_subparser(subparsers) -> argparse.ArgumentParser:
    """Registers the `validate` subcommand onto `bots.py`'s subparsers."""
    validate_parser = subparsers.add_parser(
        'validate',
        description='Sanity-check infra/config/generated/builders/.')
    validate_parser.add_argument('-q',
                                 '--quiet',
                                 action='store_true',
                                 help="Don't print anything on success.")
    validate_parser.set_defaults(func=cmd_validate)
    return validate_parser
