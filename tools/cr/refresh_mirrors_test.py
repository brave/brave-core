#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for refresh_mirrors.py.

Tests use real git repositories in temporary directories so that git
operations are exercised end-to-end. The Gerrit instance is replaced by a
`FakeGerrit` whose "projects" are local bare repos, so project creation and
pushes are real git operations against the filesystem rather than network calls.
"""

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import refresh_mirrors
from refresh_mirrors import (Gerrit, discover_cache_repos, is_cache_repo,
                             project_name_for, refresh_repo)


def _git(*cmd: str, cwd: Path) -> str:
    """Run a git command and return stripped stdout, raising on failure."""
    return subprocess.run(['git', *cmd],
                          cwd=cwd,
                          capture_output=True,
                          text=True,
                          check=True).stdout.strip()


def _make_cache_repo(path: Path, upstream_url: str) -> tuple[str, str]:
    """Create a bare cache-style repo at *path* with one commit on its branch.

    Mimics a depot_tools git_cache entry: a bare repo whose `origin` remote URL
    is the upstream the mirror is derived from. Returns (branch, head_sha).
    """
    work = path.parent / f'{path.name}.work'
    subprocess.run(['git', 'init', '-q', str(work)],
                   check=True,
                   capture_output=True)
    _git('config', 'user.email', 'test@example.com', cwd=work)
    _git('config', 'user.name', 'Test', cwd=work)
    (work / 'README').write_text('hello')
    _git('add', '.', cwd=work)
    _git('commit', '-q', '-m', 'initial', cwd=work)
    branch = _git('rev-parse', '--abbrev-ref', 'HEAD', cwd=work)
    head = _git('rev-parse', 'HEAD', cwd=work)

    subprocess.run(
        ['git', 'clone', '-q', '--bare',
         str(work), str(path)],
        check=True,
        capture_output=True)
    _git('config', 'remote.origin.url', upstream_url, cwd=path)
    return branch, head


def _add_commits(bare_repo: Path, count: int) -> None:
    """Add *count* linear commits to *bare_repo*'s default branch.

    Uses a throwaway working tree so the bare cache repo gains real history to
    seed in chunks.
    """
    work = Path(tempfile.mkdtemp())
    try:
        subprocess.run(
            ['git', 'clone', '-q',
             str(bare_repo), str(work)],
            check=True,
            capture_output=True)
        _git('config', 'user.email', 'test@example.com', cwd=work)
        _git('config', 'user.name', 'Test', cwd=work)
        # Unique filenames per call so repeated _add_commits don't collide.
        base = _git('rev-list', '--count', 'HEAD', cwd=work)
        for i in range(count):
            name = f'file_{base}_{i}'
            (work / name).write_text(str(i))
            _git('add', '.', cwd=work)
            _git('commit', '-q', '-m', f'commit {name}', cwd=work)
        _git('push', '-q', 'origin', 'HEAD', cwd=work)
    finally:
        shutil.rmtree(work, ignore_errors=True)


class FakeGerrit:
    """Stand-in for `Gerrit` whose projects are local bare repos on disk."""

    def __init__(self, root: Path, dry_run: bool = False):
        self._root = root
        self.dry_run = dry_run
        self.existing: set[str] = set()
        self.created: list[str] = []

    def project_url(self, project: str) -> str:
        return str(self._root / project)

    def project_exists(self, project: str) -> bool:
        return project in self.existing

    def create_project(self, project: str) -> None:
        if self.dry_run:
            return
        path = self._root / project
        path.mkdir(parents=True)
        subprocess.run(['git', 'init', '-q', '--bare',
                        str(path)],
                       check=True,
                       capture_output=True)
        # A real Gerrit advertises push options; enable it so pushes carrying
        # `-o skip-validation` are accepted instead of rejected outright.
        _git('config', 'receive.advertisePushOptions', 'true', cwd=path)
        self.existing.add(project)
        self.created.append(project)


class TestProjectName(unittest.TestCase):
    """Unit tests for deriving a Gerrit project name from an upstream URL."""

    def test_keeps_host_and_strips_git_suffix(self):
        self.assertEqual(
            project_name_for('https://aomedia.googlesource.com/aom.git'),
            'mirror/aomedia.googlesource.com/aom')

    def test_nested_path(self):
        self.assertEqual(
            project_name_for(
                'https://chromium.googlesource.com/chromium/dom-distiller/'
                'dist.git'),
            'mirror/chromium.googlesource.com/chromium/dom-distiller/dist')

    def test_without_git_suffix(self):
        self.assertEqual(
            project_name_for('https://chromium.googlesource.com/chromium/src'),
            'mirror/chromium.googlesource.com/chromium/src')

    def test_empty_path_raises(self):
        with self.assertRaises(ValueError):
            project_name_for('https://chromium.googlesource.com')


class TestCacheDiscovery(unittest.TestCase):
    """Tests for finding bare cache repos and skipping non-repos."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.cache = Path(self._tmp.name)

    def tearDown(self):
        self._tmp.cleanup()

    def test_finds_bare_repo(self):
        _make_cache_repo(self.cache / 'example.com-foo',
                         'https://example.com/foo.git')
        repos = discover_cache_repos(self.cache)
        self.assertIn('example.com-foo', [p.name for p in repos])

    def test_skips_locked_dir(self):
        locked = self.cache / 'example.com-foo.locked'
        locked.mkdir()
        (locked / 'config').write_text('[core]\n')
        self.assertFalse(is_cache_repo(locked))

    def test_skips_stray_file(self):
        (self.cache / 'not-a-repo').write_text('x')
        self.assertEqual(discover_cache_repos(self.cache), [])

    def test_skips_non_bare_directory(self):
        plain = self.cache / 'plain'
        plain.mkdir()
        (plain / 'config').write_text('[core]\n')
        self.assertFalse(is_cache_repo(plain))


