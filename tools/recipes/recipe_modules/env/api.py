# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `env` module API: mockable environment access.

Modules read/write process environment variables and resolve executables
through this seam instead of touching `os.environ`/`shutil.which` directly, so
recipes run deterministically under simulation. In production it is a thin
wrapper over the real environment; in test mode it reads and mutates the
simulated environment on the run's `TestContext`.
"""

from __future__ import annotations

import os
import shutil

from recipe_api import RecipeApi


class _RealEnv:  # pragma: no cover - production env backend, not simulated.
    """Production backend: the real process environment and PATH lookup."""

    def get(self, key: str, default: str | None) -> str | None:
        return os.environ.get(key, default)

    def set(self, key: str, value: str) -> None:
        os.environ[key] = value

    def contains(self, key: str) -> bool:
        return key in os.environ

    def which(self, cmd: str) -> str | None:
        return shutil.which(cmd)

    def prepend_path(self, entry: str) -> None:
        current = os.environ.get('PATH', '')
        os.environ['PATH'] = (os.pathsep.join([entry, current])
                              if current else entry)


class _SimEnv:
    """Test backend: the simulated environment on the run's TestContext."""

    def __init__(self, test) -> None:
        self._test = test

    def get(self, key: str, default: str | None) -> str | None:
        return self._test.env.get(key, default)

    def set(self, key: str, value: str) -> None:
        self._test.env[key] = value

    def contains(self, key: str) -> bool:
        return key in self._test.env

    def which(self, cmd: str) -> str | None:
        return self._test.which_map.get(cmd)

    def prepend_path(self, entry: str) -> None:
        current = self._test.env.get('PATH', '')
        self._test.env['PATH'] = (os.pathsep.join([entry, current])
                                  if current else entry)


class EnvApi(RecipeApi):
    """Get/set environment variables, test membership, and resolve commands.

    The real-vs-simulated choice is made once in `initialise()` by picking a
    backend; the methods below just delegate, so there is no per-call test-mode
    branching (the same shape as the `step` module's runner selection).
    """

    def __init__(self) -> None:
        super().__init__()
        # Default to the real environment; swapped for a simulated backend in
        # test mode by initialise() (once the engine has seeded `_test`).
        self._backend = _RealEnv()

    def initialise(self) -> None:
        if self._test is not None:
            self._backend = _SimEnv(self._test)

    def get(self, key: str, default: str | None = None) -> str | None:
        """Return env var *key*, or *default* when unset."""
        return self._backend.get(key, default)

    def set(self, key: str, value: str) -> None:
        """Set env var *key* to *value* for subsequent steps."""
        self._backend.set(key, value)

    def __contains__(self, key: str) -> bool:
        """Whether env var *key* is set."""
        return self._backend.contains(key)

    def which(self, cmd: str) -> str | None:
        """Resolve *cmd* on PATH, returning its absolute path or `None`."""
        return self._backend.which(cmd)

    def prepend_path(self, entry: str | os.PathLike) -> None:
        """Prepend *entry* to `PATH` so later steps find binaries there."""
        self._backend.prepend_path(str(entry))
