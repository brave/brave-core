# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`osx_sdk` module: install/select the Xcode pinned by a Chromium checkout's
macOS SDK.
"""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    depot_tools,
    json,
    path,
    platform,
    step,
)

from .api import OSXSDKApi as API
from .test_api import OSXSDKTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    brave_core_checkout: brave_core_checkout.API
    depot_tools: depot_tools.API
    json: json.API
    path: path.API
    platform: platform.API
    step: step.API
