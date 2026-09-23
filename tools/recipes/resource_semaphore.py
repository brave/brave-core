# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Admission control for steps that declare what they will consume.

A step attaches a `ResourceCost` (see `engine_types`), and this holds it back
until the machine has room for it, which is how a recipe caps expensive
concurrent work by what it actually costs rather than by a job count guessed up
front.
"""

from __future__ import annotations

from contextlib import contextmanager

import attr
from gevent.queue import Channel

from engine_types import ResourceCost


@attr.s
class ResourceWaiter:
    """Represents the machine's CPU, memory, disk and network as limited
    resources.

    Each cpu (according to the host's logical core count) is worth 1000
    millicores. Every subprocess that attempts to execute will first acquire its
    estimated amount of millicores before launching the subprocess. As soon as
    the subprocess completes, the held millicores are put back into the pool.

    Similarly, memory is measured in megabytes of physical system memory (i.e.
    not including swap). The assumption is that the recipe (and its
    subprocesses) is really mostly the only thing running on the machine anyway.

    Disk and net are different, however. They are unitless measures of
    'percentage of resource', where the absolute quantity of Disk (IOPS,
    read/write/seek speed) and Network (bandwidth, latency) are not considered.
    Consider them more a declaration that a given step contends on disk or
    network availability. If you had many steps which each took `10%` disk, only
    10 of them would run at a time. Similarly, if you had steps which declared
    50% of disk bandwidth usage, only 2 of them would run at a time.

    Because recipes are finite both in runtime and number of distinct steps,
    this resource class unblocks other processes greedily. Whenever a subprocess
    completes, this analyzes all the outstanding subprocesses and will unblock
    whichever ones 'fit' in the now-freed resources. This is done in roughly
    FIFO order (i.e. if two tasks could potentially fit, the first one to block
    will be chosen over the second to unblock first).

    This is different than what's deemed 'fair' in a typical scheduling
    scenario, because in a mixed workload, heavy tasks could be forced to wait
    longer while small tasks use the CPU. However, because the recipes typically
    run with a hard finite timeout, it's better to use more of the CPU earlier
    than to potentially waste time waiting for small tasks to finish in order to
    schedule a heavy task earlier.
    """

    # Required for __init__
    _millicores_available = attr.ib()
    _memory_available = attr.ib()

    # Attrs with defaults.
    _millicores_max = attr.ib()

    @_millicores_max.default
    def _millicores_max_default(self):
        return self._millicores_available

    _memory_max = attr.ib()

    @_memory_max.default
    def _memory_max_default(self):
        return self._memory_available

    _disk_available = attr.ib(default=100)
    _disk_max = attr.ib(default=100)

    _net_available = attr.ib(default=100)
    _net_max = attr.ib(default=100)

    # Each _waiters entry has a unique ID
    _waiter_uid = attr.ib(default=0)
    # list[tuple[amount, waiter_uid, Channel]]
    #
    # The `uid` is used to ensure that Channel is never used when sorting
    # this list.
    _waiters = attr.ib(factory=list)

    def _fits(self, resources: ResourceCost) -> bool:
        assert isinstance(resources, ResourceCost)
        return resources.fits(self._millicores_available,
                              self._memory_available, self._disk_available,
                              self._net_available)

    def _decr(self, resources: ResourceCost) -> None:
        assert isinstance(resources, ResourceCost)
        self._millicores_available -= resources.cpu
        self._memory_available -= resources.memory
        self._disk_available -= resources.disk
        self._net_available -= resources.net

    def _incr(self, resources: ResourceCost) -> None:
        assert isinstance(resources, ResourceCost)
        self._millicores_available += resources.cpu
        self._memory_available += resources.memory
        self._disk_available += resources.disk
        self._net_available += resources.net

    @contextmanager
    def wait_for(self, resources: ResourceCost | None, call_if_blocking=None):
        """Block until *resources* are available.

        Args:
            resources: The amount of various resources to acquire before
                yielding. If any aspect of this exceeds the maximum amount of
                resource available on the system, this will instead acquire the
                system maximum. If resources is all 0's, or is None, this does
                not block.
            call_if_blocking: `wait_for` will invoke this callback if it would
                end up blocking before yielding. It should only be used for
                reporting/diagnostics (i.e. it should not raise an exception).

        Yields control once the requisite amount of resources are available.
        Exiting the context frees up the resources.
        """
        if resources is None:
            yield
            return

        assert isinstance(resources, ResourceCost)

        if resources.cpu > self._millicores_max:
            resources = attr.evolve(resources, cpu=self._millicores_max)

        if resources.memory > self._memory_max:
            resources = attr.evolve(resources, memory=self._memory_max)

        if resources and (self._waiters or not self._fits(resources)):
            # We need some amount of resource AND someone else is already
            # waiting, or there isn't enough resource.
            if call_if_blocking:
                call_if_blocking(resources)
            wake_me = Channel()
            self._waiter_uid += 1
            self._waiters.append((resources, self._waiter_uid, wake_me))
            self._waiters.sort(reverse=True)  # stable sort
            wake_me.get()
            # At this point the greenlet that woke us already reserved our
            # resources for us, and we're free to go.
        else:
            # Just directly take our cores.
            assert self._fits(resources)
            self._decr(resources)

        try:
            yield
        finally:
            self._incr(resources)
            # We just added some resource back to the pot. Try to wake as many
            # others as we can before proceeding.

            to_wake, to_keep = [], []
            for waiting_resources, uid, chan in self._waiters:
                if self._fits(waiting_resources):
                    to_wake.append(chan)
                    self._decr(waiting_resources)
                else:
                    to_keep.append((waiting_resources, uid, chan))
            # _waiters was sorted before, so to_keep is also.
            self._waiters = to_keep
            for chan in to_wake:
                chan.put(None)
