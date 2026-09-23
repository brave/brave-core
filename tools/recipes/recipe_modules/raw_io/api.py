# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `raw_io` module API: raw data in and out of steps.

This is the bottom of the placeholder stack: `input`/`input_text` write data to
a temporary file and hand the step its path; `output`/`output_text` hand the
step a path to write to and read it back once the step is done. `output_dir`
does the same for a whole directory of results. Every other module's
placeholders (`json`, `proto`) are these wearing a codec.

Deliberately not supported: `add_output_log` (mirroring the captured data into a
step log), which needs step presentation.
"""

from __future__ import annotations

from collections.abc import Iterator, Mapping
import errno
import os
from pathlib import Path
import tempfile
from typing import Any, Callable

from recipe_api import (InputPlaceholder, OutputPlaceholder, RecipeApi,
                        returns_placeholder)

# What an output placeholder renders to under simulation. Steps never really run
# there, so a fixed, obviously-fake path keeps expectations stable (and makes an
# accidentally-real read easy to spot).
SIM_TMP_PREFIX = '/path/to/tmp/'


class InputDataPlaceholder(InputPlaceholder):
    """Writes `data` (bytes) to a temp file and renders to its path."""

    def __init__(self,
                 data: bytes,
                 suffix: str = '',
                 name: str | None = None) -> None:
        self.data = data
        self.suffix = suffix
        self._backing_file: str | None = None
        super().__init__(name=name)

    @property
    def backing_file(self) -> str | None:
        return self._backing_file

    def render(self, test) -> list[str]:
        assert not self._backing_file, 'A placeholder can only be used once'
        if test.enabled:
            # Under simulation nothing reads the file, so pretend the data was
            # passed on the command line: an expectation then shows what the
            # step was fed rather than an opaque temp path.
            self._backing_file = self.readable_test_data
        else:  # pragma: no cover - production placeholder backend.
            input_fd, self._backing_file = tempfile.mkstemp(suffix=self.suffix)
            os.close(input_fd)
            self.write_data(self._backing_file)
        return [self._backing_file]

    def cleanup(self, test_enabled: bool) -> None:
        assert self._backing_file is not None
        if not test_enabled:  # pragma: no cover - production placeholder.
            _remove(self._backing_file)
        self._backing_file = None

    def write_data(self, path: str) -> None:  # pragma: no cover - production.
        Path(path).write_bytes(self.data)

    @property
    def readable_test_data(self) -> str:
        return self.data.decode('utf-8', errors='replace')


class InputTextPlaceholder(InputDataPlaceholder):
    """As `InputDataPlaceholder`, but the data is UTF-8 text."""

    def __init__(self,
                 data: str,
                 suffix: str = '',
                 name: str | None = None) -> None:
        super().__init__(data, suffix, name=name)
        assert isinstance(data, str)

    def write_data(self, path: str) -> None:  # pragma: no cover - production.
        Path(path).write_text(self.data, encoding='utf-8', errors='replace')

    @property
    def readable_test_data(self) -> str:
        return self.data


class OutputDataPlaceholder(OutputPlaceholder):
    """Renders to a path the step writes to, then reads it back as bytes."""

    def __init__(self,
                 suffix: str = '',
                 leak_to: str | Path | None = None,
                 name: str | None = None) -> None:
        self.suffix = suffix
        self.leak_to = leak_to
        self._backing_file: str | None = None
        super().__init__(name=name)

    @property
    def backing_file(self) -> str | None:
        return self._backing_file

    def render(self, test) -> list[str]:
        assert not self._backing_file, 'A placeholder can only be used once'
        if self.leak_to:
            self._backing_file = str(self.leak_to)
        elif test.enabled:
            self._backing_file = SIM_TMP_PREFIX + self.suffix.lstrip('.')
        else:  # pragma: no cover - production placeholder backend.
            output_fd, self._backing_file = tempfile.mkstemp(
                suffix=self.suffix)
            os.close(output_fd)
        return [self._backing_file]

    def result(self, test) -> Any:
        assert self._backing_file
        if test.enabled:
            self._backing_file = None
            # A `leak_to` placeholder seeded with no data simulates a step that
            # never wrote its output file. Without `leak_to` the backing file is
            # one this placeholder just created, so it is always there.
            if self.leak_to and test.data is None:
                return None
            return self.read_test_data(test)
        try:  # pragma: no cover - production placeholder backend.
            return self.read_data()
        except OSError as ex:  # pragma: no cover - production placeholder.
            if ex.errno != errno.ENOENT:
                raise
            return None
        finally:  # pragma: no cover - production placeholder backend.
            if not self.leak_to:
                _remove(self._backing_file)
            self._backing_file = None

    def read_data(self) -> bytes:  # pragma: no cover - production placeholder.
        return Path(self._backing_file).read_bytes()

    def read_test_data(self, test) -> bytes:
        data = test.data or b''
        if not isinstance(data, bytes):
            raise TypeError(f'test data must be binary data, got {data!r}')
        return data


class OutputTextPlaceholder(OutputDataPlaceholder):
    """As `OutputDataPlaceholder`, but decodes the data as UTF-8 text."""

    def read_data(self) -> str:  # pragma: no cover - production placeholder.
        # Any byte that isn't valid UTF-8 becomes U+FFFD rather than an error,
        # so a step emitting stray bytes doesn't sink the recipe.
        return Path(self._backing_file).read_text(encoding='utf-8',
                                                  errors='replace')

    def read_test_data(self, test) -> str:
        data = test.data or ''
        if not isinstance(data, str):
            raise TypeError(f'test data must be text data, got {data!r}')
        return data


class _LazyDirectoryReader(Mapping):
    """A read-only mapping of relative path -> file content, read on demand.

    What an `output_dir` placeholder hands back. A step can leave behind more
    output than a recipe will look at (or than fits comfortably in memory), so
    nothing is read until it is asked for. `del reader[path]` drops a file's
    content again, and makes further reads of it an error.
    """

    def __init__(self, paths: Iterator[str], read: Callable[[str],
                                                            bytes]) -> None:
        self._paths = set(paths)
        self._read = read
        self._content: dict[str, bytes] = {}

    def __getitem__(self, rel_path: str) -> bytes:
        if rel_path not in self._content:
            if rel_path not in self._paths:
                raise KeyError(rel_path)
            self._content[rel_path] = self._read(rel_path)
        return self._content[rel_path]

    def __delitem__(self, rel_path: str) -> None:
        self._paths.discard(rel_path)
        self._content.pop(rel_path, None)

    def __iter__(self) -> Iterator[str]:
        return iter(self._paths)

    def __len__(self) -> int:
        return len(self._paths)


class OutputDataDirPlaceholder(OutputPlaceholder):  # pylint: disable=abstract-method
    """Renders to a directory the step writes into, then reads it back.

    Unlike the other placeholders here this one is not backed by a single file,
    so it cannot stand in for a step's std handles.

    `backing_file` doesn't apply: `is_file_backed = False` means this
    placeholder is never treated as a single file.
    """

    is_file_backed = False

    def __init__(self,
                 path_api,
                 leak_to: str | Path | None = None,
                 name: str | None = None) -> None:
        self._path_api = path_api
        self._backing_dir = leak_to
        self._used = False
        super().__init__(name=name)

    def render(self, test) -> list[str]:
        assert not self._used, 'A placeholder can only be used once'
        self._used = True
        # Without `leak_to`, a fresh temporary directory under the job's scratch
        # space. `api.path.mkdtemp` is the seam here, so nothing is created on
        # disk in test mode and the name stays stable across runs.
        self._backing_dir = str(self._backing_dir
                                or self._path_api.mkdtemp('tmp'))
        if not test.enabled:  # pragma: no cover - production placeholder.
            os.makedirs(self._backing_dir, exist_ok=True)
        return [self._backing_dir]

    def result(self, test) -> _LazyDirectoryReader:
        assert self._used, 'A placeholder has to be rendered before it reads'
        if test.enabled:
            data = test.data or {}
            return _LazyDirectoryReader(iter(data), data.__getitem__)
        return self._read_backing_dir()  # pragma: no cover - production.

    def _read_backing_dir(self):  # pragma: no cover - production placeholder.
        paths = set()
        for dir_path, _dir_names, file_names in os.walk(self._backing_dir):
            for file_name in file_names:
                paths.add(
                    os.path.relpath(os.path.join(dir_path, file_name),
                                    self._backing_dir))

        def _read(rel_path: str) -> bytes:
            return Path(self._backing_dir, rel_path).read_bytes()

        return _LazyDirectoryReader(iter(paths), _read)


def _remove(path: str) -> None:  # pragma: no cover - production placeholder.
    """Delete *path*, tolerating its absence."""
    try:
        os.remove(path)
    except OSError as ex:
        if ex.errno != errno.ENOENT:
            raise


class RawIOApi(RecipeApi):
    """Placeholders carrying raw bytes (or UTF-8 text) in and out of steps."""

    @returns_placeholder
    def input(self,
              data: bytes,
              suffix: str = '',
              name: str | None = None) -> InputDataPlaceholder:
        """A placeholder expanding to the path of a file holding *data*.

        The engine writes *data* to a temporary file, passes its path to the
        step, and deletes it once the step is done.

        Args:
            data: The bytes to hand the step. Prefer `input_text` for text.
            suffix: Suffix for the temporary file's name, when the step cares
                what its input is called (e.g. `'.py'`).
            name: Distinguishes this placeholder from others produced by the
                same method on one step.
        """
        if isinstance(data, str):
            data = data.encode('utf-8', errors='replace')
        if not isinstance(data, bytes):
            raise ValueError(f'expected bytes, got {type(data)}: {data!r}')
        return InputDataPlaceholder(data, suffix, name=name)

    @returns_placeholder
    def input_text(self,
                   data: str,
                   suffix: str = '',
                   name: str | None = None) -> InputTextPlaceholder:
        """As `input`, but for UTF-8 text.

        Any character that cannot be encoded as UTF-8 is replaced with U+FFFD.
        """
        if isinstance(data, bytes):
            data = data.decode('utf-8', errors='replace')
        if not isinstance(data, str):
            raise ValueError(f'expected text, got {type(data)}: {data!r}')
        return InputTextPlaceholder(data, suffix, name=name)

    @returns_placeholder
    def output(self,
               suffix: str = '',
               leak_to: str | Path | None = None,
               name: str | None = None) -> OutputDataPlaceholder:
        """A placeholder expanding to a path the step is expected to write.

        Once the step is done the engine reads that file back as bytes and
        files it onto the step's result as `step_result.raw_io.output`. Also
        usable as a step's `stdout`/`stderr`, in which case its content is the
        result's `stdout`/`stderr`.

        Args:
            suffix: Suffix for the temporary file's name, when the step cares
                what its output is called (e.g. `'.json'`).
            leak_to: Write to this path instead of a temporary file, and do not
                delete it afterwards (i.e. "leak" it), so a later step can pick
                it up.
            name: Distinguishes this placeholder from others produced by the
                same method on one step.
        """
        return OutputDataPlaceholder(suffix, leak_to, name=name)

    @returns_placeholder
    def output_text(self,
                    suffix: str = '',
                    leak_to: str | Path | None = None,
                    name: str | None = None) -> OutputTextPlaceholder:
        """As `output`, but decodes the data the step wrote as UTF-8 text.

        Any byte that isn't valid UTF-8 is replaced with U+FFFD.
        """
        return OutputTextPlaceholder(suffix, leak_to, name=name)

    @returns_placeholder
    def output_dir(self,
                   leak_to: str | Path | None = None,
                   name: str | None = None) -> OutputDataDirPlaceholder:
        """A placeholder expanding to a directory the step writes into.

        Once the step is done, the result filed at
        `step_result.raw_io.output_dir` is a mapping of path (relative to the
        directory, with this platform's separator) to that file's bytes. The
        files are read lazily, on first access, so a step is free to leave more
        behind than the recipe looks at.

        Args:
            leak_to: Use this directory instead of a temporary one, and do not
                delete it afterwards (i.e. "leak" it). Created if absent.
            name: Distinguishes this placeholder from others produced by the
                same method on one step.

        Example:

            result = api.step('dump', ['dump_files', api.raw_io.output_dir()])

            # 'some/file' is read right here, and cached from now on.
            assert result.raw_io.output_dir['some/file'] == b'contents'

            # To hand that memory back (and make further reads an error):
            del result.raw_io.output_dir['some/file']
        """
        return OutputDataDirPlaceholder(self.m.path, leak_to, name=name)
