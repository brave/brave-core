# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

import post_process

DEPS = ['brave_core_checkout', 'osx_sdk', 'platform', 'step']


def RunSteps(api):
    with api.osx_sdk.ensure('/b/checkout/src') as info:
        if info is not None:
            api.step('xcodebuild -version', ['xcodebuild', '-version'])


def GenTests(api):
    # Happy path on mac: reads the gni pin, installs + selects, runs a build
    # step, then resets.
    yield api.test(
        'mac',
        api.platform.name('mac'),
        api.brave_core_checkout.deployed('tools/cr/toolchains'),
        api.osx_sdk.installed(),
        api.post_process(post_process.StepCommandContains, 'read mac_sdk.gni',
                         ['/b/checkout/src/build/config/mac/mac_sdk.gni']),
        api.post_process(post_process.StepCommandContains, 'install xcode',
                         ['--sdk-version', '26.5', '--sdk-build', '25F70']),
        api.post_process(post_process.MustRun, 'xcodebuild -version'),
        api.post_process(post_process.MustRun, 'reset xcode'),
        api.post_process(post_process.StatusSuccess),
    )
    # Non-mac: ensure() is a no-op -- nothing read, installed, or reset.
    yield api.test(
        'linux',
        api.platform.name('linux'),
        api.post_process(post_process.DoesNotRun, 'read mac_sdk.gni'),
        api.post_process(post_process.DoesNotRun, 'install xcode'),
        api.post_process(post_process.DoesNotRun, 'reset xcode'),
        api.post_process(post_process.DoesNotRun, 'xcodebuild -version'),
        api.post_process(post_process.StatusSuccess),
    )
