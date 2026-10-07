# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`git_cache` module."""

from dataclasses import dataclass

from PB.recipe_modules.brave.git_cache.properties import EnvProperties
from recipe_api import RecipeScriptApi
from recipe_modules import (
    env,
    git,
    path,
    raw_io,
)

from .api import GitCacheApi as API


@dataclass
class DEPS(RecipeScriptApi):
    env: env.API
    git: git.API
    path: path.API
    raw_io: raw_io.API


ENV_PROPERTIES = EnvProperties
