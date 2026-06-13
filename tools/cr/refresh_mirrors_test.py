#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for refresh_mirrors.py.

Tests use real git repositories in temporary directories so that all git
operations are exercised end-to-end rather than mocked.
"""

import subprocess
import tempfile
import unittest
from pathlib import Path

from refresh_mirrors import BRAVE_GERRIT_URL, Mirror


def _git(*cmd: str, cwd: Path) -> str:
    """Run a git command and return stdout, raising on non-zero exit."""
    result = subprocess.run(['git', *cmd],
                            cwd=cwd,
                            capture_output=True,
                            text=True,
                            check=True)
    return result.stdout.strip()


class TestMirrorLocalPath(unittest.TestCase):
    """Unit tests for local path derivation from the mirror particle."""

    def test_local_path_joins_correctly(self):
        m = Mirror(mirror='googlesource/chromium',
                   upstream='https://example.com/up',
                   refs=[],
                   mirrors_path=Path('/mirrors'),
                   gerrit_url=BRAVE_GERRIT_URL)
        self.assertEqual(m.local_path(),
                         Path('/mirrors/googlesource/chromium'))


class TestMirrorSetup(unittest.TestCase):
    """Tests for bare repository creation and remote configuration."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)
        self.mirrors_path = self.tmp / 'mirrors'

    def tearDown(self):
        self._tmp.cleanup()

    def _make_mirror(self, upstream: str = 'https://example.com/up') -> Mirror:
        return Mirror(mirror='test/repo',
                      upstream=upstream,
                      refs=['refs/heads/*:refs/heads/*'],
                      mirrors_path=self.mirrors_path,
                      gerrit_url=BRAVE_GERRIT_URL)

    def test_setup_creates_bare_repo(self):
        self._make_mirror()._setup()
        repo = self.mirrors_path / 'test/repo'
        self.assertTrue((repo / 'HEAD').exists())

    def test_setup_creates_intermediate_directories(self):
        self._make_mirror()._setup()
        self.assertTrue((self.mirrors_path / 'test').is_dir())

    def test_setup_configures_upstream_remote(self):
        self._make_mirror('https://example.com/upstream')._setup()
        url = _git('remote',
                   'get-url',
                   'upstream',
                   cwd=self.mirrors_path / 'test/repo')
        self.assertEqual(url, 'https://example.com/upstream')

    def test_setup_configures_mirror_remote(self):
        self._make_mirror()._setup()
        url = _git('remote',
                   'get-url',
                   'mirror',
                   cwd=self.mirrors_path / 'test/repo')
        self.assertEqual(url, f'{BRAVE_GERRIT_URL}/test/repo')

    def test_setup_updates_stale_upstream_remote(self):
        # Initial setup with one URL, then update to a new one.
        Mirror(mirror='test/repo',
               upstream='https://example.com/old',
               refs=[],
               mirrors_path=self.mirrors_path,
               gerrit_url=BRAVE_GERRIT_URL)._setup()
        Mirror(mirror='test/repo',
               upstream='https://example.com/new',
               refs=[],
               mirrors_path=self.mirrors_path,
               gerrit_url=BRAVE_GERRIT_URL)._setup()
        url = _git('remote',
                   'get-url',
                   'upstream',
                   cwd=self.mirrors_path / 'test/repo')
        self.assertEqual(url, 'https://example.com/new')

    def test_setup_is_idempotent(self):
        m = self._make_mirror()
        m._setup()
        m._setup()
        url = _git('remote',
                   'get-url',
                   'upstream',
                   cwd=self.mirrors_path / 'test/repo')
        self.assertEqual(url, 'https://example.com/up')


class TestMirrorFetchPush(unittest.TestCase):
    """Integration tests for fetch/push using real local git repositories."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)
        self.mirrors_path = self.tmp / 'mirrors'

        # Upstream: a normal repo with an initial commit.
        self.upstream = self.tmp / 'upstream'
        self.upstream.mkdir()
        subprocess.run(['git', 'init', str(self.upstream)],
                       check=True,
                       capture_output=True)
        subprocess.run([
            'git', '-C',
            str(self.upstream), 'config', 'user.email', 'test@example.com'
        ],
                       check=True,
                       capture_output=True)
        subprocess.run(
            ['git', '-C',
             str(self.upstream), 'config', 'user.name', 'Test'],
            check=True,
            capture_output=True)
        (self.upstream / 'README').write_text('hello')
        subprocess.run(
            ['git', '-C', str(self.upstream), 'add', '.'],
            check=True,
            capture_output=True)
        subprocess.run(
            ['git', '-C',
             str(self.upstream), 'commit', '-m', 'initial'],
            check=True,
            capture_output=True)
        self.branch = _git('rev-parse',
                           '--abbrev-ref',
                           'HEAD',
                           cwd=self.upstream)

        # Mirror target: a bare repo that receives pushes.
        self.mirror_target = self.tmp / 'mirror_target'
        subprocess.run(['git', 'init', '--bare',
                        str(self.mirror_target)],
                       check=True,
                       capture_output=True)

    def tearDown(self):
        self._tmp.cleanup()

    def _setup_mirror(self) -> Mirror:
        """Create and set up a Mirror, then redirect its mirror remote to the
        local mirror_target so pushes work without a real Gerrit server."""
        mirror = Mirror(mirror='test/repo',
                        upstream=str(self.upstream),
                        refs=['refs/heads/*:refs/heads/*'],
                        mirrors_path=self.mirrors_path,
                        gerrit_url=BRAVE_GERRIT_URL)
        mirror._setup()
        subprocess.run(
            ['git', 'remote', 'set-url', 'mirror',
             str(self.mirror_target)],
            cwd=mirror.local_path(),
            check=True,
            capture_output=True)
        return mirror

    def test_fetch_and_push_sync_refs(self):
        mirror = self._setup_mirror()
        mirror.fetch()
        mirror.push()

        upstream_sha = _git('rev-parse', 'HEAD', cwd=self.upstream)
        mirror_sha = _git('rev-parse',
                          f'refs/heads/{self.branch}',
                          cwd=self.mirror_target)
        self.assertEqual(upstream_sha, mirror_sha)

    def test_sync_fetch_failure_raises_and_skips_push(self):
        """When fetch fails, sync raises CalledProcessError."""
        mirror = Mirror(mirror='test/repo',
                        upstream=str(self.tmp / 'no_such_repo'),
                        refs=['refs/heads/*:refs/heads/*'],
                        mirrors_path=self.mirrors_path,
                        gerrit_url=BRAVE_GERRIT_URL)

        with self.assertRaises(subprocess.CalledProcessError):
            mirror.sync()

        # Push was never called so the mirror target must have no refs.
        result = subprocess.run(
            ['git', '-C', str(self.mirror_target), 'show-ref'],
            capture_output=True,
            text=True,
            check=False)
        self.assertEqual(result.stdout.strip(), '')


if __name__ == '__main__':
    unittest.main()
