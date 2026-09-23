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
    if api.platform.is_mac:
        api.step('mac only', ['echo', 'mac'])
    if api.platform.is_linux:
        api.step('linux only', ['echo', 'linux'])
    api.step('report capacity', [
        'echo',
        str(api.platform.cpu_count),
        str(api.platform.total_memory),
    ])
    api.step('report machine',
             ['echo', str(api.platform.bits), api.platform.arch])


def GenTests(api):
    yield api.test(
        'linux',
        api.platform.name('linux'),
        api.post_process(post_process.StepCommandContains, 'report platform',
                         ['linux']),
        api.post_process(post_process.DoesNotRun, 'windows only'),
        api.post_process(post_process.MustRun, 'linux only'),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'win',
        api.platform.name('win'),
        api.post_process(post_process.StepCommandContains, 'report platform',
                         ['win']),
        api.post_process(post_process.MustRun, 'windows only'),
        api.post_process(post_process.DoesNotRun, 'mac only'),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'mac',
        api.platform.name('mac'),
        api.post_process(post_process.MustRun, 'mac only'),
        api.post_process(post_process.DoesNotRun, 'windows only'),
        api.post_process(post_process.StatusSuccess),
    )
    # A simulated host reports fixed capacity, so a `ResourceCost` schedules
    # identically wherever the tests run.
    yield api.test(
        'default capacity',
        api.platform.name('linux'),
        api.post_process(post_process.StepCommandContains, 'report capacity',
                         ['8', '16384']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # Either figure can be overridden, to exercise a step that has to queue.
    yield api.test(
        'overridden capacity',
        api.platform.name('linux'),
        api.platform.capacity(cpu_count=2, total_memory=1024),
        api.post_process(post_process.StepCommandContains, 'report capacity',
                         ['2', '1024']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # A simulated host defaults to 64-bit intel.
    yield api.test(
        'default machine',
        api.platform.name('linux'),
        api.post_process(post_process.StepCommandContains, 'report machine',
                         ['64', 'intel']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # `api.platform(...)` sets name, bitness and architecture together.
    yield api.test(
        'arm mac',
        api.platform('mac', 64, 'arm'),
        api.post_process(post_process.StepCommandContains, 'report machine',
                         ['64', 'arm']),
        api.post_process(post_process.MustRun, 'mac only'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    yield api.test(
        '32 bit windows',
        api.platform('win', 32),
        api.post_process(post_process.StepCommandContains, 'report machine',
                         ['32', 'intel']),
        api.post_process(post_process.MustRun, 'windows only'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
