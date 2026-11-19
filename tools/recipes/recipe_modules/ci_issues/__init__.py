# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`ci_issues` module: what a build found, filed on GitHub."""

from dataclasses import dataclass

from PB.recipe_modules.brave.ci_issues.properties import InputProperties
from recipe_api import RecipeScriptApi
from recipe_modules import gh, step

from .api import CiIssuesApi as API
from .api import Filed, Filing
from .policy import Action, Kind, Where
from .requests import IssueRequest
from .test_api import CiIssuesTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    gh: gh.API
    step: step.API


PROPERTIES = InputProperties

__all__ = [
    'API',
    'Action',
    'DEPS',
    'Filed',
    'Filing',
    'IssueRequest',
    'Kind',
    'PROPERTIES',
    'TEST_API',
    'Where',
]
