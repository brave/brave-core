# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`sanitizer_findings` module: the bugs a build's sanitisers found."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import step

from .api import SanitizerFindingsApi as API
from .findings import Finding, Findings


@dataclass
class DEPS(RecipeScriptApi):
    step: step.API


__all__ = ['API', 'DEPS', 'Finding', 'Findings']
