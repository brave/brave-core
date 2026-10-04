# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`brave_core_checkout` module: check out brave-core, in full or sparsely."""

from dataclasses import dataclass

from PB.recipe_modules.brave.brave_core_checkout.properties import (
    InputProperties,
)
from recipe_api import RecipeScriptApi
from recipe_modules import (
    chromium_checkout,
    context,
    depot_tools,
    env,
    file,
    git,
    git_cache,
    path,
    platform,
    raw_io,
    step,
)

from .api import BraveCoreCheckoutApi as API
from .test_api import BraveCoreCheckoutTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    chromium_checkout: chromium_checkout.API
    context: context.API
    depot_tools: depot_tools.API
    env: env.API
    file: file.API
    git: git.API
    git_cache: git_cache.API
    path: path.API
    platform: platform.API
    raw_io: raw_io.API
    step: step.API


PROPERTIES = InputProperties
