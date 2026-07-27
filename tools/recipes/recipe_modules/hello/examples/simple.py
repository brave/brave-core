# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example: the default config, and a per-module PROPERTIES-driven TARGET.

`bob` uses the schema default TARGET; `anya` supplies the `$hello` module
property, which `HelloApi.get_config_defaults` feeds in as the TARGET default.
"""

from __future__ import annotations

from post_process import DropExpectation, StepCommandRE

DEPS = ['hello']


def RunSteps(api):
    api.hello.set_config('default_tool')
    api.hello.greet()  # Greets the configured target with echo.


def GenTests(api):
    yield api.test(
        'bob',
        api.post_process(StepCommandRE, 'Greet Admired Individual',
                         [r'.*\becho', 'Hello Bob']),
        api.post_process(DropExpectation),
    )

    # The `$hello` block is this module's namespaced PROPERTIES input.
    yield api.test(
        'anya',
        api.properties(**{'$hello': {
            'target': 'anya'
        }}),
        api.post_process(StepCommandRE, 'Greet Admired Individual',
                         [r'.*\becho', 'Hello anya']),
        api.post_process(DropExpectation),
    )
