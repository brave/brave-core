# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
#
# For some of the code lifted below.
#
# Copyright 2015 The LUCI Authors
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.
"""Host for `brave/infra/config`'s named, composable GN-arg configs.

A config is a node with `args` (GN arg values), `configs` (names of other
configs it includes) and `args_file` (one `.gni` it imports). `resolve()`
merges a config with everything it includes: later entries in `configs` beat
earlier ones, and a config's own `args` beat anything it includes. At most one
`args_file` may survive a resolution. `target_os`/`target_cpu` are required in
the result.

Example:

    from lib.config import gn_args

    gn_args.config(name = "linux", args = {"target_os": "linux"})
    gn_args.config(name = "x64", args = {"target_cpu": "x64"})
    gn_args.config(
        name = "asan",
        configs = ["linux", "x64"],
        args = {"is_asan": True, "is_lsan": True},
        secrets = {"brave_services_key": "BRAVE_SERVICES_KEY"},
    )

    gn_args.resolve("asan")
    # {"gn_args": {"target_os": "linux", "target_cpu": "x64",
    #              "is_asan": True, "is_lsan": True},
    #  "secrets": {"brave_services_key": "BRAVE_SERVICES_KEY"}}
"""

from __future__ import annotations

import collections.abc
import functools
import operator
from typing import Any, Iterator, Mapping, Sequence, TypeVar

K = TypeVar('K')
V = TypeVar('V')


def freeze(obj: Any) -> Any:
    """Returns an immutable version of `obj`.

    dict -> FrozenDict, list/tuple -> tuple, set -> frozenset. Anything else is
    returned as-is, provided it is hashable (raises `TypeError` otherwise).
    """
    if isinstance(obj, dict):
        return FrozenDict((freeze(k), freeze(v)) for k, v in obj.items())
    if isinstance(obj, (list, tuple)):
        return tuple(freeze(i) for i in obj)
    if isinstance(obj, set):
        return frozenset(freeze(i) for i in obj)
    hash(obj)  # Raises TypeError for unhashable (assumed mutable) values.
    return obj


def thaw(obj: Any) -> Any:
    """Returns a mutable version of a value produced by `freeze()`."""
    if isinstance(obj, (dict, collections.abc.Mapping)):
        return {k: thaw(v) for k, v in obj.items()}
    if isinstance(obj, (list, tuple)):
        return [thaw(i) for i in obj]
    if isinstance(obj, (set, frozenset)):
        return {thaw(i) for i in obj}
    return obj


class FrozenDict(collections.abc.Mapping[K, V]):
    """An immutable, hashable mapping.

    Ported from recipes-py's `recipe_engine/engine_types.py`, trimmed of the
    `on_missing` hook and the `config_types.Path` carve-out (neither has a use
    here), and relying on `collections.abc.Mapping`'s own `__eq__` rather than
    redefining one.
    """

    def __init__(self, *args: Any, **kwargs: Any) -> None:
        self._d = dict(*args, **kwargs)
        # Computed eagerly so that constructing a FrozenDict of unhashable
        # values fails immediately, at the point of freezing, not on first use.
        self._hash = functools.reduce(
            operator.xor, (hash(i) for i in enumerate(self._d.items())), 0
        )

    def __iter__(self) -> Iterator[K]:
        return iter(self._d)

    def __len__(self) -> int:
        return len(self._d)

    def __getitem__(self, key: K) -> V:
        return self._d[key]

    def __hash__(self) -> int:
        return self._hash

    def __repr__(self) -> str:
        return 'FrozenDict(%r)' % (list(self._d.items()),)


class ConfigError(Exception):
    """Raised for any gn_args or builder configuration violation."""


class AnonymousGnConfig:
    """The inline, unnamed form of a GN config.

    Returned by `gn_args.config()` when called without `name`, for embedding
    directly in a builder definition rather than registering it under a name
    for reuse.
    """

    def __init__(
        self,
        *,
        args: Mapping[str, Any],
        configs: Sequence[str],
        args_file: str,
        secrets: Mapping[str, str],
    ) -> None:
        # GN arg key-value pairs this config contributes directly.
        self.gn_args = freeze(args)

        # Names of other configs this one includes, in override order.
        self.configs = tuple(configs)

        # Path of the one `.gni` this config imports, or '' for none.
        self.args_file = args_file

        # GN arg name -> environment variable name, resolved outside version
        # control.
        self.secrets = freeze(secrets)

    def __repr__(self) -> str:
        return (
            'AnonymousGnConfig(gn_args=%r, configs=%r, args_file=%r, '
            'secrets=%r)'
            % (self.gn_args, self.configs, self.args_file, self.secrets)
        )


