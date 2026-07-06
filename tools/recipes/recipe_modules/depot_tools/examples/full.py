# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `depot_tools` module."""

from __future__ import annotations

import post_process

DEPS = ['depot_tools', 'env', 'platform', 'step']


def RunSteps(api, _properties):
    vpython3 = api.depot_tools.vpython3()
    api.step('use vpython3', [vpython3, '--version'])


def GenTests(api):
    yield api.test(
        'clone',
        api.platform.name('linux'),
        api.post_process(post_process.MustRun, 'clone depot_tools'),
        api.post_process(post_process.MustRun, 'verify gclient'),
        api.post_process(post_process.StepCommandRE, 'use vpython3',
                         [r'vpython3$', r'--version']),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'already on path',
        api.env.on_path('gclient', '/existing/depot_tools/gclient'),
        api.post_process(post_process.DoesNotRun, 'clone depot_tools'),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'windows',
        api.platform.name('win'),
        api.post_process(post_process.StepCommandRE, 'use vpython3',
                         [r'vpython3\.bat$', r'--version']),
        api.post_process(post_process.StatusSuccess),
    )
