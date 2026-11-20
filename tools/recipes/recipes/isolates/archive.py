# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Archive built test suites in CAS, each with what it needs to run.

Each suite's runtime dependencies come from `mb isolate`, and its tree goes to
CAS as one digest. The `isolates` output property holds, by suite, the digest
and the command `isolates/run` runs it with on another machine.
"""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from PB.recipes.brave.isolates.archive import InputProperties
from recipe_api import RecipeScriptApi
from recipe_modules import cas, isolate, path, step
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    isolate: isolate.API
    path: path.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    cas: cas.TEST_API


PROPERTIES = InputProperties


def RunSteps(api: DEPS, properties: InputProperties) -> None:
    if not properties.build_dir or not properties.targets:
        raise ValueError('isolates/archive needs `build_dir` and `targets`')
    src = (
        api.path.abs(properties.src_dir)
        if properties.src_dir
        else api.path.chromium_src
    )
    isolates = api.isolate.isolate_tests(
        src, properties.build_dir, properties.targets
    )
    if missing := [t for t in properties.targets if t not in isolates]:
        raise api.step.StepFailure(f'not archived: {", ".join(missing)}')


def GenTests(api: TEST_DEPS):
    yield api.test(
        'basic',
        api.properties(
            build_dir='out/Release',
            targets=['brave_unit_tests', 'brave_browser_tests'],
        ),
        api.cas.configured(),
        api.cas.archived('isolate tests.brave_unit_tests.archive', 'a' * 64),
        api.post_process(
            post_process.StepCommandContains,
            'isolate tests.brave_unit_tests.mb isolate',
            ['isolate', '--no-build', 'out/Release', 'brave_unit_tests'],
        ),
        api.post_process(
            post_process.MustRun, 'isolate tests.brave_browser_tests.archive'
        ),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'src dir',
        api.properties(
            src_dir='/checkout/src',
            build_dir='out/Component',
            targets=['brave_cert_pinning_tests'],
        ),
        api.cas.configured(),
        api.post_process(
            post_process.StepCommandRE,
            'isolate tests.brave_cert_pinning_tests.archive.stage',
            [
                r'.*vpython3',
                r'.*stage\.py',
                '--root',
                '/checkout/src',
                '--cwd',
                '/checkout/src/out/Component',
                '--stage',
                r'.*',
                '--paths-json',
                r'(?s).*',
            ],
        ),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'archive fails',
        api.properties(build_dir='out/Release', targets=['brave_unit_tests']),
        api.cas.configured(),
        api.step_data('isolate tests.brave_unit_tests.mb isolate', retcode=1),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )
    yield api.test(
        'no targets',
        api.properties(build_dir='out/Release'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
