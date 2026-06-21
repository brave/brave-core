#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Integration tests for commit.py.

All tests run cmd.py as a subprocess against a FakeChromiumRepo, which stands
in as the brave checkout that `repository.brave.root` resolves to.  Shared
sandbox/runner infrastructure is imported from cmd_test.
"""

import os
import platform
import stat
import subprocess
import sys
import unittest
from pathlib import Path

import _boot  # noqa: F401
from cmd_test import (CMD_SCRIPT, _GIT_ENV_OVERRIDES, _Sandbox)
from alias.commit import _MarkChangeShortcut

# ---------------------------------------------------------------------------
# Tests: flag pass-through
# ---------------------------------------------------------------------------


class TestFlagPassthrough(unittest.TestCase):
    """gc must strip its own flags and forward everything else to git."""

    def setUp(self) -> None:
        self._sandbox = _Sandbox()
        self._sandbox.__enter__()
        self._sandbox.install_hook()

    def tearDown(self) -> None:
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

    def tearDown(self) -> None:
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
# Tests: --culprit upstream links (Chromium and subrepos)
# ---------------------------------------------------------------------------


class TestCulpritLinks(unittest.TestCase):
    """The commit-msg hook resolves --culprit hashes to upstream links.

    A bare hash resolves against the Chromium checkout ('../'); a
    '<subrepo>:<hash>' value (e.g. 'v8/src:...') resolves against that subrepo
    (e.g. '../v8/src') and links to its own googlesource project.
    """

    def setUp(self) -> None:
        self._sandbox = _Sandbox()
        self._sandbox.__enter__()
        self._sandbox.install_hook()

    def tearDown(self) -> None:
        self._sandbox.__exit__(None, None, None)

    def _commit_in(self, subject: str, repo_path: Path) -> str:
        """Create an empty commit in repo_path and return its hash."""
        return self._sandbox.repo.commit_empty(subject, repo_path)

    def test_chromium_hash_adds_link_and_body(self) -> None:
        """A bare culprit hash links to chromium/src and inlines the body."""
        repo = self._sandbox.repo
        culprit_hash = self._commit_in('Upstream chromium change',
                                       repo.chromium)
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(
            ['commit', '--culprit', culprit_hash, '-m', 'Fix'])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        msg = self._sandbox.last_commit_message()
        self.assertIn('Chromium changes:', msg)
        self.assertIn(
            f'https://chromium.googlesource.com/chromium/src/+/{culprit_hash}',
            msg)
        # The full upstream commit body is inlined.
        self.assertIn('Upstream chromium change', msg)

    def test_v8_subrepo_hash_links_to_v8_project(self) -> None:
        """A 'v8/src:<hash>' culprit resolves in ../v8/src, links to v8/v8."""
        repo = self._sandbox.repo
        repo.add_repo('v8/src')
        culprit_hash = self._commit_in('Some v8 change',
                                       repo.chromium / 'v8' / 'src')
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(
            ['commit', '--culprit', f'v8/src:{culprit_hash}', '-m', 'Fix'])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        msg = self._sandbox.last_commit_message()
        # The link points at the v8 project, not chromium/src.
        self.assertIn(
            f'https://chromium.googlesource.com/v8/v8/+/{culprit_hash}', msg)
        self.assertNotIn('chromium/src/+/', msg)
        self.assertIn('Some v8 change', msg)

    def test_mixed_chromium_and_subrepo_culprits(self) -> None:
        """A comma-separated mix of chromium and v8 culprits links both."""
        repo = self._sandbox.repo
        chromium_hash = self._commit_in('Chromium side', repo.chromium)
        repo.add_repo('v8/src')
        v8_hash = self._commit_in('V8 side', repo.chromium / 'v8' / 'src')
        self._sandbox.stage_change()
        result = self._sandbox.run_gc([
            'commit', '--culprit', f'{chromium_hash},v8/src:{v8_hash}', '-m',
            'Fix'
        ])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        msg = self._sandbox.last_commit_message()
        self.assertIn(
            f'https://chromium.googlesource.com/chromium/src/+/{chromium_hash}',
            msg)
        self.assertIn(f'https://chromium.googlesource.com/v8/v8/+/{v8_hash}',
                      msg)
        self.assertIn('Chromium side', msg)
        self.assertIn('V8 side', msg)

    def test_unknown_subrepo_is_rejected(self) -> None:
        """An unknown subrepo prefix aborts the commit with a helpful error."""
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(
            ['commit', '--culprit', 'skia:deadbeef', '-m', 'Fix'])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Unknown culprit subrepo 'skia'", result.stderr)
        # The known subrepos are listed to guide the user.
        self.assertIn('v8/src', result.stderr)

    def test_culprit_already_in_message_is_not_duplicated(self) -> None:
        """A culprit whose hash is already in the message adds no link block."""
        repo = self._sandbox.repo
        culprit_hash = self._commit_in('Should not be inlined', repo.chromium)
        self._sandbox.stage_change()
        result = self._sandbox.run_gc([
            'commit', '--culprit', culprit_hash, '-m',
            f'Fix (see {culprit_hash})'
        ])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        msg = self._sandbox.last_commit_message()
        self.assertNotIn('Chromium changes:', msg)
        self.assertNotIn('Should not be inlined', msg)


# ---------------------------------------------------------------------------
# Tests: cmd_commit hook sanity check
# ---------------------------------------------------------------------------


class TestCommitSanityCheck(unittest.TestCase):
    """cmd_commit must refuse early when the hook is absent or misconfigured."""

    def setUp(self) -> None:
        self._sandbox = _Sandbox()
        self._sandbox.__enter__()

    def tearDown(self) -> None:
        self._sandbox.__exit__(None, None, None)

    def test_missing_hook_is_rejected(self) -> None:
        """cmd_commit exits non-zero and mentions install-hook when absent."""
        # A fresh sandbox has no hook installed.
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(['commit', '-m', 'should fail'])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('install-hook', result.stderr)

    @unittest.skipIf(platform.system() == 'Windows',
                     'executable bit is POSIX-only')
    def test_non_executable_hook_is_rejected(self) -> None:
        """cmd_commit exits non-zero when the hook lacks the executable bit."""
        # Install as a copy so flipping the exec bit is scoped to the sandbox.
        # A symlink would point at the real HOOK_SOURCE and races with other
        # tests (in this process or in parallel test files) that chmod it +x.
        self._sandbox.install_hook(as_copy=True)
        dest = self._sandbox.hook_dest
        dest.chmod(dest.stat().st_mode & ~(stat.S_IXUSR | stat.S_IXGRP
                                           | stat.S_IXOTH))
        self._sandbox.stage_change()
        result = self._sandbox.run_gc(['commit', '-m', 'should fail'])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('install-hook', result.stderr)


# ---------------------------------------------------------------------------
# Tests: --fixup=reassign: shortcut (delegates to brockit reassign)
# ---------------------------------------------------------------------------


class TestReassignFixup(unittest.TestCase):
    """git cr commit --fixup=reassign:<ref> delegates to brockit reassign."""

    def setUp(self) -> None:
        self._sandbox = _Sandbox()
        self._sandbox.__enter__()

    def tearDown(self) -> None:
        self._sandbox.__exit__(None, None, None)

    def _commit_count(self) -> int:
        return int(
            subprocess.check_output(
                ['git', 'rev-list', '--count', 'HEAD'],
                cwd=self._sandbox.root,
                text=True,
                env={
                    **os.environ,
                    **_GIT_ENV_OVERRIDES
                },
            ).strip())

    def test_joined_form_creates_reassign_commit(self) -> None:
        """--fixup=reassign:HEAD adds an empty reassign! commit on top.

        No hook is installed: the shortcut must work without it because
        brockit commits with --no-verify.
        """
        before = self._commit_count()
        result = self._sandbox.run_gc(['commit', '--fixup=reassign:HEAD'])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        self.assertEqual(self._commit_count(), before + 1)
        msg = self._sandbox.last_commit_message()
        # brockit formats the subject as `reassign!<hash>! <original subject>`.
        self.assertTrue(msg.startswith('reassign!'),
                        msg=f'unexpected message: {msg!r}')
        self.assertIn('Add file.txt', msg)

    def test_split_form_creates_reassign_commit(self) -> None:
        """The split spelling `--fixup reassign:HEAD` is handled too."""
        before = self._commit_count()
        result = self._sandbox.run_gc(['commit', '--fixup', 'reassign:HEAD'])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        self.assertEqual(self._commit_count(), before + 1)
        self.assertTrue(
            self._sandbox.last_commit_message().startswith('reassign!'))

    def test_drop_verb_creates_drop_commit(self) -> None:
        """--fixup=drop:HEAD adds an empty drop! commit via brockit drop."""
        before = self._commit_count()
        result = self._sandbox.run_gc(['commit', '--fixup=drop:HEAD'])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        self.assertEqual(self._commit_count(), before + 1)
        msg = self._sandbox.last_commit_message()
        # brockit formats the subject as `drop!<hash>! <original subject>`.
        self.assertTrue(msg.startswith('drop!'),
                        msg=f'unexpected message: {msg!r}')
        self.assertIn('Add file.txt', msg)

    def test_empty_ref_is_rejected(self) -> None:
        """--fixup=reassign: with no ref errors out without committing."""
        before = self._commit_count()
        result = self._sandbox.run_gc(['commit', '--fixup=reassign:'])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('reassign', result.stderr)
        self.assertEqual(self._commit_count(), before)

    def test_extra_passthrough_arg_is_rejected(self) -> None:
        """Mixing the shortcut with a git arg (e.g. -m) errors, no commit."""
        before = self._commit_count()
        result = self._sandbox.run_gc(
            ['commit', '--fixup=reassign:HEAD', '-m', 'nope'])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('cannot be combined', result.stderr)
        self.assertEqual(self._commit_count(), before)

    def test_extra_wrapper_flag_is_rejected(self) -> None:
        """Mixing the shortcut with a wrapper flag (--tagged) errors too."""
        before = self._commit_count()
        result = self._sandbox.run_gc(
            ['commit', '--tagged', 'WIP', '--fixup=reassign:HEAD'])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('cannot be combined', result.stderr)
        self.assertIn('--tagged', result.stderr)
        self.assertEqual(self._commit_count(), before)

    def test_native_fixup_still_passes_through_to_git(self) -> None:
        """A plain `--fixup=<ref>` is not intercepted and reaches git.

        git produces a `fixup! Add file.txt` commit, proving the wrapper only
        claims the `reassign:` mode and leaves git's own modes alone.
        """
        self._sandbox.install_hook()
        self._sandbox.stage_change('fixup body\n')
        result = self._sandbox.run_gc(
            ['commit', '--fixup=HEAD', '--no-verify'])
        self.assertEqual(result.returncode, 0, msg=f'stderr: {result.stderr}')
        msg = self._sandbox.last_commit_message()
        self.assertTrue(msg.startswith('fixup!'),
                        msg=f'unexpected message: {msg!r}')


# ---------------------------------------------------------------------------
# Tests: _MarkChangeShortcut.from_args detection and leniency (unit)
# ---------------------------------------------------------------------------


class TestMarkChangeShortcutParsing(unittest.TestCase):
    """Unit checks for the lenient arg detection in from_args."""

    def test_detects_joined_and_split_forms(self) -> None:
        """Both `--fixup=reassign:X` and `--fixup reassign:X` are recognised."""
        joined = _MarkChangeShortcut.from_args(['--fixup=reassign:HEAD'])
        self.assertIsNotNone(joined)
        self.assertEqual(joined.verb, 'reassign')
        self.assertEqual(joined.target, 'HEAD')
        self.assertIsNotNone(
            _MarkChangeShortcut.from_args(['--fixup', 'reassign:HEAD']))

    def test_detects_drop_verb(self) -> None:
        """The `drop:` verb is recognised alongside `reassign:`."""
        for form in (['--fixup=drop:HEAD'], ['--fixup', 'drop:HEAD']):
            shortcut = _MarkChangeShortcut.from_args(form)
            self.assertIsNotNone(shortcut, msg=f'not detected: {form}')
            self.assertEqual(shortcut.verb, 'drop')
            self.assertEqual(shortcut.target, 'HEAD')

    def test_absent_when_no_marker(self) -> None:
        """Ordinary commits and git's native fixup modes are not claimed."""
        self.assertIsNone(_MarkChangeShortcut.from_args(['--fixup=HEAD']))
        self.assertIsNone(_MarkChangeShortcut.from_args(['--fixup=amend:HEAD'
                                                         ]))
        self.assertIsNone(_MarkChangeShortcut.from_args(['-m', 'a message']))

    def test_marker_outside_fixup_is_not_the_shortcut(self) -> None:
        """A verb token only in a commit message is not the shortcut."""
        self.assertIsNone(
            _MarkChangeShortcut.from_args(['-m', 'fix reassign: bug']))

    def test_malformed_fixup_does_not_terminate(self) -> None:
        """A `--fixup` with no value must not exit the process.

        A strict ArgumentParser would call sys.exit() here; the lenient parser
        instead raises, so from_args reports "not the shortcut" and the caller
        falls back to the normal commit path.
        """
        self.assertIsNone(_MarkChangeShortcut.from_args(['--fixup']))


# ---------------------------------------------------------------------------
# Tests: KeyboardInterrupt / graceful exit
# ---------------------------------------------------------------------------


class TestGracefulExit(unittest.TestCase):
    """gc must not emit Python tracebacks on Ctrl-C."""

    def setUp(self) -> None:
        self._sandbox = _Sandbox()
        self._sandbox.__enter__()
        self._sandbox.install_hook()

    def tearDown(self) -> None:
        self._sandbox.__exit__(None, None, None)

    def test_no_traceback_on_keyboard_interrupt(self) -> None:
        """Sending SIGINT produces exit code 130 with no traceback."""
        import signal

        proc = subprocess.Popen(
            [sys.executable,
             str(CMD_SCRIPT), 'commit', '-m', 'test'],
            cwd=self._sandbox.root,
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
