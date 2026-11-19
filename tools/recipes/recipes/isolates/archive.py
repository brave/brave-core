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
from recipe_modules import cas, context, depot_tools, file, path, step
from recipe_test_api import RecipeTestApi

# Brave's suites, on top of Chromium's.
ISOLATE_MAP = 'brave/infra/config/generated/testing/gn_isolate_map.pyl'


@dataclass
class DEPS(RecipeScriptApi):
    cas: cas.API
    context: context.API
    depot_tools: depot_tools.API
    file: file.API
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
    build_dir = src / properties.build_dir
    isolates = {}
    for target in properties.targets:
        with api.step.nest(target):
            with api.context(cwd=src):
                api.step(
                    'mb isolate',
                    [
                        api.depot_tools.vpython3(),
                        src / 'tools/mb/mb.py',
                        'isolate',
                        '--no-build',
                        properties.build_dir,
                        target,
                        '--isolate-map-file',
                        src / ISOLATE_MAP,
                    ],
                )
            isolate = api.file.read_json(
                'read isolate',
                build_dir / f'{target}.isolate',
                test_data={
                    'variables': {
                        'command': [f'./{target}'],
                        'files': [f'./{target}', '../../testing/test_env.py'],
                    }
                },
            )['variables']
            digest = api.cas.archive(
                'archive', src, isolate['files'], cwd=build_dir
            )
        isolates[target] = {
            'digest': digest,
            'command': isolate['command'],
            'cwd': properties.build_dir,
        }
    api.step('isolates', None).presentation.properties['isolates'] = isolates


def GenTests(api: TEST_DEPS):
    yield api.test(
        'basic',
        api.properties(
            build_dir='out/Release',
            targets=['brave_unit_tests', 'brave_browser_tests'],
        ),
        api.cas.configured(),
        api.cas.archived('brave_unit_tests.archive', 'a' * 64),
        api.post_process(
            post_process.StepCommandContains,
            'brave_unit_tests.mb isolate',
            ['isolate', '--no-build', 'out/Release', 'brave_unit_tests'],
        ),
        api.post_process(post_process.MustRun, 'brave_browser_tests.archive'),
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
            'brave_cert_pinning_tests.archive.stage',
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
        'no targets',
        api.properties(build_dir='out/Release'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
