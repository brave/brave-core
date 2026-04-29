#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Integration tests for git_cr.py.

All tests run git_cr.py as a subprocess against real git repositories created in
temporary directories.  No unittest.mock or monkeypatching is used for core
logic; every test exercises the actual Python + shell + git interaction.
"""

import os
import platform
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Optional

GC_SCRIPT: Path = Path(__file__).parent / 'git_cr.py'
HOOK_SOURCE: Path = Path(__file__).parent / 'commit-msg.py'

# These mirror the module-level constants in git_cr.py so tests can assert
# against the same paths the script uses.
_REPO_ROOT: Path = GC_SCRIPT.parent.parent.parent
_HOOK_DEST: Path = _REPO_ROOT / '.git' / 'hooks' / 'commit-msg'

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
    env: Optional[dict[str, str]] = None,
    stdin: Optional[str] = None,
) -> subprocess.CompletedProcess:
    """Run git_cr.py as a subprocess and return the result."""
    full_env = {**os.environ, **_GIT_ENV_OVERRIDES, **(env or {})}
    return subprocess.run(
        [sys.executable, str(GC_SCRIPT)] + args,
        cwd=cwd,
        env=full_env,
        capture_output=True,
        text=True,
        check=False,
        input=stdin,
    )


class _Sandbox:
    """Context manager providing a fresh, isolated git repository."""

    def __init__(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)

    def __enter__(self) -> '_Sandbox':
        _git(self.root.parent, 'init', str(self.root))
        _git(self.root, 'config', 'user.email', 'test@brave.com')
        _git(self.root, 'config', 'user.name', 'Test User')
        _git(self.root, 'config', 'commit.gpgsign', 'false')
        # Create an initial commit so HEAD exists when the hook runs.
        (self.root / 'file.txt').write_text('init\n', encoding='utf-8')
        _git(self.root, 'add', 'file.txt')
        _git(self.root, 'commit', '-m', 'Initial commit')
        return self

    def __exit__(self, *_) -> None:
        self._tmp.cleanup()

    # ------------------------------------------------------------------
    # Sandbox utilities
    # ------------------------------------------------------------------

    def install_hook(self, *, as_copy: bool = False) -> None:
        """Put commit-msg.py in .git/hooks/commit-msg.

        By default a symlink is created (mirroring 'gc install-hook').
        Pass as_copy=True to simulate a manually-copied (potentially
        stale) hook.
        """
        hooks_dir = self.root / '.git' / 'hooks'
        hooks_dir.mkdir(exist_ok=True)
        dest = hooks_dir / 'commit-msg'
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
        env: Optional[dict[str, str]] = None,
        stdin: Optional[str] = None,
    ) -> subprocess.CompletedProcess:
        """Run git_cr.py from within this sandbox."""
        return _run_gc(args, cwd=self.root, env=env, stdin=stdin)


# ---------------------------------------------------------------------------
# Tests: flag pass-through
# ---------------------------------------------------------------------------


class TestFlagPassthrough(unittest.TestCase):
    """gc must strip its own flags and forward everything else to git."""

    def setUp(self) -> None:
        self._sandbox = _Sandbox()
        self._sandbox.__enter__()
        self._hook_state = _HookState()
        self._hook_state.save()
        _run_gc(['install-hook'], cwd=_REPO_ROOT)

    def tearDown(self) -> None:
        self._hook_state.restore()
        self._sandbox.__exit__(None, None, None)

    def test_custom_flags_stripped_before_git(self) -> None:
        """--tagged, --issue, --culprit are stripped; git never receives them.

        Each flag is tested in isolation with a staged change and no hook.  If
        any flag leaked through, git would exit non-zero with 'unknown option'.
        A clean exit proves gc removed it from the argv it forwarded.
        """
        for flag, value in [
            ('--tagged', 'WIP'),
            ('--issue', '42'),
            ('--culprit', 'deadbeef'),
        ]:
            with self.subTest(flag=flag):
                self._sandbox.stage_change(f'content for {flag}\n')
                result = self._sandbox.run_gc([
                    'commit', flag, value, '--no-verify', '-m', f'test {flag}'
                ])
                self.assertEqual(
                    result.returncode,
                    0,
                    msg=f'{flag} may have leaked to git: {result.stderr!r}',
                )

    def test_unknown_flag_reaches_git(self) -> None:
        """gc forwards unknown flags to git, which then reports the error."""
        result = self._sandbox.run_gc(['commit', '--tgas', '-m', 'msg'])
        # The error must come from git, not from gc.
        self.assertNotEqual(result.returncode, 0)
        git_error = result.stderr + result.stdout
        self.assertTrue(
            'unknown' in git_error or 'error' in git_error,
            msg=f'Expected a git error, got: {git_error!r}',
        )

    def test_no_verify_passes_through(self) -> None:
        """--no-verify reaches git (bypass hook) and the commit succeeds."""
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(
            ['commit', '--no-verify', '-m', 'bare commit'])
        self.assertEqual(result.returncode, 0)

    def test_short_flags_pass_through(self) -> None:
        """Single-dash flags (-m) are forwarded verbatim."""
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(['commit', '-m', 'short flag test'])
        self.assertEqual(result.returncode, 0)

    def test_amend_passes_through(self) -> None:
        """--amend is forwarded to git without gc interference."""
        # We only care that gc does not produce its own error about --amend.
        result = self._sandbox.run_gc(['commit', '--amend', '--no-edit'])
        # git may fail (nothing new to amend), but any error is git's, not gc's.
        git_error = result.stderr + result.stdout
        self.assertNotIn('git_cr:', git_error)


# ---------------------------------------------------------------------------
# Tests: commit integration (requires hook)
# ---------------------------------------------------------------------------


class TestCommitIntegration(unittest.TestCase):
    """gc must inject env vars that the commit-msg hook reads correctly."""

    def setUp(self) -> None:
        self._sandbox = _Sandbox()
        self._sandbox.__enter__()
        self._sandbox.install_hook()
        self._hook_state = _HookState()
        self._hook_state.save()
        _run_gc(['install-hook'], cwd=_REPO_ROOT)

    def tearDown(self) -> None:
        self._hook_state.restore()
        self._sandbox.__exit__(None, None, None)

    def test_commit_without_flags_succeeds(self) -> None:
        """A plain commit via gc works end-to-end."""
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(['commit', '-m', 'Plain commit'])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        self.assertIn('Plain commit', self._sandbox.last_commit_message())

    def test_tagged_flag_prepends_tag(self) -> None:
        """--tagged WIP results in [WIP] prepended to the commit message."""
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(
            ['commit', '--tagged', 'WIP', '-m', 'My change'])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        msg = self._sandbox.last_commit_message()
        self.assertIn('[WIP]', msg)
        self.assertIn('My change', msg)

    def test_tagged_multiple_tags(self) -> None:
        """--tagged WIP,CodeHealth adds both [WIP] and [CodeHealth] tags."""
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(
            ['commit', '--tagged', 'WIP,CodeHealth', '-m', 'Multi-tag'])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        msg = self._sandbox.last_commit_message()
        self.assertIn('[WIP]', msg)
        self.assertIn('[CodeHealth]', msg)

    def test_issue_flag_appends_resolves_link(self) -> None:
        """--issue 1234 appends a Resolves link to the commit message."""
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(
            ['commit', '--issue', '1234', '-m', 'Fix something'])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        msg = self._sandbox.last_commit_message()
        self.assertIn('Resolves', msg)
        self.assertIn('1234', msg)
        self.assertIn('brave-browser/issues/1234', msg)

    def test_issue_multiple_numbers(self) -> None:
        """--issue 11,22 adds Resolves links for both issues."""
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(
            ['commit', '--issue', '11,22', '-m', 'Multi-issue fix'])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        msg = self._sandbox.last_commit_message()
        self.assertIn('issues/11', msg)
        self.assertIn('issues/22', msg)

    def test_tagged_and_issue_combined(self) -> None:
        """--tagged and --issue can be used together."""
        self._sandbox.stage_change()
        result = self._sandbox.run_gc([
            'commit', '--tagged', 'canary', '--issue', '999', '-m', 'Combined'
        ])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        msg = self._sandbox.last_commit_message()
        self.assertIn('[canary]', msg)
        self.assertIn('issues/999', msg)

    def test_fixup_commit_skips_hook_processing(self) -> None:
        """fixup! commits bypass tag insertion (hook behaviour)."""
        # First commit to have something to fixup.
        self._sandbox.stage_change('v1\n')
        self._sandbox.run_gc(['commit', '-m', 'Base commit'])
        self._sandbox.stage_change('v2\n')
        result = self._sandbox.run_gc(
            ['commit', '--tagged', 'WIP', '-m', 'fixup! Base commit'])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        msg = self._sandbox.last_commit_message()
        # The hook skips fixup! commits — no tag should be added.
        self.assertNotIn('[WIP]', msg)
        self.assertTrue(msg.startswith('fixup!'))

    def test_empty_message_rejected(self) -> None:
        """A commit with only whitespace in the message is rejected."""
        self._sandbox.stage_change()
        # Pass the message via a temp file so the hook sees it.
        result = self._sandbox.run_gc(['commit', '-m', '   '])
        # Either git or the hook rejects this; either way exit != 0.
        self.assertNotEqual(result.returncode, 0)


# ---------------------------------------------------------------------------
# Helpers: save/restore state for tests that touch the real repo
# ---------------------------------------------------------------------------


class _HookState:
    """Save and restore the real brave-core commit-msg hook around a test.

    Management commands (install-hook, self-update) write to _HOOK_DEST inside
    the actual brave-core checkout rather than a sandbox, so tests must leave
    the repo in its original state.
    """

    def __init__(self) -> None:
        self._was_symlink: bool = False
        self._symlink_target: Optional[Path] = None
        self._content: Optional[bytes] = None

    def save(self) -> None:
        if _HOOK_DEST.is_symlink():
            self._was_symlink = True
            self._symlink_target = Path(os.readlink(_HOOK_DEST))
        elif _HOOK_DEST.exists():
            self._content = _HOOK_DEST.read_bytes()

    def restore(self) -> None:
        if _HOOK_DEST.exists() or _HOOK_DEST.is_symlink():
            _HOOK_DEST.unlink()
        if self._was_symlink and self._symlink_target:
            _HOOK_DEST.symlink_to(self._symlink_target)
        elif self._content is not None:
            _HOOK_DEST.write_bytes(self._content)


def _read_alias_cr() -> str:
    """Return the current alias.cr value from brave-core's .git/config,
    or ''."""
    result = subprocess.run(
        ['git', 'config', '--local', 'alias.cr'],
        cwd=_REPO_ROOT,
        capture_output=True,
        check=False,
        text=True,
    )
    return result.stdout.strip()


def _set_alias_cr(value: str) -> None:
    """Write alias.cr to brave-core's .git/config."""
    subprocess.run(
        ['git', 'config', '--local', 'alias.cr', value],
        cwd=_REPO_ROOT,
        check=True,
        capture_output=True,
    )


