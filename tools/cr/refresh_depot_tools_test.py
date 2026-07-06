#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for refresh_depot_tools.py.

These exercise the real git plumbing end to end: local bare repositories stand
in for upstream googlesource and the brave-experiments mirror (the script's URL
constants are patched to point at them), so no network or SSH is involved.
"""

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import refresh_depot_tools as rdt


class RefreshDepotToolsTest(unittest.TestCase):

    def setUp(self):
        self._tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self._tmp, ignore_errors=True)

        # Point the script at local bare repos rather than the real remotes.
        self._orig_googlesource = rdt._GOOGLESOURCE_URL
        self._orig_brave_experiments = rdt._BRAVE_EXPERIMENTS_URL
        self.addCleanup(self._restore_urls)

        # Make git identity hermetic so commits/rebases don't depend on the
        # ambient (or absent) global git config.
        self._orig_environ = dict(os.environ)
        self.addCleanup(self._restore_environ)
        os.environ.update({
            'GIT_AUTHOR_NAME': 'Test',
            'GIT_AUTHOR_EMAIL': 'test@brave.com',
            'GIT_COMMITTER_NAME': 'Test',
            'GIT_COMMITTER_EMAIL': 'test@brave.com',
        })

    def _restore_urls(self):
        rdt._GOOGLESOURCE_URL = self._orig_googlesource
        rdt._BRAVE_EXPERIMENTS_URL = self._orig_brave_experiments

    def _restore_environ(self):
        os.environ.clear()
        os.environ.update(self._orig_environ)

    # -- Helpers ---------------------------------------------------------------

    def _git(self, cwd: Path, *args: str) -> str:
        return subprocess.check_output(['git', *args],
                                       cwd=str(cwd),
                                       text=True,
                                       stderr=subprocess.STDOUT).strip()

    def _subjects(self, repo: Path, ref: str) -> list[str]:
        """Commit subjects for `ref`, newest first."""
        return self._git(repo, 'log', '--format=%s', ref).splitlines()

    def _tree(self, repo: Path, ref: str) -> list[str]:
        """Top-level file names present at `ref`."""
        return sorted(
            self._git(repo, 'ls-tree', '--name-only', ref).splitlines())

    def _use_remotes(self, *, conflict: bool = False, with_patch: bool = True,
                     advance: bool = True) -> None:
        """Builds a scenario and points the script's URL constants at it.

        Layout produced:
          * upstream `main`: M0, then M1 when `advance` (M1 touches
            `brave_patch` when `conflict`, otherwise adds an unrelated file).
          * mirror `patches`/`main`: M0 plus a Brave patch commit (unless
            `with_patch` is False).

        With `advance=False` upstream stays at M0, so `patches` still sits on
        the current upstream tip and needs no rebase.

        Each call builds under its own subdirectory so a test can construct
        more than one scenario without path collisions.
        """
        base = Path(tempfile.mkdtemp(dir=self._tmp))
        upstream = base / 'upstream.git'
        mirror = base / 'mirror.git'
        self._git(base, 'init', '--bare', '-b', 'main', str(upstream))
        self._git(base, 'init', '--bare', '-b', 'main', str(mirror))

        seed = base / 'seed'
        self._git(base, 'init', '-b', 'main', str(seed))
        (seed / 'gclient').write_text('base\n')
        self._git(seed, 'add', '.')
        self._git(seed, 'commit', '-m', 'M0')
        self._git(seed, 'remote', 'add', 'origin', str(upstream))
        self._git(seed, 'push', 'origin', 'main')

        # Build the patches branch on top of M0 and publish it to the mirror.
        self._git(seed, 'checkout', '-b', 'patches')
        if with_patch:
            (seed / 'brave_patch').write_text('brave\n')
            self._git(seed, 'add', '.')
            self._git(seed, 'commit', '-m', 'brave patch')
        self._git(seed, 'remote', 'add', 'mirror', str(mirror))
        self._git(seed, 'push', 'mirror', 'patches')
        self._git(seed, 'push', 'mirror', 'patches:main')

        # Advance upstream main to M1 (unless the caller wants patches to stay
        # on the current tip).
        self._git(seed, 'checkout', 'main')
        if advance:
            self._advance_upstream(seed, conflict=conflict, name='M1')

        self._seed = seed
        self.upstream = upstream
        self.mirror = mirror
        rdt._GOOGLESOURCE_URL = str(upstream)
        rdt._BRAVE_EXPERIMENTS_URL = str(mirror)

    def _advance_upstream(self, seed: Path, *, conflict: bool,
                          name: str) -> None:
        if conflict:
            # Touch the same file the Brave patch adds, forcing a conflict.
            (seed / 'brave_patch').write_text(f'upstream-{name}\n')
        else:
            (seed / f'upstream_{name}').write_text('new\n')
        self._git(seed, 'add', '.')
        self._git(seed, 'commit', '-m', name)
        self._git(seed, 'push', 'origin', 'main')

    def _merge_into_patches(self, name: str) -> None:
        """Adds a commit to the mirror's patches branch out of band."""
        self._git(self._seed, 'checkout', 'patches')
        (self._seed / name).write_text('merged\n')
        self._git(self._seed, 'add', '.')
        self._git(self._seed, 'commit', '-m', name)
        self._git(self._seed, 'push', 'mirror', 'patches')
        self._git(self._seed, 'checkout', 'main')

    def _is_mid_rebase(self, checkout: Path) -> bool:
        git_dir = checkout / '.git'
        return (git_dir / 'rebase-merge').exists() or (git_dir /
                                                       'rebase-apply').exists()

    # -- Tests -----------------------------------------------------------------

    def test_fresh_clone_rebases_patches_onto_upstream(self):
        self._use_remotes()
        rdt.refresh(self._tmp / 'ws')

        # The Brave patch is replayed on top of the new upstream tip.
        self.assertEqual(self._subjects(self.mirror, 'patches'),
                         ['brave patch', 'M1', 'M0'])
        self.assertEqual(self._tree(self.mirror, 'patches'),
                         ['brave_patch', 'gclient', 'upstream_M1'])

    def test_main_is_reset_to_patches(self):
        self._use_remotes()
        rdt.refresh(self._tmp / 'ws')

        main_rev = self._git(self.mirror, 'rev-parse', 'main')
        patches_rev = self._git(self.mirror, 'rev-parse', 'patches')
        self.assertEqual(main_rev, patches_rev)

    def test_no_patch_commits_still_mirrors_upstream(self):
        # patches == M0 with no extra commits: after rebase it becomes M1.
        self._use_remotes(with_patch=False)
        rdt.refresh(self._tmp / 'ws')

        self.assertEqual(self._subjects(self.mirror, 'patches'), ['M1', 'M0'])
        self.assertEqual(self._git(self.mirror, 'rev-parse', 'main'),
                         self._git(self.mirror, 'rev-parse', 'patches'))

    def test_reused_checkout_rebases_onto_newer_upstream(self):
        self._use_remotes()
        clone_dir = self._tmp / 'ws'
        rdt.refresh(clone_dir)
        self.assertTrue((clone_dir / 'depot_tools' / '.git').is_dir())

        # Advance upstream again, then refresh reusing the same checkout.
        self._advance_upstream(self._seed, conflict=False, name='M2')
        rdt.refresh(clone_dir)

        self.assertEqual(self._subjects(self.mirror, 'patches'),
                         ['brave patch', 'M2', 'M1', 'M0'])
        self.assertEqual(self._git(self.mirror, 'rev-parse', 'main'),
                         self._git(self.mirror, 'rev-parse', 'patches'))

    def test_skips_rebase_and_main_update_when_up_to_date(self):
        # patches already sits on the upstream tip and main == patches, so
        # neither branch should be touched.
        self._use_remotes(advance=False)
        before_patches = self._git(self.mirror, 'rev-parse', 'patches')
        before_main = self._git(self.mirror, 'rev-parse', 'main')

        with self.assertLogs(rdt._LOG, level='INFO') as cm:
            rdt.refresh(self._tmp / 'ws')
        log = '\n'.join(cm.output)

        self.assertIn('skipping rebase', log)
        self.assertIn('nothing to update', log)
        self.assertEqual(self._git(self.mirror, 'rev-parse', 'patches'),
                         before_patches)
        self.assertEqual(self._git(self.mirror, 'rev-parse', 'main'),
                         before_main)

    def test_moves_main_when_commit_merged_into_patches(self):
        # No upstream movement (rebase skipped), but a commit was merged into
        # patches out of band, so main must advance to follow it.
        self._use_remotes(advance=False)
        self._merge_into_patches('feature')

        with self.assertLogs(rdt._LOG, level='INFO') as cm:
            rdt.refresh(self._tmp / 'ws')
        log = '\n'.join(cm.output)

        self.assertIn('skipping rebase', log)
        self.assertEqual(self._git(self.mirror, 'rev-parse', 'main'),
                         self._git(self.mirror, 'rev-parse', 'patches'))
        self.assertIn('feature', self._subjects(self.mirror, 'main'))

    def test_rebase_conflict_raises_and_leaves_mirror_untouched(self):
        self._use_remotes(conflict=True)

        before_patches = self._git(self.mirror, 'rev-parse', 'patches')
        before_main = self._git(self.mirror, 'rev-parse', 'main')

        clone_dir = self._tmp / 'ws'
        with self.assertRaises(rdt.RebaseError):
            rdt.refresh(clone_dir)

        # Nothing was force-pushed to the mirror.
        self.assertEqual(self._git(self.mirror, 'rev-parse', 'patches'),
                         before_patches)
        self.assertEqual(self._git(self.mirror, 'rev-parse', 'main'),
                         before_main)
        # The checkout is not left in a half-finished rebase.
        self.assertFalse(self._is_mid_rebase(clone_dir / 'depot_tools'))

    def test_main_returns_one_on_rebase_conflict(self):
        self._use_remotes(conflict=True)

        with mock.patch.object(
                rdt.sys, 'argv',
            ['refresh_depot_tools.py', '--clone-dir',
             str(self._tmp / 'ws')]):
            self.assertEqual(rdt.main(), 1)

    def test_main_returns_zero_on_success(self):
        self._use_remotes()
        with mock.patch.object(
                rdt.sys, 'argv',
            ['refresh_depot_tools.py', '--clone-dir',
             str(self._tmp / 'ws')]):
            self.assertEqual(rdt.main(), 0)


