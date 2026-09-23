# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `step`'s validation of the placeholders on its std handles.

A handle only accepts a placeholder pointing the right way: data goes *into*
`stdin` and comes *out of* `stdout`/`stderr`. A seeded `MODE` env var selects
which way to get it wrong; each one aborts the recipe.
"""

from __future__ import annotations

import post_process

DEPS = ['env', 'raw_io', 'step']


def RunSteps(api):
    if api.env.get('MODE') == 'output_on_stdin':
        api.step('cat', ['cat'], stdin=api.raw_io.output_text())
    else:
        api.step('cat', ['cat'], stdout=api.raw_io.input_text('hello'))


def GenTests(api):
    for mode in ('output_on_stdin', 'input_on_stdout'):
        yield api.test(
            mode,
            api.env.set('MODE', mode),
            api.post_process(post_process.StatusException),
            api.post_process(post_process.DropExpectation),
            status='EXCEPTION',
        )
