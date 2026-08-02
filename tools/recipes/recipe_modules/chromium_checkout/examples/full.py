# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `chromium_checkout` module."""

from __future__ import annotations

import post_process
from PB.recipe_modules.brave.chromium_checkout.examples.full import (
    InputProperties)

DEPS = ['chromium_checkout', 'env', 'platform', 'step']

PROPERTIES = InputProperties

# An arbitrary stand-in for a real `TOOLCHAIN_HASH` pin, only used to compute
# the `GYP_MSVS_HASH_*` key `win_toolchain_hash_env` below looks up -- a real
# recipe never knows this value ahead of time, since it comes back from the
# `resolve win toolchain hash` step itself.
_TEST_TOOLCHAIN_HASH = 'deadbeef00'


def RunSteps(api, properties):
    api.chromium_checkout.ensure_checkout(ref=properties.chromium_ref)
    # Surface otherwise-internal hermetic-toolchain env as steps so a test can
    # assert `checkout_ref` set them on Windows.
    toolchain = api.env.get('DEPOT_TOOLS_WIN_TOOLCHAIN_BASE_URL')
    if toolchain:
        api.step('win toolchain env', ['echo', toolchain])
    gyp_hash = api.env.get(f'GYP_MSVS_HASH_{_TEST_TOOLCHAIN_HASH}')
    if gyp_hash:
        api.step('win toolchain hash env', ['echo', gyp_hash])


def GenTests(api):
    # No existing checkout -> clone via a shared git-cache mirror, then check
    # out a release tag (also fetched into the mirror first).
    yield api.test(
        'fresh tag',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.properties(chromium_ref='151.0.7917.1'),
        api.post_process(post_process.MustRun, 'gclient config'),
        api.post_process(post_process.MustRun, 'git cache populate'),
        api.post_process(post_process.MustRun, 'clone from git cache'),
        api.post_process(post_process.MustRun, 'checkout origin/HEAD'),
        api.post_process(post_process.MustRun, 'git cache populate for ref'),
        api.post_process(post_process.MustRun, 'fetch tag'),
        api.post_process(post_process.MustRun, 'gclient sync'),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate for ref',
                         ['--ref', 'refs/tags/151.0.7917.1']),
        api.post_process(post_process.StatusSuccess),
    )
    # Existing checkout (chrome/VERSION present) + a branch ref -> no clone,
    # `fetch ref` rather than `fetch tag`.
    yield api.test(
        'existing branch',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.existing_checkout(),
        api.chromium_checkout.git_cache_populated(),
        api.properties(chromium_ref='main'),
        api.post_process(post_process.MustRun, 'check chrome/VERSION'),
        api.post_process(post_process.DoesNotRun, 'gclient config'),
        api.post_process(post_process.DoesNotRun, 'clone from git cache'),
        api.post_process(post_process.MustRun, 'point origin at git cache'),
        api.post_process(post_process.MustRun, 'fetch ref'),
        api.post_process(post_process.StatusSuccess),
    )
    # No git cache configured -> validate_git_cache raises.
    yield api.test(
        'missing git cache',
        api.properties(chromium_ref='main'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # On Windows, checkout_ref sets the hermetic toolchain base URL. No
    # toolchain index published for the upstream hash yet (the default,
    # unseeded `resolve win toolchain hash` result) -> GYP_MSVS_HASH_* stays
    # unset.
    yield api.test(
        'windows hermetic toolchain',
        api.platform.name('win'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.existing_checkout(),
        api.chromium_checkout.git_cache_populated(),
        api.properties(chromium_ref='main'),
        api.post_process(post_process.MustRun, 'win toolchain env'),
        api.post_process(
            post_process.StepCommandContains, 'win toolchain env', [
                'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-'
                '2.on.aws/windows-hermetic-toolchain/'
            ]),
        api.post_process(post_process.MustRun, 'resolve win toolchain hash'),
        api.post_process(post_process.DoesNotRun, 'win toolchain hash env'),
        api.post_process(post_process.StatusSuccess),
    )
    # Brave has already republished a toolchain for the upstream hash ->
    # GYP_MSVS_HASH_<upstream hash> gets pinned to the published one.
    yield api.test(
        'windows toolchain hash pinned',
        api.platform.name('win'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.existing_checkout(),
        api.chromium_checkout.git_cache_populated(),
        api.chromium_checkout.win_toolchain_published(_TEST_TOOLCHAIN_HASH,
                                                      'cafef00dcafe'),
        api.properties(chromium_ref='main'),
        api.post_process(post_process.MustRun, 'win toolchain hash env'),
        api.post_process(post_process.StepCommandContains,
                         'win toolchain hash env', ['cafef00dcafe']),
        api.post_process(post_process.StatusSuccess),
    )
