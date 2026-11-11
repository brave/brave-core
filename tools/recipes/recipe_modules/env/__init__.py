# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`env` module: mockable access to environment variables and PATH lookup."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import path

from .api import EnvApi as API
from .test_api import EnvTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    path: path.API