class TestEnsureGerritRemote(unittest.TestCase):
    """Tests for adding/updating the `gerrit` remote on a cache repo."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)
        self.repo = self.tmp / 'repo'
        _make_cache_repo(self.repo, 'https://example.com/foo.git')

    def tearDown(self):
        self._tmp.cleanup()

    def test_adds_remote_when_absent(self):
        refresh_mirrors._ensure_gerrit_remote(self.repo, 'ssh://host/foo')
        self.assertEqual(_git('remote', 'get-url', 'gerrit', cwd=self.repo),
                         'ssh://host/foo')

    def test_updates_stale_remote(self):
        refresh_mirrors._ensure_gerrit_remote(self.repo, 'ssh://host/old')
        refresh_mirrors._ensure_gerrit_remote(self.repo, 'ssh://host/new')
        self.assertEqual(_git('remote', 'get-url', 'gerrit', cwd=self.repo),
                         'ssh://host/new')


class TestRefreshRepo(unittest.TestCase):
    """End-to-end tests for mirroring one cache repo into a (fake) Gerrit."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)
        self.cache_repo = self.tmp / 'cache' / 'example.com-foo'
        self.cache_repo.parent.mkdir()
        self.branch, self.head = _make_cache_repo(
            self.cache_repo, 'https://example.com/group/foo.git')
        self.gerrit = FakeGerrit(self.tmp / 'gerrit')

    def tearDown(self):
        self._tmp.cleanup()

    def _mirror_head(self, project: str) -> str:
        return _git('rev-parse',
                    f'refs/heads/{self.branch}',
                    cwd=self.tmp / 'gerrit' / project)

    def test_creates_project_and_pushes(self):
        refresh_repo(self.cache_repo, self.gerrit)
        self.assertEqual(self.gerrit.created, ['mirror/example.com/group/foo'])
        self.assertEqual(self._mirror_head('mirror/example.com/group/foo'),
                         self.head)

    def test_existing_project_not_recreated(self):
        self.gerrit.create_project('mirror/example.com/group/foo')
        self.gerrit.created.clear()
        refresh_repo(self.cache_repo, self.gerrit)
        self.assertEqual(self.gerrit.created, [])
        self.assertEqual(self._mirror_head('mirror/example.com/group/foo'),
                         self.head)

    def test_force_push_after_history_rewrite(self):
        refresh_repo(self.cache_repo, self.gerrit)
        # Rewrite the cache branch to an unrelated commit (non-fast-forward).
        work = self.tmp / 'rewrite'
        subprocess.run(['git', 'init', '-q', str(work)],
                       check=True,
                       capture_output=True)
        _git('config', 'user.email', 'test@example.com', cwd=work)
        _git('config', 'user.name', 'Test', cwd=work)
        (work / 'OTHER').write_text('different')
        _git('add', '.', cwd=work)
        _git('commit', '-q', '-m', 'rewrite', cwd=work)
        rewritten = _git('rev-parse', 'HEAD', cwd=work)
        _git('push',
             '-q',
             '--force',
             str(self.cache_repo),
             f'HEAD:refs/heads/{self.branch}',
             cwd=work)

        refresh_repo(self.cache_repo, self.gerrit)
        self.assertEqual(self._mirror_head('mirror/example.com/group/foo'),
                         rewritten)

    def test_dry_run_creates_and_pushes_nothing(self):
        self.gerrit.dry_run = True
        refresh_repo(self.cache_repo, self.gerrit)
        self.assertEqual(self.gerrit.created, [])
        self.assertFalse(
            (self.tmp / 'gerrit' / 'mirror/example.com/group/foo').exists())

    def test_pushes_only_the_default_branch(self):
        # An extra branch in the cache must not reach the mirror: only the ref
        # HEAD points at is pushed.
        _git('branch', 'extra', self.head, cwd=self.cache_repo)
        refresh_repo(self.cache_repo, self.gerrit)
        mirror = self.tmp / 'gerrit' / 'mirror/example.com/group/foo'
        refs = _git('for-each-ref', '--format=%(refname)', cwd=mirror)
        self.assertIn(f'refs/heads/{self.branch}', refs.splitlines())
        self.assertNotIn('refs/heads/extra', refs.splitlines())

    def test_push_uses_skip_validation(self):
        with mock.patch.object(refresh_mirrors, '_run') as run:
            refresh_repo(self.cache_repo, self.gerrit)
        push = [c.args for c in run.call_args_list if 'push' in c.args]
        self.assertEqual(len(push), 1)
        argv = push[0]
        self.assertEqual(argv[argv.index('-o') + 1], 'skip-validation')

    def test_dangling_head_mirrors_all_branches(self):
        # Reproduce a Chromium external mirror: HEAD points at a branch that does
        # not exist, while the real branch lives elsewhere. The repo must still
        # be mirrored (all heads) rather than fail on the missing ref.
        _git('branch', '-m', self.branch, 'upstream/main', cwd=self.cache_repo)
        _git('symbolic-ref', 'HEAD', 'refs/heads/master', cwd=self.cache_repo)
        refresh_repo(self.cache_repo, self.gerrit)
        self.assertEqual(self.gerrit.created, ['mirror/example.com/group/foo'])
        mirror = self.tmp / 'gerrit' / 'mirror/example.com/group/foo'
        self.assertEqual(
            _git('rev-parse', 'refs/heads/upstream/main', cwd=mirror),
            self.head)

    def test_dangling_head_pushes_all_branches_in_batches(self):
        # Many branches + dangling HEAD (e.g. LiteRT's 2000+ chromium/* branches)
        # must be pushed in batches, not one oversized ref update.
        names = ['upstream/main', 'chromium/1', 'chromium/2', 'chromium/3']
        for name in names:
            _git('branch', name, self.head, cwd=self.cache_repo)
        _git('symbolic-ref',
             'HEAD',
             'refs/heads/nonexistent',
             cwd=self.cache_repo)
        with mock.patch.object(refresh_mirrors, 'MIRROR_REFS_PER_PUSH', 2), \
             mock.patch.object(refresh_mirrors, '_run',
                               wraps=refresh_mirrors._run) as run:
            refresh_repo(self.cache_repo, self.gerrit)
        pushes = [c.args for c in run.call_args_list if 'push' in c.args]
        # 5 branches (original + 4) at 2 per push -> 3 batched pushes.
        self.assertEqual(len(pushes), 3)
        mirror = self.tmp / 'gerrit' / 'mirror/example.com/group/foo'
        mirrored = _git('for-each-ref',
                        '--format=%(refname)',
                        'refs/heads/',
                        cwd=mirror).splitlines()
        for name in names + [f'{self.branch}']:
            self.assertIn(f'refs/heads/{name}', mirrored)

    def test_repo_without_any_branch_is_skipped(self):
        # Dangling HEAD *and* no branches at all -> nothing to mirror.
        empty = self.tmp / 'cache' / 'empty'
        subprocess.run(['git', 'init', '-q', '--bare',
                        str(empty)],
                       check=True,
                       capture_output=True)
        _git('config',
             'remote.origin.url',
             'https://example.com/empty.git',
             cwd=empty)
        refresh_repo(empty, self.gerrit)
        self.assertEqual(self.gerrit.created, [])

    def test_repo_without_origin_url_is_skipped(self):
        _git('remote', 'remove', 'origin', cwd=self.cache_repo)
        refresh_repo(self.cache_repo, self.gerrit)
        self.assertEqual(self.gerrit.created, [])


