# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from dataclasses import dataclass, field
from enum import Enum, auto
import logging
from pathlib import Path
import re
import subprocess
from typing import Optional

from repository import Repository
import repository


@dataclass(frozen=True)
class Patchfile:
    """Patchfile data class to hold the patchfile path.

    This class provides methods to manage the information regarding individual
    patches, such as the source file name, the repository, and the status of the
    patch application, etc.
    """

    # A patch's path and file name, from `patchPath`.
    # e.g. "patchPath":"patches/build-android-gyp-dex.py.patch"
    path: Path

    # The source file this patch targets, relative to the patch's repository.
    # e.g. "build/android/gyp/dex.py"
    source: Path

    # The repository the patch file is targeting to patch.
    repository: Repository = field(init=False)

    def __post_init__(self):
        object.__setattr__(self, 'repository',
                           self.get_repository_from_patch_name())

    def get_repository_from_patch_name(self) -> Repository:
        """Gets the repository for the patch file.
        """
        if self.path.suffix != '.patch':
            raise ValueError(
                f'Patch file name should end with `.patch`. {self.path}')
        if (self.path.is_absolute() or len(self.path.parents) < 2
                or self.path.parents[-2].stem != "patches"):
            raise ValueError(
                f'Patch file name should start with `patches/`. {self.path}')

        # Drops `patches/` at the beginning and the filename at the end.
        return Repository(
            repository.CHROMIUM_SRC_PATH.joinpath(*self.path.parts[1:-1]))

    class ApplyStatus(Enum):
        """The result of applying the patch.
        """

        # The patch was applied successfully.
        CLEAN = auto()

        # The patch was applied with conflicts.
        CONFLICT = auto()

        # The patch could not be applied because the source file was deleted.
        DELETED = auto()

        # The patch failed as broken.
        BROKEN = auto()

    @dataclass
    class SourceStatus:
        """The status of the source file in a given commit.
        """

        # A code for the status of the source file. (e.g 'R', 'M', 'D')
        status: str

        # The commit details for this source file status.
        commit_details: str

        # The name the source may have been renamed to.
        renamed_to: Optional[str] = None

    def source_name_from_patch_naming(self) -> str:
        """Source file name according to the patch file name.

        This function uses the patch file name to deduce the source file name.
        This works in general, but it is not the most reliable method to
        determine the source file name, and it should be used only in cases
        where git, and `apply_patches` cannot provide the source file name.

        e.g. "patches/build-android-gyp-dex.py.patch" ->
             "brave/build/android/gyp/dex.py"
        """
        return self.path.name[:-len(".patch")].replace('-', '/')

    def source_from_brave(self) -> Path:
        """The source file path relative to the `brave/` directory.
        """
        return self.repository.from_brave() / self.source

    def path_from_repo(self) -> Path:
        """The patch path relative to the repository source belongs.
        """
        return self.repository.to_brave() / self.path

    def apply(self) -> ApplyStatus:
        """Applies the patch file with `git apply --3way`.
        """
        try:
            self.repository.run_git('apply', '--3way', '--ignore-space-change',
                                    '--ignore-whitespace',
                                    self.path_from_repo().as_posix())
            return self.ApplyStatus.CLEAN
        except subprocess.CalledProcessError as e:
            if 'with conflicts' in e.stderr:
                return self.ApplyStatus.CONFLICT
            error_line = next(
                (l for l in e.stderr.splitlines() if l.startswith('error:')),
                None)
            if error_line is not None:
                [_, reason] = error_line.strip().split(': ', 1)

                if 'does not exist in index' in reason:
                    # This type of detection could occur in certain cases when
                    # `npm run init` or `sync` were not run for the working
                    # branch. It may be useful to warn.
                    #
                    # It is also of notice that this error can also occur when
                    # `apply` is run twice for the same patch with conflicts.
                    logging.warning(
                        'Patch with missing file detected during --3way apply,'
                        ' which may indicate a bad sync state before starting '
                        'to upgrade. %s', self.path)
                    return self.ApplyStatus.DELETED
                if ('No such file or directory' in reason
                        and self.path.as_posix() in reason):
                    # This should never occur as it indicates that the patch
                    # file itself is missing, which is sign something is wrong
                    # with path resolution.
                    raise e

                # All other errors are considered broken patches.
                if ('patch with only garbage' not in reason
                        and 'corrupt patch at line' not in reason):
                    # Not clear if we could have other reasons for broken
                    # patches, but it is better to flag it to keep an eye out
                    # for it.
                    logging.warning(
                        'Patch being flagged as broken, but with unexpected '
                        'reason: %s %s', self.path, reason)

                return self.ApplyStatus.BROKEN

        # We should not reach this point. Failures to apply that fail like this
        # must be investigated with `--verbose`, and fixed.
        raise NotImplementedError()

    def get_last_commit_for_source(self) -> str:
        """Gets the last commit where the source file was mentioned.

        This function uses git to get the last commit where the source file was
        mentioned, which can be used to check details of when a source was
        deleted, or renamed.

        Returns:
            The commit hash with the last mention for the source.
        """
        return self.repository.run_git('log', '--full-history', '--pretty=%h',
                                       '-1', '--', self.source.as_posix())

    def get_source_removal_status(self, commit: str) -> SourceStatus:
        """Gets the status of the source file in a given commit.

        This function retrieves the details of the source file in a given
        commit. This is useful to check if a file has been deleted, or
        renamed.

        Args:
            commit:
                The commit hash to check the source file status.

        Returns:
            The status of the source file in the commit, which also includes
            the commit description.
        """

        # Not very sure why, but by passing `--no-commit-id` the
        # output looks something like this:
        # M       chrome/VERSION
        # commit 17bb6c858e818e81744de42ed292b7060bc341e5
        # Author: Chrome Release Bot (LUCI)
        # Date:   Wed Feb 26 10:17:33 2025 -0800
        #
        #     Incrementing VERSION to 134.0.6998.45
        #
        #     Change-Id: I6e995e1d7aed40e553d3f51e98fb773cd75
        #
        # So the output is read and split from the moment a line starts with
        # `commit`.
        change = self.repository.run_git('show', '--name-status',
                                         '--no-commit-id', commit)
        [all_status, commit_details] = re.split(r'(?=^commit\s)',
                                                change,
                                                flags=re.MULTILINE)

        # let's look for the line about the source we care about.
        status_line = next(
            (s
             for s in all_status.splitlines() if self.source.as_posix() in s),
            None)
        status_code = status_line[0]

        if status_code == 'D':
            return self.SourceStatus(status=status_code,
                                     commit_details=commit_details)
        if status_code == 'R':
            # For renames the output looks something like:
            # R100       base/some_file.cc       base/renamed_to_file.cc
            return self.SourceStatus(status=status_code,
                                     commit_details=commit_details,
                                     renamed_to=status_line.split()[-1])

        # This could change in the future, but for now it only makes sense to
        # use this function for deleted or renamed files.
        raise NotImplementedError(f'unreachable: {status_line}')
