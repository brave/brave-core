# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `context` module: scoped adjustments to how steps run.

This module manipulates a few pieces of 'ambient' data that affect the steps run
within a `with` block:

  * `cwd`: the working directory steps run in.
  * `env`: whole-variable environment overrides.
  * `env_prefixes` / `env_suffixes`: path-like values prepended / appended to
    a variable (e.g. `PATH`), joined with the OS path separator by the step
    runner.

Everything is scoped with Python's `with`: there is no open-ended mutation the
way `os.environ` / `os.chdir` (or `env.prepend_path`) allow. Values are pushed
on entry and restored on exit, so a change never surprises a later step, and
nested contexts compose (prefixes accumulate, inner values shadow outer ones):

    with api.context(env_prefixes={'PATH': [node_bin]}):
        # This step sees `node_bin` prepended to PATH.
        api.step('node --version', [node, '--version'])
"""

from __future__ import annotations

import collections
from collections.abc import Mapping, Sequence
import contextlib
from pathlib import Path
from typing import Any

from recipe_api import RecipeApi


def _check_type(name: str, var: object,
                expect: type | tuple[type, ...]) -> None:
    if not isinstance(var, expect):
        expected = getattr(expect, '__name__', str(expect))
        raise TypeError(
            f'{name} is not {expected}: {var!r} ({type(var).__name__})')


class ContextApi(RecipeApi):
    """Scoped ambient settings (cwd + environment) applied to steps.

    The single per-run instance holds the current scope. `__call__` pushes new
    values for the duration of a `with` block and restores them afterwards. The
    `step` module reads the accessors below when building each step.
    """

    def __init__(self) -> None:
        super().__init__()
        # Current scope. Replaced wholesale (never mutated in place) by
        # `__call__`, so an outer scope's dicts are never disturbed.
        self._cwd: Path | None = None
        self._env: dict[str, str | None] = {}
        self._env_prefixes: dict[str, tuple[str, ...]] = {}
        self._env_suffixes: dict[str, tuple[str, ...]] = {}

    @contextlib.contextmanager
    def __call__(
        self,
        cwd: str | Path | None = None,
        env_prefixes: Mapping[str, Sequence[str | Path]] | None = None,
        env_suffixes: Mapping[str, Sequence[str | Path]] | None = None,
        env: Mapping[str, str | None] | None = None,
    ):
        """Adjust cwd/env for the steps run within the `with` block.

        Args:
            cwd: Working directory for steps in this scope.
            env_prefixes: Per-variable lists prepended to the variable (joined
                with the OS path separator) -- typically `{'PATH': [dir, ...]}`.
            env_suffixes: As `env_prefixes`, but appended.
            env: Whole-variable overrides. A value may contain `%(VAR)s`,
                substituted from the startup environment when the step runs;
                `None` removes the variable.
        """
        # member name -> value to restore on exit (mirrors upstream's
        # `deferred_assignments`), so the `finally` unwinds exactly what changed.
        deferred: dict[str, Any] = {}

        def _push(member: str, new: Any) -> None:
            deferred[member] = getattr(self, member)
            setattr(self, member, new)

        def _add(member: str, to_add: Mapping | None, adder) -> None:
            if to_add:
                _check_type(member, to_add, Mapping)
                new = dict(getattr(self, member))
                for key, val in to_add.items():
                    adder(key, val, new)
                _push(member, new)

        def _as_prefixes(key: str, val: Sequence, new: dict) -> None:
            if val:
                new[key] = tuple(str(v) for v in val) + new.get(key, ())

        def _as_suffixes(key: str, val: Sequence, new: dict) -> None:
            if val:
                new[key] = new.get(key, ()) + tuple(str(v) for v in val)

        def _as_env(key: str, val: str | None, new: dict) -> None:
            if val is not None:
                val = str(val)
                try:
                    # Add a bogus `%(foo)s` to force %-dictionary mode, then
                    # format with a defaultdict: every `%(KEY)s` lookup
                    # succeeds, but a stray sequential `%s` raises -- so an
                    # accidental non-`%(VAR)s` format is rejected here.
                    ('%(foo)s' + val) % collections.defaultdict(str)
                except Exception as exc:
                    raise ValueError(
                        'invalid %-format in env value, only %(VAR)s allowed: '
                        f'{val!r}') from exc
            new[key] = val

        try:
            if cwd is not None:
                _check_type('cwd', cwd, (str, Path))
                _push('_cwd', Path(cwd))
            _add('_env_prefixes', env_prefixes, _as_prefixes)
            _add('_env_suffixes', env_suffixes, _as_suffixes)
            _add('_env', env, _as_env)
            yield
        finally:
            for member, val in deferred.items():
                setattr(self, member, val)

    @property
    def cwd(self) -> Path | None:
        """The cwd steps run in, or `None` to inherit the engine's cwd."""
        return self._cwd

    @property
    def env(self) -> dict[str, str | None]:
        """Whole-variable environment overrides currently in effect."""
        return dict(self._env)

    @property
    def env_prefixes(self) -> dict[str, tuple[str, ...]]:
        """Per-variable path prefixes currently in effect."""
        return dict(self._env_prefixes)

    @property
    def env_suffixes(self) -> dict[str, tuple[str, ...]]:
        """Per-variable path suffixes currently in effect."""
        return dict(self._env_suffixes)