class _GnConfigNode:
    """One registered, named node in the GN-args include graph."""

    def __init__(
        self,
        name: str,
        *,
        args: Mapping[str, Any],
        configs: Sequence[str],
        args_file: str,
        secrets: Mapping[str, str],
    ) -> None:
        # The name it was registered under.
        self.name = name

        # GN arg key-value pairs this config contributes directly.
        self.gn_args = freeze(args)

        # Names of other configs this one includes, in override order.
        self.configs = tuple(configs)

        # Path of the one `.gni` this config imports, or '' for none.
        self.args_file = args_file

        # GN arg name -> environment variable name, resolved outside version
        # control.
        self.secrets = freeze(secrets)


def _merge_into(
    dst: dict,
    *,
    args_file: str,
    args: Mapping[str, Any],
    secrets: Mapping[str, str],
    config_name: str,
) -> None:
    """Merges one node's contribution into a resolution-in-progress `dst`.

    `dst`'s existing entries lose to `args`/`secrets` on key conflicts, which
    is what makes "merge children then merge self last" give the node's own
    values priority over whatever it includes. `args_file` is the exception:
    a second non-empty `args_file` raises `ConfigError` rather than winning.
    """
    if args_file:
        if dst['args_file']:
            raise ConfigError(
                'gn_args config %r: each GN config can only contain a '
                'single args_file (already have %r, got %r)'
                % (config_name, dst['args_file'], args_file)
            )
        dst['args_file'] = args_file
    dst['gn_args'].update(args)
    dst['secrets'].update(secrets)


class GnArgsRegistry:
    """A registry of named GN configs, and their resolver.

    Spec files call `config()` to declare configs. `resolve()` walks the
    include graph a declared (or builder-level) config names to produce its
    final `{args_file, gn_args, secrets}`.
    """

    def __init__(self) -> None:
        # Name -> declared config.
        self._nodes: dict[str, _GnConfigNode] = {}

        # Name -> memoized resolve() result.
        self._resolved: dict[str, dict] = {}

    def config(
        self,
        *,
        name: str | None = None,
        args: Mapping[str, Any] | None = None,
        configs: Sequence[str] | None = None,
        args_file: str = '',
        secrets: Mapping[str, str] | None = None,
    ) -> None | AnonymousGnConfig:
        """Defines a GN config, or returns one inline.

        Args:
            name: The config's name, for reference from another config's
                `configs` list or from `resolve()`. Registers the config and
                returns None. Omit to get an `AnonymousGnConfig` back instead,
                for a one-off builder that need not pollute the config
                namespace.
            args: GN arg key-value pairs this config contributes.
            configs: Names of other configs to include. Later entries beat
                earlier ones on a duplicate GN arg key. This config's own
                `args` beat all of them.
            args_file: The path of one `.gni` this config imports.
            secrets: GN arg name to environment variable name, for values
                resolved outside version control instead of checked in.

        Returns:
            None if `name` is set, otherwise an `AnonymousGnConfig`.
        """
        args = args or {}
        configs = configs or []
        secrets = secrets or {}

        if name:
            if name in self._nodes:
                raise ConfigError('gn_args config %r is already defined' % name)
            self._nodes[name] = _GnConfigNode(
                name,
                args=args,
                configs=configs,
                args_file=args_file,
                secrets=secrets,
            )
            return None

        return AnonymousGnConfig(
            args=args, configs=configs, args_file=args_file, secrets=secrets
        )

    def resolve(self, name: str) -> dict:
        """Resolves the named config's GN args, secrets and args_file.

        Returns a dict with a `gn_args` key (never absent nor empty; missing
        `target_os`/`target_cpu` in it is an error) and, only when non-empty,
        `args_file` and/or `secrets` keys. The returned dict is a fresh,
        plain-dict copy: mutating it does not affect the registry's cache.

        Raises:
            ConfigError: `name` (or something it includes, transitively) is
                undefined, the include graph has a cycle, more than one
                `args_file` survives the resolution, or the resolved args are
                missing `target_os` or `target_cpu`.
        """
        try:
            root = self._nodes[name]
        except KeyError as e:
            raise ConfigError('gn_args config %r is not defined' % name) from e

        resolved = self._resolve_node(root, stack=())

        args = dict(resolved['gn_args'])
        if 'target_os' not in args:
            raise ConfigError('target_os is required for gn_args: %s' % name)
        if 'target_cpu' not in args:
            raise ConfigError('target_cpu is required for gn_args: %s' % name)

        result = {'gn_args': args}
        if resolved['args_file']:
            result['args_file'] = resolved['args_file']
        if resolved['secrets']:
            result['secrets'] = dict(resolved['secrets'])
        return result

    def _resolve_node(
        self, node: _GnConfigNode, stack: tuple[str, ...]
    ) -> dict:
        if node.name in self._resolved:
            return self._resolved[node.name]
        if node.name in stack:
            raise ConfigError(
                'gn_args config cycle: %s -> %s'
                % (' -> '.join(stack), node.name)
            )
        stack = stack + (node.name,)

        merged = {'args_file': '', 'gn_args': {}, 'secrets': {}}

        # Merge included configs first, in the order given (DEFINITION_ORDER
        # upstream): a later entry's keys overwrite an earlier entry's, since
        # each merge is a dict.update() into the same accumulator.
        for child_name in node.configs:
            try:
                child = self._nodes[child_name]
            except KeyError as e:
                raise ConfigError(
                    'gn_args config %r includes undefined config %r'
                    % (node.name, child_name)
                ) from e
            child_resolved = self._resolve_node(child, stack)
            _merge_into(
                merged,
                args_file=child_resolved['args_file'],
                args=child_resolved['gn_args'],
                secrets=child_resolved['secrets'],
                config_name=node.name,
            )

        # Merge the node's own values last, so they win over anything included.
        _merge_into(
            merged,
            args_file=node.args_file,
            args=node.gn_args,
            secrets=node.secrets,
            config_name=node.name,
        )

        self._resolved[node.name] = merged
        return merged


