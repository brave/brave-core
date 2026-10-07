# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`chromium_checkout` module: clone / sync / validate a Chromium src/ tree."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import (
    context,
    depot_tools,
    env,
    git,
    git_cache,
    json,
    path,
    platform,
    raw_io,
    step,
)

from .api import ChromiumCheckoutApi as API
from .test_api import ChromiumCheckoutTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    context: context.API
    depot_tools: depot_tools.API
    env: env.API
    git: git.API
    git_cache: git_cache.API
    json: json.API
    path: path.API
    platform: platform.API
    raw_io: raw_io.API
    step: step.API
