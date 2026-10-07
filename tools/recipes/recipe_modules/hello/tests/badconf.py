# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Edge case: a config item raising `BadConf` surfaces as an EXCEPTION."""

from __future__ import annotations

from dataclasses import dataclass

from post_process import DropExpectation, StatusException
from recipe_api import RecipeScriptApi
from recipe_modules import hello
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    hello: hello.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    pass


def RunSteps(api: DEPS):
    # super_tool only accepts TARGET 'Charlie'; anything else raises BadConf.
    api.hello.set_config('super_tool', TARGET='Not Charlie')


def GenTests(api: TEST_DEPS):
    yield api.test(
        'badconf',
        api.post_process(StatusException),
        api.post_process(DropExpectation),
        status='EXCEPTION',
    )
