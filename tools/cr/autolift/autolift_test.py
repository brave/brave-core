#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest import mock

import autolift

_FULL_HASH = '5691202e214341a5d96a3296c1b9811cce19572b'


class StripAnsiTest(unittest.TestCase):
    def test_removes_colour_escapes(self):
        self.assertEqual(
            autolift.strip_ansi('\x1b[31;1mFAILED:\x1b[0m x'), 'FAILED: x'
        )


class SummarizeBuildTest(unittest.TestCase):
    def test_reports_status_failed_steps_and_deduped_errors(self):
        log = '\n'.join(
            [
                '...\x1b[31;1mFAILED: 82c9 "./obj/a.o" CXX obj/a.o\x1b[0m',
                '../../a.cc:29:49: error: no member named \'CURRENT_TAB\'',
                'FAILED: 92d1 "./obj/b.o" CXX obj/b.o',
                '../../a.cc:29:49: error: no member named \'CURRENT_TAB\'',
                '../../b.cc:3:1: error: unknown type name \'Foo\'',
                ' 14m56.78s Build Failure: 26405 done, 10 failed, 3 remaining',
            ]
        )
        summary = autolift.summarize_build(log)
        self.assertEqual(
            summary.status, 'Build Failure: 26405 done, 10 failed, 3 remaining'
        )
        self.assertEqual(
            summary.failed_steps,
            ['"./obj/a.o" CXX obj/a.o', '"./obj/b.o" CXX obj/b.o'],
        )
        self.assertEqual(
            summary.errors,
            [
                (2, '../../a.cc:29:49: error: no member named \'CURRENT_TAB\''),
                (1, '../../b.cc:3:1: error: unknown type name \'Foo\''),
            ],
        )

    def test_recognises_non_compiler_errors(self):
        log = '\n'.join(
            [
                'ERROR at //build/config/BUILDCONFIG.gn:588:5: Dependency not '
                'allowed.',
                '../../Foo.java:39: error: symbol not found Bar',
                'gen/app.ts:6:41 - error TS2307: Cannot find module \'./x.js\'.',
                '  25:65  error  Invalid boolean attribute expression  '
                '@webui-eslint/lit-element-expressions',
                'mold: error: undefined symbol: internal::kHistogram',
                'Type com.android.tools.r8.RecordTag is defined multiple times: '
                'a.jar, b.jar',
                ' "../../x.jar", needed by "obj/y.jar", missing and no known rule '
                'to make it',
                "ModuleNotFoundError: No module named 'utils.Table'",
            ]
        )
        errors = [e for _, e in autolift.summarize_build(log).errors]
        self.assertEqual(len(errors), 8)
        self.assertIn(
            'Invalid boolean attribute expression  '
            '@webui-eslint/lit-element-expressions',
            errors,
        )

    def test_success(self):
        summary = autolift.summarize_build(
            '.The build has finished successfully.\n'
        )
        self.assertEqual(summary.status, 'The build has finished successfully.')
        self.assertEqual(summary.failed_steps, [])
        self.assertEqual(summary.errors, [])


class ParseCulpritHashesTest(unittest.TestCase):
    def test_extracts_unique_hashes_from_culprit_block(self):
        message = (
            '[cr158] Fix something\n\nBody.\n\nChromium changes:\n'
            f'https://chromium.googlesource.com/chromium/src/+/{_FULL_HASH}\n\n'
            f'commit {_FULL_HASH}\n'
            '    Reviewed-on: https://chromium-review.googlesource.com/c/'
            'chromium/src/+/8620801\n'
            f'https://chromium.googlesource.com/chromium/src/+/{_FULL_HASH}\n'
        )
        self.assertEqual(autolift.parse_culprit_hashes(message), [_FULL_HASH])

    def test_ignores_links_outside_the_block(self):
        message = (
            'Subject\n\nSee [3c06ac856fbd0](https://chromium.googlesource.com/'
            'chromium/src/+/3c06ac856fbd0).\n\nChromium changes:\n'
            'https://chromium.googlesource.com/chromium/src/+/a1fc21843e8bb\n'
            'https://chromium.googlesource.com/chromium/src/+/59ee62c696c68\n'
            '\ncommit a1fc21843e8bb3bc0db2fbe4d338e304010bc37b\n'
        )
        self.assertEqual(
            autolift.parse_culprit_hashes(message),
            ['a1fc21843e8bb', '59ee62c696c68'],
        )

    def test_no_culprit(self):
        self.assertEqual(autolift.parse_culprit_hashes('[cr158] Subject'), [])