class TestLargeRepoSeeding(unittest.TestCase):
    """Tests for chunked seeding of repos listed in LARGE_REPOS."""

    PROJECT = 'mirror/example.com/group/foo'
    # Tiny chunk so a handful of commits seeds in several pushes.
    CHUNK = 2

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)
        self.cache_repo = self.tmp / 'cache' / 'example.com-foo'
        self.cache_repo.parent.mkdir()
        self.branch, _ = _make_cache_repo(self.cache_repo,
                                          'https://example.com/group/foo.git')
        _add_commits(self.cache_repo, 5)
        self.gerrit = FakeGerrit(self.tmp / 'gerrit')
        self._patches = [
            mock.patch.object(refresh_mirrors, 'LARGE_REPOS',
                              frozenset({self.PROJECT})),
            mock.patch.object(refresh_mirrors, 'LARGE_REPO_COMMIT_CHUNK',
                              self.CHUNK),
        ]
        for p in self._patches:
            p.start()

    def tearDown(self):
        for p in self._patches:
            p.stop()
        self._tmp.cleanup()

    def _cache_head(self) -> str:
        return _git('rev-parse',
                    f'refs/heads/{self.branch}',
                    cwd=self.cache_repo)

    def _mirror_head(self) -> str:
        return _git('rev-parse',
                    f'refs/heads/{self.branch}',
                    cwd=self.tmp / 'gerrit' / self.PROJECT)

    def _push_count(self) -> int:
        with mock.patch.object(refresh_mirrors,
                               '_run',
                               wraps=refresh_mirrors._run) as run:
            refresh_repo(self.cache_repo, self.gerrit)
        return len([c.args for c in run.call_args_list if 'push' in c.args])

    def test_checkpoints_advance_and_end_at_tip(self):
        cps = refresh_mirrors._commit_checkpoints(self.cache_repo,
                                                  f'refs/heads/{self.branch}',
                                                  self.CHUNK)
        self.assertGreater(len(cps), 1)  # split, not a single checkpoint
        self.assertEqual(cps[-1], self._cache_head())
        # Each checkpoint is an ancestor of the next (fast-forwarding order).
        for older, newer in zip(cps, cps[1:]):
            self.assertEqual(
                _git('merge-base', older, newer, cwd=self.cache_repo), older)

    def test_seeds_in_chunks_and_reaches_tip(self):
        self.assertGreater(self._push_count(), 1)  # chunked, not a single push
        self.assertEqual(self._mirror_head(), self._cache_head())

    def test_resume_advances_from_remote_tip(self):
        # Seed, then add more commits: the second run resumes from the remote
        # tip and still reaches the new head.
        refresh_repo(self.cache_repo, self.gerrit)
        self.assertEqual(self._mirror_head(), self._cache_head())
        _add_commits(self.cache_repo, 3)
        new_head = self._cache_head()
        # Resume start is the already-mirrored tip, so only the new commits ship.
        self.assertEqual(
            refresh_mirrors._remote_branch_sha(self.cache_repo,
                                               f'refs/heads/{self.branch}'),
            self._mirror_head())
        refresh_repo(self.cache_repo, self.gerrit)
        self.assertEqual(self._mirror_head(), new_head)

    def test_up_to_date_pushes_nothing(self):
        # Once fully seeded with no new commits (and no tags), a large repo
        # pushes nothing -- it never falls through to a full-ref push.
        refresh_repo(self.cache_repo, self.gerrit)
        self.assertEqual(self._push_count(), 0)

    def _mirror_tags(self) -> list[str]:
        return _git('for-each-ref',
                    '--format=%(refname)',
                    'refs/tags/',
                    cwd=self.tmp / 'gerrit' / self.PROJECT).splitlines()

    def test_pushes_tags(self):
        _git('tag', 'v1', self._cache_head(), cwd=self.cache_repo)
        _git('tag', 'v2', self._cache_head(), cwd=self.cache_repo)
        refresh_repo(self.cache_repo, self.gerrit)
        self.assertEqual(self._mirror_head(), self._cache_head())
        self.assertIn('refs/tags/v1', self._mirror_tags())
        self.assertIn('refs/tags/v2', self._mirror_tags())

    def test_only_new_tags_pushed_on_rerun(self):
        _git('tag', 'v1', self._cache_head(), cwd=self.cache_repo)
        refresh_repo(self.cache_repo, self.gerrit)  # seeds + pushes v1
        _git('tag', 'v2', self._cache_head(), cwd=self.cache_repo)
        with mock.patch.object(refresh_mirrors,
                               '_run',
                               wraps=refresh_mirrors._run) as run:
            refresh_repo(self.cache_repo, self.gerrit)
        pushed = [a for c in run.call_args_list for a in c.args]
        self.assertTrue(any('refs/tags/v2' in a for a in pushed))
        self.assertFalse(any('refs/tags/v1' in a for a in pushed))
        self.assertIn('refs/tags/v2', self._mirror_tags())


