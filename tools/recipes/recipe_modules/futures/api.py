# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Implements in-recipe concurrency via green threads.

A recipe fans work out with `spawn`, bounds how much of it runs at once with
`make_bounded_semaphore`, and collects results with `wait` or `iwait`:

    sem = api.futures.make_bounded_semaphore(4)

    def worker(url):
        with sem:
            return api.step(f'fetch {url}', ['fetch', url])

    futures = [api.futures.spawn(worker, url, __name=url) for url in urls]
    for future in api.futures.iwait(futures):
        future.result()
"""

from __future__ import annotations

from collections.abc import Callable, Iterable, Iterator
from typing import Any, Generic, TypeVar

import attr
from attr.validators import instance_of
import gevent
import gevent.lock
import gevent.queue

from engine_types import PerGreenletStateRegistry
from recipe_api import RecipeApi

T = TypeVar('T')


class Timeout(Exception):
    """Raised from Future if the requested operation is not done in time."""


@attr.s(frozen=True, slots=True)
class Future(Generic[T]):
    """Represents a unit of concurrent work.

    Modeled after Python 3's `concurrent.futures.Future`. This API can be
    expanded carefully as it is needed.
    """

    _greenlet: gevent.Greenlet = attr.ib(
        validator=instance_of(gevent.Greenlet))
    # `_greenlet.name` is not used for this: its automatically generated names
    # are not unique within a recipe run, so the module keeps its own counter
    # and assigns a UID when the caller did not pass `__name`.
    _name: str = attr.ib(validator=instance_of(str))
    _meta: Any = attr.ib()

    @property
    def name(self) -> str:
        """The name of this Future.

        Either the string provided with `__name` at spawn time, or one
        generated like `Future-<n>`, where `<n>` is globally sequential and
        guaranteed not to be reused within the same recipe run.

        This makes `name` useful for tracking Future objects when getting them
        back from e.g. `iwait`. See also `meta`, to attach metadata directly.
        """
        return self._name

    @property
    def meta(self) -> Any:
        """Metadata associated with this Future.

        This must have been associated with the Future at spawn time with the
        `__meta` kwarg. It is not interpreted or used in any way. The object
        may be mutated, but cannot be assigned to, e.g.

            fut = api.futures.spawn(..., __meta={'key': 'value'})
            fut.meta                    #=> {'key': 'value'}
            fut.meta['thing'] = 100     # OK
            fut.meta = 'something else' # FAIL
        """
        return self._meta

    def result(self, timeout: float | None = None) -> T:
        """Block until this Future is done, then return its value.

        Args:
            timeout: How long, in seconds, to wait for the Future to be done.

        Returns:
            The result, if the Future is done.

        Raises:
            Exception: The Future's exception, if it is done with an error.
            Timeout: If the Future is not done within *timeout*.
        """
        with gevent.Timeout(timeout, exception=Timeout()):
            return self._greenlet.get()

    @property
    def done(self) -> bool:
        """Whether this Future is no longer running."""
        return self._greenlet.dead

    def cancel(self) -> None:
        """Raise `GreenletExit` in the underlying greenlet.

        Does not block on the death of the greenlet, and does not switch away
        from the current greenlet.
        """
        self._greenlet.kill()

    def exception(self, timeout: float | None = None) -> BaseException | None:
        """Block until done, then return (rather than raise) the exception.

        Args:
            timeout: How long, in seconds, to wait for the Future to be done.

        Returns:
            The exception which `result` would raise, or None if there was
            none.

        Raises:
            Timeout: If the Future is not done within *timeout*.
        """
        with gevent.Timeout(timeout, exception=Timeout()):
            done = gevent.wait([self._greenlet])[0]
            return done.exception


class _IWaitWrapper(Iterator[Future[Any]]):
    """Adapts `gevent.iwait` over greenlets back into Futures."""

    __slots__ = ('_waiter', '_greenlets_to_futures')

    def __init__(self, futures: Iterable[Future[Any]], timeout: float | None,
                 count: int | None) -> None:
        # pylint: disable=protected-access
        self._greenlets_to_futures = {fut._greenlet: fut for fut in futures}
        self._waiter = gevent.iwait(list(self._greenlets_to_futures.keys()),
                                    timeout, count)

    def __enter__(self) -> Iterator[Future[Any]]:
        self._waiter.__enter__()
        return self

    def __exit__(self, typ, value, tback):
        return self._waiter.__exit__(typ, value, tback)

    def __iter__(self) -> Iterator[Future[Any]]:
        return self

    def __next__(self) -> Future[Any]:
        return self._greenlets_to_futures[self._waiter.__next__()]


class FuturesApi(RecipeApi):
    """Provides access to the recipe concurrency primitives."""

    Timeout: type[Timeout] = Timeout
    Future: type[Future[Any]] = Future

    def __init__(self) -> None:
        super().__init__()
        self._future_id = 0

    def make_bounded_semaphore(self,
                               value: int = 1) -> gevent.lock.BoundedSemaphore:
        """Return a `gevent.BoundedSemaphore` with depth *value*.

        Use it as a context manager to create concurrency-limited sections:

            def worker(api, sem, i):
                with sem:
                    api.step('one at a time', ...)
                api.step('unrestricted concurrency', ...)
        """
        return gevent.lock.BoundedSemaphore(value)

    def make_channel(self) -> gevent.queue.Channel:
        """Return a single-slot device for passing data between functions.

        This is useful for running 'background helper' type concurrent
        processes.

        Passing Channel objects outside of a recipe module is strongly
        discouraged: mediate access through a class, context manager or
        function returned to the caller instead.

        It is very rare to need a Channel. Avoid it unless the possibility of
        introducing deadlocks has been carefully considered.
        """
        return gevent.queue.Channel()

    def spawn(self, func: Callable[..., T], *args: Any,
              **kwargs: Any) -> Future[T]:
        """Prepare a Future to run `func(*args, **kwargs)` concurrently.

        Because this spawns a greenlet on the same OS thread -- rather than a
        different thread or process -- *func* may be an inner function,
        closure or lambda; neither it nor its arguments need be pickleable.

        This does NOT switch to the greenlet: that only happens once something
        blocks on a future or a step. In particular, this means the following
        is safe, since no switch point occurs between the check and the
        assignment:

            if not self._my_future:
                self._my_future = api.futures.spawn(func)

        Kwargs:
            __name: Assign this name to the spawned greenlet. Useful for
                tracking the Future when getting it back from `iwait`. See
                `Future.name`.
            __meta: Assign this metadata to the returned Future. For the
                caller's exclusive use; see `Future.meta`.
            Everything else is passed to *func*.

        Returns:
            A `Future` of *func*'s result.
        """
        name = kwargs.pop('__name', None)
        if name is None:
            name = f'Future-{self._future_id}'
        self._future_id += 1

        meta = kwargs.pop('__meta', None)

        # A leaf step left open by this greenlet is closed before control can
        # reach the new one, so two leaf steps are never open at once at the
        # same nest level. The greenlet is then attributed to the innermost
        # enclosing parent, which waits for it when it closes.
        self._step_stack.close_non_parent_step()

        # Collected in the spawning greenlet and replayed inside the new one,
        # so it starts from this greenlet's ambient state rather than the class
        # defaults. A state which does not implement the hook returns None and
        # is simply left at its defaults, as its docstring promises.
        setters = [
            setter for setter in (
                pgs._get_setter_on_spawn()  # pylint: disable=protected-access
                for pgs in PerGreenletStateRegistry) if setter is not None
        ]

        def _runner():
            for setter in setters:
                setter()
            try:
                return func(*args, **kwargs)
            finally:
                # Symmetric with the close_non_parent_step() call above spawn:
                # a leaf step this greenlet leaves open is closed as the
                # greenlet finishes.
                self._step_stack.close_non_parent_step()

        greenlet = gevent.spawn(_runner)
        greenlet.name = name
        self._step_stack.register_greenlet(greenlet)
        return Future(greenlet, name, meta)

    def spawn_immediate(self, func: Callable[..., T], *args: Any,
                        **kwargs: Any) -> Future[T]:
        """Like `spawn`, except it IMMEDIATELY switches to the new greenlet.

        Useful to e.g. launch a background step and then another step which
        waits for the daemon.

        Kwargs:
            __name: As `spawn`.
            __meta: As `spawn`.
            Everything else is passed to *func*.

        Returns:
            A `Future` of the concurrently running *func*'s result.
        """
        name = kwargs.pop('__name', None)
        meta = kwargs.pop('__meta', None)
        chan = self.make_channel()

        def _immediate_runner():
            chan.get()
            return func(*args, **kwargs)

        ret = self.spawn(_immediate_runner, __name=name, __meta=meta)
        chan.put(None)  # Pass execution to _immediate_runner.
        return ret

    @staticmethod
    def wait(futures: Iterable[Future[Any]],
             timeout: float | None = None,
             count: int | None = None) -> list[Future[Any]]:
        """Block until *count* *futures* are done, then return them.

        This is analogous to `gevent.wait`.

        Args:
            futures: The Future objects to wait for.
            timeout: How long, in seconds, to wait for the Futures to be done.
                On timeout this returns even if *count* has not been reached.
            count: How many Futures to wait for. None waits for all of them.

        Returns:
            The done Futures, in the order in which they completed.
        """
        return list(_IWaitWrapper(futures, timeout, count))

    @staticmethod
    def iwait(futures: Iterable[Future[Any]],
              timeout: float | None = None,
              count: int | None = None) -> Iterator[Future[Any]]:
        """Iteratively yield up to *count* Futures as they become done.

        This is analogous to `gevent.iwait`.

            for future in api.futures.iwait(futures):
                ...  # consume future

        If the whole iterator will not be consumed, the resource leak is
        avoided by using it as a context manager:

            with api.futures.iwait(a, b, c) as it:
                for future in it:
                    if future is a:
                        break

        Prefer `iwait` over `wait` to process a group of Futures in the order
        in which they complete. Compare:

            for task in iwait(tasks):
                ...  # task is done, do something with it

        against:

            while tasks:
                task = wait(tasks, count=1)[0]  # some task is done
                tasks.remove(task)
                ...  # do something with it

        Args:
            futures: The Future objects to wait for.
            timeout: How long, in seconds, to wait for the Futures to be done.
            count: How many Futures to yield. None yields all of them.

        Yields:
            Futures in the order in which they complete, until the timeout or
            *count* is hit.
        """
        return _IWaitWrapper(futures, timeout, count)
