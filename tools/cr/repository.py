# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from dataclasses import dataclass
from pathlib import Path, PurePath
import subprocess
from typing import Optional

from terminal import terminal

# The path to the brave/ directory.
BRAVE_CORE_PATH = next(brave for brave in PurePath(__file__).parents
                       if brave.name == 'brave')

# The path to chromium's src/ directory.
CHROMIUM_SRC_PATH = BRAVE_CORE_PATH.parent


@dataclass(frozen=True)
class Repository:
    """Repository data class to hold the repository path.

    This class provides helpers around the use of repository paths, such as
    relative paths to and from the repository, and repository specific git
    operations.
    """

    # The repository path. This path is extracted from the subdirectory section
    # of a patch file, with chromium/src being ''.
    # e.g. "third_party/search_engines_data/resources"
    path: PurePath

    @property
    def is_chromium(self) -> bool:
        """If this repo is chromium/src.
        """
        return self.path == CHROMIUM_SRC_PATH

    @property
    def is_brave(self) -> bool:
        """If this repo is brave/.
        """
        return self.path == BRAVE_CORE_PATH

    @property
    def relative_to_chromium(self) -> PurePath:
        """ Returns the path relative to src/.
        """
        if self.is_chromium:
            return PurePath('')
        return self.path.relative_to(CHROMIUM_SRC_PATH)

    def to_brave(self) -> PurePath:
        """ Returns the path from the repository to brave/.
        """
        if self.is_chromium:
            return BRAVE_CORE_PATH.relative_to(CHROMIUM_SRC_PATH)
        return PurePath(
            len(self.path.relative_to(CHROMIUM_SRC_PATH).parts) *
            '../') / BRAVE_CORE_PATH.relative_to(CHROMIUM_SRC_PATH)

    def from_brave(self, source: Optional[PurePath] = None) -> PurePath:
        """ Returns the path from brave/ to the repository.
        """
        if source:
            return PurePath('..') / self.path.relative_to(
                CHROMIUM_SRC_PATH) / source

        return PurePath('..') / self.path.relative_to(CHROMIUM_SRC_PATH)

    def run_git(self, *cmd, no_trim=False) -> str:
        """Runs a git command on the repository.
        """
        if self.is_brave:
            return terminal.run_git(*cmd, no_trim=no_trim)

        return terminal.run_git('-C', self.from_brave(), *cmd, no_trim=no_trim)

    def unstage_all_changes(self):
        """Unstages all changes in the repository.
        """
        self.run_git('reset', 'HEAD')

    def has_staged_changed(self):
        return self.run_git('diff', '--cached', '--stat') != ''

    def get_commit_short_description(self, commit: str = 'HEAD') -> str:
        """Gets the short description of a commit.

        This is just the actual first line of the commit message.
        """
        return self.run_git('log', '-1', '--pretty=%s', commit)

    def git_commit(self, message):
        """Commits the current staged changes.

        This function also prints the commit hash/message to the user.
        Args:
        message:
            The message to be used for the commit.
        """
        if not self.has_staged_changed():
            # Nothing to commit
            return

        self.run_git('commit', '-m', message)
        commit = self.run_git('log', '-1', '--pretty=oneline',
                              '--abbrev-commit')
        terminal.log_task(f'[bold]✔️ [/] [italic]{commit}')

    def is_valid_git_reference(self, reference) -> bool:
        """Checks if a name is a valid git branch name or hash.
        """
        try:
            self.run_git('rev-parse', '--verify', reference)
            return True
        except subprocess.CalledProcessError:
            return False

    def last_changed(self,
                     file: str,
                     from_commit: Optional[str] = None) -> str:
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

    def current_branch(self) -> str:
        """Gets the current branch name, or HEAD if not in any branch.
        """
        return self.run_git('rev-parse', '--abbrev-ref', 'HEAD')


# An instance to the chromium repository.
chromium = Repository(PurePath(CHROMIUM_SRC_PATH))

# An instance to the brave repository.
brave = Repository(PurePath(BRAVE_CORE_PATH))
