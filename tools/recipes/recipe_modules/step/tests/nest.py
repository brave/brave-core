# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `api.step.nest`: namespacing, dedup, and owning spawned work."""

from __future__ import annotations

import post_process

DEPS = ['futures', 'step']


def RunSteps(api):
    # Steps inside the block are named under the nest.
    with api.step.nest('build'):
        api.step('configure', ['gn', 'gen'])
        api.step('compile', ['ninja'])

    # Nesting composes, and a name is only made distinct within its own
    # namespace -- so each nest may hold its own `compile`.
    with api.step.nest('package'):
        api.step('compile', ['ninja', 'package'])
        with api.step.nest('sign'):
            api.step('compile', ['codesign'])

    # Repeating a name in one namespace is disambiguated rather than collapsed.
    api.step('twice', ['echo', 'first'])
    api.step('twice', ['echo', 'second'])

    # A nest owns the work spawned inside it: leaving the block waits for it,
    # so `after` cannot run while a fetch is still in flight.
    with api.step.nest('fan out'):
        for i in range(2):
            api.futures.spawn(api.step, f'fetch {i}', ['fetch', str(i)])
    api.step('after', ['echo', 'joined'])

    # The nest step is itself a step, and is what `active_result` sees inside
    # the block before anything else has run there.
    with api.step.nest('outer') as parent:
        api.step('reports', ['echo', parent.name])


def GenTests(api):
    yield api.test(
        'nest',
        # The nest itself is recorded, with no command.
        api.post_process(post_process.MustRun, 'build'),
        api.post_process(post_process.MustRun, 'build.configure',
                         'build.compile'),
        # Same leaf name under two different namespaces, plus a deeper nest.
        api.post_process(post_process.MustRun, 'package.compile',
                         'package.sign.compile'),
        # Repeated name in one namespace gets suffixed.
        api.post_process(post_process.MustRun, 'twice', 'twice (2)'),
        # Spawned work is namespaced under the nest that spawned it, and is
        # joined before the nest exits.
        api.post_process(post_process.MustRun, 'fan out.fetch 0',
                         'fan out.fetch 1', 'after'),
        # The parent's own name is the namespaced one.
        api.post_process(post_process.StepCommandContains, 'outer.reports',
                         ['outer']),
        api.post_process(post_process.StatusSuccess),
    )
