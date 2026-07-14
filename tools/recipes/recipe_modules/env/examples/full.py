# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `env` module's seams."""

from __future__ import annotations

import post_process

DEPS = ['env', 'step']


def RunSteps(api):
    gclient = api.env.which('gclient')
    if gclient:
        api.step('resolved gclient', ['echo', gclient])
    api.env.set('BRAVE_FLAG', 'on')
    if 'BRAVE_FLAG' in api.env:
        api.step('flag set', ['echo', api.env.get('BRAVE_FLAG')])
    api.env.prepend_path('/opt/tools')


def GenTests(api):
    yield api.test(
        'basic',
        api.env.on_path('gclient', '/depot_tools/gclient'),
        api.post_process(post_process.MustRun, 'resolved gclient'),
        api.post_process(post_process.StepCommandContains, 'resolved gclient',
                         ['/depot_tools/gclient']),
        api.post_process(post_process.MustRun, 'flag set'),
        api.post_process(post_process.StepCommandContains, 'flag set', ['on']),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'gclient absent',
        api.post_process(post_process.DoesNotRun, 'resolved gclient'),
        api.post_process(post_process.MustRun, 'flag set'),
        api.post_process(post_process.StatusSuccess),
    )
