# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `brave_tests_builder_config`: run a test as a builder."""

from __future__ import annotations

from PB.recipe_modules.brave.brave_tests_builder_config.properties import (
    BuilderConfig,
    BuilderDatabase,
    BuilderID,
    BuilderSpec,
    InputProperties,
)
from recipe_test_api import RecipeTestApi, TestData


class BraveTestsBuilderConfigTestApi(RecipeTestApi):
    def properties(self, properties: InputProperties) -> TestData:
        """Set the module's properties."""
        assert isinstance(properties, InputProperties)
        return super().properties(**{'$brave_tests_builder_config': properties})

    def ci_build(
        self,
        *,
        builder: str = 'linux-x64-asan-brave',
        builder_group: str = 'brave.sanitisers',
        **chromium_config,
    ) -> TestData:
        """Run the test as a CI builder, as its `properties.json` would.

        Args:
            builder: The builder's name.
            builder_group: The builder's group.
            chromium_config: The fields of its spec's `chromium_config`, by
                default a Linux x64 Release build.
        """
        chromium_config = {
            'build_config': 'Release',
            'host_platform': 'linux',
            'target_arch': 'intel',
            'target_bits': 64,
            'target_platform': 'linux',
            **chromium_config,
        }
        builder_id = BuilderID(builder=builder)
        return self.properties(
            InputProperties(
                builder_config=BuilderConfig(
                    builder_db=BuilderDatabase(
                        entries=[
                            BuilderDatabase.Entry(
                                builder_id=builder_id,
                                builder_spec=BuilderSpec(
                                    builder_group=builder_group,
                                    chromium_config=BuilderSpec.ChromiumConfig(
                                        **{
                                            k: v
                                            for k, v in chromium_config.items()
                                            if v is not None
                                        }
                                    ),
                                ),
                            )
                        ]
                    ),
                    builder_ids=[builder_id],
                )
            )
        )
