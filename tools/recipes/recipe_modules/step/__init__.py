# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Core `step` module: run a subprocess as a named build step."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import (
    context,
    platform,
)

from .api import StepApi as API
from .test_api import StepTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    context: context.API
    platform: platform.API
