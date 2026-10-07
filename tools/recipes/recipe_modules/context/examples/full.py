# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `context` module's scoping."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    context,
    path,
    step,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    context: context.API
    path: path.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    pass


def RunSteps(api: DEPS):
    node_bin = api.path.workspace / 'node' / 'bin'

    # Prepend a dir to PATH and override a var for steps in this scope.
    with api.context(env_prefixes={'PATH': [node_bin]}, env={'CI': 'brave'}):
        api.step('inside context', ['node', '--version'])

        # Nested contexts compose: a second PATH prefix stacks in front, and
        # cwd applies only within the inner block.
        with api.context(
            env_prefixes={'PATH': ['/opt/extra']},
            env_suffixes={'LD_LIBRARY_PATH': ['/opt/lib']},
            cwd=api.path.out,
        ):
            api.step('nested context', ['node', 'build.js'])

    # Back outside every `with`: the ambient environment is restored.
    api.step('outside context', ['node', '--version'])


def GenTests(api: TEST_DEPS):
    yield api.test(
        'basic',
        # The PATH prefix and env override are recorded on the scoped step.
        api.post_process(post_process.MustRun, 'inside context'),
        api.post_process(post_process.MustRun, 'nested context'),
        api.post_process(post_process.MustRun, 'outside context'),
        api.post_process(post_process.StatusSuccess),
    )
