# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example: a schema arg steering a config item's behaviour (DarthVader)."""

from __future__ import annotations

from post_process import DropExpectation, StepCommandRE

DEPS = ['hello']


def RunSteps(api):
    api.hello.set_config('default_tool', TARGET='DarthVader')
    api.hello.greet()  # Causes 'DarthVader' to despair, with echo.


def GenTests(api):
    yield api.test(
        'darth',
        api.post_process(StepCommandRE, 'Greet Admired Individual',
                         [r'.*\becho', 'Die in a fire DarthVader!']),
        api.post_process(DropExpectation),
    )
