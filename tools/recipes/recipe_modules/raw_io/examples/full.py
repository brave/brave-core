# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `raw_io` module's input/output placeholders."""

from __future__ import annotations

import post_process

DEPS = ['path', 'raw_io', 'step']


def RunSteps(api):
    # Read a command's stdout and stderr.
    result = api.step('echo', ['echo', 'Hello World'],
                      stdout=api.raw_io.output_text(),
                      stderr=api.raw_io.output_text())
    assert result.stdout == 'Hello World\n', result.stdout
    assert result.stderr == '', result.stderr

    # Feed a command's stdin, and read what it writes back.
    result = api.step('cat', ['cat'],
                      stdin=api.raw_io.input_text('hello'),
                      stdout=api.raw_io.output_text('.out'))
    assert result.stdout == 'hello', result.stdout

    # The same data as a command-line argument instead: the placeholder becomes
    # the path of a file holding it.
    result = api.step('cat file',
                      ['cat', api.raw_io.input_text('hello 💩')],
                      stdout=api.raw_io.output_text('.out'))
    assert result.stdout == 'hello 💩', result.stdout

    # `\xe2` is not valid UTF-8, so it has to travel as bytes.
    result = api.step('cat bytes', ['cat'],
                      stdin=api.raw_io.input(b'\xe2hello'),
                      stdout=api.raw_io.output())
    assert result.stdout == b'\xe2hello', result.stdout

    # `input` takes text (encoding it) and `input_text` takes bytes (decoding
    # them), for callers that don't get to choose which they have.
    api.step(
        'cat coerced',
        ['cat',
         api.raw_io.input('text'),
         api.raw_io.input_text(b'bytes')])

    # A step can carry its own default simulated output, so the common case
    # needs nothing seeded per test.
    result = api.step(
        'automock', ['echo', 'huh'],
        stdout=api.raw_io.output_text('.out'),
        step_test_data=lambda: api.raw_io.test_api.stream_output_text('huh\n'))
    assert result.stdout == 'huh\n', result.stdout

    result = api.step('automock stderr', ['sh', '-c', 'echo fail 1>&2'],
                      stderr=api.raw_io.output('.err'),
                      step_test_data=lambda: api.raw_io.test_api.stream_output(
                          b'fail\n', 'stderr'))
    assert result.stderr == b'fail\n', result.stderr

    # An output placeholder in the command line is filed onto the result under
    # the module and method that produced it.
    result = api.step('write file', ['write', api.raw_io.output_text('.txt')])
    assert result.raw_io.output_text == 'written', result.raw_io.output_text

    # Several placeholders from one method are told apart by name.
    result = api.step('write two', [
        'write',
        api.raw_io.output_text(name='a'),
        api.raw_io.output_text(name='b')
    ])
    assert result.raw_io.output_texts == {
        'a': 'first',
        'b': 'second'
    }, result.raw_io.output_texts

    # `leak_to` writes to a known path and leaves it there, so a later step can
    # pick it up.
    leaked = api.path.workspace / 'leaked.txt'
    result = api.step(
        'leak output',
        ['write', api.raw_io.output_text(leak_to=leaked)])
    assert result.raw_io.output_text == 'leaked', result.raw_io.output_text

    # A leaked file the step never wrote reads back as None rather than raising.
    result = api.step(
        'leak nothing',
        ['write',
         api.raw_io.output_text(leak_to=leaked, name='missing')])
    assert result.raw_io.output_text is None, result.raw_io.output_text

    # `output_dir` collects a whole directory of results. The step is handed a
    # fresh temporary directory under the job's scratch space.
    result = api.step('dump files', ['dump_files', api.raw_io.output_dir()])
    outdir = result.raw_io.output_dir
    assert set(outdir) == {'some/file', 'other_file'}, set(outdir)
    assert len(outdir) == 2, len(outdir)
    # Each file is read on first access, and cached from then on.
    assert outdir['some/file'] == b'cool contents', outdir['some/file']
    assert outdir['some/file'] == b'cool contents'
    assert outdir['other_file'] == b'whatever', outdir['other_file']
    assert 'not_here' not in outdir

    # Deleting an entry hands its memory back and makes further reads an error.
    del outdir['some/file']
    assert 'some/file' not in outdir
    assert outdir.get('some/file') is None

    # `leak_to` works here too, for a directory a later step needs.
    result = api.step(
        'dump files to a known place',
        ['dump_files',
         api.raw_io.output_dir(leak_to=api.path.out / 'dumped')])
    assert dict(result.raw_io.output_dir) == {'report': b'all good'}


def GenTests(api):
    yield api.test(
        'basic',
        api.step_data('echo',
                      stdout=api.raw_io.output_text('Hello World\n'),
                      stderr=api.raw_io.output_text('')),
        api.step_data('cat', stdout=api.raw_io.output_text('hello')),
        api.step_data('cat file', stdout=api.raw_io.output_text('hello 💩')),
        api.step_data('cat bytes', stdout=api.raw_io.output(b'\xe2hello')),
        api.step_data('write file', api.raw_io.output_text('written')),
        api.step_data(
            'write two',
            api.raw_io.output_text('first', name='a'),
            api.raw_io.output_text('second', name='b'),
        ),
        api.step_data('leak output', api.raw_io.output_text('leaked')),
        api.step_data('leak nothing',
                      api.raw_io.backing_file_missing(name='missing')),
        api.step_data(
            'dump files',
            api.raw_io.output_dir({
                'some/file': b'cool contents',
                'other_file': b'whatever',
            })),
        api.step_data('dump files to a known place',
                      api.raw_io.output_dir({'report': b'all good'})),
        api.post_process(post_process.StatusSuccess),
    )
    # `output_text` also accepts (and decodes) bytes, for tests that don't get
    # to choose which they have.
    yield api.test(
        'bytes seeded as text',
        api.step_data('echo',
                      stdout=api.raw_io.output_text(b'Hello World\n'),
                      stderr=api.raw_io.output_text('')),
        api.step_data('cat', stdout=api.raw_io.output_text('hello')),
        api.step_data('cat file', stdout=api.raw_io.output_text('hello 💩')),
        api.step_data('cat bytes', stdout=api.raw_io.output(b'\xe2hello')),
        api.step_data('write file', api.raw_io.output_text('written')),
        api.step_data(
            'write two',
            api.raw_io.output_text('first', name='a'),
            api.raw_io.output_text('second', name='b'),
        ),
        api.step_data('leak output', api.raw_io.output_text('leaked')),
        api.step_data('leak nothing',
                      api.raw_io.backing_file_missing(name='missing')),
        api.step_data(
            'dump files',
            api.raw_io.output_dir({
                'some/file': b'cool contents',
                'other_file': b'whatever',
            })),
        api.step_data('dump files to a known place',
                      api.raw_io.output_dir({'report': b'all good'})),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