def _unset_alias_cr() -> None:
    """Remove alias.cr from brave-core's .git/config if present."""
    subprocess.run(
        ['git', 'config', '--local', '--unset', 'alias.cr'],
        cwd=_REPO_ROOT,
        capture_output=True,
        check=False,
    )


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
        self._tmp = tempfile.TemporaryDirectory()
        self._fake_hooks = Path(self._tmp.name) / 'hooks'
        self._fake_hooks.mkdir()
        self._env = _fake_global_env(self._fake_hooks)
        self._hook_state = _HookState()
        self._hook_state.save()

    def tearDown(self) -> None:
        self._hook_state.restore()
        self._tmp.cleanup()

    def test_install_hook_blocked(self) -> None:
        """install-hook exits non-zero and mentions core.hooksPath."""
        result = _run_gc(['install-hook'], cwd=_REPO_ROOT, env=self._env)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('core.hooksPath', result.stderr)

    def test_install_hook_does_not_write_hook(self) -> None:
        """install-hook must not create the symlink when blocked."""
        if _HOOK_DEST.exists() or _HOOK_DEST.is_symlink():
            _HOOK_DEST.unlink()
        _run_gc(['install-hook'], cwd=_REPO_ROOT, env=self._env)
        self.assertFalse(_HOOK_DEST.exists() or _HOOK_DEST.is_symlink())

    def test_commit_blocked(self) -> None:
        """commit exits non-zero and mentions core.hooksPath."""
        sandbox = _Sandbox()
        sandbox.__enter__()
        try:
            sandbox.stage_change()
            result = sandbox.run_gc(['commit', '-m', 'msg'], env=self._env)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('core.hooksPath', result.stderr)
        finally:
            sandbox.__exit__(None, None, None)


