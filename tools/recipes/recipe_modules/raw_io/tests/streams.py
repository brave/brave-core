# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `raw_io`'s stream helpers seeding a retcode alongside output.

A step's own default simulated data can say the step failed, and still say what
it wrote on its way out -- which is how a recipe that inspects a failing
command's output gets tested.
"""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    raw_io,
    step,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    raw_io: raw_io.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    pass


def RunSteps(api: DEPS):
    result = api.step(
        'failing command',
        ['do-thing'],
        check=False,
        stdout=api.raw_io.output(),
        step_test_data=lambda: api.raw_io.test_api.stream_output(
            'nope\n', retcode=3
        ),
    )
    assert result.retcode == 3, result.retcode
    assert result.stdout == b'nope\n', result.stdout


def GenTests(api: TEST_DEPS):
    yield api.test(
        'basic',
        api.post_process(post_process.StepFailure, 'failing command'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
