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
                'gen/app.ts:6:41 - error TS2307: '
                'Cannot find module \'./x.js\'.',
                '  25:65  error  Invalid boolean attribute expression  '
                '@webui-eslint/lit-element-expressions',
                'mold: error: undefined symbol: internal::kHistogram',
                'Type com.android.tools.r8.RecordTag is defined multiple '
                'times: a.jar, b.jar',
                ' "../../x.jar", needed by "obj/y.jar", missing and no known '
                'rule to make it',
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

    def test_accepts_main_commit_of_a_cherry_pick_in_range(self):
        with mock.patch.object(autolift.repository, 'chromium') as chromium:
            # The main commit needn't be in the checkout at all.
            chromium.run_git.side_effect = subprocess.CalledProcessError(
                1, 'git'
            )
            self.assertIsNone(
                autolift.culprit_problem(
                    _FULL_HASH[:12],
                    '158.0.8127.2..158.0.8127.4',
                    [_FULL_HASH],
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


_MAIN = 'a' * 40
_M158_PICK = 'b' * 40
_M159_PICK = 'c' * 40


class CherryPickOriginsTest(unittest.TestCase):
    def test_reads_release_branch_trailer(self):
        message = (
            'Fix the thing\n\nBug: 1\n'
            f'(cherry picked from commit {_MAIN})\n\nChange-Id: I1\n'
        )
        self.assertEqual(autolift.cherry_pick_origins(message), [_MAIN])

    def test_main_commit_has_none(self):
        self.assertEqual(autolift.cherry_pick_origins('Fix\n\nBug: 1\n'), [])


class MatchNextFixesTest(unittest.TestCase):
    _PICK = autolift.Commit(_M158_PICK, 'Fix the thing', [_MAIN])

    def test_fix_citing_the_main_commit(self):
        fix = autolift.Commit('d' * 40, '[cr159] Follow the thing', [_MAIN])
        self.assertEqual(
            autolift.match_next_fixes([self._PICK], {'cr159': [fix]}),
            [autolift.NextFix(self._PICK, 'cr159', fix)],
        )

    def test_fix_citing_a_short_hash(self):
        fix = autolift.Commit('d' * 40, 'Follow', [_MAIN[:12]])
        self.assertEqual(
            len(autolift.match_next_fixes([self._PICK], {'cr159': [fix]})), 1
        )

    def test_fix_citing_its_own_branch_cherry_pick(self):
        # A maintenance cr159 cites its own cherry-pick, known by its origin.
        fix = autolift.Commit('d' * 40, 'Follow', [_M159_PICK, _MAIN])
        self.assertEqual(
            len(autolift.match_next_fixes([self._PICK], {'cr159': [fix]})), 1
        )

    def test_unrelated_fix(self):
        fix = autolift.Commit('d' * 40, 'Other', ['e' * 40])
        self.assertEqual(
            autolift.match_next_fixes([self._PICK], {'cr159': [fix]}), []
        )


class TrackerMatchTest(unittest.TestCase):
    _TRACKER = (
        '# M159 code health follow-ups\n\n## 1. Regressions\n\n'
        '### Picked change\n\n'
        f'[{_MAIN[:12]}]'
        f'(https://chromium.googlesource.com/chromium/src/+/{_MAIN})'
        ' breaks Brave.\n\n- [ ] Fix it\n\n'
        '### Startup deletes a database\n\n'
        'Upstream now calls it from '
        '`chrome/browser/chrome_browser_main.cc:812`.\n\n'
        '### Favicons\n\n'
        'Upstream changed `favicon_loader_impl.mm` the same way.\n\n'
        '- [x] Take `LargeIconResult` by value '
        '(fixed in `[cr159] Follow the favicon callback change`)\n'
        '- [ ] Add a test\n\n'
        '### Other change\n\n'
        f'[eeee](https://chromium.googlesource.com/chromium/src/+/{"e" * 40})\n'
        '\n## Tooling TODOs\n\n'
        '### `plaster` mangles `chrome_browser_main.cc`\n\n- [ ] Fix it\n'
    )

    _FAVICON_PICK = autolift.Commit(
        _M158_PICK,
        'Take results by value',
        ['f' * 40],
        ('ios/chrome/browser/favicon/model/favicon_loader_impl.mm',),
    )

    def _find(self, *picks) -> list:
        return autolift.TrackerMatch.find(list(picks), self._TRACKER)

    def _find_one(self, *picks):
        matches = self._find(*picks)
        self.assertEqual(len(matches), 1)
        return matches[0]

    def test_items_citing_a_picked_origin(self):
        pick = autolift.Commit(_M158_PICK, 'Fix the thing', [_MAIN])
        match = self._find_one(pick)
        self.assertEqual(match.title, 'Picked change')
        self.assertEqual(match.cited, [pick])
        self.assertEqual(match.named, [])
        self.assertEqual(match.state, 'open')

    def test_items_naming_a_file_a_pick_changed(self):
        pick = autolift.Commit(
            _M158_PICK,
            'Retire the database',
            ['f' * 40],
            ('chrome/browser/chrome_browser_main.cc', 'chrome/BUILD.gn'),
        )
        # Not the Tooling TODO, which names the same file.
        match = self._find_one(pick)
        self.assertEqual(match.title, 'Startup deletes a database')
        self.assertEqual(
            match.named, [('chrome/browser/chrome_browser_main.cc', pick)]
        )

    def test_bare_file_names_match_by_name(self):
        match = self._find_one(self._FAVICON_PICK)
        self.assertEqual(match.title, 'Favicons')
        self.assertEqual(
            match.named, [('favicon_loader_impl.mm', self._FAVICON_PICK)]
        )

    def test_ticked_tasks_and_the_subjects_they_were_fixed_in(self):
        match = self._find_one(self._FAVICON_PICK)
        self.assertEqual((match.ticked, match.open), (1, 1))
        self.assertEqual(match.state, 'partly ticked')
        self.assertEqual(
            match.fixed_in, ('[cr159] Follow the favicon callback change',)
        )

    def test_fixes_are_found_by_subject(self):
        match = self._find_one(self._FAVICON_PICK)
        brave = mock.MagicMock()
        brave.run_git.return_value = (
            'abc1234 [cr159] Follow the favicon callback change\n'
            'def5678 [cr159] Follow the favicon callback change (again)'
        )
        with mock.patch.object(autolift.repository, 'brave', brave):
            fixes = match.fixes('origin/cr158', 'origin/cr159')
        self.assertEqual(
            fixes, ['abc1234 [cr159] Follow the favicon callback change']
        )
        self.assertIn(
            '--grep=[cr159] Follow the favicon callback change',
            brave.run_git.call_args.args,
        )

    def test_fixes_without_a_subject_are_found_by_file(self):
        match = autolift.TrackerMatch(
            'Favicons', [], [], ('favicon_loader_impl.mm',), ticked=1
        )
        brave = mock.MagicMock()
        brave.run_git.return_value = 'abc1234 [cr159] Fix favicons'
        with mock.patch.object(autolift.repository, 'brave', brave):
            fixes = match.fixes('origin/cr158', 'origin/cr159')
        self.assertEqual(fixes, ['abc1234 [cr159] Fix favicons'])
        self.assertIn(
            ':(glob)**/favicon_loader_impl.mm*', brave.run_git.call_args.args
        )

    def test_nothing_picked(self):
        pick = autolift.Commit(
            _M158_PICK, 'Fix', ['f' * 40], ('net/http/other.cc',)
        )
        self.assertEqual(self._find(pick), [])


class PickFixTest(unittest.TestCase):
    def _pick(self, fix_message: str, culprit: str | None = _M158_PICK[:10]):
        brave = mock.MagicMock()
        brave.current_branch.return_value = 'cr158'
        brave_calls = []

        def brave_git(*cmd, **_):
            brave_calls.append(cmd)
            if cmd[0] == 'log':
                return f'{"d" * 40}\x00subject\x00{fix_message}\x01'
            return ''

        def chromium_git(*cmd, **_):
            if cmd[0] == 'rev-parse':
                return _M158_PICK
            if cmd[-1] == _M158_PICK:
                return f'Fix\n\n(cherry picked from commit {_MAIN})\n'
            return 'Fix\n'

        brave.run_git.side_effect = brave_git
        chromium = mock.MagicMock()
        chromium.run_git.side_effect = chromium_git
        with (
            mock.patch.object(autolift.repository, 'brave', brave),
            mock.patch.object(autolift.repository, 'chromium', chromium),
        ):
            result = autolift._cmd_pick_fix(
                mock.Mock(commit='d' * 40, culprit=culprit)
            )
        return result, brave_calls

    def _fix_message(self, culprit: str) -> str:
        return (
            '[cr159] Follow the thing\n\nChromium changes:\n'
            f'https://chromium.googlesource.com/chromium/src/+/{culprit}\n'
        )

    def test_cherry_picks_the_fix_as_it_is(self):
        result, calls = self._pick(self._fix_message(_MAIN))
        self.assertEqual(result, 0)
        self.assertIn(('cherry-pick', 'd' * 40), calls)

    def test_refuses_a_fix_for_another_change(self):
        result, calls = self._pick(self._fix_message('e' * 40))
        self.assertEqual(result, 1)
        self.assertFalse(any(c[0] == 'cherry-pick' for c in calls))

    def test_tracker_fix_without_a_culprit_is_picked_as_it_is(self):
        result, calls = self._pick('[cr159] Fix favicons\n', culprit=None)
        self.assertEqual(result, 0)
        self.assertIn(('cherry-pick', 'd' * 40), calls)


class MaintenanceTargetTest(unittest.TestCase):
    _CURRENT = autolift.Version('158.0.8127.2')

    def test_newer_beta_of_the_milestone(self):
        self.assertEqual(
            autolift.maintenance_target(
                158,
                self._CURRENT,
                [
                    autolift.Version('158.0.8127.4'),
                    autolift.Version('157.0.8103.24'),
                ],
            ),
            autolift.Version('158.0.8127.4'),
        )

    def test_newest_of_beta_and_stable(self):
        self.assertEqual(
            autolift.maintenance_target(
                158,
                self._CURRENT,
                [
                    autolift.Version('158.0.8127.20'),
                    autolift.Version('158.0.8127.9'),
                ],
            ),
            autolift.Version('158.0.8127.20'),
        )

    def test_ignores_other_milestones(self):
        self.assertIsNone(
            autolift.maintenance_target(
                158,
                self._CURRENT,
                [
                    autolift.Version('159.0.8131.1'),
                    autolift.Version('157.0.8103.24'),
                ],
            )
        )

    def test_no_lift_when_branch_is_already_there(self):
        self.assertIsNone(
            autolift.maintenance_target(
                158,
                self._CURRENT,
                [autolift.Version('158.0.8127.2'), None],
            )
        )


class PrepareFlowTest(unittest.TestCase):
    _RELEASES = {
        '@latest-beta': autolift.Version('158.0.8127.4'),
        '@latest-stable': autolift.Version('157.0.8103.24'),
        '@latest-m158': autolift.Version('158.0.8127.7'),
        '@latest-m159': autolift.Version('159.0.8130.0'),
    }

    def _run(
        self,
        milestone: int,
        remote_exists: bool,
        higher_exists: bool = False,
        releases: dict | None = None,
        rebased_on: str | None = '157.0.8103.15',
        checkout: str = '158.0.8127.2',
        sync_fails: bool = False,
    ):
        calls = []
        remotes = {f'cr{milestone}': remote_exists}
        remotes[f'cr{milestone + 1}'] = higher_exists
        releases = self._RELEASES if releases is None else releases

        def run_git(*cmd, **_):
            calls.append(cmd)
            if cmd[0] == 'ls-remote' and not remotes[cmd[-1]]:
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
        versions = {
            'origin/master': autolift.Version('156.0.8078.74'),
            'origin/cr157': autolift.Version('157.0.8103.15'),
            'HEAD': autolift.Version('158.0.8127.2'),
        }
        args = mock.Mock(milestone=milestone, no_rebase=False, force=False)
        run_root = self._run_root()
        with (
            mock.patch.object(autolift.repository, 'brave', brave),
            mock.patch.object(
                autolift.Version, 'from_git', side_effect=versions.get
            ),
            mock.patch.object(autolift.subprocess, 'run') as run,
            mock.patch.object(
                autolift, '_chromium_version', side_effect=releases.get
            ),
            mock.patch.object(
                autolift,
                '_rebased_on',
                return_value=rebased_on and autolift.Version(rebased_on),
            ),
            mock.patch.object(autolift, '_fork_major', return_value=156),
            mock.patch.object(autolift, '_RUN_ROOT', run_root),
            mock.patch.object(
                autolift,
                'read_chromium_version_file',
                return_value=autolift.Version(checkout),
            ),
        ):
            run.side_effect = lambda cmd, **_: mock.Mock(
                returncode=int(sync_fails and cmd[:2] == ['pnpm', 'run'])
            )
            result = autolift._cmd_prepare(args)
        target = run_root / f'cr{milestone}' / 'lift-target'
        if not target.exists():
            return result, calls, run, None
        return result, calls, run, target.read_text(encoding='utf-8').strip()

    def _run_root(self) -> Path:
        directory = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, directory)
        return directory

    def test_existing_branch_is_reset_and_rebased(self):
        result, calls, run, target = self._run(158, remote_exists=True)
        self.assertEqual(result, 0)
        self.assertIn(('fetch', 'origin', 'master', 'cr158'), calls)
        self.assertIn(('fetch', 'origin', 'cr157'), calls)
        self.assertIn(('checkout', '-B', 'cr158', 'origin/cr158'), calls)
        self.assertIn(('branch', '--set-upstream-to=origin/cr157'), calls)
        run.assert_called_once()
        self.assertIn('rebase', run.call_args.args[0])
        self.assertIn('--discard-regen-changes', run.call_args.args[0])
        self.assertEqual(target, '@latest-for-branch')

    def test_new_milestone_branch_is_created_from_base(self):
        result, calls, run, target = self._run(159, remote_exists=False)
        self.assertEqual(result, 0)
        self.assertIn(('fetch', 'origin', 'master'), calls)
        self.assertIn(('fetch', 'origin', 'cr158'), calls)
        self.assertIn(('checkout', '-B', 'cr159', 'origin/cr158'), calls)
        self.assertIn(('branch', '--set-upstream-to=origin/cr158'), calls)
        run.assert_not_called()
        self.assertEqual(target, '@latest-for-branch')

    def test_maintenance_branch_lifts_to_a_newer_release(self):
        _, _, run, target = self._run(
            158, remote_exists=True, higher_exists=True
        )
        self.assertEqual(target, '158.0.8127.4')
        self.assertIn('--discard-regen-changes', run.call_args.args[0])

    def _printed(self, **kwargs) -> str:
        with mock.patch('builtins.print') as printed:
            self._run(158, remote_exists=True, higher_exists=True, **kwargs)
        return '\n'.join(str(c.args[0]) for c in printed.call_args_list)

    def test_maintenance_lift_reports_the_ranges_once_lifted(self):
        printed = self._printed()
        self.assertIn('Bump range: 158.0.8127.2..158.0.8127.4', printed)
        self.assertIn(
            'Branch range, once lifted: 157.0.8103.15..158.0.8127.4', printed
        )

    def test_maintenance_run_without_a_lift_reports_its_branch_range(self):
        releases = dict(self._RELEASES)
        releases['@latest-beta'] = autolift.Version('157.0.8103.30')
        printed = self._printed(releases=releases)
        self.assertIn('Branch range: 157.0.8103.15..158.0.8127.2', printed)
        self.assertNotIn('Bump range', printed)

    def test_maintenance_branch_without_a_release_keeps_regen_changes(self):
        releases = dict(self._RELEASES)
        releases['@latest-beta'] = autolift.Version('157.0.8103.30')
        _, _, run, target = self._run(
            158, remote_exists=True, higher_exists=True, releases=releases
        )
        self.assertEqual(target, 'none')
        cmd = run.call_args.args[0]
        self.assertIn('--squash-minor-bumps', cmd)
        self.assertNotIn('--discard-regen-changes', cmd)

    def test_maintenance_branch_stops_when_releases_are_unknown(self):
        releases = dict(self._RELEASES)
        del releases['@latest-stable']
        result, _, run, target = self._run(
            158, remote_exists=True, higher_exists=True, releases=releases
        )
        self.assertEqual(result, 1)
        self.assertIsNone(target)
        run.assert_not_called()

    def test_base_bump_without_a_lift_discards_regen_changes(self):
        releases = dict(self._RELEASES)
        releases['@latest-beta'] = autolift.Version('157.0.8103.30')
        _, _, run, target = self._run(
            158,
            remote_exists=True,
            higher_exists=True,
            releases=releases,
            rebased_on='157.0.8103.10',
        )
        self.assertEqual(target, 'none')
        self.assertIn('--discard-regen-changes', run.call_args.args[0])

    def test_stops_when_the_base_version_is_unknown(self):
        releases = dict(self._RELEASES)
        releases['@latest-beta'] = autolift.Version('157.0.8103.30')
        result, _, run, _ = self._run(
            158,
            remote_exists=True,
            higher_exists=True,
            releases=releases,
            rebased_on=None,
        )
        self.assertEqual(result, 1)
        run.assert_not_called()

    @staticmethod
    def _commands(run) -> list[list[str]]:
        return [c.args[0] for c in run.call_args_list]

    def test_checkout_on_the_branch_version_is_not_synced(self):
        _, _, run, _ = self._run(158, remote_exists=True)
        (rebase,) = self._commands(run)
        self.assertIn('rebase', rebase)

    def test_mismatched_checkout_is_synced_before_the_rebase(self):
        _, _, run, _ = self._run(
            158, remote_exists=True, checkout='159.0.8133.1'
        )
        sync, rebase = self._commands(run)
        self.assertEqual(sync, ['pnpm', 'run', 'sync'])
        self.assertIn('rebase', rebase)

    def test_failed_sync_stops_before_the_rebase(self):
        result, _, run, _ = self._run(
            158, remote_exists=True, checkout='159.0.8133.1', sync_fails=True
        )
        self.assertEqual(result, 1)
        self.assertEqual(self._commands(run), [['pnpm', 'run', 'sync']])

    def test_new_milestone_is_not_synced(self):
        _, _, run, _ = self._run(159, remote_exists=False, checkout='1.0.0.0')
        run.assert_not_called()


class ReviewListTest(unittest.TestCase):
    """`Publish._note_for_review`, on a real repository."""

    def setUp(self):
        self.dir = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.dir)
        self._git('init', '-q', '-b', 'master')
        self._git('config', 'user.email', 'a@b.c')
        self._git('config', 'user.name', 'A')
        self._git(
            'remote', 'add', 'origin', 'git@github.com:brave/brave-core.git'
        )
        self._commit('base', 'base')
        self._git('update-ref', 'refs/remotes/origin/master', 'HEAD')
        self._git('checkout', '-q', '-b', 'cr158')
        self._git('branch', '-q', '--set-upstream-to=origin/master')

    def _git(self, *args: str) -> str:
        return subprocess.run(
            ['git', '-C', str(self.dir), *args],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()

    def _commit(self, name: str, content: str, subject: str = '') -> str:
        (self.dir / name).write_text(f'{content}\n')
        self._git('add', name)
        self._git('commit', '-q', '-m', subject or f'[cr158] Change {name}')
        return self._git('rev-parse', 'HEAD')

    def _review(self, pushed_over: str) -> list[str]:
        brave = mock.MagicMock(root=self.dir)
        brave.run_git.side_effect = lambda *args, no_trim=False: (
            subprocess.run(
                ['git', '-C', str(self.dir), *args],
                check=True,
                capture_output=True,
                text=True,
            ).stdout
            if no_trim
            else self._git(*args)
        )
        with mock.patch.object(autolift.repository, 'brave', brave):
            publish = autolift.Publish(158)
            publish._note_for_review(pushed_over)
        return publish._notes

    def test_lists_what_the_run_wrote_or_changed(self):
        self._commit('kept', 'kept')
        self._commit('changed', 'v1')
        pushed_over = self._git('rev-parse', 'HEAD')
        # The run's rebase rewrites every hash; only some diffs change.
        self._git('reset', '-q', '--hard', 'origin/master')
        self._commit('kept', 'kept')
        changed = self._commit('changed', 'v2')
        new = self._commit('fix', 'fix', '[cr158] Fix the build')
        self._commit(
            'patches',
            'p',
            'Update patches from Chromium 158.0.8127.2 to Chromium '
            '158.0.8127.4.',
        )
        self._commit(
            'version',
            '158.0.8127.4',
            'Update from Chromium 158.0.8127.2 to Chromium 158.0.8127.4.',
        )
        url = 'https://github.com/brave/brave-core/commit/'
        self.assertEqual(
            self._review(pushed_over),
            [
                'To review (2):',
                f'  - [[cr158] Change changed]({url}{changed})',
                f'  - [[cr158] Fix the build]({url}{new})',
            ],
        )

    def test_new_branch_lists_all_but_brockit_commits(self):
        only = self._commit('fix', 'fix', '[cr159] Fix the build')
        self.assertEqual(
            self._review(''),
            [
                'To review (1):',
                '  - [[cr159] Fix the build]'
                f'(https://github.com/brave/brave-core/commit/{only})',
            ],
        )

    def test_nothing_changed(self):
        self._commit('kept', 'kept')
        pushed_over = self._git('rev-parse', 'HEAD')
        self.assertEqual(
            self._review(pushed_over), ['Nothing to review: no commit changed.']
        )


_START = '1' * 40
_LANDED = '2' * 40
_HEAD = '3' * 40
_BASE = '4' * 40
_BASE_MOVED = '5' * 40


class PublishTest(unittest.TestCase):
    def setUp(self):
        self.run_root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.run_root)
        self.calls = []

    def _publish(
        self,
        remote: str | None,
        head: str = _HEAD,
        start: str | None = _START,
        ahead: str = '3',
        rewritten: bool = False,
        squash_left: bool = False,
        dry_run_fails: bool = False,
        cherry_pick_stopped: bool = False,
        base_moved: bool = False,
        base_lifted: bool = False,
        built: bool = False,
        remote_at_push: str | None = None,
        git_cache: str = '',
        chromium_mismatch: str | None = None,
    ):
        # The fake repository: picks and rebases move HEAD, and fetching the
        # base moves origin/master to GitHub's, a lift of its own if
        # `base_lifted`.
        state = {
            'head': head,
            'base': _BASE,
            'forked': _BASE,
            'squash': squash_left,
            'lookups': 0,
        }
        github_base = _BASE_MOVED if base_moved else _BASE

        def run_git(*cmd, **_):
            self.calls.append(cmd)
            if cmd[:2] == ('remote', 'get-url'):
                return (
                    git_cache
                    if cmd[-1] == 'origin' and len(cmd) == 3
                    else ('https://github.com/brave/brave-core.git')
                )
            if cmd[0] == 'ls-remote':
                if cmd[-1] == 'refs/heads/master':
                    return f'{github_base}\trefs/heads/master'
                state['lookups'] += 1
                tip = remote
                if state['lookups'] > 1 and remote_at_push:
                    tip = remote_at_push
                return f'{tip}\trefs/heads/cr158' if tip else ''
            if cmd[:3] == ('rev-parse', '--verify', '--quiet'):
                if not cherry_pick_stopped:
                    raise subprocess.CalledProcessError(1, 'git')
                return _LANDED
            if cmd[:2] == ('rev-parse', '--abbrev-ref'):
                return 'origin/master'
            if cmd == ('rev-parse', 'HEAD'):
                return state['head']
            if cmd == ('rev-parse', 'origin/master'):
                return state['base']
            if cmd == ('fetch', 'origin', 'master'):
                state['base'] = github_base
            if cmd[0] == 'cherry-pick':
                state['head'] = 'picked'
            if cmd[:2] == ('merge-base', '--is-ancestor') and rewritten:
                raise subprocess.CalledProcessError(1, 'git')
            if cmd[0] == 'rev-list':
                return ahead
            return ''

        def run(cmd, **_):
            # The fake brockit: a dry run reports whether the base moved past
            # the branch's fork, or anything is left to squash.
            if 'rebase' in cmd and '--dry-run' in cmd:
                pending = state['base'] != state['forked'] or state['squash']
                if dry_run_fails:
                    return mock.Mock(returncode=1)
                return mock.Mock(returncode=2 if pending else 0)
            if 'rebase' in cmd:
                state.update(head='rebased', forked=state['base'], squash=False)
            if 'regen' in cmd:
                state['head'] = 'regenerated'
            return mock.Mock(returncode=0)

        def from_git(ref):
            self.assertEqual(ref, 'origin/master')
            lifted = base_lifted and state['base'] == _BASE_MOVED
            return autolift.Version(
                '157.0.8103.40' if lifted else '157.0.8103.30'
            )

        brave = mock.MagicMock()
        brave.current_branch.return_value = 'cr158'
        brave.is_rebase_in_progress.return_value = False
        brave.run_git.side_effect = run_git
        if start:
            (self.run_root / 'cr158').mkdir(exist_ok=True)
            (self.run_root / 'cr158' / 'start').write_text(f'{start}\n')
        with (
            mock.patch.object(autolift.repository, 'brave', brave),
            mock.patch.object(autolift, '_RUN_ROOT', self.run_root),
            mock.patch.object(
                autolift.subprocess, 'run', side_effect=run
            ) as run_mock,
            mock.patch.object(
                autolift.Version, 'from_git', side_effect=from_git
            ),
            mock.patch.object(
                autolift, 'chromium_mismatch', return_value=chromium_mismatch
            ),
            # `ReviewListTest` covers it, on a real repository.
            mock.patch.object(autolift.Publish, '_note_for_review'),
            mock.patch.dict(
                autolift.os.environ,
                {'GIT_CACHE_PATH': git_cache} if git_cache else {},
            ),
        ):
            if not git_cache:
                autolift.os.environ.pop('GIT_CACHE_PATH', None)
            result = autolift._cmd_publish(
                mock.Mock(milestone=158, built=built)
            )
        return result, run_mock

    def _pushes(self):
        return [c for c in self.calls if c[0] == 'push']

    def test_pushes_new_branch_without_changing_upstream(self):
        result, run = self._publish(remote=None, start=None)
        self.assertEqual(result, 0)
        self.assertEqual(
            self._pushes(),
            [
                (
                    'push',
                    '--force-with-lease=refs/heads/cr158:',
                    'origin',
                    'HEAD:refs/heads/cr158',
                )
            ],
        )
        self.assertIn('update-version-issue', run.call_args.args[0])

    def test_refuses_new_branch_without_commits(self):
        result, run = self._publish(remote=None, start=None, ahead='0')
        self.assertEqual(result, 1)
        self.assertEqual(self._pushes(), [])
        self.assertFalse(
            any('update-version-issue' in c.args[0] for c in run.call_args_list)
        )

    def test_pushes_changed_branch_over_its_start(self):
        result, _ = self._publish(remote=_START)
        self.assertEqual(result, 0)
        self.assertEqual(
            self._pushes(),
            [
                (
                    'push',
                    f'--force-with-lease=refs/heads/cr158:{_START}',
                    'origin',
                    'HEAD:refs/heads/cr158',
                )
            ],
        )

    def test_unchanged_branch_is_not_pushed(self):
        result, _ = self._publish(remote=_START, head=_START)
        self.assertEqual(result, 0)
        self.assertEqual(self._pushes(), [])

    def test_picks_what_landed_meanwhile_then_pushes_over_it(self):
        result, _ = self._publish(remote=_LANDED)
        self.assertEqual(result, 2)
        self.assertIn(('fetch', 'origin', 'cr158'), self.calls)
        self.assertIn(
            ('cherry-pick', '--empty=drop', f'{_START}..{_LANDED}'), self.calls
        )
        self.assertEqual(self._pushes(), [])

        # Not before the rebuild.
        result, _ = self._publish(remote=_LANDED, head='picked')
        self.assertEqual(result, 2)
        self.assertEqual(self._pushes(), [])

        result, _ = self._publish(remote=_LANDED, head='picked', built=True)
        self.assertEqual(result, 0)
        self.assertEqual(
            self._pushes()[0][1],
            f'--force-with-lease=refs/heads/cr158:{_LANDED}',
        )

    def test_squashes_what_is_left_before_pushing(self):
        # Base where it was, but e.g. the lift's new minor bump to squash.
        result, run = self._publish(remote=_START, squash_left=True)
        self.assertEqual(result, 2)
        dry_run, rebase = (c.args[0] for c in run.call_args_list)
        self.assertIn('--dry-run', dry_run)
        self.assertIn('--squash-minor-bumps', rebase)
        self.assertNotIn('--dry-run', rebase)
        self.assertNotIn('--discard-regen-changes', rebase)
        self.assertEqual(self._pushes(), [])

    def _brockit_calls(self, run) -> list[list[str]]:
        return [c.args[0][2:] for c in run.call_args_list]

    def test_base_lift_discards_and_regenerates(self):
        result, run = self._publish(
            remote=_START, base_moved=True, base_lifted=True
        )
        self.assertEqual(result, 2)
        self.assertEqual(
            self._brockit_calls(run),
            [
                [
                    'rebase',
                    '--squash-minor-bumps',
                    '--discard-regen-changes',
                    '--dry-run',
                ],
                ['rebase', '--squash-minor-bumps', '--discard-regen-changes'],
                ['regen', '--infra-mode'],
            ],
        )
        self.assertFalse((self.run_root / 'cr158' / 'regen-needed').exists())

    def test_a_changed_branch_asks_for_a_sync_and_rebuild(self):
        with mock.patch('builtins.print') as printed:
            result, _ = self._publish(remote=_LANDED)
            self.assertEqual(result, 2)
            printed.assert_called_with(autolift.Publish.REBUILD)
            # And so does any round until `--built` says it's done.
            result, _ = self._publish(remote=_LANDED, head='picked')
            self.assertEqual(result, 2)
            printed.assert_called_with(autolift.Publish.REBUILD)
        self.assertIn('pnpm run sync', autolift.Publish.REBUILD)

    def test_mismatched_checkout_is_synced_before_the_final_rebase(self):
        _, run = self._publish(
            remote=_START, squash_left=True, chromium_mismatch='off by one'
        )
        commands = [c.args[0] for c in run.call_args_list]
        self.assertEqual(commands[1], ['pnpm', 'run', 'sync'])
        self.assertIn('rebase', commands[2])
        self.assertNotIn('--dry-run', commands[2])

    def test_base_move_without_a_lift_keeps_regen_changes(self):
        _, run = self._publish(remote=_START, base_moved=True)
        calls = self._brockit_calls(run)
        self.assertNotIn('regen', [c[0] for c in calls])
        self.assertFalse(any('--discard-regen-changes' in c for c in calls))

    def test_regen_survives_a_stopped_rebase(self):
        # A round whose rebase stopped, and was finished by hand: the base is
        # synced, but the regen is still owed.
        (self.run_root / 'cr158').mkdir()
        (self.run_root / 'cr158' / 'regen-needed').touch()
        (self.run_root / 'cr158' / 'base-synced').write_text(f'{_BASE}\n')
        result, run = self._publish(remote=_START)
        self.assertEqual(result, 2)
        self.assertIn(['regen', '--infra-mode'], self._brockit_calls(run))

    def test_writes_what_it_did_to_the_report(self):
        self._publish(remote=_LANDED, base_moved=True, base_lifted=True)
        report = (self.run_root / 'cr158' / 'report.md').read_text()
        self.assertIn('## publish 158', report)
        self.assertIn('Picked what landed on GitHub', report)
        self.assertIn('had a lift of its own', report)
        self.assertIn('Ran `brockit regen`', report)
        self.assertIn('rebuild', report)

    def test_report_records_the_push(self):
        self._publish(remote=_START)
        report = (self.run_root / 'cr158' / 'report.md').read_text()
        self.assertIn(f'Pushed cr158 over {_START[:12]}', report)

    def test_record_start_clears_an_earlier_run(self):
        directory = self.run_root / 'cr158'
        directory.mkdir()
        for name in ('synced', 'base-synced', 'rebuild-needed', 'regen-needed'):
            (directory / name).touch()
        with mock.patch.object(autolift, '_RUN_ROOT', self.run_root):
            autolift.Publish.record_start(158, _START)
        self.assertEqual(sorted(p.name for p in directory.iterdir()), ['start'])
        self.assertEqual((directory / 'start').read_text(), f'{_START}\n')

    def test_failed_dry_run_stops_without_rebasing(self):
        result, run = self._publish(remote=_START, dry_run_fails=True)
        self.assertEqual(result, 1)
        self.assertTrue(
            all('--dry-run' in c.args[0] for c in run.call_args_list)
        )
        self.assertEqual(self._pushes(), [])

    def test_nothing_to_squash_needs_no_rebase(self):
        result, run = self._publish(remote=_START)
        self.assertEqual(result, 0)
        self.assertEqual(len(run.call_args_list), 1)
        self.assertIn('--dry-run', run.call_args.args[0])

    def test_rebases_onto_the_latest_base_once(self):
        result, run = self._publish(remote=_START, base_moved=True)
        self.assertEqual(result, 2)
        self.assertIn(('fetch', 'origin', 'master'), self.calls)
        self.assertIn('--squash-minor-bumps', run.call_args.args[0])
        self.assertEqual(self._pushes(), [])

        # Master moving on again doesn't hold the push back.
        self.calls.clear()
        result, run = self._publish(
            remote=_START, head='rebased', base_moved=True, built=True
        )
        self.assertEqual(result, 0)
        self.assertNotIn(('fetch', 'origin', 'master'), self.calls)
        self.assertTrue(
            all('--dry-run' in c.args[0] for c in run.call_args_list)
        )
        self.assertEqual(len(self._pushes()), 1)

    def test_fetches_right_before_pushing(self):
        result, _ = self._publish(remote=_START)
        self.assertEqual(result, 0)
        push = self.calls.index(self._pushes()[0])
        self.assertEqual(self.calls[push - 1], ('fetch', 'origin', 'cr158'))
        self.assertFalse(any(c[0] == 'cache' for c in self.calls))

    def test_refreshes_the_git_cache_before_looking_and_pushing(self):
        cache = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, cache)
        result, _ = self._publish(remote=_START, git_cache=cache)
        self.assertEqual(result, 0)
        populate = (
            'cache',
            'populate',
            '--cache-dir',
            cache,
            'https://github.com/brave/brave-core.git',
            '--reset-fetch-config',
            '--no-fetch-tags',
        )
        self.assertEqual(self.calls.count(populate), 2)
        # The second one is part of the check right before the push.
        last = len(self.calls) - 1 - self.calls[::-1].index(populate)
        self.assertLess(last, self.calls.index(self._pushes()[0]))
        self.assertGreater(
            last, self.calls.index(('fetch', 'origin', 'master'))
        )

    def test_no_push_when_github_moves_just_before_it(self):
        result, _ = self._publish(remote=_START, remote_at_push='9' * 40)
        self.assertEqual(result, 2)
        self.assertEqual(self._pushes(), [])

    def test_refuses_rewritten_github_branch(self):
        result, _ = self._publish(remote=_LANDED, rewritten=True)
        self.assertEqual(result, 1)
        self.assertFalse(any(c[0] == 'cherry-pick' for c in self.calls))
        self.assertEqual(self._pushes(), [])

    def test_refuses_during_a_cherry_pick(self):
        result, _ = self._publish(remote=_LANDED, cherry_pick_stopped=True)
        self.assertEqual(result, 1)
        self.assertEqual(self._pushes(), [])


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
            '* Watching pipelines\n¯\\_(ツ)_/¯\n'
            '🔥🔥 Pipeline failed: http://ci/1\n'
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
    def _lift(
        self,
        log_text: str,
        returncode: int = 0,
        lift_to: str | None = '@latest-for-branch',
    ):
        directory = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, directory)
        if lift_to is not None:
            (directory / 'cr158').mkdir()
            (directory / 'cr158' / 'lift-target').write_text(f'{lift_to}\n')

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

    def test_lifts_to_the_recorded_release(self):
        _, run_mock, _ = self._lift('💥 Done!\n', lift_to='158.0.8127.4')
        self.assertIn('--to=158.0.8127.4', run_mock.call_args.args[0])

    def test_refuses_when_no_lift_is_needed(self):
        result, run_mock, _ = self._lift('', lift_to='none')
        self.assertEqual(result, 1)
        run_mock.assert_not_called()

    def test_refuses_before_prepare(self):
        result, run_mock, _ = self._lift('', lift_to=None)
        self.assertEqual(result, 1)
        run_mock.assert_not_called()


if __name__ == '__main__':
    unittest.main()
