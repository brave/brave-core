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

    # The step's name, as the tuple of nesting tokens leading to it, so a step
    # named `child` inside a nest named `parent` is `('parent', 'child')`.
    # Empty for the root entry. This is the namespace a step run inside this one
    # is named under.
    name_tokens: tuple[str, ...] = attr.ib()

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
        # namespace tokens -> {requested step name: times used}, so a repeated
        # name within one namespace can be given a distinct one. Shared by the
        # whole run rather than per greenlet, so concurrent steps cannot end up
        # with the same name.
        self._step_names: dict[tuple[str, ...], dict[str, int]] = {}

    @property
    def _stack(self) -> list[_ActiveStep]:
        """This greenlet's stack, seeded with the root entry on first use."""
        if self._storage.steps is None:
            self._storage.steps = [_ActiveStep(None, (), True)]
        return self._storage.steps

    @property
    def active_step(self) -> StepData | None:
        """The tip's step data; None while the tip is the root entry."""
        return self._stack[-1].step_data

    def record_step_name(self, name: str) -> tuple[str, ...]:
        """Reserve *name* in the current namespace, and return its tokens.

        The namespace is whatever nest the calling greenlet is inside, so a
        step's tokens are that nest's tokens plus its own name. A name already
        used in the same namespace gets ` (2)`, ` (3)` and so on appended,
        rather than silently becoming the same step twice.

        Side effect: closes the tip if it is not a parent, so the namespace is
        read from the enclosing nest rather than from the step that just ran.
        """
        self.close_non_parent_step()

        namespace = self._stack[-1].name_tokens
        used = self._step_names.setdefault(namespace, {})
        count = used.setdefault(name, 0)
        used[name] += 1
        return namespace + (name if not count else f'{name} ({count + 1})', )

    def push(self,
             step_data: StepData,
             name_tokens: tuple[str, ...],
             is_parent: bool = False) -> None:
        """Make *step_data* the tip of this greenlet's stack."""
        self._stack.append(_ActiveStep(step_data, name_tokens, is_parent))

    def make_tip_parent(self) -> None:
        """Turn the tip into a parent nesting step.

        A nest runs an ordinary (command-less) step first, then promotes it, so
        the steps that follow are named under it and it stays open until the
        nest ends.
        """
        self._stack[-1].is_parent = True

    def pop(self) -> None:
        """Close the tip and drop it, leaving the entry beneath it as the tip."""
        self._stack.pop().close()

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
