#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import json
import subprocess
import unittest
from unittest.mock import patch

import gh_cli
from gh_cli import GhCli


def _completed(stdout: str = '',
               stderr: str = '') -> subprocess.CompletedProcess:
    """Builds a fake `terminal.run` result carrying `stdout`/`stderr`."""
    return subprocess.CompletedProcess(args=['gh'],
                                       returncode=0,
                                       stdout=stdout,
                                       stderr=stderr)


class GhCliTest(unittest.TestCase):
    """Tests for the `GhCli` wrapper around the GitHub CLI."""

    def setUp(self):
        patcher = patch('gh_cli.terminal.run')
        self.run = patcher.start()
        self.addCleanup(patcher.stop)
        self.gh = GhCli()

    def _last_cmd(self) -> list[str]:
        """Returns the argument list passed to the most recent `run` call."""
        return list(self.run.call_args.args[0])

    def test_run_prefixes_gh(self):
        self.run.return_value = _completed(stdout='ok')
        self.gh._run(['pr', 'view'])
        self.assertEqual(self._last_cmd(), ['gh', 'pr', 'view'])

    def test_is_logged_in_true(self):
        self.run.return_value = _completed(
            stdout='✓ Logged in to github.com account someone')
        self.assertTrue(self.gh.is_logged_in())
        self.assertEqual(self._last_cmd(), ['gh', 'auth', 'status'])

    def test_is_logged_in_false_when_not_logged_in(self):
        self.run.return_value = _completed(stdout='You are not logged in')
        self.assertFalse(self.gh.is_logged_in())

    def test_is_logged_in_false_on_error(self):
        self.run.side_effect = subprocess.CalledProcessError(
            1, ['gh', 'auth', 'status'])
        self.assertFalse(self.gh.is_logged_in())

    def test_get_pr_base_branch_returns_remote_tracking_ref(self):
        self.run.return_value = _completed(
            stdout=json.dumps({'baseRefName': 'master'}))
        self.assertEqual(self.gh.get_pr_base_branch('cr149'), 'origin/master')
        self.assertEqual(
            self._last_cmd(),
            ['gh', 'pr', 'view', 'cr149', '--json', 'baseRefName'])

    def test_get_pr_base_branch_handles_uplift_base(self):
        self.run.return_value = _completed(
            stdout=json.dumps({'baseRefName': '1.79.x'}))
        self.assertEqual(self.gh.get_pr_base_branch('cr149'), 'origin/1.79.x')

    def test_get_pr_base_branch_none_when_no_pr(self):
        # `gh pr view` exits non-zero when the branch has no pull request.
        self.run.side_effect = subprocess.CalledProcessError(
            1, ['gh', 'pr', 'view'])
        self.assertIsNone(self.gh.get_pr_base_branch('cr149'))

    def test_get_pr_base_branch_none_when_empty_output(self):
        self.run.return_value = _completed(stdout='')
        self.assertIsNone(self.gh.get_pr_base_branch('cr149'))

    def test_list_prs_parses_json_and_builds_args(self):
        payload = [{'number': 7, 'url': 'https://example/pr/7'}]
        self.run.return_value = _completed(stdout=json.dumps(payload))
        self.assertEqual(self.gh.list_prs(head='cr149', fields='number,url'),
                         payload)
        self.assertEqual(
            self._last_cmd(),
            ['gh', 'pr', 'list', '--head', 'cr149', '--json=number,url'])

    def test_list_issues_parses_json_and_builds_args(self):
        payload = [{'number': 3, 'title': 'Bump'}]
        self.run.return_value = _completed(stdout=json.dumps(payload))
        result = self.gh.list_issues(repo='brave/brave-browser',
                                     search='Bump',
                                     state='open',
                                     fields='number,title')
        self.assertEqual(result, payload)
        self.assertEqual(self._last_cmd(), [
            'gh', 'issue', 'list', '--repo', 'brave/brave-browser', '--search',
            'Bump', '--state', 'open', '--json', 'number,title'
        ])

    def test_create_pr_builds_args_and_returns_url(self):
        self.run.return_value = _completed(stdout='https://example/pr/9\n')
        url = self.gh.create_pr(base='master',
                                head='cr149',
                                title='Bump',
                                body='Resolves #1',
                                labels=['"CI/run-audit-deps"', '"OS/Desktop"'],
                                assignees=['cdesouza-chromium', 'emerick'],
                                draft=True)
        self.assertEqual(url, 'https://example/pr/9')
        self.assertEqual(self._last_cmd(), [
            'gh', 'pr', 'create', '--base', 'master', '--head', 'cr149',
            '--title', 'Bump', '--body', 'Resolves #1', '--label',
            '"CI/run-audit-deps"', '--label', '"OS/Desktop"', '--assignee',
            'cdesouza-chromium', '--assignee', 'emerick', '--draft'
        ])

    def test_create_pr_omits_draft_flag_when_false(self):
        self.run.return_value = _completed(stdout='https://example/pr/9')
        self.gh.create_pr(base='master',
                          head='cr149',
                          title='Bump',
                          body='body',
                          labels=[],
                          assignees=[],
                          draft=False)
        self.assertNotIn('--draft', self._last_cmd())

    def test_create_issue_builds_args_and_returns_url(self):
        self.run.return_value = _completed(stdout='https://example/issue/1\n')
        url = self.gh.create_issue(repo='brave/brave-browser',
                                   title='Bump',
                                   body='body',
                                   labels=['"OS/Android"'],
                                   assignees=['emerick'])
        self.assertEqual(url, 'https://example/issue/1')
        self.assertEqual(self._last_cmd(), [
            'gh', 'issue', 'create', '--repo', 'brave/brave-browser',
            '--title', 'Bump', '--body', 'body', '--label', '"OS/Android"',
            '--assignee', 'emerick'
        ])

    def test_edit_issue_builds_args(self):
        self.run.return_value = _completed()
        self.gh.edit_issue(number=42, repo='brave/brave-browser', body='new')
        self.assertEqual(self._last_cmd(), [
            'gh', 'issue', 'edit', '42', '--repo', 'brave/brave-browser',
            '--body', 'new'
        ])

    def test_list_milestones_parses_json_and_builds_args(self):
        payload = [{'number': 1, 'title': '1.79.x - Nightly'}]
        self.run.return_value = _completed(stdout=json.dumps(payload))
        self.assertEqual(self.gh.list_milestones('brave/brave-core'), payload)
        self.assertEqual(self._last_cmd(), [
            'gh', 'api', 'repos/brave/brave-core/milestones', '--jq',
            '[.[] | {number, title}]'
        ])

    def test_set_issue_milestone_builds_args(self):
        self.run.return_value = _completed()
        self.gh.set_issue_milestone(repo='brave/brave-core',
                                    issue_number=99,
                                    milestone=1)
        self.assertEqual(self._last_cmd(), [
            'gh', 'api', '-X', 'PATCH', 'repos/brave/brave-core/issues/99',
            '-F', 'milestone=1'
        ])


if __name__ == '__main__':
    unittest.main()
