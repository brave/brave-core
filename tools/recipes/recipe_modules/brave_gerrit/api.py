# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `brave_gerrit` module API."""

from __future__ import annotations

from recipe_api import RecipeApi


class GerritApi(RecipeApi):
    """Brave's Gerrit, reached over SSH with the job's service account key."""

    HOST = 'gerrit-ssh.brave.com:29418'
    BRAVE_CORE_PROJECT = 'brave-browser/brave-core'

    def ssh_url(self, user: str, project: str = BRAVE_CORE_PROJECT) -> str:
        """The URL `git push` and `ssh ... gerrit` reach `project` at, as
        `user`."""
        return f'ssh://{user}@{self.HOST}/{project}'
