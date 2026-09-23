# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `api.step.active_result`, which reads the engine's step stack."""

from __future__ import annotations

import post_process

DEPS = ['futures', 'json', 'step']


def RunSteps(api):
    # Nothing has run yet, so there is no open step: the tip of the stack is
    # still the synthetic root entry.
    assert api.step.active_result is None, api.step.active_result

    api.step('first', ['echo', 'hello'])
    assert api.step.active_result.name == 'first', api.step.active_result.name

    # A step's placeholder results are reachable through the open result, so a
    # caller that did not keep the return value can still read them.
    api.step('emit', ['emit', api.json.output()])
    api.step('read back',
             ['echo', api.step.active_result.json.output['answer']])

    # The failing step stays open while the exception unwinds, which is the
    # point of pushing it before raising.
    try:
        api.step('fails', ['false'])
    except Exception:  # pylint: disable=broad-except
        api.step('in handler', ['echo', api.step.active_result.name])

    # Each greenlet has its own stack, seeded from the tip of its spawner's, so
    # a step run inside one does not become the parent greenlet's active
    # result.
    def worker():
        api.step('in greenlet', ['echo', 'spawned'])
        return api.step.active_result.name

    future = api.futures.spawn(worker)
    api.step('spawned saw', ['echo', future.result()])
    api.step('parent unaffected', ['echo', api.step.active_result.name])


def GenTests(api):
    yield api.test(
        'active result',
        api.step_data('emit', api.json.output({'answer': '42'})),
        api.step_data('fails', retcode=1),
        api.post_process(post_process.StepCommandContains, 'read back',
                         ['42']),
        # The failing step is what is open inside the handler.
        api.post_process(post_process.StepCommandContains, 'in handler',
                         ['fails']),
        # The greenlet sees the step it ran itself...
        api.post_process(post_process.StepCommandContains, 'spawned saw',
                         ['in greenlet']),
        # ...while the spawning greenlet still sees its own last step, which is
        # the one it ran before the spawn.
        api.post_process(post_process.StepCommandContains, 'parent unaffected',
                         ['spawned saw']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
