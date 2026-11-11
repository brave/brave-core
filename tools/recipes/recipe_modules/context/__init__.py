# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`context` module: scoped ambient step settings (cwd + environment)."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi

from .api import ContextApi as API


@dataclass
class DEPS(RecipeScriptApi):
    pass
