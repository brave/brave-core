# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `git` module."""

from __future__ import annotations

import post_process

DEPS = ['git', 'step']


def RunSteps(api):
    api.git.disable_auto_gc('/b/checkout')
    api.git.disable_auto_gc('/b/cache/some-mirror',
                            step_name='disable mirror auto-gc')


def GenTests(api):
    yield api.test(
        'disables every auto-gc knob',
        api.post_process(post_process.StepCommandContains,
                         'disable auto-gc: gc.auto=0',
                         ['config', 'gc.auto', '0']),
        api.post_process(post_process.StepCommandContains,
                         'disable auto-gc: gc.autodetach=0',
                         ['config', 'gc.autodetach', '0']),
        api.post_process(post_process.StepCommandContains,
                         'disable auto-gc: gc.autopacklimit=0',
                         ['config', 'gc.autopacklimit', '0']),
        api.post_process(post_process.StepCommandContains,
                         'disable auto-gc: maintenance.gc.enabled=false',
                         ['config', 'maintenance.gc.enabled', 'false']),
        # Every step runs with `cwd` set to the repo, not `--git-dir`, so the
        # same call works against a working tree or a bare mirror alike.
        api.post_process(post_process.StepCommandDoesNotContain,
                         'disable auto-gc: gc.auto=0', ['--git-dir']),
        # A custom step_name prefix keeps repeat calls (e.g. before/after
        # another step) distinguishable in the step list.
        api.post_process(
            post_process.MustRun,
            'disable mirror auto-gc: maintenance.gc.enabled=false'),
        api.post_process(post_process.StatusSuccess),
    )
