#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Integration tests for cmd.py.

All tests run cmd.py as a subprocess against a FakeChromiumRepo, which stands
in as the brave checkout that `repository.brave.root` resolves to.  No
unittest.mock or monkeypatching is used for core logic; every test exercises
the actual Python + shell + git interaction.

Tests for the commit subcommand live in commit_test.py and import shared
helpers (_Sandbox, _run_gc, _GIT_ENV_OVERRIDES, CMD_SCRIPT, HOOK_SOURCE)
from this module.
"""

from __future__ import annotations

import os
import platform
import shutil
import stat
import subprocess
import sys
import unittest
from pathlib import Path

import _boot  # noqa: F401
from test.fake_chromium_repo import FakeChromiumRepo

CMD_SCRIPT: Path = Path(__file__).parent / 'cmd.py'
HOOK_SOURCE: Path = Path(__file__).parent / 'commit-msg.py'

# Minimal git environment that suppresses GPG signing and user-config lookup.
_GIT_ENV_OVERRIDES: dict[str, str] = {
    'GIT_AUTHOR_NAME': 'Test User',
    'GIT_AUTHOR_EMAIL': 'test@brave.com',
    'GIT_COMMITTER_NAME': 'Test User',
    'GIT_COMMITTER_EMAIL': 'test@brave.com',
    # Prevent any system/global hooks from interfering.
    'GIT_CONFIG_NOSYSTEM': '1',
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _path_without_git_cr_shim() -> str:
    """`$PATH` with every directory holding a `git-cr` executable removed.

    git resolves `git cr` to a `git-cr` on `$PATH` *before* consulting the
    `.git/config` alias, so a bootstrap shim installed on the developer's
    machine would shadow the alias this test is exercising. Dropping those
    directories keeps `git` itself resolvable while isolating the alias path.
    """
    entries = os.environ.get('PATH', '').split(os.pathsep)
    kept = [
        entry for entry in entries if entry and not any(
            (Path(entry) / name).exists() for name in ('git-cr', 'git-cr.bat'))
    ]
    return os.pathsep.join(kept)


def _git(cwd: Path, *args: str) -> subprocess.CompletedProcess:
    """Run a git command in cwd; raise on failure."""
    return subprocess.run(
        ['git'] + list(args),
        cwd=cwd,
        check=True,
        capture_output=True,
        text=True,
        env={
            **os.environ,
            **_GIT_ENV_OVERRIDES
        },
    )


def _run_gc(
    args: list[str],
    *,
    cwd: Path,
    env: dict[str, str] | None = None,
    stdin: str | None = None,
) -> subprocess.CompletedProcess:
    """Run git_cr.py as a subprocess and return the result."""
    full_env = {**os.environ, **_GIT_ENV_OVERRIDES, **(env or {})}
    return subprocess.run(
        [sys.executable, str(CMD_SCRIPT)] + args,
        cwd=cwd,
        env=full_env,
        capture_output=True,
        text=True,
        check=False,
        input=stdin,
    )


class _Sandbox:
    """A brave-named git checkout backed by FakeChromiumRepo.

    `cmd.py` resolves brave-core via `repository.brave.root`, which walks up
    from cwd to find a directory named 'brave'.  Running cmd.py with
    `cwd=self.root` makes it treat this fixture as the brave checkout.
    """

    def __init__(self) -> None:
        self._repo = FakeChromiumRepo()

    def __enter__(self) -> '_Sandbox':
        self._repo.setup()
        _git(self.root, 'config', 'commit.gpgsign', 'false')
        # Tests reference file.txt as the working file to stage and commit.
        (self.root / 'file.txt').write_text('init\n', encoding='utf-8')
        _git(self.root, 'add', 'file.txt')
        _git(self.root, 'commit', '-m', 'Add file.txt')
        return self

    def __exit__(self, *_) -> None:
        self._repo.cleanup()

    @property
    def root(self) -> Path:
        return self._repo.brave

    @property
    def hook_dest(self) -> Path:
        return self.root / '.git' / 'hooks' / 'commit-msg'

    def install_hook(self, *, as_copy: bool = False) -> None:
        """Put commit-msg.py in .git/hooks/commit-msg.

        By default a symlink is created (mirroring 'gc install-hook').
        Pass as_copy=True to simulate a manually-copied (potentially
        stale) hook.
        """
        hooks_dir = self.root / '.git' / 'hooks'
        hooks_dir.mkdir(exist_ok=True)
        dest = self.hook_dest
        if dest.exists() or dest.is_symlink():
            dest.unlink()
        if as_copy:
            shutil.copy2(HOOK_SOURCE, dest)
        else:
            dest.symlink_to(HOOK_SOURCE)
        # Ensure the hook (and source) are executable.
        HOOK_SOURCE.chmod(HOOK_SOURCE.stat().st_mode | stat.S_IXUSR
                          | stat.S_IXGRP | stat.S_IXOTH)
        dest.chmod(dest.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP
                   | stat.S_IXOTH)

    def stage_change(self, content: str = 'change\n') -> None:
        """Modify file.txt and stage it so there is something to commit."""
        (self.root / 'file.txt').write_text(content, encoding='utf-8')
        _git(self.root, 'add', 'file.txt')

    def last_commit_message(self) -> str:
        """Return the most recent commit message, stripped of trailing
        whitespace."""
        return subprocess.check_output(
            ['git', 'log', '-1', '--format=%B'],
            cwd=self.root,
            text=True,
            env={
                **os.environ,
                **_GIT_ENV_OVERRIDES
            },
        ).strip()

    def run_gc(
        self,
        args: list[str],
        *,
        env: dict[str, str] | None = None,
        stdin: str | None = None,
    ) -> subprocess.CompletedProcess:
        """Run git_cr.py from within this sandbox."""
        return _run_gc(args, cwd=self.root, env=env, stdin=stdin)

    def read_alias_cr(self) -> str:
        """Return the current alias.cr value from this sandbox's .git/config,
        or ''."""
        result = subprocess.run(
            ['git', 'config', '--local', 'alias.cr'],
            cwd=self.root,
            capture_output=True,
            check=False,
            text=True,
        )
        return result.stdout.strip()


# ---------------------------------------------------------------------------
# Helpers: fake global git config with core.hooksPath set
# ---------------------------------------------------------------------------


def _fake_global_env(hooks_dir: Path) -> dict[str, str]:
    """Return env overrides that point git's global config to a temp gitconfig
    with core.hooksPath set to hooks_dir, without touching ~/.gitconfig."""
    cfg = hooks_dir.parent / 'gitconfig'
    cfg.write_text(f'[core]\n\thooksPath = {hooks_dir}\n', encoding='utf-8')
    return {'GIT_CONFIG_GLOBAL': str(cfg)}


# ---------------------------------------------------------------------------
# Tests: core.hooksPath guard
# ---------------------------------------------------------------------------


class TestCoreHooksPathGuard(unittest.TestCase):
    """install-hook and commit must error when core.hooksPath redirects
    hooks."""

    def setUp(self) -> None:
        self._sandbox = _Sandbox()
        self._sandbox.__enter__()
        self._fake_hooks = self._sandbox.root.parent / 'fake_hooks'
        self._fake_hooks.mkdir()
        self._env = _fake_global_env(self._fake_hooks)

    def tearDown(self) -> None:
        self._sandbox.__exit__(None, None, None)

    def test_install_hook_blocked(self) -> None:
        """install-hook exits non-zero and mentions core.hooksPath."""
        result = self._sandbox.run_gc(['install-hook'], env=self._env)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('core.hooksPath', result.stderr)

    def test_install_hook_does_not_write_hook(self) -> None:
        """install-hook must not create the symlink when blocked."""
        dest = self._sandbox.hook_dest
        if dest.exists() or dest.is_symlink():
            dest.unlink()
        self._sandbox.run_gc(['install-hook'], env=self._env)
        self.assertFalse(dest.exists() or dest.is_symlink())

    def test_commit_blocked(self) -> None:
        """commit exits non-zero and mentions core.hooksPath."""
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(['commit', '-m', 'msg'], env=self._env)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('core.hooksPath', result.stderr)


# ---------------------------------------------------------------------------
# Tests: install-hook
# ---------------------------------------------------------------------------


class TestInstallHook(unittest.TestCase):
    """git cr install-hook creates the correct symlink (or shim on Windows)."""

    def setUp(self) -> None:
        self._sandbox = _Sandbox()
        self._sandbox.__enter__()

    def tearDown(self) -> None:
        self._sandbox.__exit__(None, None, None)

    def _run(self) -> subprocess.CompletedProcess:
        return self._sandbox.run_gc(['install-hook'])

    def test_creates_symlink(self) -> None:
        """install-hook creates .git/hooks/commit-msg as a symlink on POSIX."""
        if platform.system() == 'Windows':
            self.skipTest('symlink test is POSIX-only')
        result = self._run()
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        dest = self._sandbox.hook_dest
        self.assertTrue(dest.is_symlink(), 'Expected a symlink')
        self.assertEqual(dest.resolve(), HOOK_SOURCE.resolve())

    def test_overwrites_existing_hook(self) -> None:
        """install-hook replaces an existing hook file with a symlink."""
        dest = self._sandbox.hook_dest
        dest.parent.mkdir(exist_ok=True)
        if dest.exists() or dest.is_symlink():
            dest.unlink()
        dest.write_text('old hook\n', encoding='utf-8')

        result = self._run()
        self.assertEqual(result.returncode, 0)
        self.assertTrue(dest.is_symlink())

    def test_hook_source_is_executable_after_install(self) -> None:
        """The hook source file is executable after install-hook runs."""
        if platform.system() == 'Windows':
            self.skipTest('executable bit is POSIX-only')
        self._run()
        self.assertTrue(HOOK_SOURCE.stat().st_mode & stat.S_IXUSR)

    def test_output_mentions_paths(self) -> None:
        """install-hook prints both the destination and source paths."""
        result = self._run()
        self.assertIn('commit-msg', result.stdout)
        self.assertEqual(result.returncode, 0)


# ---------------------------------------------------------------------------
# Tests: setup-alias
# ---------------------------------------------------------------------------


class TestSetupAlias(unittest.TestCase):
    """git cr setup-alias must register a working git alias in .git/config."""

    def setUp(self) -> None:
        self._sandbox = _Sandbox()
        self._sandbox.__enter__()
        self._sandbox.install_hook()

    def tearDown(self) -> None:
        self._sandbox.__exit__(None, None, None)

    def _run(self) -> subprocess.CompletedProcess:
        return self._sandbox.run_gc(['setup-alias'])

    def test_writes_alias_to_git_config(self) -> None:
        """setup-alias stores alias.cr in .git/config."""
        result = self._run()
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        self.assertNotEqual(self._sandbox.read_alias_cr(), '')

    def test_alias_references_script_and_vpython3(self) -> None:
        """The stored alias calls vpython3 with the cmd.py path."""
        self._run()
        alias = self._sandbox.read_alias_cr()
        self.assertIn('vpython3', alias)
        self.assertIn('cmd.py', alias)

    def test_alias_starts_with_exclamation(self) -> None:
        """The alias uses the '!' prefix so git treats it as a shell command."""
        self._run()
        self.assertTrue(self._sandbox.read_alias_cr().startswith('!'))

    def test_git_cr_commit_works_end_to_end(self) -> None:
        """After setup-alias, 'git cr -m msg' creates a commit in the sandbox.
        """
        self._run()
        self._sandbox.stage_change()
        result = subprocess.run(
            ['git', 'cr', 'commit', '-m', 'Alias end-to-end'],
            cwd=self._sandbox.root,
            capture_output=True,
            text=True,
            check=False,
            env={
                **os.environ,
                **_GIT_ENV_OVERRIDES,
                # Isolate the alias under test from any bootstrap `git-cr`
                # shim installed on this machine, which git would otherwise
                # prefer over the .git/config alias.
                'PATH': _path_without_git_cr_shim(),
            },
        )
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        self.assertIn('Alias end-to-end', self._sandbox.last_commit_message())


if __name__ == '__main__':
    unittest.main()
