# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `cipd` module."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import cipd, platform, step
from recipe_test_api import RecipeTestApi

PACKAGE = 'infra/rbe/client/${platform}'
VERSION = 're_client_version:0.187.0.ad10b508-gomaip'


@dataclass
class DEPS(RecipeScriptApi):
    cipd: cipd.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    platform: platform.TEST_API


def RunSteps(api: DEPS):
    remotetool = api.cipd.ensure_tool(PACKAGE, VERSION, 'remotetool')
    # A second call reuses the first's install.
    assert api.cipd.ensure_tool(PACKAGE, VERSION, 'remotetool') == remotetool
    api.step('use remotetool', [remotetool, '--help'])


def GenTests(api: TEST_DEPS):
    yield api.test(
        'linux',
        api.platform.name('linux'),
        api.post_process(
            post_process.StepCommandRE,
            f'install {PACKAGE}',
            [
                r'.*cipd',
                'ensure',
                '-root',
                r'.*cipd/infra_rbe_client/re_client_version_0\.187\.0.*',
                '-ensure-file',
                r'(?s).*',
            ],
        ),
        api.post_process(
            post_process.StepCommandRE,
            'use remotetool',
            [r'.*/remotetool', '--help'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'windows',
        api.platform.name('win'),
        api.post_process(
            post_process.StepCommandRE,
            f'install {PACKAGE}',
            [
                r'.*cipd\.bat',
                'ensure',
                '-root',
                r'.*',
                '-ensure-file',
                r'(?s).*',
            ],
        ),
        api.post_process(
            post_process.StepCommandRE,
            'use remotetool',
            [r'.*remotetool\.exe', '--help'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