class TestPushNewTags(unittest.TestCase):
    """New and changed tags are pushed; tags already identical are skipped."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)
        self.repo = self.tmp / 'repo'
        _, self.head = _make_cache_repo(self.repo, 'https://example.com/r.git')
        _git('tag', 'same', self.head, cwd=self.repo)  # identical on server
        _git('tag', 'changed', self.head, cwd=self.repo)  # differs on server
        _git('tag', 'fresh', self.head, cwd=self.repo)  # not on server

        self.server = self.tmp / 'server'
        subprocess.run(['git', 'init', '-q', '--bare',
                        str(self.server)],
                       check=True,
                       capture_output=True)
        _git('config', 'receive.advertisePushOptions', 'true', cwd=self.server)
        # Server: `same` at the same object, `changed` at a different one.
        _git('push',
             str(self.server),
             f'+{self.head}:refs/tags/same',
             cwd=self.repo)
        other = self.tmp / 'other'
        _make_cache_repo(other, 'https://example.com/o.git')
        _add_commits(other, 1)  # a genuinely distinct commit
        other_head = _git('rev-parse', 'HEAD', cwd=other)
        self.assertNotEqual(other_head, self.head)
        _git('push',
             str(self.server),
             f'+{other_head}:refs/tags/changed',
             cwd=other)
        _git('remote', 'add', 'gerrit', str(self.server), cwd=self.repo)

    def tearDown(self):
        self._tmp.cleanup()

    def _pushed_refspecs(self, run) -> list[str]:
        return [a for c in run.call_args_list for a in c.args]

    def test_pushes_new_and_changed_only(self):
        with mock.patch.object(refresh_mirrors,
                               '_run',
                               wraps=refresh_mirrors._run) as run:
            refresh_mirrors._push_new_tags(self.repo)
        pushed = self._pushed_refspecs(run)
        self.assertTrue(any('refs/tags/fresh' in a for a in pushed))  # new
        self.assertTrue(any('refs/tags/changed' in a for a in pushed))  # moved
        self.assertFalse(any('refs/tags/same' in a
                             for a in pushed))  # identical

    def test_no_push_when_identical(self):
        # Sync the server to match every local tag; a rerun pushes nothing.
        _git('push',
             str(self.server),
             '+refs/tags/fresh:refs/tags/fresh',
             '+refs/tags/changed:refs/tags/changed',
             cwd=self.repo)
        with mock.patch.object(refresh_mirrors,
                               '_run',
                               wraps=refresh_mirrors._run) as run:
            refresh_mirrors._push_new_tags(self.repo)
        self.assertEqual(
            [c.args for c in run.call_args_list if 'push' in c.args], [])


class TestManagedDepotTools(unittest.TestCase):
    """depot_tools as a managed mirror: only main, no tags or other branches."""

    PROJECT = 'brave-browser/forks/depot_tools'

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)
        self.cache = self.tmp / 'cache'
        self.cache.mkdir()

        self.upstream = self.tmp / 'upstream'
        _make_cache_repo(self.upstream, 'https://example.com/depot_tools.git')
        self.head = _git('rev-parse', 'HEAD', cwd=self.upstream)
        _git('update-ref', 'refs/heads/main', self.head, cwd=self.upstream)
        _git('update-ref', 'refs/heads/feature', self.head, cwd=self.upstream)
        _git('tag', 'v1', self.head, cwd=self.upstream)

        self.fork_path = self.tmp / 'gerrit' / self.PROJECT
        self.fork_path.mkdir(parents=True)
        subprocess.run(['git', 'init', '-q', '--bare',
                        str(self.fork_path)],
                       check=True,
                       capture_output=True)
        _git('config',
             'receive.advertisePushOptions',
             'true',
             cwd=self.fork_path)
        self.gerrit = FakeGerrit(self.tmp / 'gerrit')
        self.mirror = refresh_mirrors.ManagedMirror('depot_tools',
                                                    str(self.upstream),
                                                    self.PROJECT,
                                                    'refs/heads/main')

    def tearDown(self):
        self._tmp.cleanup()

    def _fork_refs(self, pattern: str) -> list[str]:
        return _git('for-each-ref',
                    '--format=%(refname)',
                    pattern,
                    cwd=self.fork_path).splitlines()

    def test_flags_default_off(self):
        self.assertFalse(self.mirror.channels)
        self.assertFalse(self.mirror.tags)

    def test_mirrors_only_main(self):
        with mock.patch.object(refresh_mirrors, 'LARGE_REPO_COMMIT_CHUNK', 1):
            self.assertTrue(
                refresh_mirrors.refresh_managed(self.mirror, self.cache,
                                                self.gerrit))
        self.assertEqual(self._fork_refs('refs/heads/'), ['refs/heads/main'])
        self.assertEqual(
            _git('rev-parse', 'refs/heads/main', cwd=self.fork_path),
            self.head)
        # No tags and no other branches were mirrored.
        self.assertEqual(self._fork_refs('refs/tags/'), [])
        self.assertNotIn('refs/heads/feature', self._fork_refs('refs/heads/'))
        # The project pre-exists; nothing is created.
        self.assertEqual(self.gerrit.created, [])


class TestManagedMirror(unittest.TestCase):
    """Tests for a managed (self-fetched) large mirror like brave-core."""

    PROJECT = 'brave-browser/brave-core'

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)
        self.cache = self.tmp / 'cache'
        self.cache.mkdir()

        # A local stand-in for the GitHub upstream, with master + channel
        # branches + tags + an unrelated feature branch that must NOT mirror.
        self.upstream = self.tmp / 'upstream'
        self.upstream.parent.mkdir(exist_ok=True)
        _make_cache_repo(self.upstream, 'https://example.com/brave-core.git')
        default = _git('rev-parse', '--abbrev-ref', 'HEAD', cwd=self.upstream)
        self.head = _git('rev-parse', 'HEAD', cwd=self.upstream)
        _git('update-ref', 'refs/heads/master', self.head, cwd=self.upstream)
        _git('update-ref', 'refs/heads/1.92.x', self.head, cwd=self.upstream)
        _git('update-ref', 'refs/heads/1.93.x', self.head, cwd=self.upstream)
        _git('update-ref',
             'refs/heads/feature-foo',
             self.head,
             cwd=self.upstream)
        _git('tag', 'v1.92.1', self.head, cwd=self.upstream)

        self.fork_path = self.tmp / 'gerrit' / self.PROJECT
        self.fork_path.mkdir(parents=True)
        subprocess.run(['git', 'init', '-q', '--bare',
                        str(self.fork_path)],
                       check=True,
                       capture_output=True)
        _git('config',
             'receive.advertisePushOptions',
             'true',
             cwd=self.fork_path)
        self.gerrit = FakeGerrit(self.tmp / 'gerrit')
        self.mirror = refresh_mirrors.ManagedMirror('brave-core',
                                                    str(self.upstream),
                                                    self.PROJECT,
                                                    'refs/heads/master',
                                                    channels=True,
                                                    tags=True)

    def tearDown(self):
        self._tmp.cleanup()

    def _fork_refs(self, pattern: str) -> list[str]:
        return _git('for-each-ref',
                    '--format=%(refname)',
                    pattern,
                    cwd=self.fork_path).splitlines()

    def test_mirrors_master_channels_and_tags_only(self):
        with mock.patch.object(refresh_mirrors, 'LARGE_REPO_COMMIT_CHUNK', 1):
            self.assertTrue(
                refresh_mirrors.refresh_managed(self.mirror, self.cache,
                                                self.gerrit))
        heads = self._fork_refs('refs/heads/')
        self.assertIn('refs/heads/master', heads)
        self.assertIn('refs/heads/1.92.x', heads)
        self.assertIn('refs/heads/1.93.x', heads)
        # The feature branch is not a channel branch: never mirrored.
        self.assertNotIn('refs/heads/feature-foo', heads)
        self.assertIn('refs/tags/v1.92.1', self._fork_refs('refs/tags/'))

    def test_managed_bare_repo_lives_outside_discovery(self):
        with mock.patch.object(refresh_mirrors, 'LARGE_REPO_COMMIT_CHUNK', 1):
            refresh_mirrors.refresh_managed(self.mirror, self.cache,
                                            self.gerrit)
        # The managed bare repo is nested under MANAGED_SUBDIR, so a cache scan
        # never treats it as a mirrorable cache repo.
        self.assertEqual(refresh_mirrors.discover_cache_repos(self.cache), [])
        self.assertTrue((self.cache / refresh_mirrors.MANAGED_SUBDIR /
                         'brave-core' / 'config').is_file())

    def test_reuses_managed_repo_without_recloning(self):
        with mock.patch.object(refresh_mirrors, 'LARGE_REPO_COMMIT_CHUNK', 1):
            refresh_mirrors.refresh_managed(self.mirror, self.cache,
                                            self.gerrit)
            # A second run must reuse the existing bare repo, never `git init`.
            with mock.patch.object(refresh_mirrors,
                                   '_run',
                                   wraps=refresh_mirrors._run) as run:
                refresh_mirrors.refresh_managed(self.mirror, self.cache,
                                                self.gerrit)
        inits = [c.args for c in run.call_args_list if 'init' in c.args]
        self.assertEqual(inits, [])

    def test_rerun_syncs_latest_channels_and_tags(self):
        managed = self.cache / refresh_mirrors.MANAGED_SUBDIR / 'brave-core'
        with mock.patch.object(refresh_mirrors, 'LARGE_REPO_COMMIT_CHUNK', 1):
            refresh_mirrors.refresh_managed(self.mirror, self.cache,
                                            self.gerrit)
            # Upstream changes: a channel is cut, another removed, a tag added.
            _git('update-ref',
                 'refs/heads/1.94.x',
                 self.head,
                 cwd=self.upstream)
            _git('update-ref', '-d', 'refs/heads/1.93.x', cwd=self.upstream)
            _git('tag', 'v1.94.1', self.head, cwd=self.upstream)
            refresh_mirrors.refresh_managed(self.mirror, self.cache,
                                            self.gerrit)

        local_heads = _git('for-each-ref',
                           '--format=%(refname)',
                           'refs/heads/',
                           cwd=managed).splitlines()
        self.assertIn('refs/heads/1.94.x', local_heads)  # newly cut channel
        self.assertNotIn('refs/heads/1.93.x', local_heads)  # pruned locally
        self.assertIn(
            'refs/tags/v1.94.1',
            _git('for-each-ref',
                 '--format=%(refname)',
                 'refs/tags/',
                 cwd=managed).splitlines())
        # And the new channel reached the mirror.
        self.assertIn('refs/heads/1.94.x', self._fork_refs('refs/heads/'))

    def test_channel_branch_regex(self):
        matches = refresh_mirrors.CHANNEL_BRANCH_RE.match
        self.assertTrue(matches('refs/heads/1.92.x'))
        self.assertTrue(matches('refs/heads/1.100.x'))
        self.assertFalse(matches('refs/heads/master'))
        self.assertFalse(matches('refs/heads/1.92.0'))
        self.assertFalse(matches('refs/heads/feature/1.92.x'))


class TestRefreshMirrorsSummary(unittest.TestCase):
    """Tests for the end-of-run summary and per-repo outcome tallying."""

    def test_categorizes_and_summarizes(self):
        repos = [Path('/cache/aaa'), Path('/cache/bbb'), Path('/cache/ccc')]
        with mock.patch.object(refresh_mirrors, 'discover_cache_repos',
                               return_value=repos), \
             mock.patch.object(refresh_mirrors, 'MANAGED_MIRRORS', []), \
             mock.patch.object(refresh_mirrors, 'refresh_repo') as refresh, \
             self.assertLogs(level='INFO') as logs:
            # aaa updated, bbb skipped, ccc failed.
            refresh.side_effect = [
                True, False,
                subprocess.CalledProcessError(1, ['git', 'push'])
            ]
            failures = refresh_mirrors.refresh_mirrors(Path('/cache'),
                                                       gerrit=object())

        self.assertEqual(failures, 1)
        text = '\n'.join(logs.output)
        self.assertIn('updated: 1  skipped: 1  failed: 1', text)
        self.assertIn('+ aaa', text)
        self.assertIn('. bbb', text)
        self.assertIn('! ccc', text)

    def test_no_owned_repos_refresh_skips_managed(self):
        with mock.patch.object(refresh_mirrors, 'discover_cache_repos',
                               return_value=[]), \
             mock.patch.object(refresh_mirrors, 'refresh_managed') as managed:
            refresh_mirrors.refresh_mirrors(Path('/cache'),
                                            gerrit=object(),
                                            refresh_owned_repos=False)
        managed.assert_not_called()

    def test_no_mirrors_refresh_skips_cache(self):
        with mock.patch.object(refresh_mirrors, 'MANAGED_MIRRORS', []), \
             mock.patch.object(refresh_mirrors,
                               'discover_cache_repos') as discover:
            refresh_mirrors.refresh_mirrors(Path('/cache'),
                                            gerrit=object(),
                                            refresh_cache_mirrors=False)
        discover.assert_not_called()


class TestGerritCli(unittest.TestCase):
    """Tests for the real Gerrit CLI wrappers, with subprocess mocked out."""

    def setUp(self):
        self.gerrit = Gerrit(user='bot')

    def test_project_url(self):
        self.assertEqual(self.gerrit.project_url('mirror/foo'),
                         'ssh://bot@gerrit-ssh.brave.com:29418/mirror/foo')

    def test_project_exists_exact_match(self):
        # ls-projects --prefix returns names sharing the prefix; only an exact
        # match counts as the project existing.
        out = 'mirror/foo\nmirror/foobar\n'
        with mock.patch.object(refresh_mirrors,
                               '_query',
                               return_value=subprocess.CompletedProcess(
                                   [], 0, stdout=out, stderr='')):
            self.assertTrue(self.gerrit.project_exists('mirror/foo'))
            self.assertFalse(self.gerrit.project_exists('mirror/fo'))

    def test_project_exists_raises_on_query_failure(self):
        with mock.patch.object(refresh_mirrors,
                               '_query',
                               return_value=subprocess.CompletedProcess(
                                   [], 255, stdout='', stderr='denied')):
            with self.assertRaises(RuntimeError):
                self.gerrit.project_exists('mirror/foo')

    def test_create_project_argv(self):
        with mock.patch.object(refresh_mirrors, '_run') as run:
            self.gerrit.create_project('mirror/foo')
        argv = list(run.call_args.args)
        self.assertEqual(argv[:3], ['ssh', '-p', '29418'])
        self.assertIn('create-project', argv)
        self.assertEqual(argv[argv.index('--owner') + 1], 'mirrors-admins')
        self.assertEqual(argv[argv.index('--parent') + 1], 'mirror')

    def test_create_project_dry_run_does_nothing(self):
        gerrit = Gerrit(user='bot', dry_run=True)
        with mock.patch.object(refresh_mirrors, '_run') as run:
            gerrit.create_project('mirror/foo')
        run.assert_not_called()


if __name__ == '__main__':
    unittest.main()
