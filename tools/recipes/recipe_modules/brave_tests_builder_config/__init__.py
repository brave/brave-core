# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`brave_tests_builder_config` module: the builder a build runs as.

A trimmed port of upstream's `chromium_tests_builder_config`.
"""

from dataclasses import dataclass

from PB.recipe_modules.brave.brave_tests_builder_config.properties import (
    InputProperties,
)
from recipe_api import RecipeScriptApi

from .api import BraveTestsBuilderConfigApi as API
from .builder_config import (
    BuilderConfig,
    BuilderConfigException,
    BuilderId,
    BuilderSpec,
)
from .test_api import BraveTestsBuilderConfigTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    pass


PROPERTIES = InputProperties

__all__ = [
    'API',
    'BuilderConfig',
    'BuilderConfigException',
    'BuilderId',
    'BuilderSpec',
    'DEPS',
    'PROPERTIES',
    'TEST_API',
]
