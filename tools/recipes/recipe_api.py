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
        # `brave_core_checkout` uses it; defaults to `master` until overridden.
        self._brave_core_ref: str = 'master'
        # Simulation context, seeded by the engine only in test mode. `None`
        # means production: the seam modules (`path`, `env`, `platform`, `step`)
        # touch the real machine. When set, they read/mutate this instead. Its
        # presence (`self._test is not None`) is the sole test-mode flag.
        self._test = None
        # The module's name, seeded by the engine (used in config error text).
        self._module_name: str = type(self).__name__
        # The module's configuration context (the `ConfigContext` from its
        # `config.py`), seeded by the engine, or None if the module has no
        # config. See `set_config`/`make_config`/`apply_config` below and the
        # "Configs" section of README.md.
        self._config_ctx = None
        # The module's current config blob (a `config.ConfigGroup`), or None
        # until a config is applied. A module reads it as `self.c`, and its
        # users reach it directly as `api.<module>.c`.
        self.c = None

    def initialise(self) -> None:
        """Hook run once after DEPS are injected. Override for setup."""

    # -- Configs (see the "Configs" section of README.md) ---------------------

    def get_config_defaults(self) -> dict:
        """Return dynamic default `CONFIG_VARS` for this module's schema.

        Override to compute default schema arguments at runtime. They are the
        lowest-precedence source of `CONFIG_VARS`, overridden per-invocation by
        the keyword arguments passed to `set_config`/`make_config`.
        """
        return {}

    def make_config(self,
                    config_name: str | None = None,
                    optional: bool = False,
                    **CONFIG_VARS):
        """Return a fresh config blob for this module (without storing it)."""
        return self.make_config_params(config_name, optional, **CONFIG_VARS)[0]

    def _get_config_item(self, config_name: str, optional: bool = False):
        """Look up a named config item in this module's context.

        Returns None when `optional` and the name is unknown; otherwise raises
        `KeyError` listing the module's valid config names.
        """
        ctx = self._config_ctx
        try:
            return ctx.CONFIG_ITEMS[config_name]
        except KeyError:
            if optional:
                return None
            raise KeyError(
                '%s is not the name of a configuration for module %s: %s' %
                (config_name, self._module_name, sorted(
                    ctx.CONFIG_ITEMS))) from None

    def make_config_params(self,
                           config_name: str | None,
                           optional: bool = False,
                           **CONFIG_VARS):
        """Return `(config_blob, params)` for this module.

        `params` are merged from, in increasing precedence:
          * `get_config_defaults()`
          * `CONFIG_VARS`
        and splatted into the schema factory. When `config_name` is given, the
        named config item (with its root and includes) is applied to the blob.
        """
        generic_params = self.get_config_defaults()  # generic defaults
        generic_params.update(CONFIG_VARS)  # per-invocation values

        ctx = self._config_ctx
        if optional and not ctx:
            return None, generic_params

        assert ctx, '%s has no config context' % self._module_name
        params = self.get_config_defaults()  # generic defaults
        itm = None
        if config_name:
            itm = self._get_config_item(config_name, optional)
            if not itm:
                return None, generic_params
        params.update(CONFIG_VARS)  # per-invocation values

        base = ctx.CONFIG_SCHEMA(**params)
        if config_name is None:
            return base, params
        return itm(base), params

    def set_config(self,
                   config_name: str | None = None,
                   optional: bool = False,
                   **CONFIG_VARS) -> None:
        """Set `self.c` to the named configuration for this module."""
        config, _ = self.make_config_params(config_name, optional,
                                            **CONFIG_VARS)
        if config:
            self.c = config

    def apply_config(self,
                     config_name: str,
                     config_object=None,
                     optional: bool = False) -> None:
        """Apply a named config item on top of an existing blob (`self.c`)."""
        itm = self._get_config_item(config_name)
        itm(config_object or self.c, optional=optional)
