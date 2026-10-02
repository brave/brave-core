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
        --properties '{"chromium_ref": "refs/tags/151.0.7917.1",
                       "brave_subrevision": 1}'
"""

from __future__ import annotations

import argparse
from collections.abc import Mapping, Sequence
import dataclasses
import importlib
import inspect
import json
import logging
import os
from pathlib import Path
import sys
import types
import typing

from google.protobuf import json_format as jsonpb

import config_types
import proto_support
from recipe_api import RecipeApi, RecipeScriptApi
from recipe_test_api import RecipeTestApi
from step_stack import StepStack

# Root of the recipes tree (this file's directory). Recipe modules live under
# `recipe_modules/<name>/` and recipes under `recipes/<name>.py`.
RECIPES_ROOT = Path(__file__).resolve().parent
MODULES_PKG = 'recipe_modules'
RECIPES_PKG = 'recipes'


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


def _find_api_class(
    api_module: types.ModuleType, module_name: str
) -> type[RecipeApi]:
    """Return the single `RecipeApi` subclass defined in *api_module*."""
    classes = [
        value
        for value in vars(api_module).values()
        if isinstance(value, type)
        and issubclass(value, RecipeApi)
        and value is not RecipeApi
        and value.__module__ == api_module.__name__
    ]
    if len(classes) != 1:
        raise RuntimeError(
            f"recipe module '{module_name}' must define exactly one RecipeApi "
            f'subclass in api.py; found {len(classes)}'
        )
    return classes[0]


def _find_test_api_class(
    test_module: types.ModuleType, module_name: str
) -> type[RecipeTestApi]:
    """Return the single `RecipeTestApi` subclass in *test_module*, if any."""
    classes = [
        value
        for value in vars(test_module).values()
        if isinstance(value, type)
        and issubclass(value, RecipeTestApi)
        and value is not RecipeTestApi
        and value.__module__ == test_module.__name__
    ]
    if len(classes) > 1:
        raise RuntimeError(
            f"recipe module '{module_name}' defines {len(classes)} "
            'RecipeTestApi subclasses in test_api.py; expected at most one'
        )
    return classes[0] if classes else RecipeTestApi


def _module_api_class(
    package: types.ModuleType, module_name: str
) -> type[RecipeApi]:
    """Return a module's `RecipeApi` subclass.

    Prefers the `API` its `__init__.py` exports, falling back to finding the one
    `RecipeApi` subclass defined in its `api.py`.
    """
    api_class = getattr(package, 'API', None)
    if api_class is not None:
        if not (
            isinstance(api_class, type) and issubclass(api_class, RecipeApi)
        ):
            raise RuntimeError(
                f"recipe module '{module_name}' exports API which is not a "
                'subclass of RecipeApi'
            )
        return api_class
    api_module = importlib.import_module(f'{MODULES_PKG}.{module_name}.api')
    return _find_api_class(api_module, module_name)


def _module_test_api_class(
    package: types.ModuleType, module_name: str
) -> type[RecipeTestApi]:
    """Return a module's `RecipeTestApi` subclass, or the base class.

    Prefers the `TEST_API` its `__init__.py` exports, falling back to finding
    the (optional) `RecipeTestApi` subclass defined in its `test_api.py`.
    """
    test_api_class = getattr(package, 'TEST_API', None)
    if test_api_class is not None:
        if not (
            isinstance(test_api_class, type)
            and issubclass(test_api_class, RecipeTestApi)
        ):
            raise RuntimeError(
                f"recipe module '{module_name}' exports TEST_API which is not "
                'a subclass of RecipeTestApi'
            )
        return test_api_class
    if not (RECIPES_ROOT / MODULES_PKG / module_name / 'test_api.py').exists():
        # Modules without a test_api.py contribute the base api (no helpers).
        return RecipeTestApi
    test_module = importlib.import_module(
        f'{MODULES_PKG}.{module_name}.test_api'
    )
    return _find_test_api_class(test_module, module_name)


# Field names from the base api classes, which are not DEPS.
_BASE_API_FIELDS: frozenset[str] = frozenset(
    set(typing.get_type_hints(RecipeScriptApi))
    | set(typing.get_type_hints(RecipeTestApi))
)


def parse_deps_spec(deps_spec: object, *, source: str) -> dict[str, str]:
    """Normalise a `DEPS` (or `TEST_DEPS`) specification.

    Two forms are accepted:

        DEPS = ['module', 'other_module']

        @dataclass
        class DEPS(RecipeScriptApi):
            module: module.API
            other_name: other_module.API

    Args:
      deps_spec: The deps specification.
      source: Source file where the specification lives, for errors.

    Returns `{local_name: module_name}`.
    """
    if dataclasses.is_dataclass(deps_spec):
        return _parse_deps_class(deps_spec, source)
    if not deps_spec:
        return {}
    if isinstance(deps_spec, Sequence) and not isinstance(deps_spec, str):
        return {name: name for name in deps_spec}
    raise TypeError(
        f'DEPS in {source} must be a list of module names or a dataclass; '
        f'got {deps_spec!r}'
    )


def _parse_deps_class(deps_spec: type, source: str) -> dict[str, str]:
    """Validate a `DEPS`/`TEST_DEPS` dataclass and return its deps.

    Each field's annotation must be a class from a recipe module (normally
    `<module>.API` or `<module>.TEST_API`); the module it was defined in names
    the dependency, and the field name is its local name.
    """
    for name, member in vars(deps_spec).items():
        if name.startswith('__') and name.endswith('__'):
            continue
        if inspect.isfunction(member) or isinstance(
            member, (classmethod, staticmethod, property)
        ):
            raise ValueError(
                f"Cannot define custom method '{name}' on "
                f'{deps_spec.__name__} in {source}. {deps_spec.__name__} is '
                'only used for type hinting and custom methods will not be '
                'available at runtime.'
            )

    deps = {}
    # No `globalns`: each class's annotations resolve against its own module,
    # which holds a recipe's imports since recipes are imported normally.
    for field_name, ann in typing.get_type_hints(deps_spec).items():
        if field_name in _BASE_API_FIELDS:
            continue
        parts = getattr(ann, '__module__', '').split('.')
        if len(parts) >= 2 and parts[0] == MODULES_PKG:
            deps[field_name] = parts[1]
        else:
            raise ValueError(
                f"Cannot infer DEPS path from {ann!r} in field '{field_name}' "
                f'of {deps_spec.__name__} in {source}'
            )
    return deps


def _module_deps(package: types.ModuleType) -> dict[str, str]:
    """A recipe module's normalised `DEPS`."""
    return parse_deps_spec(
        getattr(package, 'DEPS', ()), source=str(package.__file__)
    )


