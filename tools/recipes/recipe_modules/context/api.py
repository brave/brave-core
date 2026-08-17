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
from types import MappingProxyType
from typing import Any

import config_types
from engine_types import PerGreenletState
from recipe_api import RecipeApi


def _check_type(name: str, var: object,
                expect: type | tuple[type, ...]) -> None:
    if not isinstance(var, expect):
        expected = getattr(expect, '__name__', str(expect))
        raise TypeError(
            f'{name} is not {expected}: {var!r} ({type(var).__name__})')


class _State(PerGreenletState):
    """The current scope, private to whichever greenlet is reading it.

    A `with api.context(...)` block only applies to the steps that greenlet
    runs. Were this held on the API instance instead, concurrently running
    steps would share one scope and each other's cwd and env.

    A newly spawned greenlet starts from the scope of the greenlet that
    spawned it, which `_get_setter_on_spawn` carries across.
    """

    # Defaults are immutable, to prevent them becoming shared global state if
    # something ever mutates in place rather than replacing wholesale.
    cwd: Path | config_types.Path | None = None
    env: Mapping[str, str | None] = MappingProxyType({})
    env_prefixes: Mapping[str, tuple[str, ...]] = MappingProxyType({})
    env_suffixes: Mapping[str, tuple[str, ...]] = MappingProxyType({})

    def _get_setter_on_spawn(self):
        old_cwd = self.cwd
        old_env = self.env
        old_env_prefixes = self.env_prefixes
        old_env_suffixes = self.env_suffixes

        def _inner():
            self.cwd = old_cwd
            self.env = old_env
            self.env_prefixes = old_env_prefixes
            self.env_suffixes = old_env_suffixes

        return _inner


class ContextApi(RecipeApi):
    """Scoped ambient settings (cwd + environment) applied to steps.

    `__call__` pushes new values for the duration of a `with` block and
    restores them afterwards. The `step` module reads the accessors below when
    building each step.

    The scope lives in greenlet-local storage, so concurrently running steps
    each see their own; see `_State`.
    """

    def __init__(self) -> None:
        super().__init__()
        # Current scope. Replaced wholesale (never mutated in place) by
        # `__call__`, so an outer scope's dicts are never disturbed.
        self._state = _State()

    @contextlib.contextmanager
    def __call__(
        self,
        cwd: str | Path | config_types.Path | None = None,
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
        # member name -> value to restore on exit, so the `finally` unwinds
        # exactly what changed.
        deferred: dict[str, Any] = {}

        def _push(member: str, new: Any) -> None:
            deferred[member] = getattr(self._state, member)
            setattr(self._state, member, new)

        def _add(member: str, to_add: Mapping | None, adder) -> None:
            if to_add:
                _check_type(member, to_add, Mapping)
                new = dict(getattr(self._state, member))
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
                _check_type('cwd', cwd, (str, Path, config_types.Path))
                # A `config_types.Path` (simulated) is already the value we
                # want; only a plain str/real Path needs wrapping.
                _push(
                    'cwd',
                    cwd if isinstance(cwd, (Path,
                                            config_types.Path)) else Path(cwd))
            _add('env_prefixes', env_prefixes, _as_prefixes)
            _add('env_suffixes', env_suffixes, _as_suffixes)
            _add('env', env, _as_env)
            yield
        finally:
            for member, val in deferred.items():
                setattr(self._state, member, val)

    @property
    def cwd(self) -> Path | config_types.Path | None:
        """The cwd steps run in, or `None` to inherit the engine's cwd."""
        return self._state.cwd

    @property
    def env(self) -> dict[str, str | None]:
        """Whole-variable environment overrides currently in effect."""
        return dict(self._state.env)

    @property
    def env_prefixes(self) -> dict[str, tuple[str, ...]]:
        """Per-variable path prefixes currently in effect."""
        return dict(self._state.env_prefixes)

    @property
    def env_suffixes(self) -> dict[str, tuple[str, ...]]:
        """Per-variable path suffixes currently in effect."""
        return dict(self._state.env_suffixes)
