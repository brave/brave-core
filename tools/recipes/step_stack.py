# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The engine's stack of open steps.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

import attr
import gevent

from engine_types import PerGreenletState

if TYPE_CHECKING:
    from step_data import StepData


@attr.s(slots=True)
class _ActiveStep:
    """An open step, at one nest level of one greenlet's stack."""

    # The step's result, or None for the synthetic root entry.
    step_data: StepData | None = attr.ib()
    # Whether this is a parent nesting step, rather than a real (leaf) step.
    # Only the tip of a stack may be False.
    is_parent: bool = attr.ib()
    # Greenlets spawned while this entry was the innermost parent.
    greenlets: list = attr.ib(factory=list)

    def close(self) -> None:
        """Wait for the work spawned under this step."""
        gevent.wait(self.greenlets)


class _Steps(PerGreenletState):
    """Greenlet-local storage for the stack itself.

    `steps` defaults to None rather than `[]`: reading an unset attribute
    falls through to the class-level default, but in-place mutation (as
    opposed to assignment) doesn't create a greenlet-private copy, so a
    mutable default would let every greenlet append to the same shared list.
    `StepStack` seeds a fresh list per greenlet, via assignment, on first use.
    """

    steps = None

    def _get_setter_on_spawn(self):
        # Only the tip carries across: a spawned greenlet starts at the nest
        # level of the greenlet that spawned it, without inheriting its deeper
        # history.
        tip = self.steps[-1] if self.steps else None

        def _inner():
            self.steps = [tip] if tip is not None else None

        return _inner


class StepStack:
    """One run's stack of open steps, private to each greenlet.

    The engine owns a single instance and seeds it onto every module, so `step`
    and `futures` share it without depending on one another -- upstream has the
    same property, by routing both through the engine.
    """

    def __init__(self) -> None:
        self._storage = _Steps()

    @property
    def _stack(self) -> list[_ActiveStep]:
        """This greenlet's stack, seeded with the root entry on first use."""
        if self._storage.steps is None:
            self._storage.steps = [_ActiveStep(None, True)]
        return self._storage.steps

    @property
    def active_step(self) -> StepData | None:
        """The tip's step data; None while the tip is the root entry."""
        return self._stack[-1].step_data

    def push(self, step_data: StepData, is_parent: bool = False) -> None:
        """Make *step_data* the tip of this greenlet's stack."""
        self._stack.append(_ActiveStep(step_data, is_parent))

    def close_non_parent_step(self) -> None:
        """Close the tip, unless it is a parent nesting step.

        A leaf step stays open after it runs, so `api.step.active_result` can
        still reach it. Whatever happens next -- another step, a spawn, or the
        run unwinding -- closes it first.
        """
        if self._stack[-1].is_parent:
            return
        self._stack.pop().close()

    def register_greenlet(self, greenlet: gevent.Greenlet) -> None:
        """Attribute *greenlet* to the tip, to be waited for when it closes."""
        self._stack[-1].greenlets.append(greenlet)

    def unwind(self) -> None:
        """Close the tip and then the entry beneath it, at the end of a run.

        Mirrors the `finally` upstream runs around `RunSteps`: closing the root
        is what waits for work the recipe spawned but never collected.
        """
        self.close_non_parent_step()
        self._stack[-1].close()
