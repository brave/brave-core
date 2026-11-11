# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`proto` module: produce and consume protobuf data to and from steps."""

from dataclasses import dataclass

from recipe_api import RecipeScriptApi
from recipe_modules import raw_io

from .api import ProtoApi as API
from .test_api import ProtoTestApi as TEST_API


@dataclass
class DEPS(RecipeScriptApi):
    raw_io: raw_io.API
