# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from dataclasses import dataclass, field

import repository


class GitStatus:
    '''Parses `git status --porcelain` output into categorized file lists.

    Files are grouped into two areas based on the XY status code from the
    porcelain output: `staged` (X column, index vs. HEAD) and `unstaged`
    (Y column, working tree vs. index). Staged renames are stored separately
    in `renamed` as (old_path, new_path) tuples because they span two paths.

    A single file can appear in both areas simultaneously — for example, a
    file staged as modified (X='M') and then further edited (Y='M') will
    appear in both `staged.modified` and `unstaged.modified`.
    '''

    @dataclass
    class Area:
        '''A view of file changes within one git status area.

        Each field holds the relative paths of files in that state. A path
        appears in exactly one field within an area.

        Fields:
            added:    Paths that are new in this area (X='A' or Y='?').
            modified: Paths that were changed relative to the previous state
                      (X='M' or Y='M').
            deleted:  Paths that were removed relative to the previous state
                      (X='D' or Y='D').
        '''
        added: list[str] = field(default_factory=list)
        modified: list[str] = field(default_factory=list)
        deleted: list[str] = field(default_factory=list)

    def __init__(self):
        porcelain = repository.brave.run_git('status',
                                             '--porcelain',
                                             no_trim=True)

        self.staged: GitStatus.Area = GitStatus.Area()
        self.unstaged: GitStatus.Area = GitStatus.Area()

        # Staged renames as (old_path, new_path) tuples. Kept separate from
        # Area because a rename inherently involves two paths.
        self.renamed: list[tuple[str, str]] = []

        for line in porcelain.splitlines():
            xy = line[:2]
            path = line[3:]
            x, y = xy[0], xy[1]

            if x == 'M':
                self.staged.modified.append(path)
            elif x == 'A':
                self.staged.added.append(path)
            elif x == 'D':
                self.staged.deleted.append(path)
            elif x == 'R':
                old_path, new_path = path.split(' -> ', 1)
                self.renamed.append((old_path, new_path))
                path = new_path

            if y == 'M':
                self.unstaged.modified.append(path)
            elif y == '?':
                self.unstaged.added.append(path)
            elif y == 'D':
                self.unstaged.deleted.append(path)

    def get_all_staged_entries(self) -> list[str]:
        '''Returns a flat list of all paths with staged changes.

        For renames, the new (current) path is included.
        '''
        return (self.staged.added + self.staged.modified +
                self.staged.deleted +
                [new_path for _, new_path in self.renamed])

    def has_staged_files(self) -> bool:
        '''Returns True if any staged changes exist.'''
        return bool(self.staged.added or self.staged.modified
                    or self.staged.deleted or self.renamed)

    def has_deleted_patch_files(self) -> bool:
        '''Returns True if any patch files under patches/ are deleted.

        Checks both staged and unstaged deletions, since a deletion can be
        present in either area.
        '''
        all_deleted = self.staged.deleted + self.unstaged.deleted
        return any(
            path.startswith('patches/') and path.endswith('.patch')
            for path in all_deleted)

    def has_untracked_patch_files(self) -> bool:
        '''Returns True if any patch files under patches/ are untracked.'''
        return any(
            path.startswith('patches/') and path.endswith('.patch')
            for path in self.unstaged.added)
