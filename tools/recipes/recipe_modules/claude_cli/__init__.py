# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`claude_cli` module: run Claude Code unattended, as a step."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import (
    context,
    depot_tools,
    step,
)

from .api import ClaudeCliApi as API
from .test_api import ClaudeCliTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    context: context.API
    depot_tools: depot_tools.API
    step: step.API
