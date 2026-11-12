# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `brave_tests_builder_config` module API."""

from __future__ import annotations

from PB.recipe_modules.brave.brave_tests_builder_config.properties import (
    InputProperties,
)
from recipe_api import RecipeApi

from . import proto
from .builder_config import BuilderConfig, BuilderId


class BraveTestsBuilderConfigApi(RecipeApi):
    """Looks up the builder a build runs as, from the build's properties."""

    def __init__(self, properties: InputProperties) -> None:
        super().__init__()
        self._properties = properties

    def lookup_builder(self) -> tuple[BuilderId, BuilderConfig] | None:
        """Lookup a builder, getting the matching bot config.

        Unlike upstream, there is no database of builders in the recipes to
        fall back on: a build without the module's properties has no builder.

        Returns:
            None for a build without a builder, otherwise a 2-tuple:
              * The BuilderId of the builder the BuilderConfig is for.
              * The BuilderConfig for the builder.

        Raises:
            BuilderConfigException: The properties do not describe a valid
                builder config.
        """
        if not self._properties.HasField('builder_config'):
            return None
        builder_config = proto.convert_builder_config(
            self._properties.builder_config
        )
        return builder_config.builder_ids[0], builder_config
