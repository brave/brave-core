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
        self._hash = functools.reduce(operator.xor,
                                      (hash(i)
                                       for i in enumerate(self._d.items())), 0)

    def __iter__(self) -> Iterator[K]:
        return iter(self._d)

    def __len__(self) -> int:
        return len(self._d)

    def __getitem__(self, key: K) -> V:
        return self._d[key]

    def __hash__(self) -> int:
        return self._hash

    def __repr__(self) -> str:
        return 'FrozenDict(%r)' % (list(self._d.items()), )


class ConfigError(Exception):
    """Raised for any gn_args or builder configuration violation."""


class AnonymousGnConfig:
    """The inline, unnamed form of a GN config.

    Returned by `gn_args.config()` when called without `name`, for embedding
    directly in a builder definition rather than registering it under a name
    for reuse.
    """

    def __init__(self, *, args: Mapping[str, Any], configs: Sequence[str],
                 args_file: str, secrets: Mapping[str, str]) -> None:
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
        return ('AnonymousGnConfig(gn_args=%r, configs=%r, args_file=%r, '
                'secrets=%r)' %
                (self.gn_args, self.configs, self.args_file, self.secrets))


class _GnConfigNode:
    """One registered, named node in the GN-args include graph."""

    def __init__(self, name: str, *, args: Mapping[str, Any],
                 configs: Sequence[str], args_file: str,
                 secrets: Mapping[str, str]) -> None:
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


def _merge_into(dst: dict, *, args_file: str, args: Mapping[str, Any],
                secrets: Mapping[str, str], config_name: str) -> None:
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
                'single args_file (already have %r, got %r)' %
                (config_name, dst['args_file'], args_file))
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
            secrets: Mapping[str, str] | None = None
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
                raise ConfigError('gn_args config %r is already defined' %
                                  name)
            self._nodes[name] = _GnConfigNode(name,
                                              args=args,
                                              configs=configs,
                                              args_file=args_file,
                                              secrets=secrets)
            return None

        return AnonymousGnConfig(args=args,
                                 configs=configs,
                                 args_file=args_file,
                                 secrets=secrets)

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

    def _resolve_node(self, node: _GnConfigNode, stack: tuple[str,
                                                              ...]) -> dict:
        if node.name in self._resolved:
            return self._resolved[node.name]
        if node.name in stack:
            raise ConfigError('gn_args config cycle: %s -> %s' %
                              (' -> '.join(stack), node.name))
        stack = stack + (node.name, )

        merged = {'args_file': '', 'gn_args': {}, 'secrets': {}}

        # Merge included configs first, in the order given (DEFINITION_ORDER
        # upstream): a later entry's keys overwrite an earlier entry's, since
        # each merge is a dict.update() into the same accumulator.
        for child_name in node.configs:
            try:
                child = self._nodes[child_name]
            except KeyError as e:
                raise ConfigError(
                    'gn_args config %r includes undefined config %r' %
                    (node.name, child_name)) from e
            child_resolved = self._resolve_node(child, stack)
            _merge_into(merged,
                        args_file=child_resolved['args_file'],
                        args=child_resolved['gn_args'],
                        secrets=child_resolved['secrets'],
                        config_name=node.name)

        # Merge the node's own values last, so they win over anything included.
        _merge_into(merged,
                    args_file=node.args_file,
                    args=node.gn_args,
                    secrets=node.secrets,
                    config_name=node.name)

        self._resolved[node.name] = merged
        return merged


# What spec files import: `from lib.config import gn_args`.
gn_args = GnArgsRegistry()

# Sentinel distinguishing "caller didn't pass this" from an explicit falsy
# value (0, '', [], None), mirroring upstream's `args.DEFAULT`.
_UNSET = object()


class Defaults:
    """A settable group of module-level defaults for `builder()`.
    """

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


class SyncConfig:
    """What a builder needs to reach a synced tree: target platform plus
    whatever gclient overrides `init`/`sync` should apply."""

    def __init__(self, *, target_os: str, target_cpu: str,
                 **gclient_overrides: Any) -> None:
        # GN's target_os, and what `init`/`sync` sync a checkout for.
        self.target_os = target_os

        # GN's target_cpu.
        self.target_cpu = target_cpu

        # Custom vars, deps or revision pins for gclient, by name. Schema
        # not yet settled (A1).
        self.gclient_overrides = gclient_overrides


