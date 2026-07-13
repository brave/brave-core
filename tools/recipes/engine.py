#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A very small recipe engine for Brave.

Loads a recipe, resolves its `DEPS` (recursively, with caching and cycle
detection), instantiates each recipe module's `RecipeApi`, wires dependencies
onto each module's `.m` injection site, and finally calls the recipe's
`RunSteps(api, properties)`.

Run a recipe directly (recipe names are `/`-separated paths under recipes/).
`--workspace` sets the root the job runs in; recipe paths are derived from it:

    python3 engine.py toolchains/rust/package_rust \\
        --properties '{"chromium_ref": "151.0.7917.1", "brave_subrevision": 1}'
"""

from __future__ import annotations

import argparse
from collections.abc import Mapping
import importlib
import json
import logging
import os
from pathlib import Path
import sys
import types

from google.protobuf import json_format as jsonpb

import proto_support
from recipe_api import RecipeApi

# Root of the recipes tree (this file's directory). Recipe modules live under
# `recipe_modules/<name>/` and recipes under `recipes/<name>.py`.
RECIPES_ROOT = Path(__file__).resolve().parent
MODULES_PKG = 'recipe_modules'
RECIPES_PKG = 'recipes'


class RecipeScriptApi:
    """The `api` object passed to a recipe's `RunSteps`.

    Carries the recipe's top-level `DEPS`: each one is attached as an attribute
    named after the module (e.g. `api.chromium_checkout`).
    """

    def __getattr__(self, name: str):
        # DEPS are injected by the engine; a missing one means it was not
        # declared in the recipe's DEPS. (Also tells static analysis that
        # attributes are dynamic, so accessing a dep is not flagged no-member.)
        raise AttributeError(
            f'{name!r} is not a declared dependency (add it to DEPS?)')


def _ensure_on_sys_path() -> None:
    """Put the recipes root on `sys.path` so modules import as packages."""
    root = str(RECIPES_ROOT)
    if root not in sys.path:
        sys.path.insert(0, root)


# Set once the `PB` proto package has been compiled and put on `sys.path`.
_protos_ready = False


def _ensure_protos() -> None:
    """Compile the repo's `.proto` files and make the `PB` package importable.

    Recipes import their typed `PROPERTIES`/`ENV_PROPERTIES` messages from `PB`
    (e.g. `from PB.recipes.brave... import InputProperties`), so this must run
    before any recipe is imported. Idempotent: the compile is a no-op fast path
    when nothing changed, and `PB` is added to `sys.path` exactly once.
    """
    global _protos_ready
    if _protos_ready:
        return
    proto_support.append_to_syspath(proto_support.ensure_compiled())
    _protos_ready = True


def _find_api_class(api_module: types.ModuleType,
                    module_name: str) -> type[RecipeApi]:
    """Return the single `RecipeApi` subclass defined in *api_module*."""
    classes = [
        value for value in vars(api_module).values()
        if isinstance(value, type) and issubclass(value, RecipeApi)
        and value is not RecipeApi and value.__module__ == api_module.__name__
    ]
    if len(classes) != 1:
        raise RuntimeError(
            f"recipe module '{module_name}' must define exactly one RecipeApi "
            f'subclass in api.py; found {len(classes)}')
    return classes[0]


class _Engine:
    """Resolves DEPS and instantiates module APIs, caching by module name."""

    def __init__(self,
                 workspace: str | Path | None = None,
                 brave_core_ref: str = 'master',
                 test: object | None = None) -> None:
        _ensure_on_sys_path()
        # Simulation context, or None in production. When set, the engine runs
        # in test mode: it seeds this onto every module (so the seam modules
        # simulate I/O) and does not touch the real cwd.
        self._test = test
        # Root directory the job runs in. Recipe paths (chromium/src, out, ...)
        # are derived from it by the `path` module.
        if self._test is not None:
            # Fixed synthetic workspace so module-derived paths are
            # deterministic; never chdir'd into (nothing runs on disk).
            from simulation import SIM_WORKSPACE
            self._workspace = Path(str(SIM_WORKSPACE))
        elif workspace:
            self._workspace = Path(workspace).expanduser().resolve()
            # Run from the workspace so every subprocess the recipes launch
            # inherits it as their cwd.
            os.chdir(self._workspace)
        else:
            self._workspace = Path.cwd()
        # brave-core ref the checkout modules clone. Defaults to `master`;
        # overridable (mainly for testing against a non-master ref).
        self._brave_core_ref: str = brave_core_ref
        # module name -> instantiated RecipeApi (one instance per run).
        self._cache: dict[str, RecipeApi] = {}

    def _instantiate_module(self, name: str, chain: list[str]) -> RecipeApi:
        if name in self._cache:
            return self._cache[name]
        if name in chain:
            cycle = ' -> '.join(chain + [name])
            raise RuntimeError(f'cyclical DEPS detected: {cycle}')

        package = importlib.import_module(f'{MODULES_PKG}.{name}')
        deps = list(getattr(package, 'DEPS', []))
        api_module = importlib.import_module(f'{MODULES_PKG}.{name}.api')
        api_class = _find_api_class(api_module, name)

        inst = api_class()
        # Seed engine-provided values (workspace, brave-core ref) so modules can
        # use them. setattr keeps the engine out of the instance's protected
        # members directly.
        setattr(inst, '_workspace', self._workspace)
        setattr(inst, '_brave_core_ref', self._brave_core_ref)
        for dep_name in deps:
            setattr(inst.m, dep_name,
                    self._instantiate_module(dep_name, chain + [name]))
        # A module can reach itself via `self.m.<own_name>`, as in recipes_py.
        setattr(inst.m, name, inst)

        # Seed the simulation context (test mode only) after DEPS are wired but
        # before initialise(), so a module's initialise() can already use the
        # seam modules (e.g. depot_tools reading api.platform.is_win).
        if self._test is not None:
            setattr(inst, '_test', self._test)

        inst.initialise()
        self._cache[name] = inst
        return inst

    def run_recipe(self,
                   recipe_name: str,
                   properties: dict[str, object] | None = None) -> object:
        return self.run_loaded_recipe(_import_recipe(recipe_name), recipe_name,
                                      properties)

    def run_loaded_recipe(
            self,
            recipe: types.ModuleType,
            recipe_name: str,
            properties: dict[str, object] | None = None) -> object:
        """Run an already-imported *recipe* module's `RunSteps`.

        Splits the import from the run so the test runner can import a recipe
        once (from either `recipes/` or a module's `examples/`) and drive it.
        """
        api = RecipeScriptApi()
        for dep_name in getattr(recipe, 'DEPS', []):
            setattr(api, dep_name, self._instantiate_module(dep_name, []))

        run_steps = getattr(recipe, 'RunSteps', None)
        if run_steps is None:
            raise RuntimeError(f"recipe '{recipe_name}' is missing RunSteps")

        # In test mode, source ENV_PROPERTIES from the simulated environment so
        # expectations don't depend on the host's env vars.
        environ = self._test.env if self._test is not None else os.environ
        return _run_steps(run_steps, api, properties or {}, environ,
                          getattr(recipe, 'PROPERTIES', None),
                          getattr(recipe, 'ENV_PROPERTIES', None))


def _module_names() -> set[str]:
    """Names of the recipe modules under `recipe_modules/`."""
    modules_dir = RECIPES_ROOT / MODULES_PKG
    return {
        entry.name
        for entry in modules_dir.iterdir()
        if entry.is_dir() and (entry / '__init__.py').exists()
    }


def _import_recipe(recipe_name: str) -> types.ModuleType:
    """Import a recipe by its `/`-separated id.

    Ids whose first segment is a recipe module (e.g. `step/examples/full`) load
    from `recipe_modules/`; all others load from `recipes/` (e.g.
    `toolchains/rust/package_rust`).
    """
    _ensure_on_sys_path()
    # Recipes import their PROPERTIES/ENV_PROPERTIES messages from `PB`, so the
    # proto package must exist before the recipe module is imported.
    _ensure_protos()
    module_path = recipe_name.replace('/', '.')
    first = recipe_name.split('/', 1)[0]
    pkg = MODULES_PKG if first in _module_names() else RECIPES_PKG
    return importlib.import_module(f'{pkg}.{module_path}')


def _run_steps(run_steps: object, api: object, properties: dict[str, object],
               environ: Mapping[str, str], properties_def: type | None,
               env_properties_def: type | None) -> object:
    """Bind input into typed messages and invoke a recipe's `RunSteps`.

    `PROPERTIES` and `ENV_PROPERTIES` are protobuf message classes, and the
    arguments passed after `api` are determined by which are declared:

        neither                 -> RunSteps(api)
        PROPERTIES              -> RunSteps(api, properties)
        PROPERTIES + ENV_PROPS  -> RunSteps(api, properties, env_properties)
        ENV_PROPERTIES          -> RunSteps(api, env_properties)

    `PROPERTIES` is decoded from the input property JSON with reserved
    (`$`-prefixed) keys removed; `ENV_PROPERTIES` is decoded from the
    environment with keys upper-cased. Both use JSONPB with unknown fields
    ignored, so extra input (e.g. every unrelated env var) is dropped.
    """
    if properties_def is not None and not proto_support.is_message_class(
            properties_def):
        raise TypeError('PROPERTIES must be a protobuf message class; got '
                        f'{properties_def!r}')

    args = [api]
    if proto_support.is_message_class(properties_def):
        properties_without_reserved = {
            k: v
            for k, v in properties.items() if not k.startswith('$')
        }
        args.append(
            jsonpb.ParseDict(properties_without_reserved,
                             properties_def(),
                             ignore_unknown_fields=True))
    if env_properties_def is not None:
        args.append(
            jsonpb.ParseDict({
                k.upper(): v
                for k, v in environ.items()
            },
                             env_properties_def(),
                             ignore_unknown_fields=True))
    return run_steps(*args)


def run_recipe(recipe_name: str,
               properties: dict[str, object] | None = None,
               workspace: str | Path | None = None,
               brave_core_ref: str = 'master') -> object:
    """Resolve DEPS for *recipe_name* and run its `RunSteps`."""
    return _Engine(workspace,
                   brave_core_ref).run_recipe(recipe_name, properties)


def main(argv: list[str] | None = None) -> int:
    argv = sys.argv[1:] if argv is None else list(argv)
    # `engine.py test run|train|list [...]` dispatches to the simulation-test
    # runner; everything else runs a recipe (the original CLI, unchanged).
    if argv and argv[0] == 'test':
        # Imported via importlib (not a plain `import`) so there is no static
        # engine -> recipe_test_runner import edge: the runner imports engine,
        # and this keeps that dependency one-directional (no import cycle).
        runner = importlib.import_module('recipe_test_runner')
        return runner.main(argv[1:])

    parser = argparse.ArgumentParser(description='Run a Brave recipe.')
    parser.add_argument(
        'recipe',
        help='Recipe name under recipes/ (e.g. toolchains/rust/package_rust)')
    parser.add_argument('--properties',
                        default='{}',
                        help='JSON object of recipe properties')
    parser.add_argument('--workspace',
                        default=None,
                        help='Root directory the job runs in; recipe paths '
                        '(chromium/src, out, ...) are relative to it '
                        '(default: current directory)')
    parser.add_argument('--brave-core-ref',
                        default='master',
                        help='brave-core ref the recipe checks out '
                        '(default: master; override for testing)')
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose (debug) logging')
    args = parser.parse_args(argv)

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)
    run_recipe(args.recipe, json.loads(args.properties), args.workspace,
               args.brave_core_ref)
    return 0


if __name__ == '__main__':
    sys.exit(main())
