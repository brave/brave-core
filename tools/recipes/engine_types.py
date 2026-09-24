# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Engine-level value types shared by the runner and the built-in modules.

Ported from upstream `recipes-py`'s `recipe_engine/engine_types.py`, keeping
the upstream semantics (and, where practical, the upstream wording) so the
behaviour stays comparable when reading either code base:

  * `ResourceCost` -- what a step expects to consume, honoured by the step
    scheduler so too many costly steps never run at once.
  * `PerGreenletState` -- a base class for module state that must be private to
    each greenlet, with an explicit hook for carrying values into a newly
    spawned one.
"""

from __future__ import annotations

from collections.abc import Callable
from typing import Any

import attr
from gevent.local import local


@attr.s(frozen=True, slots=True)
class ResourceCost:
    """A structure defining the resources that a given step may need.

    For use with `api.step`; attaching a `ResourceCost` to a step lets the
    engine prevent too many costly steps from running concurrently.

    See `api.step.ResourceCost` for full documentation.
    """
    cpu: int = attr.ib(default=500)
    memory: int = attr.ib(default=50)
    disk: int = attr.ib(default=0)
    net: int = attr.ib(default=0)

    @classmethod
    def zero(cls) -> ResourceCost:
        """Return a `ResourceCost` with zero for all resources."""
        return cls(0, 0, 0, 0)

    def __attrs_post_init__(self) -> None:
        if self.cpu < 0:
            raise ValueError('negative cpu amount')
        if self.memory < 0:
            raise ValueError('negative memory amount')
        if self.disk < 0 or self.disk > 100:
            raise ValueError('disk not in [0,100]')
        if self.net < 0 or self.net > 100:
            raise ValueError('net not in [0,100]')

    def __bool__(self) -> bool:
        """Whether this cost asks for any resource at all.

        A cost of all zeroes never blocks, so the scheduler skips it.
        """
        return not self.fits(0, 0, 0, 0)

    def __str__(self) -> str:
        bits = []
        if self.cpu > 0:
            cores = ('%0.2f' % (self.cpu / 1000.)).rstrip('0').rstrip('.')
            bits.append(f'cpu=[{cores} cores]')
        if self.memory > 0:
            bits.append(f'memory=[{self.memory} MiB]')
        if self.disk > 0:
            bits.append(f'disk=[{self.disk}%]')
        if self.net > 0:
            bits.append(f'net=[{self.net}%]')
        return ', '.join(bits)

    def fits(self, cpu: int, memory: int, disk: int, net: int) -> bool:
        """Whether this cost fits within the given constraints."""
        return (self.cpu <= cpu and self.memory <= memory and self.disk <= disk
                and self.net <= net)


class PerGreenletState(local):
    """Subclass to get an object whose state is tied to the current greenlet.

        from engine_types import PerGreenletState

        class MyState(PerGreenletState):
            cool_stuff = True
            neat_thing = ''

            def _get_setter_on_spawn(self):
                # Called on greenlet spawn; return a closure propagating values
                # from the spawning greenlet to the new one.
                old_cool_stuff = self.cool_stuff

                def _inner():
                    self.cool_stuff = old_cool_stuff

                return _inner
    """

    def __new__(cls, *args: Any, **kwargs: Any) -> PerGreenletState:
        ret = super().__new__(cls, *args, **kwargs)
        PerGreenletStateRegistry.append(ret)
        return ret

    def _get_setter_on_spawn(self) -> Callable[[], None] | None:
        """Hook for subclasses, to carry state into a spawned greenlet.

        The engine invokes this immediately *before* spawning a new greenlet;
        it should return a 0-argument function which repopulates `self`
        immediately *after* the spawn (i.e. from inside the new greenlet).

        Returning `None` -- as this default does -- leaves the new greenlet
        with the class defaults instead of the spawning greenlet's values.
        """
        return None


class _PerGreenletStateRegistry(list):
    """A registry of every `PerGreenletState` instance.

    `futures` walks this on spawn to collect each state's setter, and the
    simulator clears it between test runs so no state leaks across cases.
    """

    def clear(self) -> None:
        """Clear the registry."""
        self[:] = []


# The (global) registry of all PerGreenletState objects.
PerGreenletStateRegistry = _PerGreenletStateRegistry()
