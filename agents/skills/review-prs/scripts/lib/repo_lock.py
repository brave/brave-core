# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Serializes review-prs' git fetches into one repository.

Two `git fetch`es into one repository collide on its ref locks and one of
them fails; review-prs fetches PR heads from several threads, and two runs can
overlap.
"""

import os

from .file_lock import DEFAULT_TIMEOUT_S, file_lock


def lock_path(repo_dir):
    return os.path.join(
        os.path.abspath(repo_dir), ".ignore", ".review-prs-git.lock"
    )


def repo_lock(repo_dir, timeout=DEFAULT_TIMEOUT_S):
    """Hold the git lock for `repo_dir`, waiting up to `timeout` seconds."""
    return file_lock(
        lock_path(repo_dir),
        timeout=timeout,
        what=f"the git repository {os.path.abspath(repo_dir)}",
    )
