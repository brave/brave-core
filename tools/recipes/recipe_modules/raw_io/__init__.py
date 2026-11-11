# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`raw_io` module: read and write raw data to and from steps."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import path

from .api import RawIOApi as API
from .test_api import RawIOTestApi as TEST_API


# `path` for the scratch directory `output_dir` places its temporary
# directories in.
@dataclass
class DEPS(RecipeScriptApi):
    path: path.API
