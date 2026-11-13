# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Fixtures shared by the bots tests: a builder's generated output, and
scratch directories standing in for the real ones."""

from __future__ import annotations

import contextlib
import json
from pathlib import Path
import tempfile
import unittest

import dotenv
import gen_paths

GN_ARGS = {'gn_args': {'target_os': 'linux', 'target_cpu': 'x64'}}

BUILDER_SPEC = {
    'builder_group': 'brave.test',
    'chromium_config': {'target_platform': 'linux'},
    'gclient_config': {'target_os': [], 'target_cpu': []},
}

SYNC = {'target_os': [], 'target_cpu': []}

TARGETS = {'compile': ['brave:all'], 'tests': ['a_test']}


def properties(
    name: str,
    builder_spec: dict | None = None,
    builder_ids: list[dict] | None = None,
) -> dict:
    """A `properties.json` with *builder_spec* as *name*'s, in its
    `builder_ids` unless they are given."""
    builder_id = {'builder': name}
    return {
        '$brave_tests_builder_config': {
            'builder_config': {
                'builder_db': {
                    'entries': [
                        {
                            'builder_id': builder_id,
                            'builder_spec': builder_spec or BUILDER_SPEC,
                        }
                    ]
                },
                'builder_ids': (
                    [builder_id] if builder_ids is None else builder_ids
                ),
            }
        }
    }


def write_builder(
    output_dir: Path,
    name: str,
    *,
    gn_args: dict | None = None,
    properties_json: dict | None = None,
    sync: dict | None = None,
    targets: dict | None = None,
    omit: tuple[str, ...] = (),
) -> None:
    """Writes a builder's generated output under *output_dir*.

    Each file defaults to valid content. Pass *omit*, file names, to leave
    some out.
    """
    builder_dir = output_dir / name
    builder_dir.mkdir(parents=True, exist_ok=True)
    files = {
        'gn-args.json': GN_ARGS if gn_args is None else gn_args,
        'properties.json': (
            properties(name) if properties_json is None else properties_json
        ),
        'sync.json': SYNC if sync is None else sync,
        'targets.json': TARGETS if targets is None else targets,
    }
    for filename, content in files.items():
        if filename not in omit:
            (builder_dir / filename).write_text(
                json.dumps(content), encoding='utf-8'
            )


def _use(test_case: unittest.TestCase, module, attr: str, value) -> None:
    original = getattr(module, attr)
    setattr(module, attr, value)
    test_case.addCleanup(setattr, module, attr, original)


def _scratch_dir(test_case: unittest.TestCase) -> Path:
    stack = contextlib.ExitStack()
    test_case.addCleanup(stack.close)
    return Path(stack.enter_context(tempfile.TemporaryDirectory())).resolve()


def use_output_dir(test_case: unittest.TestCase) -> Path:
    """Points the generated output at a scratch directory for the test."""
    output_dir = _scratch_dir(test_case)
    _use(test_case, gen_paths, 'BUILDERS_OUTPUT_DIR', output_dir)
    return output_dir


def use_src_dir(test_case: unittest.TestCase) -> Path:
    """Points Chromium's `src/` at a scratch directory for the test."""
    src_dir = _scratch_dir(test_case)
    _use(test_case, gen_paths, 'CHROMIUM_SRC_DIR', src_dir)
    return src_dir


def use_dotenv(test_case: unittest.TestCase, contents: str) -> None:
    """Points the secrets `.env` at a file with *contents* for the test."""
    path = _scratch_dir(test_case) / '.env'
    path.write_text(contents, encoding='utf-8')
    _use(test_case, dotenv, 'DEFAULT_PATH', path)
