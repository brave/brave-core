# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`depot_tools` module: ensure a usable depot_tools install is on PATH."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import (
    env,
    git,
    path,
    platform,
    step,
)

from .api import DepotToolsApi as API


@dataclass
class DEPS(RecipeScriptApi):
    env: env.API
    git: git.API
    path: path.API
    platform: platform.API
    step: step.API
