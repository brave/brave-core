# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`isolate` module: archive built test suites with what they need to run."""

from dataclasses import dataclass

from PB.recipe_modules.brave.isolate.properties import InputProperties
from recipe_api import RecipeScriptApi
from recipe_modules import cas, context, depot_tools, file, path, step

from .api import Isolate, IsolateApi as API
from .test_api import IsolateTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    cas: cas.API
    context: context.API
    depot_tools: depot_tools.API
    file: file.API
    path: path.API
    step: step.API


PROPERTIES = InputProperties
