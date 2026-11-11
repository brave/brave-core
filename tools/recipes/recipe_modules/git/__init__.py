# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`git` module: low-level git repository maintenance shared across modules."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import step

from .api import GitApi as API


@dataclass
class DEPS(RecipeScriptApi):
    step: step.API