# What spec files import: `from lib.config import gn_args`.
gn_args = GnArgsRegistry()

# Sentinel distinguishing "caller didn't pass this" from an explicit falsy
# value (0, '', [], None), mirroring upstream's `args.DEFAULT`.
_UNSET = object()


class Defaults:
    """A settable group of module-level defaults for `builder()`."""

    def __init__(self, **defaults: Any) -> None:
        self._values = dict(defaults)

    def set(self, **kwargs: Any) -> None:
        """Sets module-level defaults for the given fields."""
        for key, value in kwargs.items():
            if key not in self._values:
                raise ConfigError('%r is not a defaultable field' % key)
            self._values[key] = value

    def get(self, name: str, value: Any) -> Any:
        """Returns `value`, or the module-level default if `value` is unset."""
        if value is not _UNSET:
            return value
        return self._values[name]


class _Enum:
    """starlark-libs' `enums.enum()`: one attribute per named value, plus
    `values`, for checking a value is one of them."""

    def __init__(self, **members: str) -> None:
        if 'values' in members:
            raise ConfigError("cannot create an enum value named 'values'")
        self.__dict__.update(members)
        self.__dict__['values'] = tuple(members.values())

    def __setattr__(self, name: str, value: Any) -> None:
        raise TypeError('enum values cannot be reassigned')


_build_config = _Enum(
    RELEASE='Release',
    DEBUG='Debug',
)

_target_arch = _Enum(
    INTEL='intel',
    ARM='arm',
)

_target_platform = _Enum(
    LINUX='linux',
    WIN='win',
    MAC='mac',
    IOS='ios',
    ANDROID='android',
)

_host_platform = _Enum(
    LINUX='linux',
    WIN='win',
    MAC='mac',
)

# The CPUs Brave builds for, by their gclient (and GN) names.
_GCLIENT_CPUS = (
    'arm',
    'arm64',
    'x64',
    'x86',
)

# GN's target_cpu for a chromium_config's (target_arch, target_bits).
_GN_TARGET_CPU = {
    (_target_arch.INTEL, 32): 'x86',
    (_target_arch.INTEL, 64): 'x64',
    (_target_arch.ARM, 32): 'arm',
    (_target_arch.ARM, 64): 'arm64',
}


class GclientConfig:
    """What `init`/`sync` write into `.gclient` for a builder.

    The platforms the checkout has dependencies for, which can be more than
    the one the builder builds for (its `chromium_config`). As in gclient
    itself, the host's are always synced, so a builder that only builds for
    its host can leave both lists empty.

    And what the builder adds to the chromium solution's `custom_deps` and
    `custom_vars`, on top of `package.json`'s, which every checkout has.
    """

    def __init__(
        self,
        *,
        target_os: Sequence[str],
        target_cpu: Sequence[str],
        custom_deps: Mapping[str, str | None],
        custom_vars: Mapping[str, bool | str],
    ) -> None:
        # gclient's `target_os`: `target_platform` values.
        self.target_os = _sorted_values(
            'target_os', target_os, _target_platform.values
        )

        # gclient's `target_cpu`, e.g. ("arm64",).
        self.target_cpu = _sorted_values(
            'target_cpu', target_cpu, _GCLIENT_CPUS
        )

        # The DEPS entries to override, by path, e.g.
        # {"src/third_party/chromium-variations": None} to skip one.
        self.custom_deps = _sorted_mapping(
            'custom_deps', custom_deps, (str, type(None))
        )

        # The DEPS vars to override, e.g.
        # {"checkout_instrumented_libraries": True}.
        self.custom_vars = _sorted_mapping(
            'custom_vars', custom_vars, (bool, str)
        )


