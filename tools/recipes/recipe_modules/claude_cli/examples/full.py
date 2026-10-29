# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `claude_cli` module.

Its expectations record the full command `run` builds, so that recipes using
the module only check what they pass to it.
"""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    claude_cli,
    env,
    path,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    claude_cli: claude_cli.API
    env: env.API
    path: path.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    claude_cli: claude_cli.TEST_API
    env: env.TEST_API


def RunSteps(api: DEPS):
    log_dir = api.path.abs(api.path.cleanup_dir) / 'claude'
    if api.env.get('MODE') == 'minimal':
        api.claude_cli.run('claude', 'Say hello.', log_dir=log_dir)
        return
    checkout = api.path.abs(api.path.chromium_src)
    api.claude_cli.run(
        'claude',
        'Fix the build.\n\nThe checkout is in `src`.',
        log_dir=log_dir,
        add_dirs=[checkout],
        allowed_tools=[f'Edit(/{checkout}/**)', f'Write(/{checkout}/**)'],
    )


def GenTests(api: TEST_DEPS):
    # Every option: extra dirs and pre-approved tools.
    yield api.test(
        'full',
        api.claude_cli.prompt_matches('claude', r'Fix the build\..*`src`\.'),
        api.post_process(post_process.StatusSuccess),
    )
    # Neither extra dirs nor pre-approved tools.
    yield api.test(
        'minimal',
        api.env.set('MODE', 'minimal'),
        api.claude_cli.prompt_matches('claude', 'Say hello.'),
        api.post_process(
            post_process.StepCommandDoesNotContain, 'claude', ['--add-dir']
        ),
        api.post_process(
            post_process.StepCommandDoesNotContain, 'claude', ['--allowedTools']
        ),
        api.post_process(post_process.StatusSuccess),
    )
    # A run Claude didn't finish fails its step.
    yield api.test(
        'failed',
        api.claude_cli.failed('claude'),
        api.post_process(post_process.StepFailure, 'claude'),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )
