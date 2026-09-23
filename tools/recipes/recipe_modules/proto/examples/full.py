# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `proto` module's placeholders and codecs."""

from __future__ import annotations

from PB.recipe_modules.brave.proto.examples.full import Config

import post_process

DEPS = ['path', 'proto', 'step']

CONFIG = Config(name='release', jobs=8)


def RunSteps(api):
    # `input` renders to the path of a file holding the encoded message; each
    # codec picks its own file extension.
    api.step('cat json', ['cat', api.proto.input(CONFIG, api.proto.JSONPB)])
    api.step('cat text', ['cat', api.proto.input(CONFIG, api.proto.TEXTPB)])
    api.step('cat binary', ['cat', api.proto.input(CONFIG, api.proto.BINARY)])

    # `output` goes the other way: the step writes the file, and the message
    # comes back parsed, filed under the module and method that produced it.
    for codec in (api.proto.JSONPB, api.proto.TEXTPB, api.proto.BINARY):
        result = api.step(f'read {codec}',
                          ['read', api.proto.output(Config, codec)])
        assert result.proto.output == CONFIG, result.proto.output

    # Several placeholders from one method are told apart by name.
    result = api.step('read two', [
        'read',
        api.proto.output(Config, api.proto.JSONPB, name='a'),
        api.proto.output(Config, api.proto.JSONPB, name='b')
    ])
    assert result.proto.outputs['a'].name == 'release'
    assert result.proto.outputs['b'].jobs == 4, result.proto.outputs['b']

    # A step that writes something unparseable, or nothing at all, results in
    # None rather than an exception.
    result = api.step(
        'read garbage',
        ['read', api.proto.output(Config, api.proto.JSONPB)])
    assert result.proto.output is None, result.proto.output
    result = api.step('read nothing', [
        'read',
        api.proto.output(Config,
                         api.proto.JSONPB,
                         leak_to=api.path.workspace / 'config.json')
    ])
    assert result.proto.output is None, result.proto.output

    # Codec keyword arguments reach the encoder and decoder. Here the default
    # `preserving_proto_field_name` is turned off on the way out, and unknown
    # fields are made fatal on the way in.
    api.step('cat camel', [
        'cat',
        api.proto.input(
            CONFIG, api.proto.JSONPB, preserving_proto_field_name=False)
    ])
    result = api.step('read strict', [
        'read',
        api.proto.output(Config, api.proto.JSONPB, ignore_unknown_fields=False)
    ])
    assert result.proto.output is None, result.proto.output

    # An output placeholder also works as a step's stdout.
    result = api.step('read stdout', ['read'],
                      stdout=api.proto.output(Config, api.proto.JSONPB))
    assert result.stdout == CONFIG, result.stdout

    # `encode`/`decode` are the same codecs without a step in between.
    encoded = api.proto.encode(CONFIG, api.proto.TEXTPB)
    assert api.proto.decode(encoded, Config, api.proto.TEXTPB) == CONFIG


def GenTests(api):
    # A test seeds the message itself; the placeholder knows its own codec, so
    # nothing here has to mention the encoding.
    reads = [
        api.step_data(f'read {codec}', api.proto.output(CONFIG))
        for codec in ('JSONPB', 'TEXTPB', 'BINARY')
    ]
    yield api.test(
        'basic',
        *reads,
        api.step_data(
            'read two',
            api.proto.output(CONFIG, name='a'),
            api.proto.output(Config(name='debug', jobs=4), name='b'),
        ),
        api.step_data('read garbage', api.proto.invalid()),
        api.step_data('read nothing', api.proto.backing_file_missing()),
        # A message carrying a field `Config` doesn't have: fatal here only
        # because the step asked for `ignore_unknown_fields=False`.
        api.step_data('read strict',
                      api.proto.invalid('{"name": "release", "extra": 1}')),
        api.step_data('read stdout', stdout=api.proto.output(CONFIG)),
        api.post_process(post_process.StatusSuccess),
    )
    # The test api exposes the same `encode`/`decode` the module does, for a
    # test that needs the encoded form itself.
    yield api.test(
        'encoded by hand',
        *reads,
        api.step_data(
            'read two',
            api.proto.output(CONFIG, name='a'),
            api.proto.output(Config(name='debug', jobs=4), name='b'),
        ),
        api.step_data(
            'read garbage',
            api.proto.invalid(api.proto.encode(CONFIG, 'JSONPB')[:-1])),
        api.step_data('read nothing', api.proto.backing_file_missing()),
        api.step_data('read stdout', stdout=api.proto.output(CONFIG)),
        api.step_data(
            'read strict',
            api.proto.invalid(
                api.proto.encode(
                    api.proto.decode('name: "release"', Config, 'TEXTPB'),
                    'JSONPB').replace('}', ', "extra": 1}'))),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