def _sorted_mapping(
    field: str, mapping: Mapping[str, Any], value_types: tuple[type, ...]
) -> FrozenDict[str, Any]:
    if not isinstance(mapping, Mapping):
        raise ConfigError('%s must be a mapping, got %r' % (field, mapping))
    for key, value in mapping.items():
        if not isinstance(key, str) or not isinstance(value, value_types):
            raise ConfigError('invalid %s entry: %r: %r' % (field, key, value))
    return FrozenDict(sorted(mapping.items()))


def _sorted_values(
    field: str, values: Sequence[str], allowed: Sequence[str]
) -> tuple[str, ...]:
    if isinstance(values, str):
        raise ConfigError('%s must be a list, got %r' % (field, values))
    for value in values:
        if value not in allowed:
            raise ConfigError('unknown %s: %r' % (field, value))
    return tuple(sorted(set(values)))


class ChromiumConfig:
    """The one platform a builder builds for, and how.

    Mirrors upstream's `builder_config.chromium_config()`, minus the recipe
    config item names (`config`, `apply_configs`) and the CrOS boards, which
    have no use here.
    """

    def __init__(
        self,
        *,
        target_platform: str,
        host_platform: str | None,
        build_config: str | None,
        target_arch: str | None,
        target_bits: int | None,
    ) -> None:
        if (
            build_config is not None
            and build_config not in _build_config.values
        ):
            raise ConfigError('unknown build_config: %r' % (build_config,))
        if target_arch is not None and target_arch not in _target_arch.values:
            raise ConfigError('unknown target_arch: %r' % (target_arch,))
        if target_bits is not None and target_bits not in (32, 64):
            raise ConfigError('unknown target_bits: %r' % (target_bits,))
        if (
            host_platform is not None
            and host_platform not in _host_platform.values
        ):
            raise ConfigError('unknown host_platform: %r' % (host_platform,))
        if target_platform not in _target_platform.values:
            raise ConfigError(
                'unknown target_platform: %r' % (target_platform,)
            )

        # A `target_platform` value: GN's target_os.
        self.target_platform = target_platform

        # A `host_platform` value: what the builder runs on.
        self.host_platform = host_platform

        # A `build_config` value.
        self.build_config = build_config

        # A `target_arch` value; with `target_bits`, GN's target_cpu.
        self.target_arch = target_arch

        # 32 or 64.
        self.target_bits = target_bits

    def gn_target_cpu(self) -> str | None:
        """GN's target_cpu, if `target_arch` and `target_bits` are set."""
        if self.target_arch is None or self.target_bits is None:
            return None
        return _GN_TARGET_CPU[(self.target_arch, self.target_bits)]


class BuilderSpec:
    """How a builder syncs and what it builds for."""

    def __init__(
        self, *, gclient_config: GclientConfig, chromium_config: ChromiumConfig
    ) -> None:
        if not isinstance(gclient_config, GclientConfig):
            raise ConfigError(
                'gclient_config must be a builder_config.gclient_config()'
            )
        if not isinstance(chromium_config, ChromiumConfig):
            raise ConfigError(
                'chromium_config must be a builder_config.chromium_config()'
            )

        # This builder's `gclient_config()`.
        self.gclient_config = gclient_config

        # This builder's `chromium_config()`.
        self.chromium_config = chromium_config


class _BuilderConfig:
    """What spec files reach as `builder_config`, as upstream's
    `builder_config.star` exports it."""

    build_config = _build_config
    target_arch = _target_arch
    target_platform = _target_platform
    host_platform = _host_platform

    @staticmethod
    def gclient_config(
        *,
        target_os: Sequence[str] = (),
        target_cpu: Sequence[str] = (),
        custom_deps: Mapping[str, str | None] | None = None,
        custom_vars: Mapping[str, bool | str] | None = None,
    ) -> GclientConfig:
        """The platforms `init`/`sync` sync for, besides the host's, and what
        the builder adds to the chromium solution's `custom_deps` and
        `custom_vars`.

        Unlike upstream's, which names gclient recipe config items, this
        takes what they set directly: `target_os` takes `target_platform`
        values.
        """
        return GclientConfig(
            target_os=target_os,
            target_cpu=target_cpu,
            custom_deps=custom_deps or {},
            custom_vars=custom_vars or {},
        )

    @staticmethod
    def chromium_config(
        *,
        target_platform: str,
        host_platform: str | None = None,
        build_config: str | None = None,
        target_arch: str | None = None,
        target_bits: int | None = None,
    ) -> ChromiumConfig:
        return ChromiumConfig(
            target_platform=target_platform,
            host_platform=host_platform,
            build_config=build_config,
            target_arch=target_arch,
            target_bits=target_bits,
        )

    @staticmethod
    def builder_spec(
        *, gclient_config: GclientConfig, chromium_config: ChromiumConfig
    ) -> BuilderSpec:
        return BuilderSpec(
            gclient_config=gclient_config, chromium_config=chromium_config
        )