# ---------------------------------------------------------------------------
# Tests: install-hook
# ---------------------------------------------------------------------------


class TestInstallHook(unittest.TestCase):
    """git cr install-hook creates the correct symlink (or shim on Windows)."""

    def setUp(self) -> None:
        self._state = _HookState()
        self._state.save()

    def tearDown(self) -> None:
        self._state.restore()

    def _run(self) -> subprocess.CompletedProcess:
        return _run_gc(['install-hook'], cwd=_REPO_ROOT)

    def test_creates_symlink(self) -> None:
        """install-hook creates .git/hooks/commit-msg as a symlink on POSIX."""
        if platform.system() == 'Windows':
            self.skipTest('symlink test is POSIX-only')
        result = self._run()
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        self.assertTrue(_HOOK_DEST.is_symlink(), 'Expected a symlink')
        self.assertEqual(_HOOK_DEST.resolve(), HOOK_SOURCE.resolve())

    def test_overwrites_existing_hook(self) -> None:
        """install-hook replaces an existing hook file with a symlink."""
        _HOOK_DEST.parent.mkdir(exist_ok=True)
        if _HOOK_DEST.exists() or _HOOK_DEST.is_symlink():
            _HOOK_DEST.unlink()
        _HOOK_DEST.write_text('old hook\n', encoding='utf-8')

        result = self._run()
        self.assertEqual(result.returncode, 0)
        self.assertTrue(_HOOK_DEST.is_symlink())

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
        self._prior_alias = _read_alias_cr()
        self._hook_state = _HookState()
        self._hook_state.save()
        _run_gc(['install-hook'], cwd=_REPO_ROOT)

    def tearDown(self) -> None:
        self._hook_state.restore()
        if self._prior_alias:
            _set_alias_cr(self._prior_alias)
        else:
            _unset_alias_cr()

    def _run(self) -> subprocess.CompletedProcess:
        return _run_gc(['setup-alias'], cwd=_REPO_ROOT)

    def test_writes_alias_to_git_config(self) -> None:
        """setup-alias stores alias.cr in .git/config."""
        result = self._run()
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        self.assertNotEqual(_read_alias_cr(), '')

    def test_alias_references_script_and_python3(self) -> None:
        """The stored alias calls python3 with the git_cr.py path."""
        self._run()
        alias = _read_alias_cr()
        self.assertIn('python3', alias)
        self.assertIn('git_cr.py', alias)

    def test_alias_starts_with_exclamation(self) -> None:
        """The alias uses the '!' prefix so git treats it as a shell command."""
        self._run()
        self.assertTrue(_read_alias_cr().startswith('!'))

    def test_git_cr_commit_works_end_to_end(self) -> None:
        """After setup-alias, 'git cr -m msg' creates a commit in a sandbox.

        setup-alias writes to _REPO_ROOT's .git/config; the alias value is read
        back and re-applied to the sandbox repo so git cr works there too.
        """
        self._run()
        alias = _read_alias_cr()
        sandbox = _Sandbox()
        sandbox.__enter__()
        try:
            # Mirror the alias into the sandbox so 'git cr' resolves there.
            subprocess.run(['git', 'config', '--local', 'alias.cr', alias],
                           cwd=sandbox.root,
                           check=True,
                           capture_output=True)
            sandbox.install_hook()
            sandbox.stage_change()
            result = subprocess.run(
                ['git', 'cr', 'commit', '-m', 'Alias end-to-end'],
                cwd=sandbox.root,
                capture_output=True,
                text=True,
                check=False,
                env={
                    **os.environ,
                    **_GIT_ENV_OVERRIDES
                },
            )
            msg = sandbox.last_commit_message()
        finally:
            sandbox.__exit__(None, None, None)
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        self.assertIn('Alias end-to-end', msg)


