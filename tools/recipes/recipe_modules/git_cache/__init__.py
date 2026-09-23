# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`git_cache` module.
"""

from PB.recipe_modules.brave.git_cache.properties import EnvProperties

DEPS = ['env', 'git', 'path', 'raw_io', 'step']

ENV_PROPERTIES = EnvProperties