def recipe_test_deps(recipe: types.ModuleType) -> dict[str, str]:
    """A recipe's normalised `TEST_DEPS`, falling back to its `DEPS`."""
    spec = getattr(recipe, 'TEST_DEPS', None) or getattr(recipe, 'DEPS', ())
    return parse_deps_spec(spec, source=str(recipe.__file__))


def instantiate_test_module(
    name: str, chain: list[str], cache: dict[str, RecipeTestApi]
) -> RecipeTestApi:
    """Instantiate a module's TEST_API, wiring its DEPS onto `.m` (cached).

    Every run builds these, not just simulated ones: a module reaches its own
    test api as `self.test_api` to describe what its steps should return under
    simulation (`step_test_data=`), so the attribute has to be there in
    production too.
    """
    if name in cache:
        return cache[name]
    if name in chain:
        cycle = ' -> '.join(chain + [name])
        raise RuntimeError(f'cyclical DEPS detected: {cycle}')

    _ensure_protos()
    package = importlib.import_module(f'{MODULES_PKG}.{name}')
    inst = _module_test_api_class(package, name)(module=name)
    for local_name, dep_name in _module_deps(package).items():
        setattr(
            inst.m,
            local_name,
            instantiate_test_module(dep_name, chain + [name], cache),
        )
    setattr(inst.m, name, inst)
    cache[name] = inst
    return inst


