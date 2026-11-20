#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Integration tests for rebase_feature_branch.py, running cmd.py as a
subprocess against a FakeChromiumRepo."""

import subprocess
import unittest

import _boot  # noqa: F401
from cmd_test import _Sandbox, _git
from exceptions import RebaseNeededException


_SYNCED = '140.0.7000.0'
_NEXT = '140.0.7001.0'


class TestRebaseFeatureBranch(unittest.TestCase):
    def setUp(self) -> None:
        self._sandbox = self.enterContext(_Sandbox())
        self._sandbox.repo.add_tag(_SYNCED)
        self._sandbox.repo.update_brave_version(_SYNCED)
        self._base = self._rev_parse('--abbrev-ref', 'HEAD')

    def _bump_base(self, version: str) -> None:
        _git(self._sandbox.root, 'checkout', self._base)
        self._sandbox.repo.update_brave_version(version)
        _git(self._sandbox.root, 'checkout', 'feature')

    def _rev_parse(self, *args: str) -> str:
        return _git(self._sandbox.root, 'rev-parse', *args).stdout.strip()

    def _commit_file(self, name: str) -> str:
        (self._sandbox.root / name).write_text(f'{name}\n', encoding='utf-8')
        _git(self._sandbox.root, 'add', name)
        _git(self._sandbox.root, 'commit', '-m', f'Add {name}')
        return self._rev_parse('HEAD')

    def _fork_feature(self, *track: str) -> str:
        """Commits feature.txt on a new `feature` branch, then moves the base
        on. Returns the fork point."""
        fork = self._rev_parse('HEAD')
        _git(self._sandbox.root, 'checkout', '-b', 'feature', *track)
        self._commit_file('feature.txt')
        _git(self._sandbox.root, 'checkout', self._base)
        self._commit_file('base.txt')
        _git(self._sandbox.root, 'checkout', 'feature')
        return fork

    def test_rebases_onto_the_upstream_from_the_merge_base(self) -> None:
        fork = self._fork_feature('--track', self._base)

        result = self._sandbox.run_gc(['rebase-feature-branch'])

        self.assertEqual(result.returncode, 0, msg=result.stderr)
        self.assertEqual(self._rev_parse('HEAD^'), self._rev_parse(self._base))
        self.assertRegex(result.stdout, rf'starting from\b[\s\S]*\b{fork}\b')

    def test_to_ref_picks_the_branch_to_rebase_onto(self) -> None:
        self._fork_feature('--no-track')

        result = self._sandbox.run_gc(
            ['rebase-feature-branch', '--to-ref', self._base]
        )

        self.assertEqual(result.returncode, 0, msg=result.stderr)
        self.assertEqual(self._rev_parse('HEAD^'), self._rev_parse(self._base))

    def test_no_upstream_is_an_error(self) -> None:
        self._fork_feature('--no-track')

        result = self._sandbox.run_gc(['rebase-feature-branch'])

        self.assertEqual(result.returncode, 1)
        self.assertIn('no upstream', result.stderr)

    def test_dry_run_reports_a_needed_rebase(self) -> None:
        self._fork_feature('--track', self._base)
        head = self._rev_parse('HEAD')

        result = self._sandbox.run_gc(['rebase-feature-branch', '--dry-run'])

        self.assertEqual(result.returncode, RebaseNeededException.EXIT_CODE)
        self.assertEqual(self._rev_parse('HEAD'), head)

    def test_dry_run_after_rebasing_needs_nothing(self) -> None:
        self._fork_feature('--track', self._base)
        self._sandbox.run_gc(['rebase-feature-branch'])

        result = self._sandbox.run_gc(['rebase-feature-branch', '--dry-run'])

        self.assertEqual(result.returncode, 0, msg=result.stderr)

    def _stop_on_a_conflict(self) -> subprocess.CompletedProcess:
        """Starts a rebase whose only commit conflicts with the base."""
        _git(
            self._sandbox.root,
            'checkout',
            '-b',
            'feature',
            '--track',
            self._base,
        )
        self._commit_file('clash.txt')
        _git(self._sandbox.root, 'checkout', self._base)
        (self._sandbox.root / 'clash.txt').write_text(
            'other\n', encoding='utf-8'
        )
        _git(self._sandbox.root, 'add', 'clash.txt')
        _git(self._sandbox.root, 'commit', '-m', 'Add a different clash.txt')
        _git(self._sandbox.root, 'checkout', 'feature')

        result = self._sandbox.run_gc(['rebase-feature-branch'])
        self.assertEqual(result.returncode, 1)
        return result

    def test_a_conflict_stop_says_to_continue_with_git_cr(self) -> None:
        result = self._stop_on_a_conflict()

        self.assertRegex(
            result.stdout + result.stderr,
            r'git cr rebase-feature-branch\s+--continue',
        )

    def test_chromium_not_synced_to_the_target_is_refused(self) -> None:
        self._fork_feature('--track', self._base)
        self._bump_base(_NEXT)
        head = self._rev_parse('HEAD')

        result = self._sandbox.run_gc(['rebase-feature-branch'])

        self.assertEqual(result.returncode, 1)
        self.assertIn(f'Sync Chromium to {_NEXT}', result.stderr)
        self.assertEqual(self._rev_parse('HEAD'), head)

    def test_chromium_synced_to_the_target_rebases(self) -> None:
        self._fork_feature('--track', self._base)
        self._bump_base(_NEXT)
        self._sandbox.repo.add_tag(_NEXT)

        result = self._sandbox.run_gc(['rebase-feature-branch'])

        self.assertEqual(result.returncode, 0, msg=result.stderr)
        self.assertEqual(self._rev_parse('HEAD^'), self._rev_parse(self._base))

    def test_a_branch_changing_the_version_is_refused(self) -> None:
        self._fork_feature('--track', self._base)
        self._sandbox.repo.update_brave_version(_NEXT)
        head = self._rev_parse('HEAD')

        result = self._sandbox.run_gc(['rebase-feature-branch'])

        self.assertEqual(result.returncode, 1)
        self.assertIn('changes the Chromium version', result.stderr)
        self.assertEqual(self._rev_parse('HEAD'), head)

    def test_dry_run_with_chromium_not_synced_warns(self) -> None:
        self._fork_feature('--track', self._base)
        self._bump_base(_NEXT)

        result = self._sandbox.run_gc(['rebase-feature-branch', '--dry-run'])

        self.assertEqual(result.returncode, RebaseNeededException.EXIT_CODE)
        self.assertIn(f'sync Chromium to {_NEXT} first', result.stderr)

    def test_continue_rechecks_the_chromium_version(self) -> None:
        self._stop_on_a_conflict()
        self._sandbox.repo.add_tag(_NEXT)
        (self._sandbox.root / 'clash.txt').write_text(
            'fixed\n', encoding='utf-8'
        )
        _git(self._sandbox.root, 'add', 'clash.txt')

        result = self._sandbox.run_gc(['rebase-feature-branch', '--continue'])

        self.assertEqual(result.returncode, 1)
        self.assertIn(f'Sync Chromium to {_SYNCED}', result.stderr)

    def test_continue_without_a_rebase_is_an_error(self) -> None:
        result = self._sandbox.run_gc(['rebase-feature-branch', '--continue'])

        self.assertEqual(result.returncode, 1)

    def test_to_ref_is_refused_with_continue(self) -> None:
        result = self._sandbox.run_gc(
            ['rebase-feature-branch', '--continue', '--to-ref', self._base]
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn('--to-ref', result.stderr)


if __name__ == '__main__':
    unittest.main()
