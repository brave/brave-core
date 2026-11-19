# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`flake_analysis` module: tell a build's flaky and upstream failures from
its own."""

from dataclasses import dataclass

from PB.recipe_modules.brave.flake_analysis.properties import InputProperties
from recipe_api import RecipeScriptApi
from recipe_modules import (
    depot_tools,
    file,
    isolate,
    json,
    path,
    platform,
    step,
)

from .api import Analysis, FlakeAnalysisApi as API, Rerunnable, Verdict
from .test_api import FlakeAnalysisTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    depot_tools: depot_tools.API
    file: file.API
    isolate: isolate.API
    json: json.API
    path: path.API
    platform: platform.API
    step: step.API


PROPERTIES = InputProperties