# ---------------------------------------------------------------------------
# Tests: cmd_commit hook sanity check
# ---------------------------------------------------------------------------


class TestCommitSanityCheck(unittest.TestCase):
    """cmd_commit must refuse early when the hook is absent or misconfigured."""

    def setUp(self) -> None:
        self._sandbox = _Sandbox()
        self._sandbox.__enter__()
        self._hook_state = _HookState()
        self._hook_state.save()

    def tearDown(self) -> None:
        self._hook_state.restore()
        self._sandbox.__exit__(None, None, None)

    def test_missing_hook_is_rejected(self) -> None:
        """cmd_commit exits non-zero and mentions install-hook when absent."""
        if _HOOK_DEST.exists() or _HOOK_DEST.is_symlink():
            _HOOK_DEST.unlink()
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(['commit', '-m', 'should fail'])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('install-hook', result.stderr)

    @unittest.skipIf(platform.system() == 'Windows',
                     'executable bit is POSIX-only')
    def test_non_executable_hook_is_rejected(self) -> None:
        """cmd_commit exits non-zero when the hook lacks the executable bit."""
        _run_gc(['install-hook'], cwd=_REPO_ROOT)
        # _HOOK_DEST is a symlink → _HOOK_SOURCE; stat() follows the link, so
        # stripping the bit from the source is sufficient to fail the check.
        original_mode = HOOK_SOURCE.stat().st_mode
        HOOK_SOURCE.chmod(original_mode & ~(stat.S_IXUSR | stat.S_IXGRP
                                            | stat.S_IXOTH))
        try:
            self._sandbox.stage_change()
            result = self._sandbox.run_gc(['commit', '-m', 'should fail'])
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('install-hook', result.stderr)
        finally:
            HOOK_SOURCE.chmod(original_mode)


