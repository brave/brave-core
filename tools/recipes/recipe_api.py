# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Base class for Brave recipe modules.

A deliberately tiny base class. Every recipe module exposes exactly one
`RecipeApi` subclass (in its `api.py`). After constructing it, the engine attaches the
module's resolved `DEPS` onto `self.m`, which is the "module injection site",
so a module reaches each dependency as `self.m.<dep_name>` (and itself as
`self.m.<own_name>`).
"""

from __future__ import annotations

from pathlib import Path


class ModuleInjectionSite:
    """Namespace holding a module's resolved DEPS (and the module itself).

    The engine populates one per module instance: each entry in the module's
    `DEPS` becomes an attribute named after that dependency module. Attributes
    are set dynamically by the engine, so the class declares no members itself.
    """

    def __getattr__(self, name: str):
        # Dependencies are injected by the engine; a missing one means it was
        # not declared in DEPS. (Also tells static analysis that attributes are
        # dynamic, so accessing an injected dep is not flagged as no-member.)
        raise AttributeError(
            f'{name!r} is not a declared dependency (add it to DEPS?)')


class RecipeApi:
    """Base class every recipe module's API subclasses."""

    def __init__(self) -> None:
        # Populated by the engine after construction with this module's DEPS.
        self.m: ModuleInjectionSite = ModuleInjectionSite()
        # The job's workspace root, seeded by the engine after construction.
        # The `path` module derives the named job paths from it; most modules
        # ignore it. Defaults to `.` until the engine overrides it.
        self._workspace: Path = Path()
        # brave-core ref the checkout modules clone, seeded by the engine.
        # `brave_core_shallow` uses it; defaults to `master` until overridden.
        self._brave_core_ref: str = 'master'
        # Simulation context, seeded by the engine only in test mode. `None`
        # means production: the seam modules (`path`, `env`, `platform`, `step`)
        # touch the real machine. When set, they read/mutate this instead. Its
        # presence (`self._test is not None`) is the sole test-mode flag.
        self._test = None

    def initialise(self) -> None:
        """Hook run once after DEPS are injected. Override for setup."""
