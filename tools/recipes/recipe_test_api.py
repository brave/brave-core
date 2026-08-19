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
"""

from __future__ import annotations

from collections import defaultdict
from collections.abc import Callable
import functools
import inspect
from typing import Any, NamedTuple

from google.protobuf import json_format as jsonpb
from google.protobuf.message import Message as PBMessage

from recipe_api import ModuleInjectionSite


class BaseTestData:
    """Common base for the simulated-data containers below.

    Its one job is the `enabled` flag: production code paths get a
    `DisabledTestData`, whose `enabled` is False, so a placeholder can tell
    "I am being simulated" from "I am really running" without the modules
    having to branch on test mode themselves.
    """

    def __init__(self, enabled: bool = True) -> None:
        self._enabled = enabled

    @property
    def enabled(self) -> bool:
        return self._enabled


class PlaceholderTestData(BaseTestData):
    """The simulated data for one placeholder on one step.

    `data` is whatever the placeholder's `result()` should hand back (a
    `None` means "pretend the backing file wasn't there"), and `name` matches
    the placeholder's caller-chosen name, so
    `api.json.output(..., name='cfg')` seeds only the `cfg` placeholder.
    """

    def __init__(self, data: Any = None, name: str | None = None) -> None:
        super().__init__()
        self.data = data
        self.name = name

    def __repr__(self) -> str:
        if self.name is None:
            return f'PlaceholderTestData(DEFAULT, {self.data!r})'
        return f'PlaceholderTestData({self.name!r}, {self.data!r})'


class StepTestData(BaseTestData):
    """Simulated result for a single step: retcode plus placeholder data.

    Seeded in `GenTests` via `api.step_data(name, ...)`, or by a step's own
    `step_test_data=` default. During simulation the `step` module builds the
    step's `StepData` from this, and raises `CalledProcessError` when the step
    is `check`ed and `retcode` is non-zero.
    """

    def __init__(self) -> None:
        super().__init__()
        # (module, method, placeholder name) -> PlaceholderTestData, for the
        # output placeholders in the step's command.
        self.placeholder_data: dict[tuple[str, str, str | None],
                                    PlaceholderTestData] = {}
        # When True this fragment replaces (rather than merges into) whatever
        # came before it -- see `api.override_step_data`.
        self.override = False
        # `retcode` is kept optional internally so merging can tell "unset"
        # from an explicit 0; `.retcode` exposes the effective value.
        self._retcode: int | None = None
        self._stdout: PlaceholderTestData | None = None
        self._stderr: PlaceholderTestData | None = None

    def __add__(self, other: StepTestData) -> StepTestData:
        """Merge two fragments into a new one.

        Placeholder data is last-set-wins per key, and so are the std handles.
        Two fragments setting *different* retcodes for one step is a mistake in
        the test rather than something to silently resolve. An `override`
        fragment discards everything to its left.
        """
        base = StepTestData() if other.override else self
        merged = StepTestData()
        merged.override = other.override
        merged.placeholder_data = {
            **base.placeholder_data,
            **other.placeholder_data
        }
        merged._stdout = other._stdout or base._stdout
        merged._stderr = other._stderr or base._stderr
        merged._retcode = base._retcode
        if other._retcode is not None:
            if (merged._retcode is not None
                    and merged._retcode != other._retcode):
                raise ValueError('Conflicting retcode values: '
                                 f'{merged._retcode} and {other._retcode}')
            merged._retcode = other._retcode
        return merged

    def unwrap_placeholder(self) -> PlaceholderTestData:
        """Return this fragment's single placeholder datum.

        Used to reuse an output placeholder's test data as a step's `stdout`
        or `stderr`, e.g. `api.raw_io.stream_output_text('hi')`.
        """
        if len(self.placeholder_data) != 1:
            raise ValueError('Cannot unwrap step test data holding '
                             f'{len(self.placeholder_data)} placeholders')
        return next(iter(self.placeholder_data.values()))

    def pop_placeholder(self, module: str, method: str,
                        name: str | None) -> PlaceholderTestData:
        """Take the datum seeded for one placeholder (empty if none was)."""
        return self.placeholder_data.pop((module, method, name),
                                         PlaceholderTestData())

    @property
    def retcode(self) -> int:
        return self._retcode or 0

    @retcode.setter
    def retcode(self, value: int | None) -> None:
        self._retcode = value

    @property
    def stdout(self) -> PlaceholderTestData:
        return self._stdout or PlaceholderTestData()

    @stdout.setter
    def stdout(self, value: PlaceholderTestData) -> None:
        assert isinstance(value, PlaceholderTestData)
        self._stdout = value

    @property
    def stderr(self) -> PlaceholderTestData:
        return self._stderr or PlaceholderTestData()

    @stderr.setter
    def stderr(self, value: PlaceholderTestData) -> None:
        assert isinstance(value, PlaceholderTestData)
        self._stderr = value

    @property
    def stdin(self) -> PlaceholderTestData:
        # An input placeholder has nothing to hand back, so there is nothing to
        # seed either; this exists so the `step` module can treat all three
        # handles alike.
        return PlaceholderTestData()

    def __repr__(self) -> str:
        return ('StepTestData(%r)' % {
            'placeholder_data': self.placeholder_data,
            'stdout': self._stdout,
            'stderr': self._stderr,
            'retcode': self._retcode,
            'override': self.override,
        })


class DisabledTestData(BaseTestData):
    """Stand-in for simulated data on the production path.

    Every lookup answers with itself, and `enabled` is False all the way down,
    so a placeholder rendering for a real step sees "no test data" no matter
    what it asks for.
    """

    def __init__(self) -> None:
        super().__init__(False)

    def __getattr__(self, name: str) -> DisabledTestData:
        return self

    def pop_placeholder(self, module: str, method: str,
                        name: str | None) -> DisabledTestData:
        del module, method, name
        return self


class PostprocessHookContext(NamedTuple):
    """A `post_process` callback plus where it was registered.

    `filename`/`lineno` locate the `api.post_process(...)` call site, so a failed
    check can render the `added <file>:<lineno>` footer and the call repr.
    """
    func: Callable[..., Any]
    args: tuple
    kwargs: dict
    filename: str
    lineno: int


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
        # Per-step simulated results, keyed by step name. A defaultdict so a
        # fragment builder can set one field (a retcode, say) on a step that
        # has no data yet.
        self.step_data: dict[str, StepTestData] = defaultdict(StepTestData)
        # Per-module seed values consumed to build the run's TestContext
        # (e.g. mod_data['platform'] = {'name': 'mac'}).
        self.mod_data: dict[str, dict[str, Any]] = {}
        # post_process checks run against the recorded steps after simulation.
        self.post_process_hooks: list[PostprocessHookContext] = []
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
        merged.step_data.update(self.step_data)
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

    def get_step_test_data(
            self, step_name: str,
            step_test_data_fn: Callable[[], StepTestData]) -> StepTestData:
        """The simulated data for the step named *step_name*.

        `step_test_data_fn` builds the step's own default data (its
        `step_test_data=` argument), which whatever this case seeded for the
        step is then merged on top of -- so a test can leave the default alone,
        add to it, or replace it wholesale with `api.override_step_data`.

        Note that step names are not deduplicated by this engine, so a recipe
        that runs the same step twice sees the seeded data both times.
        """
        step_test_data = step_test_data_fn()
        if step_name in self.step_data:
            try:
                step_test_data += self.step_data[step_name]
            except ValueError as ex:
                raise ValueError(f'in step {step_name!r}: {ex}') from ex
        return step_test_data


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


def _placeholder_step_data(func, placeholder_name: str | None = None):

    @functools.wraps(func)
    def inner(self, *args, **kwargs):
        data = func(self, *args, **kwargs)
        if isinstance(data, StepTestData):
            # The method delegated to another `placeholder_step_data` one;
            # adopt its single datum (and retcode) as this one's.
            placeholder_data = data.unwrap_placeholder()
            retcode = data.retcode if data.retcode else None
        else:
            value, retcode, name = data
            placeholder_data = PlaceholderTestData(data=value, name=name)

        fragment = StepTestData()
        method = placeholder_name or func.__name__
        fragment.placeholder_data[(
            self._module,
            method,  # pylint: disable=protected-access
            placeholder_data.name)] = placeholder_data
        fragment.retcode = retcode
        return fragment

    return inner


def placeholder_step_data(func_or_name):
    """Decorate a module `TEST_API` method to seed one output placeholder.

    A decorated method returns the plain
    `(<placeholder data>, <retcode or None>, <name or None>)` triple, and this
    wraps it into the `StepTestData` fragment `api.step_data` expects, keyed by
    the module and method name so it lines up with the matching
    `returns_placeholder` method on the module's `api.py`. So for the `json`
    module:

        class JsonTestApi(RecipeTestApi):
            @placeholder_step_data
            def output(self, data, retcode=None, name=None):
                return json.dumps(data), retcode, name

    `api.json.output({'passed': 791})` then seeds the data an
    `api.json.output()` placeholder hands back.

    A decorated method may also return the result of calling another decorated
    method, to reuse its data under this method's name.

    Decorate as `@placeholder_step_data('other_name')` to seed the placeholder
    produced by a *differently* named api method -- that is how, e.g.,
    `api.json.invalid(...)` seeds an `api.json.output()` placeholder.
    """
    if isinstance(func_or_name, str):
        if not func_or_name:
            raise ValueError('placeholder_step_data needs a non-empty name')
        return lambda func: _placeholder_step_data(func, func_or_name)
    if not callable(func_or_name):
        raise ValueError('Expected either a function or a string; got '
                         f'{func_or_name!r}')
    return _placeholder_step_data(func_or_name)


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
    # builder.
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
                  *data: StepTestData,
                  retcode: int | None = None,
                  stdout: StepTestData | None = None,
                  stderr: StepTestData | None = None,
                  override: bool = False) -> TestData:
        """A fragment seeding the simulated result of the step named *name*.

        Args:
            name: The name of the step being seeded.
            data: Zero or more `StepTestData` fragments, each supplying one
                output placeholder's data (and possibly a retcode) -- e.g.
                `api.json.output({'ok': True})`.
            retcode: The step's exit code. Overrides any retcode carried by
                *data*; unset means the step exits 0.
            stdout: A `StepTestData` holding a single placeholder datum, used
                as the step's standard output -- e.g.
                `stdout=api.raw_io.output_text('hello')`.
            stderr: As *stdout*, for standard error.
            override: Discard whatever was seeded for this step so far
                (including the step's own `step_test_data=` default) instead of
                merging into it.

        Example:

            yield api.test(
                'winning',
                api.step_data('run tests', api.json.output({'passed': 791})),
                api.step_data('flaky', retcode=1),
                api.step_data('greet', stdout=api.raw_io.output_text('hi\\n')),
            )
        """
        fragment = TestData()
        step = fragment.step_data[name]
        for datum in data:
            if not isinstance(datum, StepTestData):
                raise ValueError(
                    f'api.step_data({name!r}, ...) takes step test data '
                    f'fragments. Got: {datum!r} (type {type(datum)!r})')
            step += datum
        if retcode is not None:
            step.retcode = retcode
        if stdout is not None:
            step.stdout = stdout.unwrap_placeholder()
        if stderr is not None:
            step.stderr = stderr.unwrap_placeholder()
        step.override = override
        fragment.step_data[name] = step
        return fragment

    @staticmethod
    def override_step_data(name: str, *data: StepTestData,
                           **kwargs: Any) -> TestData:
        """Like `step_data`, but replaces this step's data rather than adding.

        Use this to drop a step's own `step_test_data=` default (or an earlier
        fragment's data) instead of merging on top of it.
        """
        return RecipeTestApi.step_data(name, *data, override=True, **kwargs)

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
        filename = (caller.f_code.co_filename
                    if caller is not None else '<unknown>')
        lineno = caller.f_lineno if caller is not None else 0
        data = TestData()
        data.post_process_hooks.append(
            PostprocessHookContext(func, args, kwargs, filename, lineno))
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
