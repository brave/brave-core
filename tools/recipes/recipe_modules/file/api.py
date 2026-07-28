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
`{ok, errno_name, message}` result and an `Error` exception, covering every
operation that doesn't need to inject arbitrary file *content* into the step.

Deliberately not supported: `write_text`/`write_raw`/`write_json`, and
`read_proto`/`write_proto`. Those would need a way to carry arbitrary content
into a step via a file (e.g. a materialized temp-file path/content), which
this engine has no primitive for -- only a step's own stdout (used here for
`read_text` and the other value-returning operations below). `read_json` still
works, since `read_text` (a plain step) plus a client-side `json.loads` is
enough.
"""

from __future__ import annotations

import json
import os
import subprocess
from pathlib import Path
from typing import Any

from recipe_api import RecipeApi

# The resource script `_run` invokes for every operation. Lives alongside this
# module (not under brave-core), so it's always present -- no sparse checkout
# needed, unlike e.g. `tools/cr/toolchains/ephemeral_xcode.py`.
_FILEUTIL = Path(__file__).resolve().parent / 'resources' / 'fileutil.py'


class FileApi(RecipeApi):
    """Basic filesystem operations (read, copy, move, remove, ...) as steps."""

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

    def _run(self, name: str, args: list[str]) -> str:
        """Run a `fileutil.py` operation and return its captured stdout.

        Parses the `{ok, errno_name, message}` result `fileutil.py` prints to
        stderr, defaulting to success when nothing was seeded under
        simulation, and raises `Error` when it reports failure.
        """
        vpython3 = self.m.depot_tools.vpython3()
        result = self.m.step(name, [vpython3, '-u', _FILEUTIL, *args],
                             capture_output=True)
        status = (json.loads(result.stderr) if result.stderr else {
            'ok': True,
            'errno_name': '',
            'message': ''
        })
        if not status['ok']:
            raise self.Error(name, status['errno_name'], status['message'])
        return result.stdout or ''

    def read_text(self, name: str, source: str | Path) -> str:
        """Return the UTF-8 text content of *source*.

        Args:
            name: The name of the step.
            source: Path, on the host the step runs on, of the file to read.

        Returns:
            The file's content.

        Raises:
            Error: If the file could not be read.
        """
        return self._run(name, ['read_text', str(source)])

    def read_json(self, name: str, source: str | Path) -> Any:
        """Return the parsed JSON content of *source*.

        Args:
            name: The name of the step.
            source: Path, on the host the step runs on, of the file to read.

        Returns:
            The file's content, parsed as JSON.

        Raises:
            Error: If the file could not be read.
        """
        return json.loads(self.read_text(name, source))

    def copy(self, name: str, source: str | Path, dest: str | Path) -> None:
        """Copy a file (including mode bits) from *source* to *dest*.

        Behaves like `shutil.copy`. If *dest* is a directory, the basename of
        *source* is appended to derive the destination file path.

        Raises:
            Error: If the copy failed.
        """
        self._run(name, ['copy', str(source), str(dest)])

    def copytree(self,
                 name: str,
                 source: str | Path,
                 dest: str | Path,
                 *,
                 symlinks: bool = False,
                 hardlink: bool = False,
                 allow_override: bool = False) -> None:
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
        self._run(name, args)

    def move(self, name: str, source: str | Path, dest: str | Path) -> None:
        """Move/rename *source* to *dest*. Behaves like `shutil.move`.

        Raises:
            Error: If the move failed.
        """
        self._run(name, ['move', str(source), str(dest)])

    def chmod(self,
              name: str,
              path: str | Path,
              mode: int,
              *,
              recursive: bool = False) -> None:
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
        self._run(name, args)

    def remove(self, name: str, source: str | Path) -> None:
        """Remove a file. Not an error if it doesn't already exist.

        Raises:
            Error: If the removal failed (for a reason other than the file
                already being absent).
        """
        self._run(name, ['remove', str(source)])

    def rmtree(self, name: str, source: str | Path) -> None:
        """Recursively remove a directory. A no-op if it doesn't exist.

        Raises:
            Error: If the removal failed.
        """
        self._run(name, ['rmtree', str(source)])

    def rmcontents(self, name: str, source: str | Path) -> None:
        """Remove the contents of *source*, but not *source* itself.

        Useful e.g. for clearing out the current working directory, where
        removing it outright would break subsequent relative-path operations.

        Raises:
            Error: If the removal failed.
        """
        self._run(name, ['rmcontents', str(source)])

    def rmglob(self,
               name: str,
               source: str | Path,
               pattern: str,
               *,
               recursive: bool = True,
               include_hidden: bool = True) -> None:
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
        self._run(name, args)

    def glob_paths(self,
                   name: str,
                   source: str | Path,
                   pattern: str,
                   *,
                   include_hidden: bool = False) -> list[Path]:
        """Return paths under *source* matching the glob *pattern*.

        Args:
            name: The name of the step.
            source: The directory to glob under.
            pattern: The glob pattern (stdlib `glob`, `recursive=True` rules).
            include_hidden: Include files beginning with `.`.

        Returns:
            The matching paths, as absolute paths under *source*.

        Raises:
            Error: If the glob failed.
        """
        args = ['glob', str(source), pattern]
        if include_hidden:
            args.append('--hidden')
        out = self._run(name, args)
        return [Path(source) / line for line in out.splitlines()]

    def listdir(self,
                name: str,
                source: str | Path,
                *,
                recursive: bool = False) -> list[Path]:
        """Return every file inside *source*.

        Args:
            name: The name of the step.
            source: The directory to list.
            recursive: List files at any depth under *source* (as paths
                relative to it), rather than only its direct contents.

        Returns:
            The absolute paths of every entry found.

        Raises:
            Error: If the listing failed.
        """
        args = ['listdir', str(source)]
        if recursive:
            args.append('--recursive')
        out = self._run(name, args)
        return [Path(source) / line for line in out.splitlines()]

    def ensure_directory(self,
                         name: str,
                         dest: str | Path,
                         *,
                         mode: int = 0o777) -> None:
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
        self._run(name, ['ensure_directory', str(dest), '--mode', oct(mode)])

    def filesizes(self, name: str, files: list[str | Path]) -> list[int]:
        """Return the size, in bytes, of each of *files*.

        Raises:
            Error: If any file's size could not be read.
        """
        out = self._run(name, ['filesizes', *[str(f) for f in files]])
        return [int(line) for line in out.splitlines()]

    def symlink(self, name: str, source: str | Path,
                linkname: str | Path) -> None:
        """Create a symlink at *linkname* pointing to *source*.

        Behaves like `os.symlink`.

        Raises:
            Error: If the symlink could not be created.
        """
        self._run(name, ['symlink', str(source), str(linkname)])

    def truncate(self,
                 name: str,
                 path: str | Path,
                 size_mb: int = 100) -> None:
        """Create an empty file at *path*, sized *size_mb* megabytes.

        Raises:
            Error: If the file could not be created.
        """
        self._run(name, ['truncate', str(path), str(size_mb)])

    def flatten_single_directories(self, name: str, path: str | Path) -> None:
        """Move the contents of nested singular directories up to *path*.

        For example, given `path/only/nested/dir/{a,b}`, this moves `a` and
        `b` up to `path/{a,b}`, removing the now-empty intermediate
        directories. Useful for archives that extract into a single
        top-level directory whose exact name isn't worth hard-coding or
        looking up.

        Raises:
            Error: If flattening failed.
        """
        self._run(name, ['flatten_single_directories', str(path)])

    def compute_hash(self, name: str, paths: list[str | Path],
                     base_path: str | Path) -> str:
        """Return a hash of *paths* (files and/or directories).

        The hash covers each path's name (relative to *base_path*) and
        content, so it changes if any file is added, removed, renamed, or
        edited.

        Args:
            name: The name of the step.
            paths: The files/directories to hash.
            base_path: Base directory *paths* are hashed relative to.

        Returns:
            The hex-encoded hash.

        Raises:
            Error: If hashing failed.
        """
        rel_paths = [os.path.relpath(str(p), str(base_path)) for p in paths]
        out = self._run(name, ['compute_hash', str(base_path), *rel_paths])
        return out.strip()

    def file_hash(self, name: str, file_path: str | Path) -> str:
        """Return a hash of *file_path*'s content.

        Raises:
            Error: If hashing failed.
        """
        return self._run(name, ['file_hash', str(file_path)]).strip()

    def is_executable(self, name: str, path: str | Path) -> bool:
        """Return whether *path* is executable.

        Raises:
            Error: If the check failed.
        """
        return self._run(name, ['is_executable', str(path)]).strip() == 'True'
