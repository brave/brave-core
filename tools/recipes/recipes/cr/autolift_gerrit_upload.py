# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Upload the `cr<N>` branch autolift ran on to Gerrit, as a relation chain.

Runs after `cr/autolift` and then `gerrit/mirror_brave_core`, in the workspace
the lift ran in, on its brave-core checkout, which is on `cr<N>`. Brockit finds
where the branch's own commits start, and `gerrit_upload.py` pushes GitHub's
`cr<N>` to `refs/for/master` with topic `cr<N>`, and abandons the topic's CLs
whose commits left the branch.
"""

from __future__ import annotations

from dataclasses import dataclass
import re

import post_process
from PB.recipes.brave.cr.autolift_gerrit_upload import InputProperties
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    brave_gerrit,
    chromium_checkout,
    depot_tools,
    git,
    raw_io,
    step,
)
from recipe_test_api import RecipeTestApi

_CR_BRANCH = re.compile(r'cr\d+')
_BASE_COMMIT = re.compile(r'base commit: ([0-9a-f]{40})')


@dataclass
class DEPS(RecipeScriptApi):
    brave_core_checkout: brave_core_checkout.API
    brave_gerrit: brave_gerrit.API
    depot_tools: depot_tools.API
    git: git.API
    raw_io: raw_io.API
    step: step.API


PROPERTIES = InputProperties


def RunSteps(api: DEPS, properties: InputProperties) -> None:
    with api.brave_core_checkout.brave_core_layout():
        branch = api.git(
            'rev-parse',
            '--abbrev-ref',
            'HEAD',
            name='find branch',
            stdout=api.raw_io.output_text(),
        ).stdout.strip()
        if not _CR_BRANCH.fullmatch(branch):
            raise RuntimeError(
                f'the checkout is on {branch}, not on a cr<N> branch'
            )
        # Where `brockit rebase` would start replaying the branch: before its
        # last major bump, as its base is always on the previous milestone.
        shown = api.step(
            'find branch start',
            ['brockit', 'show', '--from-ref-hash=@previous-major'],
            stdout=api.raw_io.output_text(),
        ).stdout
        match = _BASE_COMMIT.search(shown)
        if not match:
            raise RuntimeError(f'brockit found no start for {branch}')
        cmd = [
            api.depot_tools.vpython3(),
            api.resource('gerrit_upload.py'),
            '--url',
            api.brave_gerrit.ssh_url(properties.gerrit_user),
            '--branch',
            branch,
            '--start',
            match[1],
            '--head',
            f'origin/{branch}',
        ]
        if properties.dry_run:
            cmd.append('--dry-run')
        api.step('upload', cmd)


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_core_checkout: brave_core_checkout.TEST_API
    chromium_checkout: chromium_checkout.TEST_API
    raw_io: raw_io.TEST_API


def GenTests(api: TEST_DEPS):
    start = 'f' * 40

    def checkout(branch: str = 'cr160', shown: str = f'base commit: {start}\n'):
        return (
            api.properties(gerrit_user='autolift-bot')
            # The workspace the lift ran in.
            + api.brave_core_checkout.with_git_cache()
            + api.chromium_checkout.existing_checkout()
            + api.step_data(
                'find branch', stdout=api.raw_io.output_text(f'{branch}\n')
            )
            + api.step_data(
                'find branch start', stdout=api.raw_io.output_text(shown)
            )
        )

    yield api.test(
        'uploads',
        checkout(),
        api.post_process(
            post_process.StepCommandContains,
            'upload',
            [
                '--url',
                'ssh://autolift-bot@gerrit-ssh.brave.com:29418/'
                'brave-browser/brave-core',
                '--branch',
                'cr160',
                '--start',
                start,
                '--head',
                'origin/cr160',
            ],
        ),
        api.post_process(
            post_process.StepCommandDoesNotContain, 'upload', ['--dry-run']
        ),
        api.post_process(post_process.StatusSuccess),
    )

    yield api.test(
        'dry run',
        checkout(),
        api.properties(dry_run=True),
        api.post_process(
            post_process.StepCommandContains, 'upload', ['--dry-run']
        ),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )

    yield api.test(
        'refused upload fails',
        checkout(),
        api.step_data('upload', retcode=1),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )

    yield api.test(
        'not on a cr branch',
        checkout(branch='master'),
        api.post_process(post_process.DoesNotRun, 'find branch start'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )

    yield api.test(
        'no branch start',
        checkout(shown='upstream version: 160.0.8158.0\n'),
        api.post_process(post_process.DoesNotRun, 'upload'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
