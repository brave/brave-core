# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import logging
import subprocess
import sys
from rich.markup import escape

from terminal import terminal

# The basename of the brave-core directory inside chromium/src. Used where we
# need the literal 'brave' segment, since with relative path constants the
# expression `brave_root.relative_to(chromium_root)` is no longer a valid
# lexical operation.
BRAVE_DIR_NAME = 'brave'


def _compute_brave_core_path() -> Path:
    """Returns the cwd-relative path to the brave-core repo root.

    Computed once at module import via `git rev-parse --show-cdup`. The
    relative form is intentional: it remains valid for the duration of the
    script even when the resolved absolute path differs (e.g. tests chdir
    into a fake brave repo before importing tools/cr modules, and
    subprocesses inherit the same cwd). This relies on the invariant that
    cwd does not change during the lifetime of a tools/cr script.
    """
    try:
        cdup = terminal.run_git('rev-parse', '--show-cdup')
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        raise RuntimeError(
            'tools/cr must be invoked from within a brave-core git work tree '
            '(git rev-parse --show-cdup failed).') from e
    brave_core = Path(cdup) if cdup else Path('.')
    if brave_core.resolve().name != BRAVE_DIR_NAME:
        raise RuntimeError(
            f'tools/cr expected to be inside a "{BRAVE_DIR_NAME}" git work '
            f'tree; resolved root is {brave_core.resolve()}.')
    if brave_core != Path('.'):
        logging.debug(
            'Running %s from relative path: brave/%s, brave-core root set as:'
            ' %s (resolves to %s)', (Path(sys.argv[0]).name if sys.argv
                                     and sys.argv[0] else '<unknown>'),
            Path.cwd().relative_to(brave_core.resolve()), brave_core,
            brave_core.resolve())
    return brave_core


# The path to the brave/ directory, relative to cwd at module import.
_BRAVE_CORE_PATH = _compute_brave_core_path()

# The path to chromium's src/ directory, relative to cwd at module import.
_CHROMIUM_SRC_PATH = _BRAVE_CORE_PATH / '..'


