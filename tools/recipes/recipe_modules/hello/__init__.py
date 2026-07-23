# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`hello` module: a worked example of the config system (see README.md).

Also demonstrates per-module `PROPERTIES`: the `$hello` block of the input
property JSON is decoded into `InputProperties` and injected into
`HelloApi.__init__`, which feeds it to the config system as the `TARGET`
default.
"""

from PB.recipe_modules.brave.hello.properties import InputProperties

DEPS = ['path', 'step']

PROPERTIES = InputProperties