def build_root_test_api(recipe: types.ModuleType) -> RecipeTestApi:
    """Build the `api` passed to a recipe's `GenTests`.

    Each of the recipe's `TEST_DEPS` (or, absent those, `DEPS`) is injected
    under its local name. A dataclass `TEST_DEPS` is instantiated with them.
    """
    cache: dict[str, RecipeTestApi] = {}
    deps = {
        local_name: instantiate_test_module(dep_name, [], cache)
        for local_name, dep_name in recipe_test_deps(recipe).items()
    }
    test_deps_cls = getattr(recipe, 'TEST_DEPS', None)
    if dataclasses.is_dataclass(test_deps_cls):
        return test_deps_cls(**deps)
    root = RecipeTestApi(module=None)
    for local_name, dep in deps.items():
        setattr(root, local_name, dep)
    return root


def _load_config_ctx(module_name: str):
    """Return the `ConfigContext` from a module's `config.py`, or None.

    A module opts into the config system by defining a `config.py` that assigns
    exactly one `config_item_context(...)` result at module scope (the module's
    CONFIG_CTX). Modules without a `config.py` return None (they have no
    config). See the "Configs" section of README.md.
    """
    config_path = RECIPES_ROOT / MODULES_PKG / module_name / 'config.py'
    if not config_path.exists():
        return None
    _ensure_protos()
    from config import ConfigContext

    config_module = importlib.import_module(
        f'{MODULES_PKG}.{module_name}.config'
    )
    contexts = [
        value
        for value in vars(config_module).values()
        if isinstance(value, ConfigContext)
    ]
    if len(contexts) != 1:
        raise RuntimeError(
            f"recipe module '{module_name}' has a config.py but defines "
            f'{len(contexts)} config contexts; expected exactly one '
            '(from config_item_context(...))'
        )
    return contexts[0]


