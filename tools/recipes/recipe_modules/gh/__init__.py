# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`gh` module: GitHub, through the `gh` CLI."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import json, raw_io, step

from .api import GhApi as API
from .test_api import GhTestApi as TEST_API
from .types import Comment, Issue, IssueState, Label, StateReason


@dataclass
class DEPS(RecipeScriptApi):
    json: json.API
    raw_io: raw_io.API
    step: step.API


__all__ = [
    'API',
    'Comment',
    'DEPS',
    'Issue',
    'IssueState',
    'Label',
    'StateReason',
    'TEST_API',
]