# What spec files import: `from lib.config import builder_config`.
builder_config = _BuilderConfig()


def check_builder_spec(
    builder: Builder, resolved_gn_args: Mapping[str, Any]
) -> None:
    """Checks a builder's spec against its resolved GN args.

    The build target is stated by both `chromium_config` and the GN args, so
    they must agree. A builder declaring its `host_platform` must also sync
    the platform it builds for, and one built with `is_msan` must check out
    the instrumented libraries.

    Raises:
        ConfigError: they disagree.
    """
    chromium_config = builder.builder_spec.chromium_config
    if resolved_gn_args.get('target_os') != chromium_config.target_platform:
        raise ConfigError(
            'builder %r: chromium_config target_platform %r does not match '
            'GN target_os %r'
            % (
                builder.name,
                chromium_config.target_platform,
                resolved_gn_args.get('target_os'),
            )
        )
    target_cpu = chromium_config.gn_target_cpu()
    if (
        target_cpu is not None
        and resolved_gn_args.get('target_cpu') != target_cpu
    ):
        raise ConfigError(
            'builder %r: chromium_config target_arch %r and target_bits %r '
            'make GN target_cpu %r, not %r'
            % (
                builder.name,
                chromium_config.target_arch,
                chromium_config.target_bits,
                target_cpu,
                resolved_gn_args.get('target_cpu'),
            )
        )
    gclient_config = builder.builder_spec.gclient_config
    if resolved_gn_args.get('is_msan') and not gclient_config.custom_vars.get(
        'checkout_instrumented_libraries'
    ):
        raise ConfigError(
            'builder %r: builds with is_msan, but gclient_config custom_vars '
            'does not set checkout_instrumented_libraries' % builder.name
        )
    if (
        chromium_config.host_platform is not None
        and chromium_config.target_platform != chromium_config.host_platform
        and chromium_config.target_platform not in gclient_config.target_os
    ):
        raise ConfigError(
            'builder %r: builds for %r on %r, but gclient_config target_os '
            'does not sync it'
            % (
                builder.name,
                chromium_config.target_platform,
                chromium_config.host_platform,
            )
        )


class Binary:
    """A test binary: a GN target that runs tests (see `targets.binaries`)."""

    def __init__(self, *, name: str, label: str, binary_type: str) -> None:
        # The ninja target name, which tests refer to the binary by.
        self.name = name

        # The GN label of the target, e.g. "//brave/test:brave_unit_tests".
        self.label = label

        # "console_test_launcher", or "windowed_test_launcher" for a binary
        # that needs a display (Xvfb on Linux).
        self.type = binary_type


class GTestTest:
    """A gtest-based test, as `targets.tests.gtest_test()` declares one."""

    def __init__(
        self,
        *,
        name: str,
        binary: str,
        args: Sequence[str],
        mixins: Sequence[Any],
    ) -> None:
        # The name that refers to the test, and its step name.
        self.name = name

        # The name of the binary the test runs.
        self.binary = binary

        # The arguments passed to the binary.
        self.args = tuple(args)

        # The mixins applied to the test, before any of its bundles'.
        self.mixins = tuple(mixins)


class Mixin:
    """Values added to every test a bundle or test applies it to."""

    def __init__(self, *, args: Sequence[str]) -> None:
        # Arguments appended to the test's.
        self.args = tuple(args)


class Bundle:
    """A grouping of targets to build and test (see `targets.bundle()`)."""

    def __init__(
        self,
        *,
        targets: Sequence[Any],  # pylint: disable=redefined-outer-name
        additional_compile_targets: Sequence[str],
        mixins: Sequence[Any],
    ) -> None:
        # The tests and bundles it includes, by name or inline.
        self.targets = tuple(targets)

        # The compile targets it builds but does not run, by name.
        self.additional_compile_targets = tuple(additional_compile_targets)

        # The mixins applied to every test it includes.
        self.mixins = tuple(mixins)


