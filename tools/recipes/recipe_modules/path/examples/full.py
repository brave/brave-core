# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `path` module's filesystem seams."""

from __future__ import annotations

import post_process

DEPS = ['path', 'step']


def RunSteps(api):
    # Probe a seeded file, then create and re-probe a directory: the created
    # directory must "exist" for the rest of the run.
    if api.path.exists(api.path.chromium_src / 'chrome/VERSION'):
        api.step('found version', ['echo', 'present'])
    api.path.mkdir(api.path.out)
    if api.path.is_dir(api.path.out):
        api.step('out ready', ['echo', str(api.path.out)])


def GenTests(api):
    yield api.test(
        'seeded',
        api.path.files('chromium/src/chrome/VERSION'),
        api.post_process(post_process.MustRun, 'found version'),
        api.post_process(post_process.MustRun, 'out ready'),
        api.post_process(post_process.StepCommandContains, 'out ready',
                         ['[WORKSPACE]/out']),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'absent',
        api.post_process(post_process.DoesNotRun, 'found version'),
        api.post_process(post_process.MustRun, 'out ready'),
        api.post_process(post_process.StatusSuccess),
    )
