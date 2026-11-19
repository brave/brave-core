# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`cipd` module: install pinned CIPD packages, and the tools in them."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import depot_tools, path, platform, raw_io, step

from .api import CipdApi as API


@dataclass
class DEPS(RecipeScriptApi):
    depot_tools: depot_tools.API
    path: path.API
    platform: platform.API
    raw_io: raw_io.API
    step: step.API