class Targets:
    """What a builder compiles and tests: its bundle, expanded."""

    def __init__(
        self,
        *,
        additional_compile_targets: Sequence[str],
        gtest_tests: Sequence[Mapping[str, Any]],
    ) -> None:
        # The ninja targets to build but not run.
        self.additional_compile_targets = tuple(additional_compile_targets)

        # The gtest tests to run: each with its `name`, its `test` (the
        # binary) and, when there are any, its `args`.
        self.gtest_tests = tuple(dict(test) for test in gtest_tests)


class _Binaries:
    def __init__(self, registry: TargetsRegistry) -> None:
        self._registry = registry

    def console_test_launcher(self, *, name: str, label: str) -> None:
        """Defines a gtest binary that runs without a display."""
        self._registry._add_binary(name, label, 'console_test_launcher')

    def windowed_test_launcher(self, *, name: str, label: str) -> None:
        """Defines a gtest binary that needs a display (Xvfb on Linux)."""
        self._registry._add_binary(name, label, 'windowed_test_launcher')


class _Tests:
    def __init__(self, registry: TargetsRegistry) -> None:
        self._registry = registry

    def gtest_test(
        self,
        *,
        name: str,
        binary: str | None = None,
        args: Sequence[str] | None = None,
        mixins: Sequence[Any] | None = None,
    ) -> None:
        """Defines a gtest-based test.

        Args:
            name: The name that refers to the test in bundles, and its step
                name.
            binary: The binary the test runs, the one named `name` without it.
            args: The arguments passed to the binary.
            mixins: Mixins to apply to the test, by name or inline.
        """
        self._registry._add_test(
            GTestTest(
                name=name,
                binary=binary or name,
                args=args or (),
                mixins=mixins or (),
            )
        )


class TargetsRegistry:
    """A registry of test binaries, tests, compile targets, mixins and
    bundles, and their expansion into a builder's `Targets`."""

    def __init__(self) -> None:
        # Functions declaring binaries, which tests run.
        self.binaries = _Binaries(self)

        # Functions declaring tests.
        self.tests = _Tests(self)

        # Name -> declared binary, test, compile target, mixin and bundle.
        self._binaries: dict[str, Binary] = {}
        self._tests: dict[str, GTestTest] = {}
        self._compile_targets: dict[str, str] = {}
        self._mixins: dict[str, Mixin] = {}
        self._bundles: dict[str, Bundle] = {}

    def _add_binary(self, name: str, label: str, binary_type: str) -> None:
        if name in self._binaries:
            raise ConfigError('binary %r is already defined' % name)
        self._binaries[name] = Binary(
            name=name, label=label, binary_type=binary_type
        )

    def _add_test(self, test: GTestTest) -> None:
        if test.name in self._tests:
            raise ConfigError('test %r is already defined' % test.name)
        self._tests[test.name] = test

    def compile_target(self, *, name: str, label: str) -> None:
        """Defines a target that bundles can build without running it.

        Args:
            name: The ninja target name, which bundles refer to it by.
            label: The target's GN label.
        """
        if name in self._compile_targets:
            raise ConfigError('compile target %r is already defined' % name)
        self._compile_targets[name] = label

    def mixin(
        self, *, name: str | None = None, args: Sequence[str] | None = None
    ) -> None | Mixin:
        """Defines a mixin, or returns one inline without `name`.

        Args:
            name: The name tests and bundles refer to it by.
            args: Arguments appended to those of every test it applies to.
        """
        mixin = Mixin(args=args or ())
        if name is None:
            return mixin
        if name in self._mixins:
            raise ConfigError('mixin %r is already defined' % name)
        self._mixins[name] = mixin
        return None

    def bundle(
        self,
        *,
        name: str | None = None,
        # pylint: disable-next=redefined-outer-name
        targets: Sequence[Any] | None = None,
        additional_compile_targets: Sequence[str] | None = None,
        mixins: Sequence[Any] | None = None,
    ) -> None | Bundle:
        """Defines a bundle, or returns one inline without `name`.

        Args:
            name: The name bundles and builders refer to it by.
            targets: The tests and bundles it includes, by name or inline.
            additional_compile_targets: Compile targets it builds but does
                not run, by name.
            mixins: Mixins applied to every test it includes, after the
                tests' own, by name or inline.
        """
        bundle = Bundle(
            targets=targets or (),
            additional_compile_targets=additional_compile_targets or (),
            mixins=mixins or (),
        )
        if name is None:
            return bundle
        if name in self._bundles:
            raise ConfigError('bundle %r is already defined' % name)
        self._bundles[name] = bundle
        return None

    def all_binaries(self) -> tuple[Binary, ...]:
        """Every declared binary, by name."""
        return tuple(self._binaries[name] for name in sorted(self._binaries))

    def expand(self, bundle: str | Bundle) -> Targets:
        """A bundle's tests and compile targets, every bundle it includes
        expanded and every mixin applied.

        Raises:
            ConfigError: something it refers to is undefined, it includes a
                bundle cycle, or it includes a test twice.
        """
        compile_targets: dict[str, None] = {}
        tests: dict[str, dict] = {}
        self._expand(bundle, (), (), compile_targets, tests)
        return Targets(
            additional_compile_targets=list(compile_targets),
            gtest_tests=list(tests.values()),
        )

    def _expand(
        self,
        target: str | Bundle,
        outer_mixins: tuple[Mixin, ...],
        stack: tuple[str, ...],
        compile_targets: dict[str, None],
        tests: dict[str, dict],
    ) -> None:
        if isinstance(target, str) and target in self._tests:
            self._add_expanded_test(self._tests[target], outer_mixins, tests)
            return
        bundle = self._bundle(target)
        if isinstance(target, str):
            if target in stack:
                raise ConfigError(
                    'bundle cycle: %s -> %s' % (' -> '.join(stack), target)
                )
            stack += (target,)
        for name in bundle.additional_compile_targets:
            if name not in self._compile_targets:
                raise ConfigError('compile target %r is not defined' % name)
            compile_targets[name] = None
        mixins = tuple(self._mixin(m) for m in bundle.mixins) + outer_mixins
        for child in bundle.targets:
            self._expand(child, mixins, stack, compile_targets, tests)

    def _add_expanded_test(
        self,
        test: GTestTest,
        outer_mixins: tuple[Mixin, ...],
        tests: dict[str, dict],
    ) -> None:
        if test.name in tests:
            raise ConfigError('test %r is included twice' % test.name)
        if test.binary not in self._binaries:
            raise ConfigError(
                'test %r: binary %r is not defined' % (test.name, test.binary)
            )
        args = list(test.args)
        for mixin in tuple(self._mixin(m) for m in test.mixins) + outer_mixins:
            args.extend(mixin.args)
        spec: dict[str, Any] = {'name': test.name, 'test': test.binary}
        if args:
            spec['args'] = args
        tests[test.name] = spec

    def _bundle(self, target: str | Bundle) -> Bundle:
        if isinstance(target, Bundle):
            return target
        try:
            return self._bundles[target]
        except KeyError as e:
            raise ConfigError(
                '%r is neither a defined test nor a defined bundle' % target
            ) from e

    def _mixin(self, mixin: str | Mixin) -> Mixin:
        if isinstance(mixin, Mixin):
            return mixin
        try:
            return self._mixins[mixin]
        except KeyError as e:
            raise ConfigError('mixin %r is not defined' % mixin) from e


