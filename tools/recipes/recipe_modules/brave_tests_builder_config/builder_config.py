# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
#
# For some of the code lifted below.
#
# Copyright 2020 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""`BuilderSpec`, `BuilderId` and `BuilderConfig`, trimmed from upstream's
`chromium_tests_builder_config` module to the fields our builders set."""

from __future__ import annotations

from collections.abc import Mapping, Sequence
import dataclasses
import types
from typing import Any


class BuilderConfigException(Exception):
    """Exception indicating an attempt to create an invalid BuilderConfig."""


@dataclasses.dataclass(frozen=True)
class BuilderSpec:
    """An immutable specification for a bot.

    Upstream also names the config items to set on the `chromium` and
    `gclient` modules (`chromium_config`, `gclient_config` and their
    `*_apply_config`); ours only carries the keyword arguments, and the
    recipe picks the config item.
    """

    # The keyword arguments used when setting the config for the module that
    # builds (`brave_core_checkout`)
    chromium_config_kwargs: Mapping[str, Any] = dataclasses.field(
        default_factory=lambda: types.MappingProxyType({})
    )


@dataclasses.dataclass(frozen=True)
class BuilderId:
    """The identity of a builder: its group and name."""

    group: str
    builder: str

    @classmethod
    def create_for_group(cls, group: str, builder: str) -> BuilderId:
        return cls(group=group, builder=builder)

    def __str__(self) -> str:
        return f'{self.group}:{self.builder}'


class _BuilderSpecProperty:
    """A non-data descriptor that delegates attributes to BuilderSpecs.

    The value for the attribute will be read from each of the BuilderSpecs
    that are wrapped by the BuilderConfig instance and an exception will be
    raised if there is more than one value.
    """

    def __init__(self, attr: str) -> None:
        self._attr = attr

    def __get__(self, obj: BuilderConfig | None, objtype=None) -> Any:
        del objtype
        if obj is None:
            return self  # pragma: no cover
        per_builder_values = {
            builder_id: getattr(obj.builder_db[builder_id], self._attr)
            for builder_id in obj.builder_ids
        }
        # Compared by equality rather than hashed, unlike upstream: our specs
        # hold mappings.
        values = []
        for value in per_builder_values.values():
            if value not in values:
                values.append(value)
        if len(values) != 1:
            message = [f'Inconsistent value for {self._attr!r}:']
            message.extend(f'{k}: {v!r}' for k, v in per_builder_values.items())
            raise ValueError('\n  '.join(message))
        return values[0]


@dataclasses.dataclass(frozen=True)
class BuilderConfig:
    """ "Static" configuration for a builder.

    BuilderConfig wraps multiple builder specs and provides the means for
    getting values in a manner that ensures they are compatible between all of
    the wrapped specs: reading an attribute that is defined on the specs
    returns its value if it is the same for every builder in `builder_ids`, and
    raises otherwise.
    """

    # The database of builder specs, by builder ID
    builder_db: Mapping[BuilderId, BuilderSpec]
    # IDs of builders that configuration will be applied for
    builder_ids: Sequence[BuilderId]

    @classmethod
    def create(
        cls,
        builder_db: Mapping[BuilderId, BuilderSpec],
        builder_ids: Sequence[BuilderId],
    ) -> BuilderConfig:
        """Create a BuilderConfig, checking that every one of `builder_ids` is
        in `builder_db`.

        Raises:
            BuilderConfigException: `builder_ids` is empty, or names a builder
                that `builder_db` has no spec for.
        """
        if not builder_ids:
            raise BuilderConfigException('builder_ids must not be empty')
        missing = [str(b) for b in builder_ids if b not in builder_db]
        if missing:
            raise BuilderConfigException(
                'No spec for builders: ' + ', '.join(missing)
            )
        return cls(
            builder_db=types.MappingProxyType(dict(builder_db)),
            builder_ids=tuple(builder_ids),
        )

    chromium_config_kwargs = _BuilderSpecProperty('chromium_config_kwargs')
