# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests that the working checkout is never made shallow.
"""

from __future__ import annotations

import post_process

DEPS = ['chromium_checkout', 'env']


def RunSteps(api):
    api.chromium_checkout.ensure_checkout(ref=api.env.get('REF'))


def GenTests(api):
    yield api.test(
        'fresh clone is not shallow',
        api.env.set('REF', 'main'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.post_process(post_process.StepCommandRE, 'clone from git cache', [
            'git', 'clone', '--no-checkout', '--local', '--shared', '.*', '.*'
        ]),
        api.post_process(post_process.StepCommandDoesNotContain,
                         'git cache populate', ['--depth']),
        api.post_process(post_process.StepCommandContains, 'checkout ref',
                         ['main']),
        api.post_process(post_process.DoesNotRun, 'fetch ref'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # A fully-qualified ref is the one clone path that still fetches; it must
    # not deepen either.
    yield api.test(
        'fresh release branch fetch is not shallow',
        api.env.set('REF', 'refs/branch-heads/7917'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.post_process(post_process.StepCommandDoesNotContain, 'fetch ref',
                         ['--depth']),
        api.post_process(post_process.StepCommandContains, 'fetch ref',
                         ['--no-show-forced-updates']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    yield api.test(
        'existing checkout branch fetch is not shallow',
        api.env.set('REF', 'main'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.existing_checkout(),
        api.chromium_checkout.git_cache_populated(),
        api.post_process(post_process.StepCommandDoesNotContain,
                         'git cache populate for ref', ['--depth']),
        api.post_process(post_process.StepCommandDoesNotContain, 'fetch ref',
                         ['--depth']),
        api.post_process(post_process.StepCommandContains, 'fetch ref',
                         ['--no-show-forced-updates']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    yield api.test(
        'existing checkout tag fetch is not shallow',
        api.env.set('REF', '151.0.7917.1'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.existing_checkout(),
        api.chromium_checkout.git_cache_populated(),
        api.post_process(post_process.StepCommandDoesNotContain, 'fetch tag',
                         ['--depth']),
        api.post_process(post_process.StepCommandContains, 'fetch tag',
                         ['--no-show-forced-updates']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
