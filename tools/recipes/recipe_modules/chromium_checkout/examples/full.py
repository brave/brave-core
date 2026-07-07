# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `chromium_checkout` module."""

from __future__ import annotations

from dataclasses import dataclass

import post_process

DEPS = ['chromium_checkout']


@dataclass(frozen=True)
class InputProperties:
    chromium_ref: str = 'main'


PROPERTIES = InputProperties


def RunSteps(api, properties):
    api.chromium_checkout.ensure_checkout(ref=properties.chromium_ref)


def GenTests(api):
    # No existing checkout -> clone, then check out a release tag.
    yield api.test(
        'fresh tag',
        api.chromium_checkout.with_git_cache(),
        api.properties(chromium_ref='151.0.7917.1'),
        api.post_process(post_process.MustRun, 'fetch chromium'),
        api.post_process(post_process.MustRun, 'fetch tag'),
        api.post_process(post_process.MustRun, 'gclient sync'),
        api.post_process(post_process.StatusSuccess),
    )
    # Existing checkout (chrome/VERSION present) + a branch ref -> no clone,
    # `fetch ref` rather than `fetch tag`.
    yield api.test(
        'existing branch',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.existing_checkout(),
        api.properties(chromium_ref='main'),
        api.post_process(post_process.MustRun, 'check chrome/VERSION'),
        api.post_process(post_process.DoesNotRun, 'fetch chromium'),
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
