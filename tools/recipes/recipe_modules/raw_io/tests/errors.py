# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `raw_io`'s rejection of data (and test data) of the wrong type.

A seeded `MODE` env var selects which mistake to make. Each one aborts the
recipe, which is the point: a placeholder handed something it cannot carry is a
bug in the recipe (or in the test), not a step failure to recover from.
"""

from __future__ import annotations

import post_process

DEPS = ['env', 'raw_io', 'step']

# Each mode, and the stdout placeholder the step should use for it.
MODES = {
    # `input` carries bytes (or text it can encode), not arbitrary objects.
    'input_not_bytes': None,
    # Likewise `input_text` carries text, or bytes it can decode.
    'input_text_not_text': None,
    # A placeholder's name is what tells several of them apart, so it has to be
    # a string.
    'bad_placeholder_name': None,
    # A text placeholder seeded with bytes (and vice versa): the test data and
    # the placeholder disagree about what the step produces.
    'text_placeholder_given_bytes': 'output_text',
    'bytes_placeholder_given_text': 'output',
    # A step's own default simulated data has to be of the right type too.
    'output_test_data_not_bytes': 'output',
    'output_text_test_data_not_text': 'output_text',
    # An `output_dir` placeholder stands for a directory, not a file, so it
    # cannot be a step's stdout.
    'output_dir_on_stdout': None,
    # ... and its test data is a mapping of relative path to bytes.
    'output_dir_test_data_not_a_dict': None,
    'output_dir_test_data_bad_path': None,
    'output_dir_test_data_not_bytes': None,
}

# The test data each `output_dir_test_data_*` mode feeds the placeholder.
BAD_OUTPUT_DIR_DATA = {
    'output_dir_test_data_not_a_dict': ['some/file'],
    'output_dir_test_data_bad_path': {
        1: b'contents'
    },
    'output_dir_test_data_not_bytes': {
        'some/file': 'text, not bytes'
    },
}


def RunSteps(api):
    mode = api.env.get('MODE')
    if mode == 'input_not_bytes':
        api.step('cat', ['cat', api.raw_io.input(123)])
    elif mode == 'input_text_not_text':
        api.step('cat', ['cat', api.raw_io.input_text(123)])
    elif mode == 'bad_placeholder_name':
        api.step('cat', ['cat', api.raw_io.output_text(name=123)])
    elif mode == 'output_test_data_not_bytes':
        api.step('cat', ['cat'],
                 stdout=api.raw_io.output(),
                 step_test_data=lambda: api.raw_io.test_api.output(123))
    elif mode == 'output_text_test_data_not_text':
        api.step('cat', ['cat'],
                 stdout=api.raw_io.output_text(),
                 step_test_data=lambda: api.raw_io.test_api.output_text(123))
    elif mode == 'output_dir_on_stdout':
        api.step('cat', ['cat'], stdout=api.raw_io.output_dir())
    elif mode in BAD_OUTPUT_DIR_DATA:
        api.step('dump', ['dump_files', api.raw_io.output_dir()],
                 step_test_data=lambda: api.raw_io.test_api.output_dir(
                     BAD_OUTPUT_DIR_DATA[mode]))
    else:
        api.step('cat', ['cat'], stdout=getattr(api.raw_io, MODES[mode])())


def GenTests(api):
    mismatched = {
        'text_placeholder_given_bytes': api.raw_io.output(b'bytes'),
        'bytes_placeholder_given_text': api.raw_io.output_text('text'),
    }
    for mode in MODES:
        yield api.test(
            mode,
            api.env.set('MODE', mode),
            *([api.step_data('cat', stdout=mismatched[mode])]
              if mode in mismatched else []),
            api.post_process(post_process.StatusException),
            api.post_process(post_process.DropExpectation),
            status='EXCEPTION',
        )
