# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`reset xcode` must run even when `install xcode` itself fails.

Mirrors `EphemeralXcode.deploy()`'s own guarantee that the machine is never
left pointing at an ephemeral Xcode -- here at the recipe-module level, where
`install`/`reset` are two separate steps rather than one Python `with` block.
"""

from __future__ import annotations

import post_process

DEPS = ['brave_core_checkout', 'osx_sdk', 'platform', 'step']


def RunSteps(api):
    with api.osx_sdk.ensure('/b/checkout/src'):
        pass


def GenTests(api):
    yield api.test(
        'install fails',
        api.platform.name('mac'),
        api.brave_core_checkout.deployed('tools/cr/toolchains'),
        api.osx_sdk.mac_sdk_gni(),
        api.step_data('install xcode', retcode=1),
        api.post_process(post_process.StepFailure, 'install xcode'),
        api.post_process(post_process.MustRun, 'reset xcode'),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
    )
    # Covers the `with` block's (no-op) body, which the failure case above
    # never reaches.
    yield api.test(
        'install succeeds',
        api.platform.name('mac'),
        api.brave_core_checkout.deployed('tools/cr/toolchains'),
        api.osx_sdk.installed(),
        api.post_process(post_process.MustRun, 'reset xcode'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
