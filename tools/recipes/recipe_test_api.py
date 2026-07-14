# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test-side API for Brave recipes.

A recipe declares `def GenTests(api): ...` and yields one `TestData` per
simulated run. `api` is the root `RecipeTestApi`. Each of the recipe's `DEPS`
modules contributes its own `TEST_API` (from the module's `test_api.py`),
injected as `api.<module>`, so a recipe writes, e.g.:

    def GenTests(api):
        yield api.test(
            'linux',
            api.platform.name('linux'),
            api.properties(chromium_ref='151.0.7917.1'),
            api.step_data('fetch chromium', retcode=0),
            api.post_process(post_process.MustRun, 'fetch chromium'),
            api.post_process(post_process.StatusSuccess),
        )

Each `api.*` call returns a `TestData` *fragment*; `api.test(name, *fragments)`
folds them together (via `TestData.__add__`) into the case the engine runs.

Notable simplifications vs upstream: no output placeholders, and a simple
`check(cond, hint)` collector rather than the AST-introspecting `Checker`.
"""

from __future__ import annotations

from collections.abc import Callable
import inspect
from typing import Any, NamedTuple

from google.protobuf import json_format as jsonpb
from google.protobuf.message import Message as PBMessage

from recipe_api import ModuleInjectionSite


class StepTestData:
    """Simulated result for a single step: retcode and captured output.

    Seeded in `GenTests` via `api.step_data(name, ...)`. During simulation the
    `step` module returns a `CompletedProcess` built from this, and raises
    `CalledProcessError` when the step is `check`ed and `retcode` is non-zero.
    """

    def __init__(self,
                 retcode: int | None = None,
                 stdout: str | None = None,
                 stderr: str | None = None) -> None:
        # `retcode` is kept optional internally so merging can tell "unset" from
        # an explicit 0; `.retcode` exposes the effective value (0 when unset).
        self._retcode = retcode
        self.stdout = stdout
        self.stderr = stderr

    @property
    def retcode(self) -> int:
        return self._retcode or 0

    def __add__(self, other: StepTestData) -> StepTestData:
        """Merge two fragments.

        A non-zero retcode from either side wins (0 is the unset default, so a
        later success fragment doesn't clobber an earlier failure); captured
        output is last-set-wins.
        """
        return StepTestData(
            retcode=other._retcode or self._retcode,
            stdout=other.stdout if other.stdout is not None else self.stdout,
            stderr=other.stderr if other.stderr is not None else self.stderr,
        )


class PostprocessHook(NamedTuple):
    """A `post_process` callback plus where it was registered (for messages)."""
    func: Callable[..., Any]
    args: tuple
    kwargs: dict
    context: str  # "<file>:<lineno>" of the api.post_process(...) call.


class TestData:
    """One simulated recipe run (or a fragment merged into one).

    Fragments returned by the `api.*` builders each populate one aspect
    (properties, a step's data, a module's seed values, a post-process hook, the
    expected overall status). `api.test` sums them into the final case.
    """

    def __init__(self, name: str = '') -> None:
        self.name = name
        # Recipe PROPERTIES payload (keys match the recipe's PROPERTIES fields).
        self.properties: dict[str, Any] = {}
        # ENV_PROPERTIES payload: env var name -> string value. Set via
        # `api.properties.environ(...)`; folded into the run's environment so
        # the engine can decode it into the recipe's ENV_PROPERTIES message.
        self.environ: dict[str, str] = {}
        # Per-step simulated results, keyed by step name.
        self.step_data: dict[str, StepTestData] = {}
        # Per-module seed values consumed to build the run's TestContext
        # (e.g. mod_data['platform'] = {'name': 'mac'}).
        self.mod_data: dict[str, dict[str, Any]] = {}
        # post_process checks run against the recorded steps after simulation.
        self.post_process_hooks: list[PostprocessHook] = []
        # Expected overall status ('SUCCESS' | 'FAILURE' | 'EXCEPTION'); None
        # means "don't assert" (a mismatch during a run is still reported).
        self.expected_status: str | None = None
        # brave-core ref the engine seeds (mirrors --brave-core-ref).
        self.brave_core_ref: str = 'master'
        # Absolute path of the expectation JSON; filled in by the test runner.
        self.expect_file: str | None = None

    def __add__(self, other: TestData) -> TestData:
        """Associatively merge *other* into a copy of *self*.

        Dicts update, lists concatenate, and last-set-wins for scalar fields --
        so fragment order in `api.test(...)` reads left-to-right.
        """
        merged = TestData(self.name or other.name)
        merged.properties = {**self.properties, **other.properties}
        merged.environ = {**self.environ, **other.environ}
        merged.mod_data = _merge_mod_data(self.mod_data, other.mod_data)
        merged.step_data = dict(self.step_data)
        for step_name, data in other.step_data.items():
            existing = merged.step_data.get(step_name)
            merged.step_data[step_name] = (existing +
                                           data if existing else data)
        merged.post_process_hooks = (self.post_process_hooks +
                                     other.post_process_hooks)
        merged.expected_status = (other.expected_status
                                  if other.expected_status is not None else
                                  self.expected_status)
        merged.brave_core_ref = (other.brave_core_ref if other.brave_core_ref
                                 != 'master' else self.brave_core_ref)
        return merged


def _merge_mod_data(a: dict[str, dict], b: dict[str, dict]) -> dict[str, dict]:
    """Merge per-module seed dicts: lists concatenate, dicts update, else last.

    List concatenation lets repeated fragments accumulate (two
    `api.path.files(...)` calls extend one seeded file list rather than the
    second clobbering the first); dict update lets `api.env.set(...)` calls pile
    up into one env mapping.
    """
    out: dict[str, dict] = {}
    for module in {**a, **b}:
        left, right = a.get(module, {}), b.get(module, {})
        merged = dict(left)
        for key, value in right.items():
            current = merged.get(key)
            if isinstance(value, list) and isinstance(current, list):
                merged[key] = current + value
            elif isinstance(value, dict) and isinstance(current, dict):
                merged[key] = {**current, **value}
            else:
                merged[key] = value
        out[module] = merged
    return out


class _PropertiesTestApi:
    """Builds PROPERTIES / ENV_PROPERTIES fragments for a test case.

    Exposed as `api.properties`. `api.properties(...)` sets the recipe's
    PROPERTIES payload and `api.properties.environ(...)` sets its ENV_PROPERTIES
    payload. Accepts protobuf message instances (merged via their JSONPB
    representation) and/or explicit key/value pairs.
    """

    def __call__(self, *proto_msgs: PBMessage, **kwargs: Any) -> TestData:
        """A fragment supplying the recipe's PROPERTIES payload.

        Positional args must be protobuf messages; their JSONPB representations
        are merged together with `dict.update`. Keyword args are merged into the
        properties at the top level.
        """
        data = TestData()
        for msg in proto_msgs:
            if not isinstance(msg, PBMessage):
                raise ValueError(
                    'Positional arguments for api.properties must be protobuf '
                    f'messages. Got: {msg!r} (type {type(msg)!r})')
            data.properties.update(
                jsonpb.MessageToDict(msg, preserving_proto_field_name=True))
        for key, value in kwargs.items():
            if isinstance(value, PBMessage):
                value = jsonpb.MessageToDict(value,
                                             preserving_proto_field_name=True)
            data.properties[key] = value
        return data

    def environ(self, *proto_msgs: PBMessage, **kwargs: Any) -> TestData:
        """A fragment supplying the recipe's ENV_PROPERTIES payload.

        Values (from message fields or kwargs) are stringified, since the
        engine decodes ENV_PROPERTIES from the environment.
        """
        data = TestData()
        to_apply = []
        for msg in proto_msgs:
            if not isinstance(msg, PBMessage):
                raise ValueError(
                    'Positional arguments for api.properties.environ must be '
                    f'protobuf messages. Got: {msg!r} (type {type(msg)!r})')
            to_apply.append(
                jsonpb.MessageToDict(msg, preserving_proto_field_name=True))
        to_apply.append(kwargs)

        for dictionary in to_apply:
            for key, value in dictionary.items():
                if not isinstance(value, (int, float, str)):
                    raise ValueError(
                        'Environment values must be int, float or string. '
                        f'Got: {key!r}={value!r} (type {type(value)!r})')
                data.environ[key] = str(value)
        return data


class RecipeTestApi:
    """Root test API (passed to `GenTests`) and base for module `TEST_API`s.

    Constructed with `module=None` for the root, where `self.m is self`, so
    `api.<module>` reaches an injected module test API and `api.test(...)` are
    called directly. A module's `TEST_API` is constructed with its module
    name. The engine wires that module's DEPS onto `self.m`, matching how the
    production engine wires `RecipeApi.m`.
    """

    def __init__(self, module: str | None = None) -> None:
        self._module = module
        self.m: ModuleInjectionSite | RecipeTestApi = (
            self if module is None else ModuleInjectionSite())

    def __getattr__(self, name: str):
        # DEPS module test APIs are injected by the runner (onto the root api,
        # or onto `self.m`); a missing one means it was not declared in DEPS.
        # (Also tells static analysis that these attributes are dynamic, so
        # accessing an injected dep is not flagged as no-member.)
        raise AttributeError(
            f'{name!r} is not an injected module test API (add it to DEPS?)')

    # -- Fragment builders available on the root api (and inherited by modules).

    # `api.properties(...)` / `api.properties.environ(...)`. A shared, stateless
    # builder (mirrors recipes_py's `properties` module TEST_API).
    properties = _PropertiesTestApi()

    @staticmethod
    def test(name: str,
             *test_data: TestData,
             status: str | None = None) -> TestData:
        """Fold *test_data* fragments into a single named test case."""
        base = TestData(name)
        if status is not None:
            base.expected_status = status
        for fragment in test_data:
            base = base + fragment
        return base

    @staticmethod
    def step_data(name: str,
                  retcode: int = 0,
                  stdout: str | None = None,
                  stderr: str | None = None) -> TestData:
        """A fragment seeding the simulated result of the step named *name*."""
        data = TestData()
        data.step_data[name] = StepTestData(retcode=retcode,
                                            stdout=stdout,
                                            stderr=stderr)
        return data

    @staticmethod
    def brave_core_ref(ref: str) -> TestData:
        """A fragment overriding the engine-seeded brave-core ref."""
        data = TestData()
        data.brave_core_ref = ref
        return data

    @staticmethod
    def post_process(func: Callable[..., Any], *args: Any,
                     **kwargs: Any) -> TestData:
        """A fragment registering a post-process check (see `post_process`)."""
        frame = inspect.currentframe()
        caller = frame.f_back if frame is not None else None
        context = (f'{caller.f_code.co_filename}:{caller.f_lineno}'
                   if caller is not None else '<unknown>')
        data = TestData()
        data.post_process_hooks.append(
            PostprocessHook(func, args, kwargs, context))
        return data

    @staticmethod
    def empty_test_data() -> TestData:
        """An empty fragment (useful as a merge identity in conditionals)."""
        return TestData()

    # -- Helper for module TEST_APIs to seed their own module's values.

    def _mod_data(self, **values: Any) -> TestData:
        """A fragment seeding this module's entry in `TestData.mod_data`.

        Only valid on a module `TEST_API` (which knows its module name); the
        root api has no module of its own.
        """
        assert self._module is not None, (
            '_mod_data is only available on a module TEST_API')
        data = TestData()
        data.mod_data[self._module] = dict(values)
        return data
