# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Spawned work is waited for when the run unwinds, even if never collected.

`spawn` attributes the greenlet to the innermost enclosing parent on the step
stack -- with no nesting, that is the root -- and the engine closes the root
after `RunSteps` returns. So a recipe that spawns and forgets still has its
greenlets run to completion rather than being dropped on the floor.
"""

from __future__ import annotations

import post_process

DEPS = ['futures', 'step']


def RunSteps(api):
    # Deliberately never waited on. `spawn` does not switch to the greenlet, so
    # nothing here gives it a chance to run.
    api.futures.spawn(api.step, 'forgotten', ['echo', 'ran anyway'])
    api.step('last', ['echo', 'done'])


def GenTests(api):
    yield api.test(
        'joined at run end',
        api.post_process(post_process.MustRun, 'last'),
        # Only reached because unwinding the stack waits for the greenlet.
        api.post_process(post_process.MustRun, 'forgotten'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
