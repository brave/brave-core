# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example: the plain default configuration (`default_tool`, TARGET 'Bob')."""

from __future__ import annotations

from post_process import DropExpectation, StepCommandRE

DEPS = ['hello']


def RunSteps(api):
    api.hello.set_config('default_tool')
    api.hello.greet()  # Greets 'Bob' with echo.


def GenTests(api):
    yield api.test(
        'bob',
        api.post_process(StepCommandRE, 'Greet Admired Individual',
                         [r'.*\becho', 'Hello Bob']),
        api.post_process(DropExpectation),
    )
