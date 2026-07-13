# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `platform` module's seams."""

from __future__ import annotations

import post_process

DEPS = ['platform', 'step']


def RunSteps(api):
    api.step('report platform', ['echo', api.platform.name])
    if api.platform.is_win:
        api.step('windows only', ['echo', 'win'])


def GenTests(api):
    yield api.test(
        'linux',
        api.platform.name('linux'),
        api.post_process(post_process.StepCommandContains, 'report platform',
                         ['linux']),
        api.post_process(post_process.DoesNotRun, 'windows only'),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'win',
        api.platform.name('win'),
        api.post_process(post_process.StepCommandContains, 'report platform',
                         ['win']),
        api.post_process(post_process.MustRun, 'windows only'),
        api.post_process(post_process.StatusSuccess),
    )
