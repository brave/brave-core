# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Converts the module's properties into a `BuilderConfig`, as upstream's
`chromium_tests_builder_config/proto.py` does."""

from __future__ import annotations

import types

from PB.recipe_modules.brave.brave_tests_builder_config.properties import (
    BuilderConfig as BuilderConfigProto,
    BuilderSpec as BuilderSpecProto,
)

from .builder_config import BuilderConfig, BuilderId, BuilderSpec


def _convert_builder_spec(obj: BuilderSpecProto) -> BuilderSpec:
    chromium_config = obj.chromium_config
    chromium_config_kwargs = {}
    for a in (
        'build_config',
        'host_platform',
        'target_arch',
        'target_bits',
        'target_platform',
    ):
        if chromium_config.HasField(a):
            chromium_config_kwargs[a.upper()] = getattr(chromium_config, a)
    return BuilderSpec(
        chromium_config_kwargs=types.MappingProxyType(chromium_config_kwargs),
    )


def convert_builder_config(obj: BuilderConfigProto) -> BuilderConfig:
    # The ID in the protos is the builder's name alone, whereas the one in the
    # recipes is (group, builder), as upstream maps buildbucket's.
    builder_id_by_name = {
        entry.builder_id.builder: BuilderId.create_for_group(
            entry.builder_spec.builder_group, entry.builder_id.builder
        )
        for entry in obj.builder_db.entries
    }
    return BuilderConfig.create(
        builder_db={
            builder_id_by_name[entry.builder_id.builder]: _convert_builder_spec(
                entry.builder_spec
            )
            for entry in obj.builder_db.entries
        },
        builder_ids=[
            builder_id_by_name.get(b.builder) or BuilderId('', b.builder)
            for b in obj.builder_ids
        ],
    )
