# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `cas` module."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import cas, path, step
from recipe_test_api import RecipeTestApi

SERVICE = 'rbe.example.com:443'


@dataclass
class DEPS(RecipeScriptApi):
    cas: cas.API
    path: path.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    cas: cas.TEST_API


def RunSteps(api: DEPS):
    out = api.path.chromium_src / 'out' / 'Release'
    digest = api.cas.archive(
        'archive foo_unittests',
        api.path.chromium_src,
        ['./foo_unittests', '../../brave/test/data/'],
        cwd=out,
    )
    with api.cas.with_instance('tests'):
        assert api.cas.instance == 'tests'
        api.cas.download('download foo_unittests', digest, api.path.mkdtemp())
    assert api.cas.instance == 'default'


def GenTests(api: TEST_DEPS):
    yield api.test(
        'basic',
        api.cas.configured(SERVICE),
        api.cas.archived('archive foo_unittests', 'a' * 64, 498),
        api.post_process(
            post_process.StepCommandRE,
            'archive foo_unittests.stage',
            [
                r'.*vpython3',
                r'.*stage\.py',
                '--root',
                r'.*b/src',
                '--cwd',
                r'.*b/src/out/Release',
                '--stage',
                r'.*',
                '--paths-json',
                r'(?s).*',
            ],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'archive foo_unittests.upload',
            ['--service', SERVICE, '--instance', 'default'],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'download foo_unittests',
            ['--instance', 'tests', '--digest', f'{"a" * 64}/498'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'no credentials',
        api.properties(**{'$cas': {'service': SERVICE}}),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