class Builder:
    """A registered builder: metadata plus what it builds and runs.

    Its GN args are not stored here — `builder()` registers them into
    `gn_args` under this builder's own name, so `gn_args.resolve(name)` is
    the one way to read them back, same as for any other config.
    """

    def __init__(
        self,
        *,
        name: str,
        builder_group: str | None,
        recipe: str | None,
        execution_timeout_mins: int | None,
        channel: str | None,
        notifies: Sequence[str],
        siso_remote_jobs: int | None,
        siso_reapi_priority: int | None,
        builder_spec: BuilderSpec,
        targets: Targets,  # pylint: disable=redefined-outer-name
    ) -> None:
        # The name it was registered under; also its `gn_args` node name.
        self.name = name

        # The builder group it belongs to, e.g. "brave.sanitisers".
        self.builder_group = builder_group

        # The recipe its builds run, e.g. "brave_builder".
        self.recipe = recipe

        # How long the pipeline may run before it's killed.
        self.execution_timeout_mins = execution_timeout_mins

        # The release channel this builder builds for, e.g. "nightly".
        self.channel = channel

        # Who to notify on a build result worth telling someone about.
        self.notifies = tuple(notifies)

        # How many jobs siso runs remotely at once, as upstream's
        # `siso_remote_jobs`; siso's own limit when None.
        self.siso_remote_jobs = _positive_int(
            name, 'siso_remote_jobs', siso_remote_jobs
        )

        # The remote execution priority of the builder's actions; the
        # service's default when None.
        self.siso_reapi_priority = _positive_int(
            name, 'siso_reapi_priority', siso_reapi_priority
        )

        # This builder's `builder_config.builder_spec()`.
        self.builder_spec = builder_spec

        # What this builder compiles and tests: its bundle, expanded.
        self.targets = targets


def _positive_int(builder_name: str, field: str, value: Any) -> int | None:
    if value is not None and (
        not isinstance(value, int) or isinstance(value, bool) or value < 1
    ):
        raise ConfigError(
            'builder %r: %s must be a positive integer, got %r'
            % (builder_name, field, value)
        )
    return value


