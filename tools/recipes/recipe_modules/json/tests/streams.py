# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `json.output_stream` seeding a retcode alongside the JSON.

A step's own default simulated data can say the step failed, and still say what
JSON it wrote on its way out -- which is how a recipe that inspects a failing
command's report gets tested.
"""

from __future__ import annotations

import post_process

DEPS = ['json', 'step']


def RunSteps(api):
    result = api.step('run tests', ['run_tests'],
                      check=False,
                      stdout=api.json.output(),
                      step_test_data=lambda: api.json.test_api.output_stream(
                          {'failed': ['a', 'b']}, retcode=1))
    assert result.retcode == 1, result.retcode
    assert result.stdout == {'failed': ['a', 'b']}, result.stdout


def GenTests(api):
    yield api.test(
        'basic',
        api.post_process(post_process.StepFailure, 'run tests'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