class _Engine:
    """Resolves DEPS and instantiates module APIs, caching by module name."""

    def __init__(
        self, workspace: str | Path | None = None, test: object | None = None
    ) -> None:
        _ensure_on_sys_path()
        # Simulation context, or None in production. When set, the engine runs
        # in test mode: it seeds this onto every module (so the seam modules
        # simulate I/O) and does not touch the real cwd.
        self._test = test

        # Root directory the job runs in. Recipe paths (b/src, out, ...) are
        # derived from it by the `path` module. In test mode this value is
        # never actually read: `PathApi`'s properties branch on `self._test`
        # directly and return a `[WORKSPACE]`-rooted `config_types.Path`
        # instead.
        if self._test is not None:
            self._workspace = Path.cwd()
        elif workspace:
            self._workspace = Path(workspace).expanduser().resolve()
            self._workspace.mkdir(parents=True, exist_ok=True)
            # Run from the workspace so every subprocess the recipes launch
            # inherits it as their cwd.
            os.chdir(self._workspace)
        else:
            self._workspace = Path.cwd()
        # The run's input property JSON and environment, seeded by
        # `run_loaded_recipe` before any module is instantiated. A module's
        # `PROPERTIES`/`ENV_PROPERTIES` are bound from these (see
        # `_module_property_args`). Empty defaults let tests instantiate a
        # module directly (without a recipe) -- a module then just sees its
        # proto defaults.
        self._properties: dict[str, object] = {}
        self._environ: Mapping[str, str] = {}
        # The run's stack of open steps, shared by every module so `step` and
        # `futures` reach the same one without depending on each other.
        self._step_stack = StepStack()
        # module name -> instantiated RecipeApi (one instance per run).
        self._cache: dict[str, RecipeApi] = {}
        # module name -> instantiated RecipeTestApi, attached to each module as
        # its `test_api` (see `instantiate_test_module`).
        self._test_api_cache: dict[str, RecipeTestApi] = {}

    def _module_property_args(
        self, name: str, package: types.ModuleType
    ) -> list[object]:
        """Bind a module's declared `PROPERTIES`/`ENV_PROPERTIES` messages.

        Mirrors the recipe-level binding in `_run_steps`, but for a recipe
        module. A module opts in by declaring `PROPERTIES` and/or
        `ENV_PROPERTIES` (protobuf message classes) in its `__init__.py`; the
        engine passes the bound messages positionally to the module's
        `RecipeApi.__init__`, in the same order `RunSteps` receives them:

            neither                 -> API()
            PROPERTIES              -> API(properties)
            PROPERTIES + ENV_PROPS  -> API(properties, env_properties)
            ENV_PROPERTIES          -> API(env_properties)

        A module's `PROPERTIES` are namespaced: they are read from the
        `$<module_name>` block of the input property JSON (the single-repo
        analogue of upstream's `$<repo>/<module>` key), so per-module input is
        kept separate from the recipe's own top-level properties. `ENV_PROPERTIES`
        are read from the environment with keys upper-cased. Both decode with
        unknown fields ignored.
        """
        properties_def = getattr(package, 'PROPERTIES', None)
        env_properties_def = getattr(package, 'ENV_PROPERTIES', None)

        args: list[object] = []
        if properties_def is not None:
            if not proto_support.is_message_class(properties_def):
                raise TypeError(
                    f"module '{name}' PROPERTIES must be a protobuf message "
                    f'class; got {properties_def!r}'
                )
            block = self._properties.get(f'${name}', {})
            args.append(
                jsonpb.ParseDict(
                    block, properties_def(), ignore_unknown_fields=True
                )
            )
        if env_properties_def is not None:
            args.append(
                jsonpb.ParseDict(
                    {k.upper(): v for k, v in self._environ.items()},
                    env_properties_def(),
                    ignore_unknown_fields=True,
                )
            )
        return args

    def _instantiate_module(self, name: str, chain: list[str]) -> RecipeApi:
        if name in self._cache:
            return self._cache[name]
        if name in chain:
            cycle = ' -> '.join(chain + [name])
            raise RuntimeError(f'cyclical DEPS detected: {cycle}')

        # A module's __init__.py/api.py may import its typed PROPERTIES message
        # from `PB`, so the proto package must exist before we import it.
        _ensure_protos()
        package = importlib.import_module(f'{MODULES_PKG}.{name}')
        api_class = _module_api_class(package, name)

        inst = api_class(*self._module_property_args(name, package))
        # Seed engine-provided values (workspace, and the
        # module's name and config context) so modules can use them. setattr
        # keeps the engine out of the instance's protected members directly.
        setattr(inst, '_workspace', self._workspace)
        setattr(inst, '_step_stack', self._step_stack)
        setattr(inst, '_module_name', name)
        setattr(inst, '_module_dir', Path(package.__file__).resolve().parent)
        setattr(inst, '_config_ctx', _load_config_ctx(name))
        inst.test_api = instantiate_test_module(name, [], self._test_api_cache)
        for local_name, dep_name in _module_deps(package).items():
            setattr(
                inst.m,
                local_name,
                self._instantiate_module(dep_name, chain + [name]),
            )
        # A module can reach itself via `self.m.<own_name>`.
        setattr(inst.m, name, inst)

        # Seed the simulation context (test mode only) after DEPS are wired but
        # before initialise(), so a module's initialise() can already use the
        # seam modules (e.g. depot_tools reading api.platform.is_win).
        if self._test is not None:
            setattr(inst, '_test', self._test)

        inst.initialise()
        self._cache[name] = inst
        return inst

    def run_recipe(
        self, recipe_name: str, properties: dict[str, object] | None = None
    ) -> object:
        return self.run_loaded_recipe(
            _import_recipe(recipe_name), recipe_name, properties
        )

    def run_loaded_recipe(
        self,
        recipe: types.ModuleType,
        recipe_name: str,
        properties: dict[str, object] | None = None,
    ) -> object:
        """Run an already-imported *recipe* module's `RunSteps`.

        Splits the import from the run so the test runner can import a recipe
        once (from either `recipes/` or a module's `examples/`) and drive it.
        """
        # Seed the run's input before instantiating any module, so a module's
        # PROPERTIES/ENV_PROPERTIES can be bound from it (see
        # `_module_property_args`). In test mode ENV_PROPERTIES is sourced from
        # the simulated environment so expectations don't depend on host env.
        self._properties = properties or {}
        self._environ = self._test.env if self._test is not None else os.environ

        run_steps = getattr(recipe, 'RunSteps', None)
        if run_steps is None:
            raise RuntimeError(f"recipe '{recipe_name}' is missing RunSteps")

        recipe_file = Path(recipe.__file__).resolve()
        resources_dir = recipe_file.parent / f'{recipe_file.stem}.resources'
        deps_spec = getattr(recipe, 'DEPS', ())
        deps = {
            local_name: self._instantiate_module(dep_name, [])
            for local_name, dep_name in parse_deps_spec(
                deps_spec, source=str(recipe_file)
            ).items()
        }
        if dataclasses.is_dataclass(deps_spec):
            api = deps_spec(self._test, recipe_name, resources_dir, **deps)
        else:
            api = RecipeScriptApi(self._test, recipe_name, resources_dir)
            for local_name, dep in deps.items():
                setattr(api, local_name, dep)

        try:
            return _run_steps(
                run_steps,
                api,
                self._properties,
                self._environ,
                getattr(recipe, 'PROPERTIES', None),
                getattr(recipe, 'ENV_PROPERTIES', None),
            )
        finally:
            # Closes the last step, then the root, which waits for any work the
            # recipe spawned and never collected. A recipe that fails partway
            # unwinds the same way, so nothing is left running.
            self._step_stack.unwind()


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


