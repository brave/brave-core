# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for a step's `cost`: what it declares it will consume.

Whether a cost actually makes a step queue is decided by `ResourceWaiter`, and
is covered by `unittests/resource_semaphore_test.py`: a simulated step finishes
without yielding, so two of them never contend here.
"""

from __future__ import annotations

import post_process

DEPS = ['platform', 'step']


def RunSteps(api):
    # Capacity comes from the host, and sizes the pool costs are admitted
    # against.
    assert api.step.MAX_CPU == api.platform.cpu_count * api.step.CPU_CORE
    assert api.step.MAX_MEMORY == api.platform.total_memory

    # Left alone, a step takes the default cost.
    api.step('default cost', ['echo', 'hi'])

    # A step that knows it is expensive says so. This one wants two cores and
    # most of the disk, so nothing else heavy runs beside it.
    api.step('heavy', ['git', 'fetch'],
             cost=api.step.ResourceCost(cpu=2 * api.step.CPU_CORE,
                                        memory=4096,
                                        disk=80,
                                        net=50))

    # Asking for more than the machine has is clamped to the machine, so the
    # step still runs -- on its own -- rather than waiting for capacity that
    # will never exist.
    huge = api.step.ResourceCost(cpu=99 * api.step.CPU_CORE,
                                 memory=1024 * 1024)
    assert huge.cpu == api.step.MAX_CPU, huge.cpu
    assert huge.memory == api.step.MAX_MEMORY, huge.memory
    api.step('clamped', ['echo', 'still runs'], cost=huge)

    # `None` opts out of admission control, for a step that does no real work.
    api.step('free', ['echo', 'cheap'], cost=None)

    # A command-less step never queues, having nothing to run.
    api.step('no command', None)


def GenTests(api):
    yield api.test(
        'cost',
        api.post_process(post_process.MustRun, 'default cost', 'heavy',
                         'clamped', 'free', 'no command'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # A smaller host clamps a cost further down, but still runs the step.
    yield api.test(
        'small host',
        api.platform.capacity(cpu_count=1, total_memory=512),
        api.post_process(post_process.MustRun, 'heavy', 'clamped'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