class EnsureRemoteTest(unittest.TestCase):
    """Unit tests for remote configuration helpers on a plain repo."""

    def setUp(self):
        self._tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self._tmp, ignore_errors=True)
        self.repo = self._tmp / 'repo'
        subprocess.check_call(['git', 'init', '-q', str(self.repo)])

    def _remote_url(self, name: str) -> str | None:
        return rdt._fetch_remotes(self.repo).get(name)

    def test_adds_missing_remote(self):
        rdt._ensure_remote(self.repo, rdt._fetch_remotes(self.repo), 'origin',
                           'https://example.com/a.git')
        self.assertEqual(self._remote_url('origin'),
                         'https://example.com/a.git')

    def test_updates_remote_url_when_changed(self):
        rdt._ensure_remote(self.repo, rdt._fetch_remotes(self.repo), 'origin',
                           'https://example.com/a.git')
        rdt._ensure_remote(self.repo, rdt._fetch_remotes(self.repo), 'origin',
                           'https://example.com/b.git')
        self.assertEqual(self._remote_url('origin'),
                         'https://example.com/b.git')

    def test_noop_when_url_matches(self):
        url = 'https://example.com/a.git'
        rdt._ensure_remote(self.repo, rdt._fetch_remotes(self.repo), 'origin',
                           url)
        # Passing the already-known remotes with the same URL must not error.
        rdt._ensure_remote(self.repo, rdt._fetch_remotes(self.repo), 'origin',
                           url)
        self.assertEqual(self._remote_url('origin'), url)


if __name__ == '__main__':
    unittest.main()
