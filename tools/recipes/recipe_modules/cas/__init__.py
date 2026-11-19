# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`cas` module: trees of files in a remote execution service's CAS."""

from dataclasses import dataclass

from PB.recipe_modules.brave.cas.properties import (
    EnvProperties,
    InputProperties,
)
from recipe_api import RecipeScriptApi
from recipe_modules import cipd, depot_tools, json, path, step

from .api import CasApi as API
from .test_api import CasTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    cipd: cipd.API
    depot_tools: depot_tools.API
    json: json.API
    path: path.API
    step: step.API


PROPERTIES = InputProperties
ENV_PROPERTIES = EnvProperties
