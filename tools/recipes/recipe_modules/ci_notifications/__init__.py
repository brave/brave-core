# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`ci_notifications` module: what a build has to tell the people following
it."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import step

from .api import CiNotificationsApi as API
from .notifications import Audience, Notification


@dataclass
class DEPS(RecipeScriptApi):
    step: step.API


__all__ = ['API', 'Audience', 'DEPS', 'Notification']
