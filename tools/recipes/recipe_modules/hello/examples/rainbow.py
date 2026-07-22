# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example: overriding a schema arg via CONFIG_VARS (`super_tool`, 'Charlie')."""

from __future__ import annotations

from post_process import DropExpectation, StepCommandRE

DEPS = ['hello']


def RunSteps(api):
    api.hello.set_config('super_tool', TARGET='Charlie')
    api.hello.greet()  # Greets 'Charlie' with unicorn.py.


def GenTests(api):
    yield api.test(
        'charlie',
        api.post_process(StepCommandRE, 'Greet Admired Individual',
                         [r'.*\bunicorn.py', 'Hello Charlie']),
        api.post_process(DropExpectation),
    )