@dataclass(frozen=True)
class Repository:
    """Repository data class to hold the repository root path.

    This class provides helpers around the use of repository paths, such as
    relative paths to and from the repository, and repository specific git
    operations.
    """

    # The repository's root path, relative to cwd at module import time.
    root: Path

    @property
    def is_chromium(self) -> bool:
        """If this repo is chromium/src.
        """
        return self.root == _CHROMIUM_SRC_PATH

    @property
    def is_brave(self) -> bool:
        """If this repo is brave/.
        """
        return self.root == _BRAVE_CORE_PATH

    @property
    def relative_to_chromium(self) -> Path:
        """ Returns the path relative to src/.
        """
        if self.is_chromium:
            return Path('')
        # The brave repo's root is `Path('.')` and chromium's is `Path('..')`,
        # which `relative_to` cannot reconcile lexically, so name it directly.
        if self.is_brave:
            return Path(BRAVE_DIR_NAME)
        return self.root.relative_to(_CHROMIUM_SRC_PATH)

    def to_brave(self) -> Path:
        """ Returns the path from the repository to brave/.
        """
        if self.is_chromium:
            return Path(BRAVE_DIR_NAME)
        return Path(
            len(self.relative_to_chromium.parts) * '../') / BRAVE_DIR_NAME

    def from_brave(self, source: Path | None = None) -> Path:
        """ Returns the path from brave/ to the repository.
        """
        if source:
            return _BRAVE_CORE_PATH / Path(
                '..') / self.relative_to_chromium / source

        return _BRAVE_CORE_PATH / Path('..') / self.relative_to_chromium

    def to_repo_relative(self, p: Path) -> Path:
        """Returns p expressed relative to this repo's root.

        Accepts an absolute path or one relative to cwd; resolves both sides so
        the comparison is lexical-after-normalisation.
        """
        return Path(p).resolve().relative_to(self.root.resolve())

    def run_git(self,
                *cmd,
                no_trim=False,
                env: dict[str, str] | None = None) -> str:
        """Runs a git command on this repository.

        Strongly preferred over calling `terminal.run_git` directly because
        the call site makes it obvious which repository the command targets.

        Caveat: git is invoked as if from this repo's root (via
        `git -C <repo-root>`). Any relative path arguments are therefore
        resolved against the repo root, not against the caller's cwd. If a
        command needs paths relative to cwd, call `terminal.run_git` directly
        instead.

        `env` is forwarded to `terminal.run_git`, which merges it on top of
        `os.environ` for this invocation (useful for the commit-msg hook's
        `tags` / `culprit` vars).
        """
        if self.root == Path('.'):
            return terminal.run_git(*cmd, no_trim=no_trim, env=env)

        return terminal.run_git('-C',
                                self.from_brave(),
                                *cmd,
                                no_trim=no_trim,
                                env=env)

    def unstage_all_changes(self):
        """Unstages all changes in the repository.
        """
        self.run_git('reset', 'HEAD')

    def has_staged_changes(self) -> bool:
        return self.run_git('diff', '--cached', '--stat') != ''

    def get_commit_short_description(self, commit: str = 'HEAD') -> str:
        """Gets the short description of a commit.

        This is just the actual first line of the commit message.
        """
        return self.run_git('log', '-1', '--pretty=%s', commit)

    def _git_commit_internal(self,
                             args: list[str],
                             *,
                             allows_empty: bool,
                             no_verify: bool = False,
                             env: dict[str, str] | None = None):
        """Shared implementation for git commit operations.

        Args:
        args:
            The git commit arguments (everything after 'commit').
        allows_empty:
            Whether to allow empty commits.
        no_verify:
            Whether to skip pre-commit and commit-msg hooks.
        env:
            Optional environment variables to be forwarded to
            `terminal.run_git`.
        """
        has_staged_changes = self.has_staged_changes()
        if not allows_empty and not has_staged_changes:
            # Nothing to commit
            return
        if allows_empty and has_staged_changes:
            # Throwing an error if anything is staged as that could result in
            # unintentionally committing changes.
            raise ValueError(
                'Cannot allow empty commits if there are staged changes.')

        if allows_empty:
            args.append('--allow-empty')
        if no_verify:
            args.append('--no-verify')
        self.run_git('commit', *args, env=env)

        commit = self.run_git('log', '-1', '--pretty=oneline',
                              '--abbrev-commit')
        terminal.log_task(f'[bold]✔️ [/] [italic]{escape(commit)}')

    def git_commit(self,
                   message: str,
                   *,
                   allows_empty: bool = False,
                   no_verify: bool = False,
                   env: dict[str, str] | None = None):
        """Commits the current staged changes.

    This function calls `git commit` and prints a user friendly message as a
    result. No commit will be greated if nothing is staged, unless allows_empty
    is set to True.

        Args:
        message:
            The message to be used for the commit.
        allows_empty:
            Whether to allow empty commits.
        no_verify:
            Whether to skip pre-commit and commit-msg hooks.
        env:
            Optional environment variables to be forwarded to `terminal.run`.
        """
        self._git_commit_internal(['-m', message],
                                  allows_empty=allows_empty,
                                  no_verify=no_verify,
                                  env=env)

    def git_commit_fixup(self, commit: str, *, allows_empty: bool = False):
        """Commits the current staged changes as a fixup for a given commit.

    This function calls `git commit --fixup` and prints a user friendly message
    as a result. No commit will be created if nothing is staged, unless
    allows_empty is set to True.

        Args:
        commit:
            The commit hash to create a fixup for.
        allows_empty:
            Whether to allow empty commits.
        """
        self._git_commit_internal(['--fixup', commit],
                                  allows_empty=allows_empty)

    def is_valid_git_reference(self, reference: str) -> bool:
        """Checks if a name is a valid git branch name or hash.
        """
        try:
            self.run_git('rev-parse', '--verify', reference)
            return True
        except subprocess.CalledProcessError:
            return False

    def last_changed(self, file: str, from_commit: str | None = None) -> str:
        """Gets the last commit for a file.
        """
        args = ['log', '--pretty=%h', '-1']
        if from_commit:
            args.append(from_commit)
        args.append(file)
        return self.run_git(*args)

    def read_file(self, *files, commit: str = 'HEAD') -> str:
        """Reads the content of a file in the repository.

    Args:
        files:
            The file paths to read.
        commit:
            The commit to read the file from. This can be a branch or a tag to,
            but the name commit is being used to be more self-explanatory.

    Return:
        The contents of the file read. If more than one file is provided, the
        contents of all files are appended to the same string.
        """
        return self.run_git(
            'show',
            *[f'{commit}:{Path(file).as_posix()}' for file in files],
            no_trim=True)

    def get_patch_stats(self, patch: Path) -> list[Path]:
        """Returns the files affected by the given patch.

        Uses --numstat for unabbreviated paths.
        """
        return [
            Path(line.split('\t')[2]) for line in self.run_git(
                'apply', '--numstat', patch.as_posix()).splitlines()
            if '\t' in line
        ]

    def current_branch(self) -> str:
        """Gets the current branch name, or HEAD if not in any branch.
        """
        return self.run_git('rev-parse', '--abbrev-ref', 'HEAD')


# An instance to the chromium repository.
chromium = Repository(_CHROMIUM_SRC_PATH)

# An instance to the brave repository.
brave = Repository(_BRAVE_CORE_PATH)
