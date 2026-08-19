# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `json` module's placeholders and encoding."""

from __future__ import annotations

from google.protobuf import struct_pb2

import post_process

DEPS = ['json', 'path', 'step']


def RunSteps(api):
    # Read a step's stdout as JSON: the value comes back parsed.
    result = api.step('echo list', ['echo', '[1, 2, 3]'],
                      stdout=api.json.output())
    assert result.stdout == [1, 2, 3], result.stdout

    # A step can carry its own default simulated output, so the common case
    # needs nothing seeded per test.
    result = api.step(
        'echo more', ['echo', '[2, 3, 4]'],
        stdout=api.json.output(),
        step_test_data=lambda: api.json.test_api.output_stream([2, 3, 4]))
    assert result.stdout == [2, 3, 4], result.stdout

    # An output placeholder in the command line is filed onto the result under
    # the module and method that produced it.
    result = api.step('run tests', ['run_tests', api.json.output()])
    assert result.json.output == {'passed': 791}, result.json.output

    # Several placeholders from one method are told apart by name.
    result = api.step('run more tests', [
        'run_tests',
        api.json.output(name='fast'),
        api.json.output(name='slow')
    ])
    assert result.json.outputs == {
        'fast': [1, 2, 3],
        'slow': ['x', 'y']
    }, result.json.outputs

    # `input` goes the other way: the placeholder becomes the path of a file
    # holding the rendered JSON.
    config = {'y': 2, 'x': 1}
    api.step('cat config', ['cat', api.json.input(config)])
    # ... and with insertion order preserved, for a step that cares.
    api.step('cat unsorted config',
             ['cat', api.json.input(config, sort_keys=False)])

    # A step that writes something that isn't JSON, or nothing at all, results
    # in None rather than an exception.
    result = api.step('write garbage', ['write', api.json.output()])
    assert result.json.output is None, result.json.output
    result = api.step(
        'write nothing',
        ['write',
         api.json.output(leak_to=api.path.workspace / 'out.json')])
    assert result.json.output is None, result.json.output

    # Encoding: `dumps` handles the types recipes pass around routinely; other
    # types raise `TypeError`, same as stdlib's `json.dumps` (see
    # `recipe_modules/json/tests/errors.py`).
    assert api.json.dumps(api.path.workspace) == '"[WORKSPACE]"'
    assert api.json.dumps(
        struct_pb2.Struct(fields={'foo': struct_pb2.Value(
            string_value='bar')})) == ('{"foo": "bar"}')

    # Decoding: a whole float that arrived where an int was meant comes back as
    # an int, at any depth.
    assert api.json.loads('[1.0, 2.5, {"a": 3.0}, "s"]') == [
        1, 2.5, {
            'a': 3
        }, 's'
    ]
    # ... but not one too large to have been an int in the first place.
    assert api.json.loads('[1e300]') == [1e300]


def GenTests(api):
    yield api.test(
        'basic',
        # `api.json.loads`/`dumps` on the test api are the same functions the
        # module offers, for building the text a step is meant to have written.
        api.step_data('echo list',
                      stdout=api.json.output(
                          api.json.loads('[1.0, 2.0, 3.0]'))),
        api.step_data('run tests', api.json.output({'passed': 791})),
        api.step_data(
            'run more tests',
            api.json.output([1, 2, 3], name='fast'),
            api.json.output(['x', 'y'], name='slow'),
        ),
        api.step_data(
            'write garbage',
            # Valid JSON with its last character chopped off.
            api.json.invalid(api.json.dumps({'passed': 791})[:-1])),
        api.step_data('write nothing', api.json.backing_file_missing()),
        api.post_process(post_process.StatusSuccess),
    )
    # A step's default simulated output can be replaced outright.
    yield api.test(
        'override default',
        # `api.json.loads`/`dumps` on the test api are the same functions the
        # module offers, for building the text a step is meant to have written.
        api.step_data('echo list',
                      stdout=api.json.output(
                          api.json.loads('[1.0, 2.0, 3.0]'))),
        api.override_step_data('echo more', stdout=api.json.output([2, 3, 4])),
        api.step_data('run tests', api.json.output({'passed': 791})),
        api.step_data(
            'run more tests',
            api.json.output([1, 2, 3], name='fast'),
            api.json.output(['x', 'y'], name='slow'),
        ),
        api.step_data(
            'write garbage',
            # Valid JSON with its last character chopped off.
            api.json.invalid(api.json.dumps({'passed': 791})[:-1])),
        api.step_data('write nothing', api.json.backing_file_missing()),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
