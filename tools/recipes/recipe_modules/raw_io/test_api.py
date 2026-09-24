# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `raw_io`: seed what an output placeholder hands back.

`api.raw_io.output_text('hello')` is a fragment for `api.step_data`, either as
one of its positional arguments (seeding an `api.raw_io.output_text()` in the
step's command) or as its `stdout=`/`stderr=` (seeding the step's std handle):

    api.step_data('greet', api.raw_io.output_text('hello'))
    api.step_data('greet', stdout=api.raw_io.output_text('hello\\n'))

`stream_output`/`stream_output_text` build the same thing in one call, for a
step's own `step_test_data=` default.

Input placeholders need no helpers: what they carry into a step comes from the
recipe, not from the test.
"""

from __future__ import annotations

from recipe_test_api import (RecipeTestApi, StepTestData,
                             placeholder_step_data)


class RawIOTestApi(RecipeTestApi):
    """Seed the simulated data of a `raw_io` output placeholder."""

    @placeholder_step_data
    def output(self,
               data: bytes | str | None,
               retcode: int | None = None,
               name: str | None = None):
        """Seed an `api.raw_io.output()` placeholder with *data* (bytes)."""
        if isinstance(data, str):
            data = data.encode('utf-8')
        if not isinstance(data, (type(None), bytes)):
            raise ValueError(f'expected bytes, got {type(data)}: {data!r}')
        return data, retcode, name

    @placeholder_step_data
    def output_text(self,
                    data: str | bytes | None,
                    retcode: int | None = None,
                    name: str | None = None):
        """Seed an `api.raw_io.output_text()` placeholder with *data* (text)."""
        if isinstance(data, bytes):
            data = data.decode('utf-8')
        if not isinstance(data, (type(None), str)):
            raise ValueError(f'expected text, got {type(data)}: {data!r}')
        return data, retcode, name

    def stream_output(self,
                      data: bytes | str | None,
                      stream: str = 'stdout',
                      retcode: int | None = None,
                      name: str | None = None) -> StepTestData:
        """Seed *data* as the step's `stdout` (or `stderr`), as bytes."""
        return self._stream_output(self.output(data,
                                               retcode=retcode,
                                               name=name),
                                   stream=stream,
                                   retcode=retcode)

    def stream_output_text(self,
                           data: str | bytes | None,
                           stream: str = 'stdout',
                           retcode: int | None = None,
                           name: str | None = None) -> StepTestData:
        """Seed *data* as the step's `stdout` (or `stderr`), as text."""
        return self._stream_output(self.output_text(data,
                                                    retcode=retcode,
                                                    name=name),
                                   stream=stream,
                                   retcode=retcode)

    @staticmethod
    def _stream_output(placeholder_data: StepTestData,
                       stream: str,
                       retcode: int | None = None) -> StepTestData:
        assert stream in ('stdout', 'stderr')
        fragment = StepTestData()
        setattr(fragment, stream, placeholder_data.unwrap_placeholder())
        if retcode:
            fragment.retcode = retcode
        return fragment

    @placeholder_step_data
    def output_dir(self,
                   files: dict[str, bytes],
                   retcode: int | None = None,
                   name: str | None = None):
        """Seed the files an `api.raw_io.output_dir()` placeholder finds.

        *files* maps a path relative to the directory to that file's bytes. Use
        the separator of the platform the test is targeting, since that is what
        the recipe will see.

        Example:

            api.step_data('dump', api.raw_io.output_dir({
                'some/file': b'contents of some/file',
            }))
        """
        if not isinstance(files, dict):
            raise ValueError(f'expected a dict, got {type(files)}: {files!r}')
        for key, value in files.items():
            if not isinstance(key, str):
                raise ValueError(f'expected a path, got {type(key)}: {key!r}')
            if not isinstance(value, bytes):
                raise ValueError(f'expected bytes for {key!r}, got '
                                 f'{type(value)}: {value!r}')
        return files, retcode, name

    @placeholder_step_data('output')
    def backing_file_missing(self,
                             retcode: int | None = None,
                             name: str | None = None):
        """Seed an output placeholder as if the step never wrote its file.

        Only meaningful for a placeholder created with `leak_to`; without it the
        engine creates the backing file itself, so it is always there.
        """
        # `None` data is what tells a placeholder to behave, under simulation,
        # as though its backing file were absent.
        return None, retcode, name
