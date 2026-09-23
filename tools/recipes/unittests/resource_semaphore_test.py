#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `ResourceWaiter`, the admission control behind a step's `cost`.

These run greenlets that actually block, which a simulated recipe cannot: a
simulated step completes without ever yielding, so two of them never hold
resources at the same time and the scheduler is never contended.
"""

import os
import sys
import unittest

import gevent

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
from engine_types import ResourceCost
from resource_semaphore import ResourceWaiter


def _run(waiter, cost, log, label, hold=0.01):
    """Acquire *cost*, record entry and exit around a blocking hold."""
    with waiter.wait_for(cost):
        log.append(f'+{label}')
        gevent.sleep(hold)
        log.append(f'-{label}')


class ResourceWaiterTest(unittest.TestCase):

    def test_fitting_costs_run_concurrently(self):
        # Two 40% disk steps fit inside the 100% pool at once, so they overlap.
        waiter = ResourceWaiter(8000, 16384)
        log = []
        gevent.joinall([
            gevent.spawn(_run, waiter, ResourceCost(0, 0, 40, 0), log, 'a'),
            gevent.spawn(_run, waiter, ResourceCost(0, 0, 40, 0), log, 'b'),
        ])
        self.assertEqual(log, ['+a', '+b', '-a', '-b'])

    def test_oversubscribed_costs_serialise(self):
        # Three 60% disk steps cannot overlap, so each waits for the last.
        #
        # Note the order they resume in: waiters are sorted by cost descending,
        # and equal costs tie-break on a monotonic id -- also descending. So
        # among equally sized waiters the most recent to block is released
        # first, which is the opposite of the "roughly FIFO" upstream's
        # docstring describes. Ported as-is; the behaviour only decides which
        # of several equally sized steps goes next.
        waiter = ResourceWaiter(8000, 16384)
        log = []
        gevent.joinall([
            gevent.spawn(_run, waiter, ResourceCost(0, 0, 60, 0), log, label)
            for label in 'abc'
        ])
        self.assertEqual(log, ['+a', '-a', '+c', '-c', '+b', '-b'])

    def test_blocking_callback_reports_what_is_waited_for(self):
        waiter = ResourceWaiter(8000, 16384)
        blocked = []
        log = []

        def waits():
            with waiter.wait_for(ResourceCost(0, 0, 60, 0), blocked.append):
                log.append('second')

        gevent.joinall([
            gevent.spawn(_run, waiter, ResourceCost(0, 0, 60, 0), log,
                         'first'),
            gevent.spawn(waits),
        ])
        self.assertEqual([str(cost) for cost in blocked], ['disk=[60%]'])

    def test_cost_above_capacity_is_clamped_and_still_runs(self):
        # A step asking for more than the machine has runs on its own rather
        # than deadlocking.
        waiter = ResourceWaiter(1000, 100)
        log = []
        gevent.joinall([
            gevent.spawn(_run, waiter, ResourceCost(cpu=99000, memory=99000),
                         log, 'huge'),
            gevent.spawn(_run, waiter, ResourceCost(cpu=1000, memory=100), log,
                         'also huge'),
        ])
        self.assertEqual(log, ['+huge', '-huge', '+also huge', '-also huge'])

    def test_zero_cost_never_blocks(self):
        # A pool with nothing left still admits a zero cost.
        waiter = ResourceWaiter(0, 0)
        log = []
        gevent.joinall([
            gevent.spawn(_run, waiter, ResourceCost.zero(), log, label)
            for label in 'ab'
        ])
        self.assertEqual(log, ['+a', '+b', '-a', '-b'])

    def test_none_cost_opts_out(self):
        waiter = ResourceWaiter(0, 0)
        log = []
        gevent.joinall(
            [gevent.spawn(_run, waiter, None, log, label) for label in 'ab'])
        self.assertEqual(log, ['+a', '+b', '-a', '-b'])

    def test_larger_waiter_is_preferred_when_room_frees_up(self):
        # Greedy scheduling: when the holder releases, waiters are considered
        # largest first, so `big` is let through ahead of `small` even though
        # `small` queued earlier and both fit.
        waiter = ResourceWaiter(8000, 16384)
        log = []
        holder = gevent.spawn(_run, waiter, ResourceCost(0, 0, 100, 0), log,
                              'hold', 0.05)
        gevent.sleep(0)  # Let the holder take the whole pool first.
        small = gevent.spawn(_run, waiter, ResourceCost(0, 0, 10, 0), log,
                             'small')
        gevent.sleep(0)  # ...and let `small` queue ahead of `big`.
        big = gevent.spawn(_run, waiter, ResourceCost(0, 0, 70, 0), log, 'big')
        gevent.joinall([holder, small, big])
        self.assertEqual(log[:3], ['+hold', '-hold', '+big'])


if __name__ == '__main__':
    unittest.main()