class CulpritProblemTest(unittest.TestCase):
    def test_rejects_hash_that_does_not_resolve(self):
        with mock.patch.object(autolift.repository, 'chromium') as chromium:
            chromium.run_git.side_effect = subprocess.CalledProcessError(
                1, 'git'
            )
            self.assertEqual(
                autolift.culprit_problem(
                    _FULL_HASH, '158.0.8117.1..158.0.8127.2'
                ),
                f'{_FULL_HASH} is not a Chromium commit',
            )

    def test_rejects_commit_outside_range(self):
        with mock.patch.object(autolift.repository, 'chromium') as chromium:
            # rev-parse succeeds, then the commit is already in the old tag.
            chromium.run_git.side_effect = [_FULL_HASH, '', '']
            self.assertEqual(
                autolift.culprit_problem(
                    '5691202e', '158.0.8117.1..158.0.8127.2'
                ),
                '5691202e is not in 158.0.8117.1..158.0.8127.2',
            )

    def test_accepts_commit_in_range(self):
        with mock.patch.object(autolift.repository, 'chromium') as chromium:
            chromium.run_git.side_effect = [
                _FULL_HASH,
                '',
                subprocess.CalledProcessError(1, 'git'),
            ]
            self.assertIsNone(
                autolift.culprit_problem(
                    _FULL_HASH, '158.0.8117.1..158.0.8127.2'
                )
            )

    def test_rejects_malformed_range(self):
        with self.assertRaises(ValueError):
            autolift.culprit_problem(_FULL_HASH, '158.0.8127.2')


class ChooseBaseTest(unittest.TestCase):
    def test_previous_milestone_branch_until_it_lands(self):
        self.assertEqual(autolift.choose_base(158, 156), 'origin/cr157')

    def test_master_once_previous_milestone_landed(self):
        self.assertEqual(autolift.choose_base(158, 157), 'origin/master')
        self.assertEqual(autolift.choose_base(158, 158), 'origin/master')


class PrepareUnpushedTest(unittest.TestCase):
    def test_refuses_to_drop_unpushed_commits(self):
        calls = []

        def run_git(*cmd, **_):
            calls.append(cmd)
            if cmd[0] == 'rev-list':
                return '35'
            return ''

        brave = mock.MagicMock()
        brave.is_rebase_in_progress.return_value = False
        brave.run_git.side_effect = run_git
        args = mock.Mock(milestone=158, no_rebase=True, force=False)
        with mock.patch.object(autolift.repository, 'brave', brave):
            self.assertEqual(autolift._cmd_prepare(args), 1)
        self.assertIn(('rev-list', '--count', 'origin/cr158..cr158'), calls)
        self.assertFalse(any(c[0] == 'checkout' for c in calls))


class MovesToMasterTest(unittest.TestCase):
    def test_first_rebase_after_the_previous_milestone_lands(self):
        self.assertTrue(autolift.moves_to_master(158, 'origin/master', 156))

    def test_already_on_master(self):
        self.assertFalse(autolift.moves_to_master(158, 'origin/master', 157))

    def test_previous_milestone_branch_as_base(self):
        self.assertFalse(autolift.moves_to_master(158, 'origin/cr157', 156))

    def test_unknown_fork(self):
        self.assertFalse(autolift.moves_to_master(158, 'origin/master', None))


class PrepareTest(unittest.TestCase):
    def _prepare(self, brave: mock.MagicMock, **kwargs) -> int:
        args = mock.Mock(milestone=158, no_rebase=True, force=False)
        for key, value in kwargs.items():
            setattr(args, key, value)
        with mock.patch.object(autolift.repository, 'brave', brave):
            return autolift._cmd_prepare(args)

    def test_refuses_during_rebase(self):
        brave = mock.MagicMock()
        brave.is_rebase_in_progress.return_value = True
        self.assertEqual(self._prepare(brave), 1)
        brave.run_git.assert_not_called()

    def test_refuses_to_discard_tracked_changes(self):
        brave = mock.MagicMock()
        brave.is_rebase_in_progress.return_value = False
        brave.run_git.return_value = ' M DEPS'
        self.assertEqual(self._prepare(brave), 1)
        brave.run_git.assert_called_once_with(
            'status', '--porcelain', '--untracked-files=no'
        )


