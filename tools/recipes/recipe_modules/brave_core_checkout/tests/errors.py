# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the `brave_core_checkout` module's argument validation."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    env,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    brave_core_checkout: brave_core_checkout.API
    env: env.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_core_checkout: brave_core_checkout.TEST_API
    env: env.TEST_API


def RunSteps(api: DEPS):
    mode = api.env.get('MODE')
    if mode == 'dotenv':
        api.brave_core_checkout.set_config('brave')
        api.brave_core_checkout.ensure_checkout()
    elif mode == 'ref':
        api.brave_core_checkout.checkout()
    else:
        # deploy() requires at least one path.
        api.brave_core_checkout.deploy([])


def GenTests(api: TEST_DEPS):
    yield api.test(
        'empty paths',
        api.brave_core_checkout.with_git_cache(),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # A `brave_core_ref` git would read as an option is refused.
    yield api.test(
        'option as ref',
        api.env.set('MODE', 'ref'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.brave_core_ref('--upload-pack=evil'),
        api.post_process(post_process.DoesNotRun, 'git cache populate'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # A line break in a `.env` value, which would inject another entry, is
    # refused before anything is written.
    yield api.test(
        'line break in dotenv',
        api.env.set('MODE', 'dotenv'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.brave_core_checkout.rbe(siso_cache_dir='/b/siso\nis_asan=true'),
        api.post_process(post_process.DoesNotRun, 'write .env'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
