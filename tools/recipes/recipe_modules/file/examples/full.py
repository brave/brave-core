# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising `file.read_text`/`read_json` and `_run`'s core
`{ok, errno_name, message}` handling. See `examples/operations.py` for the
rest of the module's operations.
"""

from __future__ import annotations

import post_process

DEPS = ['file', 'step']


def RunSteps(api):
    text = api.file.read_text('read greeting', '/etc/greeting')
    api.step('echo', ['echo', text])


def GenTests(api):
    # Happy path: seeded content flows through to the next step.
    yield api.test(
        'basic',
        api.file.read_text('read greeting', 'hello\n'),
        api.post_process(post_process.StepCommandContains, 'read greeting',
                         ['read_text', '/etc/greeting']),
        api.post_process(post_process.StepCommandContains, 'echo',
                         ['echo', 'hello\n']),
        api.post_process(post_process.StatusSuccess),
    )
    # Nothing seeded: defaults to success with empty content, so a caller
    # that doesn't care about the result doesn't need to seed anything.
    yield api.test(
        'unseeded defaults to success',
        api.post_process(post_process.MustRun, 'read greeting'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # A filesystem-level failure raises Error, a checked-step (non-infra)
    # failure.
    yield api.test(
        'not found',
        api.file.error('read greeting', errno_name='ENOENT'),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
    )
