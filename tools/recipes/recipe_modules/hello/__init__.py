# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`hello` module: a worked example of the config system (see README.md).

Also demonstrates per-module `PROPERTIES`: the `$hello` block of the input
property JSON is decoded into `InputProperties` and injected into
`HelloApi.__init__`, which feeds it to the config system as the `TARGET`
default.
"""

from dataclasses import dataclass

from PB.recipe_modules.brave.hello.properties import InputProperties
from recipe_api import RecipeScriptApi
from recipe_modules import (
    path,
    step,
)

from .api import HelloApi as API


@dataclass
class DEPS(RecipeScriptApi):
    path: path.API
    step: step.API


PROPERTIES = InputProperties
