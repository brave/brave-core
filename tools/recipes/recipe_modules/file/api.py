# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `file` module API: basic filesystem operations as recipe steps.

A recipe module has no seam onto the local filesystem's *content* (`path`
only ever answers exists/is_dir/is_file); anything that reads or mutates a
file's content has to go through a step like everything else that touches
the real machine, so it is simulatable. This module wraps a resource script
(`resources/fileutil.py`) run as that step, reporting back a shared
`{ok, errno_name, message}` result and an `Error` exception.

Content moves in and out through placeholders (see the "Getting data back
from a step" section of README.md), which is why reading and writing files
both come down to `copy`: `read_text` copies the file *to* an output
placeholder the engine then reads, and `write_text` copies *from* an input
placeholder the engine has already filled in. Every method that returns a
value also takes a `test_data` argument, giving the step a default result
under simulation so the common case needs nothing seeded per test.

Deliberately not supported: `include_log` (mirroring the content read or
written into a step log), which needs step presentation, and `symlink_tree`
(building up a tree of links to realize in one step).
"""

from __future__ import annotations

from collections.abc import Callable, Sequence
from pathlib import Path
import subprocess
from typing import Any, TypeVar

from google.protobuf.message import Message

import config_types
from recipe_api import OutputPlaceholder, Placeholder, RecipeApi
from recipe_test_api import StepTestData
from step_data import StepData

ProtoMessage = TypeVar('ProtoMessage', bound=Message)


def _as_path(
        source: str | Path | config_types.Path) -> Path | config_types.Path:
    """*source* as a Path, without re-wrapping an already-Path-like value.

    Re-wrapping a `config_types.Path` (simulated) in a real `pathlib.Path`
    would force it back onto the host's native separator.
    """
    if isinstance(source, (Path, config_types.Path)):
        return source
    return Path(source)


class FileApi(RecipeApi):
    """Basic filesystem operations (read, write, copy, remove, ...) as steps."""

    class Error(subprocess.CalledProcessError):
        """A `fileutil.py` operation reported a filesystem-level failure.

        Carries the failing operation's `errno_name` (e.g. `'ENOENT'`)
        alongside a human message (the inherited `.output`), so a filesystem
        error is distinguishable from an arbitrary non-zero step exit.
        Subclasses `CalledProcessError` so this engine's own checked-step-
        failure handling -- a `StatusFailure`, not an infra-level
        `StatusException` -- applies to it unchanged.
        """

        def __init__(self, step_name: str, errno_name: str,
                     message: str) -> None:
            super().__init__(1, step_name, output=message)
            self.errno_name = errno_name

    def _run(self,
             name: str,
             args: Sequence[str | Path | Placeholder],
             step_test_data: Callable[[], StepTestData] | None = None,
             stdout: OutputPlaceholder | None = None) -> StepData:
        """Run a `fileutil.py` operation, raising `Error` if it failed.

        The operation's `{ok, errno_name, message}` result comes back through a
        JSON output placeholder, leaving the step's stdout free for whatever the
        operation actually returns. Unless a caller says otherwise, a step
        defaults under simulation to reporting success (`test_api.errno`), so a
        test only has to seed the steps it cares about.
        """
        vpython3 = self.m.depot_tools.vpython3()
        cmd = [
            vpython3, '-u',
            self.resource('fileutil.py'), '--json-output',
            self.m.json.output(), *args
        ]
        result = self.m.step(name,
                             cmd,
                             step_test_data=step_test_data
                             or self.test_api.errno,
                             stdout=stdout)
        status = result.json.output
        if not status['ok']:
            raise self.Error(name, status['errno_name'], status['message'])
        return result

    def read_raw(self,
                 name: str,
                 source: str | Path,
                 test_data: bytes = b'') -> bytes:
        """Return the raw (binary) content of *source*.

        Args:
            name: The name of the step.
            source: Path, on the host the step runs on, of the file to read.
            test_data: What this step returns under simulation.

        Returns:
            The file's content, as bytes.

        Raises:
            Error: If the file could not be read.
        """
        result = self._run(
            name,
            ['copy', str(source), self.m.raw_io.output()],
            step_test_data=lambda: self.test_api.read_raw(test_data))
        return result.raw_io.output

    def write_raw(self, name: str, dest: str | Path, data: bytes) -> StepData:
        """Write raw (binary) *data* to *dest*.

        Args:
            name: The name of the step.
            dest: Path, on the host the step runs on, of the file to write.
            data: The bytes to write.

        Raises:
            Error: If the file could not be written.
        """
        return self._run(name, ['copy', self.m.raw_io.input(data), str(dest)])

    def read_text(self,
                  name: str,
                  source: str | Path,
                  test_data: str = '') -> str:
        """Return the UTF-8 text content of *source*.

        Args:
            name: The name of the step.
            source: Path, on the host the step runs on, of the file to read.
            test_data: What this step returns under simulation.

        Returns:
            The file's content.

        Raises:
            Error: If the file could not be read.
        """
        result = self._run(
            name, ['copy', str(source),
                   self.m.raw_io.output_text()],
            step_test_data=lambda: self.test_api.read_text(test_data))
        return result.raw_io.output_text

    def write_text(self, name: str, dest: str | Path,
                   text_data: str) -> StepData:
        """Write UTF-8 *text_data* to *dest*.

        Args:
            name: The name of the step.
            dest: Path, on the host the step runs on, of the file to write.
            text_data: The text to write.

        Raises:
            Error: If the file could not be written.
        """
        return self._run(
            name,
            ['copy', self.m.raw_io.input_text(text_data),
             str(dest)])

    def read_json(self,
                  name: str,
                  source: str | Path,
                  test_data: Any = None) -> Any:
        """Return the parsed JSON content of *source*.

        Args:
            name: The name of the step.
            source: Path, on the host the step runs on, of the file to read.
            test_data: What this step returns under simulation, as a
                JSON-serializable value.

        Returns:
            The file's content, parsed as JSON.

        Raises:
            Error: If the file could not be read.
        """
        text = self.read_text(name,
                              source,
                              test_data=self.m.json.dumps(test_data, indent=2))
        return self.m.json.loads(text)

    def write_json(self,
                   name: str,
                   dest: str | Path,
                   data: Any,
                   indent: int | str | None = None,
                   sort_keys: bool = True) -> StepData:
        """Write JSON-serializable *data* to *dest*.

        Args:
            name: The name of the step.
            dest: Path, on the host the step runs on, of the file to write.
            data: The value to write as JSON.
            indent: Indent of the written JSON (see `json.dump`).
            sort_keys: Sort the keys in *data* (see `api.json.input`).

        Raises:
            Error: If the file could not be written.
        """
        return self.write_text(
            name, dest,
            self.m.json.dumps(data, indent=indent, sort_keys=sort_keys))

    def read_proto(self,
                   name: str,
                   source: str | Path,
                   msg_class: type[ProtoMessage],
                   codec: str,
                   test_proto: ProtoMessage | None = None,
                   decoding_kwargs: dict | None = None) -> ProtoMessage:
        """Return the content of *source*, parsed as a protobuf message.

        Args:
            name: The name of the step.
            source: Path, on the host the step runs on, of the file to read.
            msg_class: The message type to read.
            codec: Which wire format the file is in (see the `proto` module).
            test_proto: What this step returns under simulation; defaults to an
                empty *msg_class*.
            decoding_kwargs: Passed to the codec's decoder.

        Returns:
            The parsed message.

        Raises:
            Error: If the file could not be read.
        """
        if test_proto is None:
            test_proto = msg_class()
        result = self._run(
            name, [
                'copy',
                str(source),
                self.m.proto.output(msg_class, codec, **(decoding_kwargs
                                                         or {}))
            ],
            step_test_data=lambda: self.test_api.read_proto(test_proto))
        return result.proto.output

    def write_proto(self,
                    name: str,
                    dest: str | Path,
                    proto_msg: Message,
                    codec: str,
                    encoding_kwargs: dict | None = None) -> StepData:
        """Write *proto_msg* to *dest*.

        Args:
            name: The name of the step.
            dest: Path, on the host the step runs on, of the file to write.
            proto_msg: The message to write.
            codec: Which wire format to write it in (see the `proto` module).
            encoding_kwargs: Passed to the codec's encoder.

        Raises:
            Error: If the file could not be written.
        """
        return self._run(name, [
            'copy',
            self.m.proto.input(proto_msg, codec, **(encoding_kwargs or {})),
            str(dest)
        ])

    def copy(self, name: str, source: str | Path,
             dest: str | Path) -> StepData:
        """Copy a file (including mode bits) from *source* to *dest*.

        Behaves like `shutil.copy`. If *dest* is a directory, the basename of
        *source* is appended to derive the destination file path.

        Raises:
            Error: If the copy failed.
        """
        return self._run(name, ['copy', str(source), str(dest)])

    def copytree(self,
                 name: str,
                 source: str | Path,
                 dest: str | Path,
                 *,
                 symlinks: bool = False,
                 hardlink: bool = False,
                 allow_override: bool = False) -> StepData:
        """Recursively copy a directory tree from *source* to *dest*.

        Behaves like `shutil.copytree`.

        Args:
            name: The name of the step.
            source: The directory to copy.
            dest: Where to copy it to.
            symlinks: Preserve symlinks as symlinks.
            hardlink: Hardlink files instead of copying them.
            allow_override: Allow existing files in *dest* to be overwritten,
                rather than failing if *dest* already has content.

        Raises:
            Error: If the copy failed.
        """
        args = ['copytree']
        if symlinks:
            args.append('--symlinks')
        if hardlink:
            args.append('--hardlink')
        if allow_override:
            args.append('--allow-override')
        args += [str(source), str(dest)]
        return self._run(name, args)

    def move(self, name: str, source: str | Path,
             dest: str | Path) -> StepData:
        """Move/rename *source* to *dest*. Behaves like `shutil.move`.

        Raises:
            Error: If the move failed.
        """
        return self._run(name, ['move', str(source), str(dest)])

    def chmod(self,
              name: str,
              path: str | Path,
              mode: int,
              *,
              recursive: bool = False) -> StepData:
        """Set the access mode for a file or directory.

        Args:
            name: The name of the step.
            path: The file or directory to chmod.
            mode: The access mode, in octal (e.g. `0o755`).
            recursive: Apply *mode* recursively.

        Raises:
            Error: If the chmod failed.
        """
        args = ['chmod', str(path), '--mode', oct(mode)]
        if recursive:
            args.append('--recursive')
        return self._run(name, args)

    def remove(self, name: str, source: str | Path) -> StepData:
        """Remove a file. Not an error if it doesn't already exist.

        Raises:
            Error: If the removal failed (for a reason other than the file
                already being absent).
        """
        return self._run(name, ['remove', str(source)])

    def rmtree(self, name: str, source: str | Path) -> StepData:
        """Recursively remove a directory. A no-op if it doesn't exist.

        Raises:
            Error: If the removal failed.
        """
        return self._run(name, ['rmtree', str(source)])

    def rmcontents(self, name: str, source: str | Path) -> StepData:
        """Remove the contents of *source*, but not *source* itself.

        Useful e.g. for clearing out the current working directory, where
        removing it outright would break subsequent relative-path operations.

        Raises:
            Error: If the removal failed.
        """
        return self._run(name, ['rmcontents', str(source)])

    def rmglob(self,
               name: str,
               source: str | Path,
               pattern: str,
               *,
               recursive: bool = True,
               include_hidden: bool = True) -> StepData:
        """Remove entries under *source* matching the glob *pattern*.

        Args:
            name: The name of the step.
            source: The directory to remove matching entries from.
            pattern: The glob pattern (stdlib `glob`, `recursive=True` rules)
                to match under *source*.
            recursive: Match *pattern* at any depth under *source*, not just
                directly inside it.
            include_hidden: Include files beginning with `.`.

        Raises:
            Error: If the removal failed.
        """
        if recursive and not pattern.startswith('**'):
            pattern = f'**/{pattern}'
        args = ['rmglob', str(source), pattern]
        if include_hidden:
            args.append('--hidden')
        return self._run(name, args)

    def glob_paths(
        self,
        name: str,
        source: str | Path,
        pattern: str,
        *,
        include_hidden: bool = False,
        test_data: Sequence[str] = ()) -> list[Path]:
        """Return paths under *source* matching the glob *pattern*.

        Args:
            name: The name of the step.
            source: The directory to glob under.
            pattern: The glob pattern (stdlib `glob`, `recursive=True` rules).
            include_hidden: Include files beginning with `.`.
            test_data: The paths this step finds under simulation, relative to
                *source*.

        Returns:
            The matching paths, as absolute paths under *source*.

        Raises:
            Error: If the glob failed.
        """
        args = ['glob', str(source), pattern]
        if include_hidden:
            args.append('--hidden')
        result = self._run(
            name,
            args,
            step_test_data=lambda: self.test_api.glob_paths(test_data),
            stdout=self.m.raw_io.output_text())
        return [_as_path(source) / line for line in result.stdout.splitlines()]

    def listdir(
        self,
        name: str,
        source: str | Path,
        *,
        recursive: bool = False,
        test_data: Sequence[str] = ()) -> list[Path]:
        """Return every file inside *source*.

        Args:
            name: The name of the step.
            source: The directory to list.
            recursive: List files at any depth under *source* (as paths
                relative to it), rather than only its direct contents.
            test_data: The entries this step finds under simulation, relative to
                *source*.

        Returns:
            The absolute paths of every entry found.

        Raises:
            Error: If the listing failed.
        """
        args = ['listdir', str(source)]
        if recursive:
            args.append('--recursive')
        result = self._run(
            name,
            args,
            step_test_data=lambda: self.test_api.listdir(test_data),
            stdout=self.m.raw_io.output_text())
        return [_as_path(source) / line for line in result.stdout.splitlines()]

    def ensure_directory(self,
                         name: str,
                         dest: str | Path,
                         *,
                         mode: int = 0o777) -> StepData:
        """Ensure *dest* exists and is a directory.

        Args:
            name: The name of the step.
            dest: The directory to ensure.
            mode: The mode to create *dest* with, if it doesn't exist yet.
                Not applied if *dest* already exists.

        Raises:
            Error: If *dest* exists but is not a directory, or creation
                failed.
        """
        return self._run(name,
                         ['ensure_directory',
                          str(dest), '--mode',
                          oct(mode)])

    def filesizes(self,
                  name: str,
                  files: Sequence[str | Path],
                  *,
                  test_data: Sequence[int] = ()) -> list[int]:
        """Return the size, in bytes, of each of *files*.

        Args:
            name: The name of the step.
            files: The files to size.
            test_data: The sizes this step reports under simulation.

        Raises:
            Error: If any file's size could not be read.
        """
        result = self._run(
            name, ['filesizes', *[str(f) for f in files]],
            step_test_data=lambda: self.test_api.filesizes(test_data),
            stdout=self.m.raw_io.output_text())
        return [int(line) for line in result.stdout.splitlines()]

    def symlink(self, name: str, source: str | Path,
                linkname: str | Path) -> StepData:
        """Create a symlink at *linkname* pointing to *source*.

        Behaves like `os.symlink`.

        Raises:
            Error: If the symlink could not be created.
        """
        return self._run(name, ['symlink', str(source), str(linkname)])

    def truncate(self,
                 name: str,
                 path: str | Path,
                 size_mb: int = 100) -> StepData:
        """Create an empty file at *path*, sized *size_mb* megabytes.

        Raises:
            Error: If the file could not be created.
        """
        return self._run(name, ['truncate', str(path), str(size_mb)])

    def flatten_single_directories(self, name: str,
                                   path: str | Path) -> StepData:
        """Move the contents of nested singular directories up to *path*.

        For example, given `path/only/nested/dir/{a,b}`, this moves `a` and
        `b` up to `path/{a,b}`, removing the now-empty intermediate
        directories. Useful for archives that extract into a single
        top-level directory whose exact name isn't worth hard-coding or
        looking up.

        Raises:
            Error: If flattening failed.
        """
        return self._run(name, ['flatten_single_directories', str(path)])

    def compute_hash(self,
                     name: str,
                     paths: Sequence[str | Path],
                     base_path: str | Path,
                     *,
                     test_data: str = '') -> str:
        """Return a hash of *paths* (files and/or directories).

        The hash covers each path's name (relative to *base_path*) and
        content, so it changes if any file is added, removed, renamed, or
        edited.

        Args:
            name: The name of the step.
            paths: The files/directories to hash.
            base_path: Base directory *paths* are hashed relative to.
            test_data: The hash this step reports under simulation.

        Returns:
            The hex-encoded hash.

        Raises:
            Error: If hashing failed.
        """
        rel_paths = [self.m.path.relpath(p, base_path) for p in paths]
        result = self._run(
            name, ['compute_hash', str(base_path), *rel_paths],
            step_test_data=lambda: self.test_api.compute_hash(test_data),
            stdout=self.m.raw_io.output_text())
        return result.stdout.strip()

    def file_hash(self,
                  name: str,
                  file_path: str | Path,
                  *,
                  test_data: str = '') -> str:
        """Return a hash of *file_path*'s content.

        Args:
            name: The name of the step.
            file_path: The file to hash.
            test_data: The hash this step reports under simulation.

        Raises:
            Error: If hashing failed.
        """
        result = self._run(
            name, ['file_hash', str(file_path)],
            step_test_data=lambda: self.test_api.file_hash(test_data),
            stdout=self.m.raw_io.output_text())
        return result.stdout.strip()

    def is_executable(self,
                      name: str,
                      path: str | Path,
                      *,
                      test_data: bool = True) -> bool:
        """Return whether *path* is executable.

        Args:
            name: The name of the step.
            path: The file to check.
            test_data: The answer this step reports under simulation.

        Raises:
            Error: If the check failed.
        """
        result = self._run(
            name, ['is_executable', str(path)],
            step_test_data=lambda: self.test_api.is_executable(test_data),
            stdout=self.m.raw_io.output_text())
        return result.stdout.strip() == 'True'