# ---------------------------------------------------------------------------
# Tests: KeyboardInterrupt / graceful exit
# ---------------------------------------------------------------------------


class TestGracefulExit(unittest.TestCase):
    """gc must not emit Python tracebacks on Ctrl-C."""

    def setUp(self) -> None:
        self._hook_state = _HookState()
        self._hook_state.save()
        _run_gc(['install-hook'], cwd=_REPO_ROOT)

    def tearDown(self) -> None:
        self._hook_state.restore()

    def test_no_traceback_on_keyboard_interrupt(self) -> None:
        """Sending SIGINT produces exit code 130 with no traceback."""
        import signal

        with tempfile.TemporaryDirectory() as tmp:
            proc = subprocess.Popen(
                [sys.executable,
                 str(GC_SCRIPT), 'commit', '-m', 'test'],
                cwd=tmp,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                env={
                    **os.environ,
                    **_GIT_ENV_OVERRIDES
                },
            )
            try:
                proc.send_signal(signal.SIGINT)
                _, stderr = proc.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                _, stderr = proc.communicate()

        self.assertNotIn('Traceback', stderr)
        # 130 = Python caught KeyboardInterrupt and called sys.exit(130).
        # -2  = OS killed the process with SIGINT before Python handled it.
        # Both indicate clean termination; which one occurs is timing-dependent.
        self.assertIn(proc.returncode, (-2, 130))


if __name__ == '__main__':
    unittest.main()