class PrepareFlowTest(unittest.TestCase):
    def _run(self, milestone: int, remote_exists: bool):
        calls = []

        def run_git(*cmd, **_):
            calls.append(cmd)
            if cmd[0] == 'ls-remote' and not remote_exists:
                raise subprocess.CalledProcessError(2, 'git')
            if cmd[:2] == ('rev-parse', '--abbrev-ref'):
                upstream = [c for c in calls if c[0] == 'branch'][-1]
                return upstream[1].removeprefix('--set-upstream-to=')
            if cmd[0] == 'rev-list':
                return '0'
            return ''

        brave = mock.MagicMock()
        brave.is_rebase_in_progress.return_value = False
        brave.run_git.side_effect = run_git
        args = mock.Mock(milestone=milestone, no_rebase=False, force=False)
        with (
            mock.patch.object(autolift.repository, 'brave', brave),
            mock.patch.object(
                autolift.Version,
                'from_git',
                return_value=autolift.Version('156.0.8078.74'),
            ),
            mock.patch.object(autolift.subprocess, 'run') as run,
            mock.patch.object(
                autolift, '_target_tag', return_value='159.0.8130.0'
            ),
            mock.patch.object(autolift, '_fork_major', return_value=156),
            mock.patch.object(autolift, '_RUN_ROOT', self._run_root()),
        ):
            run.return_value.returncode = 0
            result = autolift._cmd_prepare(args)
        return result, calls, run

    def _run_root(self) -> Path:
        directory = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, directory)
        return directory

    def test_existing_branch_is_reset_and_rebased(self):
        result, calls, run = self._run(158, remote_exists=True)
        self.assertEqual(result, 0)
        self.assertIn(('fetch', 'origin', 'master', 'cr158'), calls)
        self.assertIn(('fetch', 'origin', 'cr157'), calls)
        self.assertIn(('checkout', '-B', 'cr158', 'origin/cr158'), calls)
        self.assertIn(('branch', '--set-upstream-to=origin/cr157'), calls)
        run.assert_called_once()
        self.assertIn('rebase', run.call_args.args[0])

    def test_new_milestone_branch_is_created_from_base(self):
        result, calls, run = self._run(159, remote_exists=False)
        self.assertEqual(result, 0)
        self.assertIn(('fetch', 'origin', 'master'), calls)
        self.assertIn(('fetch', 'origin', 'cr158'), calls)
        self.assertIn(('checkout', '-B', 'cr159', 'origin/cr158'), calls)
        self.assertIn(('branch', '--set-upstream-to=origin/cr158'), calls)
        run.assert_not_called()


class PublishTest(unittest.TestCase):
    def _publish(self, remote_exists: bool, ahead: str = '3'):
        calls = []

        def run_git(*cmd, **_):
            calls.append(cmd)
            if cmd[0] == 'ls-remote' and not remote_exists:
                raise subprocess.CalledProcessError(2, 'git')
            if cmd[:2] == ('rev-parse', '--abbrev-ref'):
                return 'origin/cr158'
            if cmd[0] == 'rev-list':
                return ahead
            return ''

        brave = mock.MagicMock()
        brave.current_branch.return_value = 'cr159'
        brave.run_git.side_effect = run_git
        with (
            mock.patch.object(autolift.repository, 'brave', brave),
            mock.patch.object(autolift.subprocess, 'run') as run,
        ):
            run.return_value.returncode = 0
            result = autolift._cmd_publish(mock.Mock(milestone=159))
        return result, calls, run

    def test_pushes_new_branch_without_changing_upstream(self):
        result, calls, run = self._publish(remote_exists=False)
        self.assertEqual(result, 0)
        self.assertIn(('push', 'origin', 'cr159:refs/heads/cr159'), calls)
        self.assertFalse(any('--set-upstream' in c or '-u' in c for c in calls))
        self.assertIn('update-version-issue', run.call_args.args[0])

    def test_refuses_existing_branch(self):
        result, calls, run = self._publish(remote_exists=True)
        self.assertEqual(result, 1)
        self.assertFalse(any(c[0] == 'push' for c in calls))
        run.assert_not_called()

    def test_refuses_branch_without_commits(self):
        result, calls, run = self._publish(remote_exists=False, ahead='0')
        self.assertEqual(result, 1)
        self.assertFalse(any(c[0] == 'push' for c in calls))
        run.assert_not_called()


_LIFT_LOG = """\
[18:10:59] 🚀 Brockit!                                          terminal.py:409
[18:11:00] * Changes for this bump: https://chromium.googlesource.com/chromium/src/+log/158.0.8117.1..158.0.8127.2?pretty=fuller&n=10000  terminal.py:358
           * Changes since base version: https://chromium.googlesource.com/chromium/src/+log/157.0.8103.15..158.0.8127.2?pretty=fuller&n=10000  terminal.py:358
[18:17:11] * Files that cannot be patched anymore (action needed):  terminal.py:358
               ✘ ../chrome/android/java/src/org/chromium/chrome/browser/ntp/RecentTabsCoordinator.java  brockit.py:1267
                   (renamed to ../chrome/android/java/src/org/chromium/chrome/browser/recent_tabs/RecentTabsCoordinator.java)
                   commit 71ab205f74b2b5734547c8c3fd873e57e5482356  brockit.py:1277
                       [RecentTab] Move RecentTabs* UI classes to recent_tabs package
               ✘ chrome/browser/ui/views/tabs/tab_container_impl.cc (deleted)  brockit.py:1255
           * Manually resolve conflicts for (action needed):    terminal.py:358
               ✘ ../net/http/transport_security_state.cc
           👋 (Address all sections with (action needed) above, and then rerun 🚀Brockit! with --continue)  exceptions.py:39
"""