def _run_steps(
    run_steps: object,
    api: object,
    properties: dict[str, object],
    environ: Mapping[str, str],
    properties_def: type | None,
    env_properties_def: type | None,
) -> object:
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
        properties_def
    ):
        raise TypeError(
            'PROPERTIES must be a protobuf message class; got '
            f'{properties_def!r}'
        )

    args = [api]
    if proto_support.is_message_class(properties_def):
        properties_without_reserved = {
            k: v for k, v in properties.items() if not k.startswith('$')
        }
        args.append(
            jsonpb.ParseDict(
                properties_without_reserved,
                properties_def(),
                ignore_unknown_fields=True,
            )
        )
    if env_properties_def is not None:
        args.append(
            jsonpb.ParseDict(
                {k.upper(): v for k, v in environ.items()},
                env_properties_def(),
                ignore_unknown_fields=True,
            )
        )
    return run_steps(*args)


def run_recipe(
    recipe_name: str,
    properties: dict[str, object] | None = None,
    workspace: str | Path | None = None,
) -> object:
    """Resolve DEPS for *recipe_name* and run its `RunSteps`."""
    return _Engine(workspace).run_recipe(recipe_name, properties)


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
        help='Recipe name under recipes/ (e.g. toolchains/rust/package_rust)',
    )
    parser.add_argument(
        '--properties', default='{}', help='JSON object of recipe properties'
    )
    parser.add_argument(
        '--workspace',
        default=None,
        help='Root directory the job runs in; recipe paths '
        '(b/src, out, ...) are relative to it; '
        'created if missing (default: current directory)',
    )
    parser.add_argument(
        '--verbose', action='store_true', help='Enable verbose (debug) logging'
    )
    args = parser.parse_args(argv)

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)
    run_recipe(args.recipe, json.loads(args.properties), args.workspace)
    return 0


if __name__ == '__main__':
    sys.exit(main())
