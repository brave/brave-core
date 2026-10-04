# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`file` module: basic filesystem operations (read, write, copy, move,
remove, ...) as recipe steps.

File content travels in and out of the step through placeholders (see
`api.py`).
"""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import (
    depot_tools,
    json,
    path,
    proto,
    raw_io,
    step,
)

from .api import FileApi as API
from .test_api import FileTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    depot_tools: depot_tools.API
    json: json.API
    path: path.API
    proto: proto.API
    raw_io: raw_io.API
    step: step.API
