# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `brave_tests_builder_config` module."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from PB.recipe_modules.brave.brave_tests_builder_config.properties import (
    BuilderConfig,
    BuilderDatabase,
    BuilderID,
    BuilderSpec,
    InputProperties,
)
from recipe_api import RecipeScriptApi
from recipe_modules import brave_tests_builder_config, step
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    brave_tests_builder_config: brave_tests_builder_config.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_tests_builder_config: brave_tests_builder_config.TEST_API


def RunSteps(api: DEPS):
    builder = api.brave_tests_builder_config.lookup_builder()
    if builder is None:
        api.step('no builder', [])
        return
    builder_id, builder_config = builder
    kwargs = ' '.join(
        f'{k}={v}'
        for k, v in sorted(builder_config.chromium_config_kwargs.items())
    )
    api.step(f'builder {builder_id}', ['echo', kwargs])


def _entry(builder, build_config):
    return BuilderDatabase.Entry(
        builder_id=BuilderID(builder=builder),
        builder_spec=BuilderSpec(
            builder_group='brave.test',
            chromium_config=BuilderSpec.ChromiumConfig(
                build_config=build_config, target_platform='linux'
            ),
        ),
    )


def GenTests(api: TEST_DEPS):
    yield api.test(
        'no builder',
        api.post_process(post_process.MustRun, 'no builder'),
        api.post_process(post_process.DropExpectation),
    )
    yield api.test(
        'ci builder',
        api.brave_tests_builder_config.ci_build(),
        api.post_process(
            post_process.StepCommandContains,
            'builder brave.sanitisers:linux-x64-asan-brave',
            [
                'BUILD_CONFIG=Release HOST_PLATFORM=linux TARGET_ARCH=intel '
                'TARGET_BITS=64 TARGET_PLATFORM=linux'
            ],
        ),
        api.post_process(post_process.DropExpectation),
    )
    yield api.test(
        'unset fields are left out',
        api.brave_tests_builder_config.ci_build(
            build_config=None, host_platform=None, target_arch=None
        ),
        api.post_process(
            post_process.StepCommandContains,
            'builder brave.sanitisers:linux-x64-asan-brave',
            ['TARGET_BITS=64 TARGET_PLATFORM=linux'],
        ),
        api.post_process(post_process.DropExpectation),
    )
    # Specs that disagree cannot be read from.
    yield api.test(
        'inconsistent builders',
        api.brave_tests_builder_config.properties(
            InputProperties(
                builder_config=BuilderConfig(
                    builder_db=BuilderDatabase(
                        entries=[
                            _entry('a', 'Release'),
                            _entry('b', 'Debug'),
                        ]
                    ),
                    builder_ids=[
                        BuilderID(builder='a'),
                        BuilderID(builder='b'),
                    ],
                )
            )
        ),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    yield api.test(
        'builder without a spec',
        api.brave_tests_builder_config.properties(
            InputProperties(
                builder_config=BuilderConfig(
                    builder_db=BuilderDatabase(
                        entries=[_entry('a', 'Release')]
                    ),
                    builder_ids=[BuilderID(builder='b')],
                )
            )
        ),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    yield api.test(
        'no builder ids',
        api.brave_tests_builder_config.properties(
            InputProperties(
                builder_config=BuilderConfig(
                    builder_db=BuilderDatabase(entries=[_entry('a', 'Release')])
                )
            )
        ),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
