#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `cherry_picks`.

The module's whole job is to drive real `git` against a checkout, so the tests
exercise it end to end over throwaway on-disk repositories rather than mocking
subprocess. Each test builds its repos in a temp dir, runs the `cherry_picks`
context manager, and asserts on the resulting git state (HEAD, commit metadata,
working tree).

Coverage:

* `NoopTest` — the paths that must leave the checkout untouched: an empty commit
  list, and commits already reachable from HEAD (which are skipped without even
  contacting `origin`).
* `ApplyTest` — a commit that needs applying is cherry-picked inside the context
  and HEAD is restored on exit, including when the body raises and when the
  commit must first be fetched from `origin`.
* `MetadataTest` — the committer identity and date are pinned (author is left as
  the original, matching `git cherry-pick` semantics) and the override table is
  configurable.
"""

from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import cherry_picks as m


def _git(repo: Path, *args: str, env: dict | None = None) -> str:
    """Run `git -C <repo> <args...>` and return stripped stdout."""
    return subprocess.run(('git', '-C', str(repo), *args),
                          check=True,
                          capture_output=True,
                          text=True,
                          env=env).stdout.strip()


def _init_repo(repo: Path) -> None:
    """Create *repo* as a git repo with a deterministic identity and one commit.

    Also enables `uploadpack.allowAnySHA1InWant` so a sibling repo can fetch an
    arbitrary commit SHA from it over the local transport (exercising the
    fetch-if-missing branch).
    """
    repo.mkdir(parents=True, exist_ok=True)
    _git(repo, 'init', '-q', '-b', 'main')
    _git(repo, 'config', 'user.name', 'Test')
    _git(repo, 'config', 'user.email', 'test@example.com')
    _git(repo, 'config', 'commit.gpgsign', 'false')
    _git(repo, 'config', 'uploadpack.allowAnySHA1InWant', 'true')
    _git(repo, 'config', 'uploadpack.allowReachableSHA1InWant', 'true')
    _commit_file(repo, 'base.txt', 'base\n', 'base commit')


def _commit_file(repo: Path, name: str, content: str, message: str) -> str:
    """Write *name*, commit it in *repo*, and return the new commit SHA."""
    (repo / name).write_text(content, encoding='utf-8')
    _git(repo, 'add', name)
    _git(repo, 'commit', '-q', '--no-gpg-sign', '-m', message)
    return _git(repo, 'rev-parse', 'HEAD')


class _RepoTestCase(unittest.TestCase):
    """Base fixture providing a temp dir and a helper to build repos."""

    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.root = Path(self._tmp.name)

    def _new_repo(self, name: str) -> Path:
        repo = self.root / name
        _init_repo(repo)
        return repo

    def _head(self, repo: Path) -> str:
        return _git(repo, 'rev-parse', 'HEAD')


class NoopTest(_RepoTestCase):
    """Paths where nothing is applied and the checkout is left untouched."""

    def test_empty_commit_list_yields_untouched(self) -> None:
        repo = self._new_repo('repo')
        head = self._head(repo)
        entered = False
        with m.cherry_picks(repo, []):
            entered = True
        self.assertTrue(entered)
        self.assertEqual(self._head(repo), head)

    def test_already_present_commit_is_skipped(self) -> None:
        repo = self._new_repo('repo')
        # A commit reachable from HEAD is already applied; the context is a
        # no-op and HEAD is unchanged.
        present = self._head(repo)
        with m.cherry_picks(repo, [present]):
            pass
        self.assertEqual(self._head(repo), present)

    def test_present_commit_does_not_contact_origin(self) -> None:
        repo = self._new_repo('repo')
        # Point origin at a path that does not exist. Because the commit object
        # is already present and reachable, the fetch branch must be skipped
        # entirely, so a broken remote is never contacted.
        _git(repo, 'remote', 'add', 'origin',
             str(self.root / 'does-not-exist'))
        present = self._head(repo)
        with m.cherry_picks(repo, [present]):
            pass
        self.assertEqual(self._head(repo), present)


class ApplyTest(_RepoTestCase):
    """A commit that needs applying is cherry-picked, then HEAD is restored."""

    def _repo_with_unmerged_commit(self) -> tuple[Path, str, str]:
        """Return (repo, base_head, commit) where commit is not on HEAD.

        The commit lives on a side branch and adds a new file, so it is neither
        reachable from `main`'s HEAD nor conflicting when cherry-picked onto it.
        """
        repo = self._new_repo('repo')
        base = self._head(repo)
        _git(repo, 'checkout', '-q', '-b', 'feature')
        commit = _commit_file(repo, 'feature.txt', 'feature\n',
                              'feature commit')
        _git(repo, 'checkout', '-q', 'main')
        self.assertEqual(self._head(repo), base)
        return repo, base, commit

    def test_applies_inside_and_restores_head_on_exit(self) -> None:
        repo, base, commit = self._repo_with_unmerged_commit()
        with m.cherry_picks(repo, [commit]):
            # Inside the context the commit is applied: HEAD advanced and the
            # file it introduced is present in the working tree.
            self.assertNotEqual(self._head(repo), base)
            self.assertTrue((repo / 'feature.txt').is_file())
        # On exit the checkout is reset to exactly the original HEAD.
        self.assertEqual(self._head(repo), base)
        self.assertFalse((repo / 'feature.txt').is_file())

    def test_head_restored_when_body_raises(self) -> None:
        repo, base, commit = self._repo_with_unmerged_commit()

        class _Boom(Exception):
            pass

        with self.assertRaises(_Boom):
            with m.cherry_picks(repo, [commit]):
                self.assertNotEqual(self._head(repo), base)
                raise _Boom()
        self.assertEqual(self._head(repo), base)

    def test_untracked_build_output_survives_restore(self) -> None:
        repo, _, commit = self._repo_with_unmerged_commit()
        # `reset --hard` on exit must not clobber untracked files (build
        # outputs), only tracked state.
        (repo / 'build-output.bin').write_text('artifact', encoding='utf-8')
        with m.cherry_picks(repo, [commit]):
            pass
        self.assertTrue((repo / 'build-output.bin').is_file())
        self.assertEqual((repo / 'build-output.bin').read_text(), 'artifact')

    def test_missing_object_is_fetched_from_origin(self) -> None:
        origin = self._new_repo('origin')
        # A commit that exists only on a side branch of `origin`, built on the
        # shared base so it cherry-picks cleanly onto the clone's HEAD.
        _git(origin, 'checkout', '-q', '-b', 'feature')
        commit = _commit_file(origin, 'fetched.txt', 'fetched\n',
                              'origin only')
        _git(origin, 'checkout', '-q', 'main')  # main stays at the base commit

        # A single-branch clone of `main` shares the base but never fetches the
        # `feature` branch, so the side commit's object is absent locally.
        # `--no-local` forces real fetch negotiation; without it a same-filesystem
        # clone hardlinks the entire object database and pulls the commit anyway.
        local = self.root / 'local'
        _git(self.root, 'clone', '-q', '--no-local', '--single-branch',
             '--branch', 'main', str(origin), str(local))
        _git(local, 'config', 'user.name', 'Test')
        _git(local, 'config', 'user.email', 'test@example.com')
        self.assertNotEqual(
            subprocess.run(('git', '-C', str(local), 'cat-file', '-e',
                            f'{commit}^{{commit}}'),
                           check=False,
                           capture_output=True).returncode, 0)

        local_base = self._head(local)
        with m.cherry_picks(local, [commit]):
            self.assertTrue((local / 'fetched.txt').is_file())
        self.assertEqual(self._head(local), local_base)

    def test_mixed_present_and_missing(self) -> None:
        repo, base, commit = self._repo_with_unmerged_commit()
        # `base` is already reachable (skipped); `commit` still needs applying.
        with m.cherry_picks(repo, [base, commit]):
            self.assertTrue((repo / 'feature.txt').is_file())
        self.assertEqual(self._head(repo), base)


class MetadataTest(_RepoTestCase):
    """The cherry-pick commit's identity is pinned deterministically."""

    def _apply_and_read(self, overrides: dict | None) -> dict[str, str]:
        repo = self._new_repo('repo')
        _git(repo, 'checkout', '-q', '-b', 'feature')
        commit = _commit_file(repo, 'feature.txt', 'feature\n',
                              'feature commit')
        _git(repo, 'checkout', '-q', 'main')
        fmt = '%cn%n%ce%n%cd%n%an%n%ae'
        kwargs = {} if overrides is None else {'metadata_overrides': overrides}
        result: dict[str, str] = {}
        with m.cherry_picks(repo, [commit], **kwargs):
            lines = _git(repo, 'log', '-1', f'--format={fmt}').splitlines()
            keys = ('committer_name', 'committer_email', 'committer_date',
                    'author_name', 'author_email')
            result = dict(zip(keys, lines))
        return result

    def test_default_overrides_pin_committer(self) -> None:
        got = self._apply_and_read(None)
        # `git cherry-pick` preserves the *author* of the original commit and
        # only the committer is rewritten, which is what the overrides pin.
        self.assertEqual(got['committer_name'],
                         m.GIT_METADATA_OVERRIDES['GIT_COMMITTER_NAME'])
        self.assertEqual(got['committer_email'],
                         m.GIT_METADATA_OVERRIDES['GIT_COMMITTER_EMAIL'])
        self.assertIn('2099', got['committer_date'])
        # The author is the original commit's, not the override.
        self.assertEqual(got['author_name'], 'Test')
        self.assertEqual(got['author_email'], 'test@example.com')

    def test_custom_overrides_are_honoured(self) -> None:
        overrides = {
            'GIT_COMMITTER_NAME': 'Custom Person',
            'GIT_COMMITTER_EMAIL': 'custom@example.com',
            'GIT_COMMITTER_DATE': '2001-02-03 04:05:06',
        }
        got = self._apply_and_read(overrides)
        self.assertEqual(got['committer_name'], 'Custom Person')
        self.assertEqual(got['committer_email'], 'custom@example.com')
        self.assertIn('2001', got['committer_date'])


if __name__ == '__main__':
    unittest.main()
