# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `raw_io`'s stream helpers seeding a retcode alongside output.

A step's own default simulated data can say the step failed, and still say what
it wrote on its way out -- which is how a recipe that inspects a failing
command's output gets tested.
"""

from __future__ import annotations

import post_process

DEPS = ['raw_io', 'step']


def RunSteps(api):
    result = api.step('failing command', ['do-thing'],
                      check=False,
                      stdout=api.raw_io.output(),
                      step_test_data=lambda: api.raw_io.test_api.stream_output(
                          'nope\n', retcode=3))
    assert result.retcode == 3, result.retcode
    assert result.stdout == b'nope\n', result.stdout


def GenTests(api):
    yield api.test(
        'basic',
        api.post_process(post_process.StepFailure, 'failing command'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