class Targets:
    """What a builder compiles and runs."""

    def __init__(
            self,
            *,
            compile: Sequence[str],  # pylint: disable=redefined-builtin
            tests: Sequence[str]) -> None:
        # GN target labels to compile, e.g. "brave:all".
        self.compile = tuple(compile)

        # Test target names to run.
        self.tests = tuple(tests)


class Builder:
    """A registered builder: metadata plus what it builds and runs.

    Its GN args are not stored here — `builder()` registers them into
    `gn_args` under this builder's own name, so `gn_args.resolve(name)` is
    the one way to read them back, same as for any other config.
    """

    def __init__(self, *, name: str, builder_group: str | None,
                 execution_timeout_mins: int | None, channel: str | None,
                 notifies: Sequence[str], sync_config: SyncConfig,
                 targets: Targets) -> None:
        # The name it was registered under; also its `gn_args` node name.
        self.name = name

        # The builder group it belongs to, e.g. "brave.sanitisers".
        self.builder_group = builder_group

        # How long the pipeline may run before it's killed.
        self.execution_timeout_mins = execution_timeout_mins

        # The release channel this builder builds for, e.g. "nightly".
        self.channel = channel

        # Who to notify on a build result worth telling someone about.
        self.notifies = tuple(notifies)

        # This builder's `sync_config()`.
        self.sync_config = sync_config

        # This builder's `targets()`.
        self.targets = targets


class BuildersRegistry:
    """A registry of builders, and the `sync_config`/`targets` constructors
    used to describe them.

    Spec files call `builder()` to declare a builder; `defaults.set()` sets
    module-level fallbacks for the fields a group of builders share, the way
    a `chromium.<group>.star` file's `ci.defaults.set(...)` does upstream.
    """

    def __init__(self, gn_args_registry: GnArgsRegistry) -> None:
        # Where a builder's GN args get registered (see `_register_gn_args`).
        # Explicit rather than reaching for the module-level `gn_args` below,
        # so a registry under test isn't wired to shared global state.
        self._gn_args = gn_args_registry

        # Module-level fallbacks for builder(), settable via `defaults.set()`.
        self.defaults = Defaults(
            builder_group=None,
            execution_timeout_mins=None,
            channel=None,
            notifies=(),
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
            self._gn_args.config(name=builder_name,
                                 configs=list(value.configs),
                                 args=dict(value.gn_args),
                                 args_file=value.args_file,
                                 secrets=dict(value.secrets))
        else:
            raise ConfigError(
                'builder %r: gn_args must be a config name or an anonymous '
                'gn_args.config(), got %r' % (builder_name, value))

    def sync_config(self, *, target_os: str, target_cpu: str,
                    **gclient_overrides: Any) -> SyncConfig:
        return SyncConfig(target_os=target_os,
                          target_cpu=target_cpu,
                          **gclient_overrides)

    def targets(
            self,
            *,
            compile: Sequence[str],  # pylint: disable=redefined-builtin
            tests: Sequence[str]) -> Targets:
        return Targets(compile=compile, tests=tests)

    def builder(
            self,
            *,
            name: str,
            sync_config: SyncConfig,
            gn_args: Any,  # pylint: disable=redefined-outer-name
            targets: Targets,
            builder_group: str | None = _UNSET,
            execution_timeout_mins: int | None = _UNSET,
            channel: str | None = _UNSET,
            notifies: Sequence[str] = _UNSET) -> Builder:
        """Defines a builder.

        Args:
            name: The builder's name. Also what its GN args are registered
                under (see `_register_gn_args`).
            sync_config: This builder's `sync_config()`.
            gn_args: The config name, or `gn_args.config()` called without
                `name`, describing this builder's GN args.
            targets: This builder's `targets()`.
            builder_group, execution_timeout_mins, channel, notifies: Fall
                back to `defaults` when omitted.

        Returns:
            The registered `Builder`.
        """
        if name in self._builders:
            raise ConfigError('builder %r is already defined' % name)

        self._register_gn_args(name, gn_args)

        builder = Builder(
            name=name,
            builder_group=self.defaults.get('builder_group', builder_group),
            execution_timeout_mins=self.defaults.get('execution_timeout_mins',
                                                     execution_timeout_mins),
            channel=self.defaults.get('channel', channel),
            notifies=self.defaults.get('notifies', notifies),
            sync_config=sync_config,
            targets=targets,
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
builders = BuildersRegistry(gn_args)
