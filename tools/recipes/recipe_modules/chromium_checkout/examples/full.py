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

# An arbitrary 40-hex-char stand-in for a real Chromium commit hash.
_TEST_COMMIT_HASH = 'ef35003457e93c278f911a334b06e4a5f8967e06'


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
    # No existing checkout -> clone via a shared git-cache mirror (populated
    # with the release tag up front) straight onto the tag, skipping a
    # separate `origin/HEAD` checkout and re-fetch.
    yield api.test(
        'fresh tag',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.properties(chromium_ref='151.0.7917.1'),
        api.post_process(post_process.MustRun, 'gclient config'),
        api.post_process(post_process.MustRun, 'git cache populate'),
        api.post_process(post_process.MustRun, 'clone from git cache'),
        api.post_process(post_process.MustRun, 'checkout tag'),
        api.post_process(post_process.DoesNotRun, 'checkout origin/HEAD'),
        api.post_process(post_process.DoesNotRun,
                         'git cache populate for ref'),
        api.post_process(post_process.DoesNotRun, 'fetch tag'),
        api.post_process(post_process.MustRun, 'gclient sync'),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate',
                         ['--ref', 'refs/tags/151.0.7917.1']),
        api.post_process(post_process.StepCommandContains, 'checkout tag',
                         ['refs/tags/151.0.7917.1']),
        api.post_process(post_process.StatusSuccess),
    )
    # No existing checkout + a fully-qualified ref outside `refs/heads/*` --
    # Chromium's actual release branches live under `refs/branch-heads/*`, not
    # `refs/heads/*`. The mirror is populated with it up front like a tag,
    # but unlike a tag a plain clone never copies it in and it can't be
    # DWIM-resolved by name afterwards, so it needs an explicit `fetch` +
    # `checkout FETCH_HEAD`, same as the existing-checkout path below.
    yield api.test(
        'fresh release branch',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.properties(chromium_ref='refs/branch-heads/6834'),
        api.post_process(post_process.MustRun, 'clone from git cache'),
        api.post_process(post_process.MustRun, 'fetch ref'),
        api.post_process(post_process.MustRun, 'checkout ref'),
        api.post_process(post_process.MustRun, 'gclient sync'),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate',
                         ['--ref', 'refs/branch-heads/6834']),
        api.post_process(post_process.StepCommandContains, 'fetch ref',
                         ['refs/branch-heads/6834']),
        api.post_process(post_process.StepCommandContains, 'checkout ref',
                         ['FETCH_HEAD']),
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
        api.post_process(post_process.StepCommandContains,
                         'git cache populate for ref', ['--ref', 'main']),
        api.post_process(post_process.StatusSuccess),
    )
    # Existing checkout + a release tag -> re-fetches the tag into the mirror
    # and fetches it directly (the "unknown current state" path `checkout_ref`
    # still needs; unlike a fresh clone, `ref`'s tag-ness can't be resolved up
    # front here).
    yield api.test(
        'existing tag',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.existing_checkout(),
        api.chromium_checkout.git_cache_populated(),
        api.properties(chromium_ref='151.0.7917.1'),
        api.post_process(post_process.DoesNotRun, 'clone from git cache'),
        api.post_process(post_process.MustRun, 'point origin at git cache'),
        api.post_process(post_process.MustRun, 'fetch tag'),
        api.post_process(post_process.MustRun, 'gclient sync'),
        api.post_process(post_process.StatusSuccess),
    )
    # No `ref` requested on a fresh checkout -> clone straight onto
    # `origin/HEAD`; no ref-specific work (toolchain pin, `gclient sync`)
    # runs since there's no ref to sync dependencies for.
    yield api.test(
        'fresh no ref',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.post_process(post_process.MustRun, 'clone from git cache'),
        api.post_process(post_process.MustRun, 'checkout origin/HEAD'),
        api.post_process(post_process.DoesNotRun, 'checkout tag'),
        api.post_process(post_process.DoesNotRun, 'checkout ref'),
        api.post_process(post_process.DoesNotRun, 'gclient sync'),
        api.post_process(post_process.StatusSuccess),
    )
    # No existing checkout + a bare commit hash -> the mirror is populated
    # with `--commit <hash>` (a hash isn't a valid `--ref` refspec), then the
    # clone checks it out directly, same as a tag.
    yield api.test(
        'fresh commit',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.properties(chromium_ref=_TEST_COMMIT_HASH),
        api.post_process(post_process.MustRun, 'clone from git cache'),
        api.post_process(post_process.MustRun, 'checkout commit'),
        api.post_process(post_process.DoesNotRun, 'checkout tag'),
        api.post_process(post_process.MustRun, 'gclient sync'),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate',
                         ['--commit', _TEST_COMMIT_HASH]),
        api.post_process(post_process.StepCommandContains, 'checkout commit',
                         [_TEST_COMMIT_HASH]),
        api.post_process(post_process.StatusSuccess),
    )
    # Existing checkout + a bare commit hash -> re-fetches it into the
    # mirror via `--commit`, then fetches/checks it out the same way a
    # branch would be.
    yield api.test(
        'existing commit',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.existing_checkout(),
        api.chromium_checkout.git_cache_populated(),
        api.properties(chromium_ref=_TEST_COMMIT_HASH),
        api.post_process(post_process.DoesNotRun, 'clone from git cache'),
        api.post_process(post_process.MustRun, 'point origin at git cache'),
        api.post_process(post_process.MustRun, 'fetch commit'),
        api.post_process(post_process.DoesNotRun, 'fetch ref'),
        api.post_process(post_process.MustRun, 'gclient sync'),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate for ref',
                         ['--commit', _TEST_COMMIT_HASH]),
        api.post_process(post_process.StatusSuccess),
    )
    # Existing checkout + no `ref` requested -> nothing to do at all: no
    # fetch, no toolchain pin, no `gclient sync`.
    yield api.test(
        'existing no ref',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.existing_checkout(),
        api.post_process(post_process.MustRun, 'check chrome/VERSION'),
        api.post_process(post_process.DoesNotRun, 'clone from git cache'),
        api.post_process(post_process.DoesNotRun, 'point origin at git cache'),
        api.post_process(post_process.DoesNotRun, 'gclient sync'),
        api.post_process(post_process.StatusSuccess),
    )
    # No git cache configured -> git_cache.validate() raises.
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
