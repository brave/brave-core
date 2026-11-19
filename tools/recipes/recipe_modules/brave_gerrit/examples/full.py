# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `brave_gerrit` module."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_gerrit,
    step,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    brave_gerrit: brave_gerrit.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    pass


def RunSteps(api: DEPS):
    api.step('brave-core', ['echo', api.brave_gerrit.ssh_url('autolift-bot')])
    api.step(
        'other project',
        [
            'echo',
            api.brave_gerrit.ssh_url('autolift-bot', 'brave-browser/forks/x'),
        ],
    )


def GenTests(api: TEST_DEPS):
    yield api.test(
        'full',
        api.post_process(
            post_process.StepCommandContains,
            'brave-core',
            [
                'ssh://autolift-bot@gerrit-ssh.brave.com:29418/'
                'brave-browser/brave-core'
            ],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'other project',
            [
                'ssh://autolift-bot@gerrit-ssh.brave.com:29418/'
                'brave-browser/forks/x'
            ],
        ),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
