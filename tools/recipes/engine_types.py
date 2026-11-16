# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Engine-level value types shared by the runner and the built-in modules.

Ported from upstream `recipes-py`'s `recipe_engine/engine_types.py`, keeping
the upstream semantics (and, where practical, the upstream wording) so the
behaviour stays comparable when reading either code base:

  * `StepPresentation` -- what a step reports about itself beyond its result;
    here, the output properties it exports from the recipe.
  * `ResourceCost` -- what a step expects to consume, honoured by the step
    scheduler so too many costly steps never run at once.
  * `PerGreenletState` -- a base class for module state that must be private to
    each greenlet, with an explicit hook for carrying values into a newly
    spawned one.
"""

from __future__ import annotations

from collections.abc import Callable, Mapping
import copy
import json
from typing import Any

import attr
from gevent.local import local
from google.protobuf import json_format as json_pb
from google.protobuf import message

# The upper bound, in UTF-8 bytes, of a recipe's summary markdown.
MAX_SUMMARY_MARKDOWN_SIZE = 4000


class StepPresentation:
    """StepPresentation allows recipes to alter the presentation of their
    recipe step.

    You can obtain a StepPresentation object after running a step like:

        result = api.step(step_name, ['program', ...])
        presentation = result.presentation  # StepPresentation

    Your code has read/write access to the properties of this StepPresentation
    until it is finalised. Finalisation happens when either the next step in
    the same greenlet (aka "Future") is started, or when the current greenlet
    or 'nest step' (i.e. `with api.step.nest(...):`) exits.

    # properties
    Properties are a way to export structured data from the OVERALL recipe.

    Property keys here correspond to the TOP LEVEL output property key of the
    whole build. If multiple steps emit the same output property key, only the
    lastmost step (i.e. the step which was finalised last) will have its value
    preserved. There is no merging or combining of values.

    For example:

        api.step('one', ...).presentation.properties['propname'] = [1000]
        api.step('two', ...).presentation.properties['propname'] = ['hi']

    leaves the build's output properties as `{"propname": ["hi"]}`.

    A value is anything JSON-serialisable, or a protobuf message, which is
    exported in its JSONPB form. A message is what keeps a property typed from
    end to end: a consumer that declares a field of the same message type in
    its `PROPERTIES` reads the value back as that message.

    Effectively, output properties are "write only": a recipe cannot read back
    what an earlier step exported, so they cannot be used to pass state
    between otherwise unrelated pieces of code as a sort of global variable.
    """

    def __init__(self, step_name: str) -> None:
        self._name: str = step_name
        self._finalised: bool = False
        self._properties: dict[str, Any] = {}

    # pylint: disable-next=method-hidden
    @property
    def properties(self) -> dict[str, Any]:
        if not self._finalised:
            return self._properties
        return copy.deepcopy(self._properties)

    # pylint: disable-next=method-hidden
    @properties.setter
    def properties(self, val: Mapping[str, Any]) -> None:
        assert not self._finalised, 'Changing finalised step %r' % self._name
        assert isinstance(val, Mapping)
        self._properties = dict(val)

    def finalize(self) -> dict[str, Any]:
        """Stop accepting changes; return the output properties to export.

        Each value comes back in its JSON form, as the build records it.
        """
        self._finalised = True
        exported = {}
        for key, value in sorted(self._properties.items()):
            if isinstance(value, message.Message):
                value = json_pb.MessageToDict(value)
            exported[key] = json.loads(json.dumps(value, sort_keys=True))
        return exported


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
            cores = ('%0.2f' % (self.cpu / 1000.0)).rstrip('0').rstrip('.')
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
        return (
            self.cpu <= cpu
            and self.memory <= memory
            and self.disk <= disk
            and self.net <= net
        )


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
