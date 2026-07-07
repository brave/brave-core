# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `step` module (success and failure)."""

from __future__ import annotations

import post_process

DEPS = ['step']


def RunSteps(api, _properties):
    api.step('first', ['echo', 'hello'])
    # A checked step that fails aborts the recipe: 'after' won't run.
    api.step('might fail', ['do-thing'])
    api.step('after', ['echo', 'done'])


def GenTests(api):
    yield api.test(
        'all pass',
        api.post_process(post_process.MustRun, 'first'),
        api.post_process(post_process.MustRun, 'after'),
        api.post_process(post_process.StepSuccess, 'might fail'),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'step fails',
        api.step_data('might fail', retcode=1),
        api.post_process(post_process.MustRun, 'might fail'),
        api.post_process(post_process.StepFailure, 'might fail'),
        api.post_process(post_process.DoesNotRun, 'after'),
        api.post_process(post_process.StatusFailure),
        status='FAILURE',
    )
