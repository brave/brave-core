# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`brave_tests` module: run a builder's tests."""

from dataclasses import dataclass

from PB.recipe_modules.brave.brave_tests.properties import InputProperties
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    depot_tools,
    file,
    json,
    path,
    step,
)

from .api import BraveTestsApi as API
from .gtest import GTest
from .results import SuiteResult, TestRun
from .test_api import BraveTestsTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    brave_core_checkout: brave_core_checkout.API
    depot_tools: depot_tools.API
    file: file.API
    json: json.API
    path: path.API
    step: step.API


PROPERTIES = InputProperties