class BuildersRegistry:
    """A registry of builders, and the `targets` constructor used to
    describe them.

    Spec files call `builder()` to declare a builder; `defaults.set()` sets
    module-level fallbacks for the fields a group of builders share, the way
    a `chromium.<group>.star` file's `ci.defaults.set(...)` does upstream.
    """

    def __init__(
        self,
        gn_args_registry: GnArgsRegistry,
        targets_registry: TargetsRegistry,
    ) -> None:
        # Where a builder's GN args get registered (see `_register_gn_args`).
        # Explicit rather than reaching for the module-level `gn_args` below,
        # so a registry under test isn't wired to shared global state.
        self._gn_args = gn_args_registry

        # Where the bundle a builder names is expanded from.
        self._targets = targets_registry

        # Module-level fallbacks for builder(), settable via `defaults.set()`.
        self.defaults = Defaults(
            builder_group=None,
            recipe=None,
            execution_timeout_mins=None,
            channel=None,
            notifies=(),
            siso_remote_jobs=None,
            siso_reapi_priority=None,
        )

        # Name -> registered builder.
        self._builders: dict[str, Builder] = {}

    def _register_gn_args(self, builder_name: str, value: Any) -> None:
        """Registers `value` as the named `gn_args` config for a builder.

        a builder's GN args become a node in the same include graph as any other
        config, named after the builder itself. Takes either a config name to
        include as-is, or the anonymous struct `gn_args.config()` returns when
        called without `name`.
        """
        if isinstance(value, str):
            self._gn_args.config(name=builder_name, configs=[value])
        elif isinstance(value, AnonymousGnConfig):
            self._gn_args.config(
                name=builder_name,
                configs=list(value.configs),
                args=dict(value.gn_args),
                args_file=value.args_file,
                secrets=dict(value.secrets),
            )
        else:
            raise ConfigError(
                'builder %r: gn_args must be a config name or an anonymous '
                'gn_args.config(), got %r' % (builder_name, value)
            )

    def builder(
        self,
        *,
        name: str,
        builder_spec: BuilderSpec,
        gn_args: Any,  # pylint: disable=redefined-outer-name
        targets: str | Bundle,  # pylint: disable=redefined-outer-name
        builder_group: str | None = _UNSET,
        recipe: str | None = _UNSET,
        execution_timeout_mins: int | None = _UNSET,
        channel: str | None = _UNSET,
        notifies: Sequence[str] = _UNSET,
        siso_remote_jobs: int | None = _UNSET,
        siso_reapi_priority: int | None = _UNSET,
    ) -> Builder:
        """Defines a builder.

        Args:
            name: The builder's name. Also what its GN args are registered
                under (see `_register_gn_args`).
            builder_spec: This builder's `builder_config.builder_spec()`.
            gn_args: The config name, or `gn_args.config()` called without
                `name`, describing this builder's GN args.
            targets: The bundle saying what this builder compiles and tests,
                by name or as `targets.bundle()` returns it without `name`.
            builder_group, recipe, execution_timeout_mins, channel, notifies,
            siso_remote_jobs, siso_reapi_priority: Fall back to `defaults`
                when omitted.

        Returns:
            The registered `Builder`.
        """
        if name in self._builders:
            raise ConfigError('builder %r is already defined' % name)
        if not isinstance(builder_spec, BuilderSpec):
            raise ConfigError(
                'builder %r: builder_spec must be a '
                'builder_config.builder_spec()' % name
            )

        self._register_gn_args(name, gn_args)
        try:
            expanded_targets = self._targets.expand(targets)
        except ConfigError as e:
            raise ConfigError('builder %r: %s' % (name, e)) from e

        builder = Builder(
            name=name,
            builder_group=self.defaults.get('builder_group', builder_group),
            recipe=self.defaults.get('recipe', recipe),
            execution_timeout_mins=self.defaults.get(
                'execution_timeout_mins', execution_timeout_mins
            ),
            channel=self.defaults.get('channel', channel),
            notifies=self.defaults.get('notifies', notifies),
            siso_remote_jobs=self.defaults.get(
                'siso_remote_jobs', siso_remote_jobs
            ),
            siso_reapi_priority=self.defaults.get(
                'siso_reapi_priority', siso_reapi_priority
            ),
            builder_spec=builder_spec,
            targets=expanded_targets,
        )
        self._builders[name] = builder
        return builder

    def get(self, name: str) -> Builder:
        try:
            return self._builders[name]
        except KeyError as e:
            raise ConfigError('builder %r is not defined' % name) from e

    def all(self) -> tuple[Builder, ...]:
        """Every registered builder, in definition order."""
        return tuple(self._builders.values())


# What spec files import: `from lib.config import builders`.
targets = TargetsRegistry()
builders = BuildersRegistry(gn_args, targets)
