# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Run a test suite `isolates/archive` archived, with no checkout or build.

The suite's tree comes from CAS by its digest, and its command runs in the
tree's build directory.
"""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from PB.recipes.brave.isolates.run import InputProperties
from recipe_api import RecipeScriptApi
from recipe_modules import cas, context, depot_tools, path, step
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    cas: cas.API
    context: context.API
    depot_tools: depot_tools.API
    path: path.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    cas: cas.TEST_API


PROPERTIES = InputProperties


def RunSteps(api: DEPS, properties: InputProperties) -> None:
    if not properties.digest or not properties.command:
        raise ValueError('isolates/run needs `digest` and `command`')
    tree = api.path.mkdtemp('isolate')
    api.cas.download('download', properties.digest, tree)
    # Commands start with `vpython3`, which depot_tools puts on PATH.
    api.depot_tools.ensure_on_path()
    with api.context(cwd=tree / properties.cwd):
        api.step('run', list(properties.command))


def GenTests(api: TEST_DEPS):
    yield api.test(
        'basic',
        api.properties(
            digest=f'{"a" * 64}/498',
            command=['vpython3', '../../testing/test_env.py', './foo_tests'],
            cwd='out/Release',
        ),
        api.cas.configured(),
        api.post_process(
            post_process.StepCommandContains,
            'download',
            ['--digest', f'{"a" * 64}/498'],
        ),
        api.post_process(
            post_process.StepCommandRE,
            'run',
            ['vpython3', r'\.\./\.\./testing/test_env\.py', r'\./foo_tests'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'no digest',
        api.properties(command=['./foo_tests']),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
