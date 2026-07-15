# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A thin wrapper around the GitHub CLI (`gh`).

All interactions with `gh` live here so that command construction and JSON
parsing sit in one place, and can be tested without talking to GitHub.
"""

from __future__ import annotations

import json
import subprocess

from terminal import terminal


class GhCli:
    """Wraps the `gh` command-line tool.

    Methods are thin: they build the `gh` argument list, run it, and parse the
    output. Brave-specific policy (which labels/assignees to use, how to match
    an issue by title, etc.) is left to the caller.
    """

    def _run(self, args: list[str]) -> subprocess.CompletedProcess:
        """Runs `gh` with `args`, returning the completed process.

        Raises `subprocess.CalledProcessError` when `gh` exits non-zero.
        """
        return terminal.run(['gh', *args])

    def _run_json(self, args: list[str]):
        """Runs `gh` with `args` and parses stdout as JSON."""
        return json.loads(self._run(args).stdout.strip())

    def is_logged_in(self) -> bool:
        """Returns True when `gh` is logged in to a github.com account."""
        try:
            result = self._run(['auth', 'status']).stdout.strip()
        except subprocess.CalledProcessError:
            return False
        return 'Logged in to github.com account' in result

    def get_pr_base_branch(self, branch: str) -> str | None:
        """Returns the base branch of `branch`'s pull request as a
        remote-tracking ref (e.g. `origin/master`).

        Returns None when the lookup fails or there is no pull request for the
        branch.
        """
        try:
            result = self._run(['pr', 'view', branch, '--json',
                                'baseRefName']).stdout.strip()
        except subprocess.CalledProcessError:
            return None
        base_ref = json.loads(result).get('baseRefName') if result else None
        if not base_ref:
            return None
        return f'origin/{base_ref}'

    def list_prs(self, *, head: str, fields: str) -> list:
        """Lists pull requests whose source branch is `head`.

        `fields` is a comma-separated list of JSON fields to return (e.g.
        `number,url`).
        """
        return self._run_json(
            ['pr', 'list', '--head', head, f'--json={fields}'])

    def create_pr(self,
                  *,
                  base: str,
                  head: str,
                  title: str,
                  body: str,
                  labels: list[str],
                  assignees: list[str],
                  draft: bool = False) -> str:
        """Creates a pull request and returns its URL.

        Raises `subprocess.CalledProcessError` when `gh` exits non-zero.
        """
        args = [
            'pr', 'create', '--base', base, '--head', head, '--title', title,
            '--body', body
        ]
        for label in labels:
            args += ['--label', label]
        for assignee in assignees:
            args += ['--assignee', assignee]
        if draft:
            args.append('--draft')
        return self._run(args).stdout.strip()

    def list_issues(self, *, repo: str, search: str, state: str,
                    fields: str) -> list:
        """Lists issues in `repo` matching `search` in the given `state`.

        `fields` is a comma-separated list of JSON fields to return (e.g.
        `number,title,url,body`).
        """
        return self._run_json([
            'issue', 'list', '--repo', repo, '--search', search, '--state',
            state, '--json', fields
        ])

    def create_issue(self, *, repo: str, title: str, body: str,
                     labels: list[str], assignees: list[str]) -> str:
        """Creates an issue in `repo` and returns its URL."""
        args = [
            'issue', 'create', '--repo', repo, '--title', title, '--body', body
        ]
        for label in labels:
            args += ['--label', label]
        for assignee in assignees:
            args += ['--assignee', assignee]
        return self._run(args).stdout.strip()

    def edit_issue(self, *, number: int | str, repo: str, body: str) -> None:
        """Edits the body of issue `number` in `repo`."""
        self._run(
            ['issue', 'edit',
             str(number), '--repo', repo, '--body', body])

    def list_milestones(self, repo: str) -> list:
        """Returns the `{number, title}` of every milestone in `repo`."""
        return self._run_json([
            'api', f'repos/{repo}/milestones', '--jq',
            '[.[] | {number, title}]'
        ])

    def set_issue_milestone(self, *, repo: str, issue_number: int | str,
                            milestone: int | str) -> None:
        """Assigns `milestone` to issue/PR `issue_number` in `repo`."""
        self._run([
            'api', '-X', 'PATCH', f'repos/{repo}/issues/{issue_number}', '-F',
            f'milestone={milestone}'
        ])
