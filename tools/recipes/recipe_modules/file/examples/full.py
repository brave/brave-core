# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising `file`'s content reads and writes, and `_run`'s
core `{ok, errno_name, message}` handling. See `examples/operations.py` for the
rest of the module's operations.
"""

from __future__ import annotations

from PB.recipe_modules.brave.file.examples.full import Greeting

import post_process

DEPS = ['file', 'step']


def RunSteps(api):
    # Each read takes a `test_data`: the result the step reports under
    # simulation, so a test only has to seed the reads it wants to steer.
    text = api.file.read_text('read greeting',
                              '/etc/greeting',
                              test_data='hello\n')
    api.step('echo', ['echo', text])

    raw = api.file.read_raw('read raw greeting',
                            '/etc/greeting',
                            test_data=b'hello bytes')
    api.step('echo raw', ['echo', raw.decode('utf-8')])

    config = api.file.read_json('read config',
                                '/etc/config.json',
                                test_data={'key': 'value'})
    api.step('echo config', ['echo', str(config)])

    greeting = api.file.read_proto('read proto greeting',
                                   '/etc/greeting.json',
                                   Greeting,
                                   'JSONPB',
                                   test_proto=Greeting(text='hello proto'))
    api.step('echo proto', ['echo', greeting.text])

    # Left to itself, a `read_proto` returns an empty message under simulation.
    empty = api.file.read_proto('read empty proto', '/etc/empty.json',
                                Greeting, 'JSONPB')
    api.step('echo empty proto', ['echo', repr(empty.text)])

    # Writes carry their content into the step, so there is nothing to seed.
    api.file.write_text('write greeting', '/tmp/greeting', 'hello again\n')
    api.file.write_raw('write raw greeting', '/tmp/greeting.bin',
                       b'\x00binary')
    api.file.write_json('write config',
                        '/tmp/config.json', {'key': 'value'},
                        indent=2)
    api.file.write_proto('write proto greeting', '/tmp/greeting.json',
                         Greeting(text='bye proto'), 'JSONPB')


def GenTests(api):
    # Happy path: the recipe's own `test_data` supplies each read's result,
    # which flows through to the step after it.
    yield api.test(
        'basic',
        api.post_process(post_process.StepCommandContains, 'echo',
                         ['echo', 'hello\n']),
        api.post_process(post_process.StepCommandContains, 'echo raw',
                         ['echo', 'hello bytes']),
        api.post_process(post_process.StepCommandContains, 'echo config',
                         ['echo', "{'key': 'value'}"]),
        api.post_process(post_process.StepCommandContains, 'echo proto',
                         ['echo', 'hello proto']),
        api.post_process(post_process.StatusSuccess),
    )
    # A test can override a read's default by seeding the step directly.
    yield api.test(
        'seeded reads',
        api.step_data('read greeting', api.file.read_text('seeded\n')),
        api.step_data('read raw greeting', api.file.read_raw(b'seeded bytes')),
        api.step_data('read config', api.file.read_json({'seeded': True})),
        api.step_data('read proto greeting',
                      api.file.read_proto(Greeting(text='seeded proto'))),
        api.post_process(post_process.StepCommandContains, 'echo',
                         ['echo', 'seeded\n']),
        api.post_process(post_process.StepCommandContains, 'echo raw',
                         ['echo', 'seeded bytes']),
        api.post_process(post_process.StepCommandContains, 'echo config',
                         ['echo', "{'seeded': True}"]),
        api.post_process(post_process.StepCommandContains, 'echo proto',
                         ['echo', 'seeded proto']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # A filesystem-level failure raises Error, a checked-step (non-infra)
    # failure.
    yield api.test(
        'not found',
        api.step_data('read greeting', api.file.errno('ENOENT')),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )
