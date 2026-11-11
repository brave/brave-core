# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`platform` module: mockable host OS identification."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi

from .api import PlatformApi as API
from .test_api import PlatformTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    pass
