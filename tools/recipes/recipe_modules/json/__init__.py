# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`json` module: produce and consume JSON to and from steps."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import raw_io

from .api import JsonApi as API
from .test_api import JsonTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    raw_io: raw_io.API
