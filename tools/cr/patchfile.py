# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from dataclasses import dataclass, field, replace
from enum import Enum, auto
import logging
from pathlib import PurePath, Path
import re
import subprocess
from typing import Optional, NamedTuple

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
    path: PurePath

    # This field holds the name of the source file as provided by
    # `update_patches`, which is not a primary source.
    # e.g. "path":"build/android/gyp/dex.py"
    provided_source: Optional[str] = field(default=None)

    # This field holds the most reliable resolution to the source file name,
    # as its value is assigned from a git operation.
    source_from_git: Optional[str] = field(default=None)

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

    class ApplyResult(NamedTuple):
        """The result of applying the patch.
        """
        # The status of the patch operation
        status: "ApplyStatus"

        # A copy of the patch file data, with updated information.
        patch: Optional["Patchfile"]

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

    def source(self) -> PurePath:
        """The source file name for the patch file.

        Tries to use the most reliable source file name, going from git, to the
        one provide by `apply_patches`, to finally deducing from the name.
        """
        if self.source_from_git is not None:
            return self.source_from_git
        if self.provided_source is not None:
            return self.provided_source
        return self.source_name_from_patch_naming()

    def source_from_brave(self) -> PurePath:
        """The source file path relative to the `brave/` directory.
        """
        return f'{self.repository.from_brave() / self.source()}'

    def path_from_repo(self) -> PurePath:
        """The patch path relative to the repository source belongs.
        """
        return f'{self.repository.to_brave() / self.path}'

    def apply(self) -> ApplyResult:
        """Applies the patch file with `git apply --3way`.

        This function applies the patch file with `git apply --3way` to the
        repository, and it returns the status of the operation.

        Returns:
            The result of the patch application, and an updated patch instance
            with the source file name, if any is provided by git.
        """

        try:
            self.repository.run_git('apply', '--3way', '--ignore-space-change',
                                    '--ignore-whitespace',
                                    self.path_from_repo())
            return self.ApplyResult(status=self.ApplyStatus.CLEAN, patch=None)
        except subprocess.CalledProcessError as e:
            # If the process fails, we need to collect the files that failed to
            # apply for manual conflict resolution.
            if 'with conflicts' in e.stderr:
                # Output in this case should look like:
                #   Applied patch to 'build/android/gyp/dex.py' with conflicts.
                #   U build/android/gyp/dex.py
                # We get the file name from the last line.
                return self.ApplyResult(
                    self.ApplyStatus.CONFLICT,
                    replace(self,
                            source_from_git=e.stderr.strip().splitlines()
                            [-1].split()[-1]))
            if e.stderr.startswith('error:'):
                [_, reason] = e.stderr.strip().split(': ', 1)

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
                    return self.ApplyResult(
                        self.ApplyStatus.DELETED,
                        replace(self, source_from_git=reason.split(': ',
                                                                   2)[0]))
                if ('No such file or directory' in reason
                        and self.path in reason):
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

                return self.ApplyResult(status=self.ApplyStatus.BROKEN,
                                        patch=None)

        raise NotImplementedError()

    def fetch_source_from_git(self) -> "Patchfile":
        """Gets the source file name from git.

        This function uses git to get the source file name for the patch file,
        but only if such name has not been retrieved yet from git.

        Returns:
            A patch instance with the source file name from git.
        """

        if self.source_from_git is not None:
            return self

        # The command below has an output similar to:
        # 8	0  base/some_file.cc
        return replace(self,
                       source_from_git=self.repository.run_git(
                           'apply', '--numstat', '-z',
                           self.path_from_repo()).strip().split()[2][:-1])

    def get_last_commit_for_source(self) -> str:
        """Gets the last commit where the source file was mentioned.

        This function uses git to get the last commit where the source file was
        mentioned, which can be used to check details of when a source was
        deleted, or renamed.

        Returns:
            The commit hash with the last mention for the source.
        """
        return self.repository.run_git('log', '--full-history', '--pretty=%h',
                                       '-1', '--', self.source())

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
            (s for s in all_status.splitlines() if str(self.source()) in s),
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
