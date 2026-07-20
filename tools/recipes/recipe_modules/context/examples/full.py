# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `context` module's scoping."""

from __future__ import annotations

import post_process

DEPS = ['context', 'path', 'step']


def RunSteps(api):
    node_bin = api.path.workspace / 'node' / 'bin'

    # Prepend a dir to PATH and override a var for steps in this scope.
    with api.context(env_prefixes={'PATH': [node_bin]}, env={'CI': 'brave'}):
        api.step('inside context', ['node', '--version'])

        # Nested contexts compose: a second PATH prefix stacks in front, and
        # cwd applies only within the inner block.
        with api.context(env_prefixes={'PATH': ['/opt/extra']},
                         env_suffixes={'LD_LIBRARY_PATH': ['/opt/lib']},
                         cwd=api.path.out):
            api.step('nested context', ['node', 'build.js'])

    # Back outside every `with`: the ambient environment is restored.
    api.step('outside context', ['node', '--version'])


def GenTests(api):
    yield api.test(
        'basic',
        # The PATH prefix and env override are recorded on the scoped step.
        api.post_process(post_process.MustRun, 'inside context'),
        api.post_process(post_process.MustRun, 'nested context'),
        api.post_process(post_process.MustRun, 'outside context'),
        api.post_process(post_process.StatusSuccess),
    )