_CONTINUE_LOG = """\
[18:57:49] * GitHub issue updated https://github.com/brave/brave-browser/issues/59914.  terminal.py:358
           * ❌ Plaster check. Please investigate it.            terminal.py:358
[18:58:30] * ✔️  6df16e5f58b [cr158] `gnrt` run for Chromium 158.0.8127.2.  terminal.py:358
           💥 Done!
"""


class SummarizeLiftTest(unittest.TestCase):
    def test_ranges_sections_and_items(self):
        summary = autolift.summarize_lift(_LIFT_LOG)
        self.assertEqual(summary.bump_range, '158.0.8117.1..158.0.8127.2')
        self.assertEqual(summary.branch_range, '157.0.8103.15..158.0.8127.2')
        self.assertEqual(
            [t for t, _ in summary.sections],
            [
                'Files that cannot be patched anymore',
                'Manually resolve conflicts for',
            ],
        )
        files = summary.sections[0][1]
        self.assertEqual(len(files), 2)
        self.assertIn(
            '(renamed to ../chrome/android/java/src/org/chromium/'
            'chrome/browser/recent_tabs/RecentTabsCoordinator.java)',
            files[0],
        )
        self.assertEqual(
            files[1],
            'chrome/browser/ui/views/tabs/tab_container_impl.cc (deleted)',
        )
        self.assertEqual(
            summary.sections[1][1], ['../net/http/transport_security_state.cc']
        )
        self.assertFalse(summary.done)
        self.assertTrue(summary.needs_attention)

    def test_failed_check_needs_attention_even_when_done(self):
        summary = autolift.summarize_lift(_CONTINUE_LOG)
        self.assertTrue(summary.done)
        self.assertEqual(
            summary.flags, ['Plaster check. Please investigate it.']
        )
        self.assertTrue(summary.needs_attention)

    def test_clean_finish(self):
        summary = autolift.summarize_lift('[18:58:30] 💥 Done!\n')
        self.assertFalse(summary.needs_attention)

    def test_error_before_any_commit(self):
        summary = autolift.summarize_lift(
            '* Watching pipelines\n¯\\_(ツ)_/¯\n🔥🔥 Pipeline failed: http://ci/1\n'
        )
        self.assertEqual(summary.flags, ['Pipeline failed: http://ci/1'])
        self.assertTrue(summary.needs_attention)


class WithPackageRangeTest(unittest.TestCase):
    def test_fills_missing_bump_range_only(self):
        summary = autolift.with_package_range(
            autolift.summarize_lift('💥 Done!\n'), '158.0.1.0..159.0.2.0'
        )
        self.assertEqual(summary.bump_range, '158.0.1.0..159.0.2.0')
        self.assertIsNone(summary.branch_range)

    def test_keeps_printed_bump_range(self):
        summary = autolift.with_package_range(
            autolift.summarize_lift(_LIFT_LOG), '1.0.0.0..2.0.0.0'
        )
        self.assertNotEqual(summary.bump_range, '1.0.0.0..2.0.0.0')
        self.assertIsNotNone(summary.bump_range)


class LiftCommandTest(unittest.TestCase):
    def _lift(self, log_text: str, returncode: int = 0):
        directory = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, directory)

        def run(_cmd, stdout, **_):
            stdout.write(log_text.encode('utf-8'))
            return mock.Mock(returncode=returncode)

        brave = mock.MagicMock()
        brave.current_branch.return_value = 'cr158'
        with (
            mock.patch.object(autolift.repository, 'brave', brave),
            mock.patch.object(autolift, '_RUN_ROOT', directory),
            mock.patch.object(
                autolift.subprocess, 'run', side_effect=run
            ) as run_mock,
            mock.patch.object(autolift, '_package_range', return_value=None),
        ):
            result = autolift._cmd_lift(mock.Mock(continue_=True))
        return result, run_mock, directory / 'cr158' / 'report.md'

    def test_exits_non_zero_when_brockit_exits_zero_with_work_left(self):
        result, run_mock, report = self._lift(_CONTINUE_LOG)
        self.assertEqual(result, 2)
        cmd = run_mock.call_args.args[0]
        self.assertIn('--with-github', cmd)
        self.assertIn('--to=@latest-for-branch', cmd)
        self.assertIn('--infra-mode', cmd)
        self.assertIn('--continue', cmd)
        text = report.read_bytes().decode('utf-8')
        self.assertIn('Plaster check. Please investigate it.', text)

    def test_clean_run_exits_zero(self):
        result, _, report = self._lift('💥 Done!\n')
        self.assertEqual(result, 0)
        self.assertIn(
            'Finished with nothing left to do.',
            report.read_bytes().decode('utf-8'),
        )


if __name__ == '__main__':
    unittest.main()
