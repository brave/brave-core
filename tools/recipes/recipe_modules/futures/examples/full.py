# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `futures` module."""

from __future__ import annotations

import post_process

DEPS = ['context', 'futures', 'path', 'step']


def RunSteps(api):
    # Fan work out, then collect it in completion order. A simulated step never
    # blocks, so each greenlet runs to completion once it is switched to, and
    # the steps come out in spawn order.
    futures = [
        api.futures.spawn(api.step,
                          f'work {i}', ['echo', str(i)],
                          __name=f'w{i}') for i in range(3)
    ]
    names = [future.name for future in api.futures.iwait(futures)]
    api.step('collected', ['echo', *names])

    # Consuming only part of an `iwait` leaks resources unless it is used as a
    # context manager, which unwinds the wait on the way out.
    partial = [
        api.futures.spawn(api.step, f'partial {i}', ['echo', str(i)])
        for i in range(2)
    ]
    with api.futures.iwait(partial) as pending:
        for future in pending:
            api.step('first done', ['echo', future.name])
            break
    api.futures.wait(partial)

    # `wait` returns the done futures rather than yielding them, and a result
    # is the spawned callable's return value.
    done = api.futures.wait([api.futures.spawn(lambda: 'inner')])
    api.step('waited', ['echo', done[0].result(), str(done[0].done)])

    # A bounded semaphore caps how much of a fan-out runs at once.
    sem = api.futures.make_bounded_semaphore(2)

    def limited(i):
        with sem:
            api.step(f'limited {i}', ['echo', str(i)])
        return i

    api.futures.wait(
        [api.futures.spawn(limited, i, __meta={'i': i}) for i in range(3)])

    # `spawn_immediate` switches to the new greenlet straight away, so its step
    # runs before the one after the spawn.
    immediate = api.futures.spawn_immediate(api.step, 'immediate',
                                            ['echo', 'now'])
    api.step('after immediate', ['echo', 'later'])
    immediate.result()

    # Ambient context carries into a spawned greenlet, and a scope entered
    # inside one does not leak back out.
    with api.context(cwd=api.path.workspace / 'outer'):

        def scoped():
            api.step('inherits cwd', ['echo', 'inherited'])
            with api.context(cwd=api.path.workspace / 'inner'):
                api.step('own cwd', ['echo', 'scoped'])

        api.futures.spawn(scoped).result()
        api.step('parent cwd intact', ['echo', 'outer'])

    # A failing greenlet surfaces its exception through the Future rather than
    # at the spawn site, and `__meta` rides along with it.
    failing = api.futures.spawn(api.step,
                                'boom', ['false'],
                                __meta='meta-value')
    exc = failing.exception()
    api.step('caught', ['echo', type(exc).__name__, failing.meta])

    # Cancelling a greenlet that has not been switched to kills it outright.
    cancelled = api.futures.spawn(api.step, 'never runs', ['echo', 'nope'])
    cancelled.cancel()
    api.futures.wait([cancelled])
    api.step('after cancel', ['echo', str(cancelled.done)])


def GenTests(api):
    yield api.test(
        'full',
        api.step_data('boom', retcode=1),
        # Fanned-out work completes in spawn order and is collected by name.
        api.post_process(post_process.MustRun, 'work 0', 'work 1', 'work 2'),
        api.post_process(post_process.StepCommandContains, 'collected',
                         ['w0', 'w1', 'w2']),
        api.post_process(post_process.StepCommandContains, 'waited',
                         ['inner', 'True']),
        api.post_process(post_process.MustRun, 'limited 0', 'limited 1',
                         'limited 2'),
        # `spawn_immediate` runs its step before the following one.
        api.post_process(post_process.MustRun, 'immediate', 'after immediate'),
        # The expectation records each step's cwd, so the golden is what
        # asserts that the spawned greenlet inherits its parent's cwd, that a
        # scope entered inside it applies only there, and that the parent's
        # scope is undisturbed afterwards.
        api.post_process(post_process.MustRun, 'inherits cwd', 'own cwd',
                         'parent cwd intact'),
        # A failure is delivered through the Future, so the recipe carries on.
        api.post_process(post_process.StepCommandContains, 'caught',
                         ['CalledProcessError', 'meta-value']),
        api.post_process(post_process.DoesNotRun, 'never runs'),
        api.post_process(post_process.StepCommandContains, 'after cancel',
                         ['True']),
        api.post_process(post_process.StatusSuccess),
    )
