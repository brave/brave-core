# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import repository


class GitStatus:
    """Runs `git status` and provides a summary.
    """

    def __init__(self):
        self.git_status = repository.brave.run_git('status', '--short')

        # a list of all deleted files, regardless of their staged status.
        self.deleted = []

        # a list of all modified files, regardless of their staged status.
        self.modified = []

        # a list of all untracked files.
        self.untracked = []

        for line in self.git_status.splitlines():
            [status, path] = line.lstrip().split(' ', 1)
            if status == 'D':
                self.deleted.append(path)
            elif status == 'M':
                self.modified.append(path)
            elif status == '??':
                self.untracked.append(path)

    def has_deleted_patch_files(self):
        return any(path.endswith('.patch') for path in self.deleted)

    def has_untracked_patch_files(self):
        return any(path.endswith('.patch') for path in self.untracked)
