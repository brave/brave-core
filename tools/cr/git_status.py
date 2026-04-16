# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import repository


class GitStatus:
    """Runs `git status` and provides a summary.

    This class is very simple and it is designed around the use case where we
    want to check for the status of the repository before committing the common
    version bump patches.
    """

    def __init__(self):
        self.git_status = repository.brave.run_git('status',
                                                   '--porcelain',
                                                   no_trim=True)

        # a list of all deleted files, regardless of their staged status.
        self.deleted = []

        # a list of all modified files, regardless of their staged status.
        self.modified = []

        # a list of all untracked files.
        self.untracked = []

        # a list of all staged files.
        self.staged = []

        for line in self.git_status.splitlines():
            xy = line[:2]
            path = line[3:]
            x, y = xy[0], xy[1]
            if x not in (' ', '?'):
                self.staged.append(path)
            if x == 'D' or y == 'D':
                self.deleted.append(path)
            elif x == 'M' or y == 'M':
                self.modified.append(path)
            elif xy == '??':
                self.untracked.append(path)

    def has_staged_files(self):
        return len(self.staged) > 0

    def has_deleted_patch_files(self):
        return any(path.endswith('.patch') for path in self.deleted)

    def has_untracked_patch_files(self):
        return any(path.endswith('.patch') for path in self.untracked)
