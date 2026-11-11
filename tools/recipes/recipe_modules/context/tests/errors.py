# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the `context` module's rejection of bad inputs.

A seeded `MODE` env var selects which input to feed `api.context`; the invalid
ones raise on `__enter__`, the valid one runs the block.
"""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    context,
    env,
    step,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    context: context.API
    env: env.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    env: env.TEST_API


def RunSteps(api: DEPS):
    mode = api.env.get('MODE')
    if mode == 'bad_cwd':
        # cwd must be a str/Path; an int is rejected by _check_type.
        cm = api.context(cwd=123)
    elif mode == 'bad_env':
        # A bare `%s` is not a valid `%(VAR)s` env template and is rejected.
        cm = api.context(env={'BAD': '%s'})
    else:
        cm = api.context(env={'OK': 'value'})
    with cm:
        api.step('inside context', ['echo', 'ok'])


def GenTests(api: TEST_DEPS):
    yield api.test(
        'bad cwd type',
        api.env.set('MODE', 'bad_cwd'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    yield api.test(
        'bad env format',
        api.env.set('MODE', 'bad_env'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    yield api.test(
        'valid context',
        api.env.set('MODE', 'ok'),
        api.post_process(post_process.MustRun, 'inside context'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
