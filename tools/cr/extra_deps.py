# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Loads the `EXTRA_DEPS` table and provides the cheap deployment check.

`EXTRA_DEPS` itself lives in the sibling `src/brave/EXTRA_DEPS` data file (see
its header for the entry schema); it is loaded here as data, never executed, so
this module stays stdlib-only and importable under plain `python3`.
"""

from __future__ import annotations

import ast
from pathlib import Path

# The extension we use for all sidecar files. Using stamp is deliberate to avoid
# having gclient trying to read our sidecars.
_STAMP = '.stamp'

# Our EXTRA_DEPS file at the root of the repo.
_EXTRA_DEPS_FILE = Path(__file__).resolve().parents[2] / 'EXTRA_DEPS'


def _load_extra_deps() -> dict:
    """Load the `extra_deps` literal from `_EXTRA_DEPS_FILE`.

    The file is a pure declarative table: its `extra_deps = {...}` assignment is
    read with `ast.literal_eval`, never executed.
    """
    source = _EXTRA_DEPS_FILE.read_text(encoding='utf-8')
    for node in ast.parse(source).body:
        if (isinstance(node, ast.Assign) and len(node.targets) == 1
                and isinstance(node.targets[0], ast.Name)
                and node.targets[0].id == 'extra_deps'):
            return ast.literal_eval(node.value)
    raise ValueError(f'No extra_deps assignment found in {_EXTRA_DEPS_FILE}')


EXTRA_DEPS = _load_extra_deps()


def sidecar_path(dest_dir: Path, object_name: str, suffix: str) -> Path:
    """The `.{prefix}{suffix}.stamp` sidecar path in `dest_dir`.

    `prefix` is the object name with every `/` and `.` mapped to `_`, matching
    how gclient keys the sidecars it drops beside a `gcs` dep's extracted tree.
    """
    prefix = object_name.replace('/', '_').replace('.', '_')
    return dest_dir / f'.{prefix}{suffix}{_STAMP}'


def is_deployed(dest_dir: Path, object_name: str, sha256sum: str) -> bool:
    """True when `dest_dir` holds `object_name` at `sha256sum`.

    The cheapest possible check: it reads the `_hash` sidecar and compares, so
    it reports whether the pinned version is deployed (not merely that files
    exist).
    """
    hash_file = sidecar_path(dest_dir, object_name, '_hash')
    if not hash_file.is_file():
        return False
    return hash_file.read_bytes().decode('utf-8').strip() == sha256sum


def check_extra_deps_installed(root: Path, path: str) -> bool:
    """Whether the entry `path` is already deployed under `root`.

    `root` is the workspace root (the parent of `src`); `path` is the entry's
    key in `EXTRA_DEPS`. Raises `KeyError` if `path` is not a known entry, and
    `ValueError` for a multi-object entry -- picking one of its objects needs
    host-condition resolution this check does not do.
    """
    objects = EXTRA_DEPS[path]['objects']
    if len(objects) != 1:
        raise ValueError(
            f'{path}: check_extra_deps_installed supports single-object '
            f'entries only, but this one has {len(objects)}')
    obj = objects[0]
    return is_deployed(root / path, obj['object_name'], obj['sha256sum'])
