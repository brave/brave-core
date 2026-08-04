# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`ensure()` reads the gni pin from the `path` module's `chromium_src` when
no explicit checkout is given.
"""

from __future__ import annotations

import post_process

DEPS = ['brave_core_checkout', 'osx_sdk', 'platform', 'step']


def RunSteps(api):
    with api.osx_sdk.ensure():
        pass


def GenTests(api):
    yield api.test(
        'mac',
        api.platform.name('mac'),
        api.brave_core_checkout.deployed('tools/cr/toolchains'),
        api.osx_sdk.installed(),
        api.post_process(post_process.StepCommandContains, 'read mac_sdk.gni',
                         ['[WORKSPACE]/b/src/build/config/mac/mac_sdk.gni']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
