# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Base class for Brave recipe modules, plus the step placeholder protocol.

A deliberately tiny base class. Every recipe module exposes exactly one
`RecipeApi` subclass (in its `api.py`). After constructing it, the engine attaches the
module's resolved `DEPS` onto `self.m`, which is the "module injection site",
so a module reaches each dependency as `self.m.<dep_name>` (and itself as
`self.m.<own_name>`).

This is also where the `Placeholder` hierarchy lives. A placeholder stands in
for a file that a step reads from or writes to: a module hands one to
`api.step(...)`, the engine renders it into real command-line arguments just
before the step runs, and (for outputs) reads the data back afterwards. See the
"Getting data back from a step" section of README.md.
"""

from __future__ import annotations

from collections.abc import Callable
import functools
from pathlib import Path
from typing import Any

import config_types
from step_stack import StepStack


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


class Placeholder:
    """Base class for a step's command-line placeholders. Not used directly.

    A placeholder is a stand-in for a file that only exists for the duration of
    a step. Put one in a step's `cmd` (or pass it as the step's `stdin` /
    `stdout` / `stderr`) and the engine will:

      1. call `render()` just before the step runs, replacing the placeholder
         with whatever command-line arguments it returns (usually a single
         path), and
      2. once the step is done, call `cleanup()` on an `InputPlaceholder`, or
         `result()` on an `OutputPlaceholder` to read the data back.

    Every placeholder has a `namespaces` pair, with the name of the module that
    produced it and the name of the method that returned it, e.g.
    `('json', 'output')` for `api.json.output()`. The `returns_placeholder`
    decorator fills that in, and the engine uses it to file an output
    placeholder's result onto the step's result as
    `step_result.json.output`. A placeholder may additionally carry a
    caller-chosen `name`, to tell several placeholders from the same method
    apart (see `step_data.StepData`).
    """

    # Whether this placeholder stands for a single file, and so can be a step's
    # `stdin`/`stdout`/`stderr`. False for the ones that stand for something
    # else, like a whole directory of results.
    is_file_backed = True

    def __init__(self, name: str | None = None) -> None:
        if name is not None and not isinstance(name, str):
            raise ValueError('Expected a string name for a placeholder, but '
                             f'got {name!r}')
        self.name = name
        # (module name, method name); filled in by `returns_placeholder`.
        self.namespaces: tuple[str, str] | None = None

    @property
    def backing_file(self) -> str:
        """The path of the file holding (or receiving) the data.

        Only valid once `render` has been called. This is what the engine
        redirects a step's std handle to when the placeholder is passed as
        `stdin`/`stdout`/`stderr`.
        """
        raise NotImplementedError

    def render(self, test) -> list[str]:
        """Return the command-line arguments this placeholder expands to.

        *test* is the `PlaceholderTestData` for this placeholder; when
        `test.enabled` the placeholder must not touch the real filesystem.
        """
        raise NotImplementedError

    @property
    def label(self) -> str:
        """A human-readable `<module>.<method>[<name>]` label."""
        module, method = self.namespaces
        if self.name is None:
            return f'{module}.{method}'
        return f'{module}.{method}[{self.name}]'

    def __repr__(self) -> str:
        namespaced = ('<unnamespaced>'
                      if self.namespaces is None else self.label)
        return f'{type(self).__name__}({namespaced})'


class InputPlaceholder(Placeholder):
    """A placeholder carrying data *into* a step. Not used directly."""

    # Abstract base: concrete subclasses implement `render`/`backing_file`.
    # pylint: disable=abstract-method

    def cleanup(self, test_enabled: bool) -> None:
        """Called once the step has finished, to drop the backing file."""


class OutputPlaceholder(Placeholder):
    """A placeholder carrying data *out of* a step. Not used directly."""

    # Abstract base: concrete subclasses implement `render`/`backing_file`.
    # pylint: disable=abstract-method

    def result(self, test) -> Any:
        """Called once the step has finished; the return value is the result.

        The engine files it onto the step's `StepData` under this
        placeholder's namespaces (so `api.json.output()`'s result shows up as
        `step_result.json.output`).
        """


def _returns_placeholder(func: Callable[..., Placeholder],
                         alternate_name: str | None = None):

    @functools.wraps(func)
    def inner(self, *args, **kwargs):
        placeholder = func(self, *args, **kwargs)
        assert isinstance(placeholder, Placeholder), (
            f'{func.__name__} is decorated with returns_placeholder but '
            f'returned {placeholder!r}')
        placeholder.namespaces = (self._module_name, alternate_name
                                  or func.__name__)
        return placeholder

    return inner


def returns_placeholder(func_or_name):
    """Decorate a placeholder-returning `RecipeApi` method to namespace it.

    The namespace defaults to `(<module name>, <method name>)`, which is what
    the engine files an output placeholder's result under. Decorate as
    `@returns_placeholder('other_name')` to substitute a different method name,
    so several methods can produce placeholders that land in the same place.
    """
    if isinstance(func_or_name, str):
        if not func_or_name:
            raise ValueError('returns_placeholder needs a non-empty name')
        return lambda func: _returns_placeholder(func, func_or_name)
    if not callable(func_or_name):
        raise ValueError('Expected either a function or a string; got '
                         f'{func_or_name!r}')
    return _returns_placeholder(func_or_name)


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

        # The run's stack of open steps, seeded by the engine. `step` pushes
        # each step onto it and `futures` registers spawned greenlets against
        # it; every other module ignores it. Defaults to a private stack so a
        # module instantiated outside the engine still works.
        self._step_stack = StepStack()

        # Simulation context, seeded by the engine only in test mode. `None`
        # means production: the seam modules (`path`, `env`, `platform`, `step`)
        # touch the real machine. When set, they read/mutate this instead. Its
        # presence (`self._test is not None`) is the sole test-mode flag.
        self._test = None

        # The module's name, seeded by the engine (used in config error text
        # and to namespace the placeholders this module hands out).
        self._module_name: str = type(self).__name__

        # This module's own directory (`recipe_modules/<name>`), seeded by the
        # engine. `resource()` derives `<module>/resources/...` from it.
        self._module_dir: Path = Path()

        # This module's own `TEST_API` instance (from its `test_api.py`), seeded
        # by the engine, or None if the module has no test api. Modules use it
        # to build the default simulated data for the steps they run, e.g.
        # `step_test_data=self.test_api.errno`.
        self.test_api = None

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

    def resource(self, *pieces: str) -> config_types.Path:
        """Path to a file under this module's `resources/` directory.
        """
        base = config_types.ResolvedBasePath.for_recipe_module(
            self._test is not None, self._module_name, str(self._module_dir))
        return config_types.Path(base, 'resources', *pieces)

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
