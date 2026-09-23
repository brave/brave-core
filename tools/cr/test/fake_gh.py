# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A stand-in for the `gh` CLI.

`gh_cli.GhCli` shells out through `terminal.run`, so faking GitHub is a matter
of answering the `gh` command lines it builds. `FakeGh` is callable with the
same signature as `terminal.run`, which makes it usable either as a
`side_effect` for a patched `terminal.run` or as the `gh` route of
`fake_terminal.FakeTerminal`.

Every invocation is recorded in `calls`, and the canned responses are
configured through the constructor, so a test can drive a specific code path
without touching the network or the real CLI.
"""

from __future__ import annotations

import json
import subprocess
from types import SimpleNamespace

# The URLs handed back for a created issue and a created pull request.
ISSUE_URL = 'https://github.com/brave/brave-browser/issues/4242'
PR_URL = 'https://github.com/brave/brave-core/pull/123'


class FakeGh:
    """Answers the `gh` calls made through `gh_cli.GhCli`."""

    def __init__(self,
                 *,
                 logged_in: bool = True,
                 issue_list: list | None = None,
                 issue_create_url: str = ISSUE_URL,
                 pr_list: list | None = None,
                 pr_create_url: str = PR_URL,
                 pr_create_error: Exception | None = None,
                 pr_base_branch: str | None = None,
                 milestones: list | None = None) -> None:
        self.logged_in = logged_in
        self.issue_list = issue_list if issue_list is not None else []
        self.issue_create_url = issue_create_url
        self.pr_list = pr_list if pr_list is not None else []
        self.pr_create_url = pr_create_url
        self.pr_create_error = pr_create_error
        # The base branch `gh pr view <branch> --json baseRefName` reports.
        # None answers as if the branch had no pull request.
        self.pr_base_branch = pr_base_branch
        self.milestones = milestones if milestones is not None else []
        self.calls: list[list[str]] = []

    def __call__(self, cmd, **kwargs) -> SimpleNamespace:
        del kwargs  # `gh` is never run with a cwd, env, or stdin.
        cmd = [str(x) for x in cmd]
        self.calls.append(cmd)
        verb = cmd[1:3]
        if verb == ['auth', 'status']:
            if not self.logged_in:
                raise subprocess.CalledProcessError(1, cmd, stderr='no auth')
            return SimpleNamespace(
                stdout='Logged in to github.com account fake')
        if verb == ['issue', 'list']:
            return SimpleNamespace(stdout=json.dumps(self.issue_list))
        if verb == ['issue', 'create']:
            return SimpleNamespace(stdout=f'{self.issue_create_url}\n')
        if verb == ['issue', 'edit']:
            return SimpleNamespace(stdout='')
        if verb == ['pr', 'list']:
            return SimpleNamespace(stdout=json.dumps(self.pr_list))
        if verb == ['pr', 'view']:
            if self.pr_base_branch is None:
                raise subprocess.CalledProcessError(1, cmd, stderr='no pr')
            return SimpleNamespace(
                stdout=json.dumps({'baseRefName': self.pr_base_branch}))
        if verb == ['pr', 'create']:
            if self.pr_create_error is not None:
                raise self.pr_create_error
            return SimpleNamespace(stdout=f'{self.pr_create_url}\n')
        if cmd[1] == 'api':
            if cmd[2] == '-X':  # PATCH to set the milestone.
                return SimpleNamespace(stdout='')
            return SimpleNamespace(stdout=json.dumps(self.milestones))
        raise AssertionError(f'Unexpected gh call: {cmd}')

    def call_matching(self, *prefix: str) -> list[str] | None:
        """Returns the first recorded call whose start matches `prefix`."""
        prefix = list(prefix)
        return next((c for c in self.calls if c[:len(prefix)] == prefix), None)

    def pr_create_cmd(self) -> list[str] | None:
        """The recorded `gh pr create` call, if any."""
        return self.call_matching('gh', 'pr', 'create')

    def issue_create_cmd(self) -> list[str] | None:
        """The recorded `gh issue create` call, if any."""
        return self.call_matching('gh', 'issue', 'create')

    def issue_edit_cmd(self) -> list[str] | None:
        """The recorded `gh issue edit` call, if any."""
        return self.call_matching('gh', 'issue', 'edit')
