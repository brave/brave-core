#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import contextlib
from datetime import timedelta
import io
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

import autolift

_FULL_HASH = '5691202e214341a5d96a3296c1b9811cce19572b'


def _git_env() -> dict[str, str]:
    """Isolates git from the user's config, e.g. `tag.gpgsign` opening an
    editor that hangs the test."""
    return {
        **os.environ,
        'GIT_CONFIG_GLOBAL': os.devnull,
        'GIT_CONFIG_NOSYSTEM': '1',
    }


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
        self.assertEqual(summary.output, [])

    def test_remote_execution_failure_is_the_environment(self):
        summary = autolift.summarize_build(
            '\n'.join(
                [
                    ' 3.58s Error: failed to check writable: upload batch: rpc '
                    'error: code = PermissionDenied desc = Failed to verify '
                    'the TLS certificate.',
                    'The build has finished with an error.',
                ]
            )
        )
        self.assertEqual(len(summary.errors), 1)
        self.assertEqual(summary.environment_errors, [summary.errors[0][1]])
        self.assertTrue(summary.errors[0][1].startswith('Error: failed'))

    def test_code_errors_are_not_the_environment(self):
        summary = autolift.summarize_build(
            '../../a.cc:3:10: fatal error: \'b.h\' file not found\n'
        )
        self.assertEqual(summary.environment_errors, [])

    def test_counts_clang_fatal_errors(self):
        errors = autolift.summarize_build(
            '../../a.cc:3:10: fatal error: \'b.h\' file not found\n'
        ).errors
        self.assertEqual(
            errors,
            [(1, '../../a.cc:3:10: fatal error: \'b.h\' file not found')],
        )


_SETUP = [
    '$ node ./build/commands/scripts/build.ts Static --target=brave:all',
    'update branding...',
    'generate ninja files...',
]
_SISO_START = [
    'Detected AI agent env (CLAUDECODE, AI_AGENT). Prepending --quiet',
    'Run `siso ps -C out/x` in another terminal to see build status.',
]


class BuildOutputTest(unittest.TestCase):
    def test_failed_build_shows_its_output_after_the_siso_banner(self):
        summary = autolift.summarize_build(
            '\n'.join(
                _SETUP
                + _SISO_START
                + [
                    'FAILED: 5dfc "./obj/a.stamp" ACTION //a:a__errorprone',
                    'stderr:',
                    '../../A.java:119: warning: [NullablePrimitiveArray] x',
                    '        void f(@Nullable byte[] b);',
                    '                         ^',
                    '19.66s Build Failure: 38 done, 1 failed, 1 remaining',
                ]
            )
        )
        self.assertEqual(summary.errors, [])
        self.assertEqual(summary.output[0][:7], 'FAILED:')
        self.assertIn(
            '../../A.java:119: warning: [NullablePrimitiveArray] x',
            summary.output,
        )
        self.assertNotIn('update branding...', summary.output)

    def test_gn_failure_shows_output_from_gn_gen(self):
        summary = autolift.summarize_build(
            '\n'.join(_SETUP + ['ERROR at //a/BUILD.gn:1:1: Assertion failed.'])
        )
        self.assertEqual(
            summary.output,
            [
                'generate ninja files...',
                'ERROR at //a/BUILD.gn:1:1: Assertion failed.',
            ],
        )

    def test_cuts_long_lines_and_collapses_repeats(self):
        command = 'clang++ ' + 'x' * 1000
        summary = autolift.summarize_build(
            '\n'.join(_SISO_START + [command, 'same', 'same', 'Build Failure'])
        )
        self.assertEqual(len(summary.output), 3)
        self.assertTrue(summary.output[0].endswith('(708 more columns)'))
        self.assertEqual(summary.output[1:], ['same', 'Build Failure'])

    def test_elides_the_middle_of_long_output(self):
        lines = [f'line {i}' for i in range(500)]
        summary = autolift.summarize_build('\n'.join(_SISO_START + lines))
        self.assertEqual(summary.output[0], 'line 0')
        self.assertEqual(summary.output[-1], 'line 499')
        self.assertIn('320 lines omitted', summary.output[150])

    def test_reports_gn_args_with_no_effect_on_success(self):
        summary = autolift.summarize_build(
            '\n'.join(
                [
                    'WARNING at //out/a/args_generated.gni:39:31: Build '
                    'argument has no effect.',
                    'The variable "android_override_version_name" was set as '
                    'a build argument',
                    '.The build has finished successfully.',
                ]
            )
        )
        self.assertEqual(
            summary.no_effect_args, ['android_override_version_name']
        )
        self.assertEqual(summary.output, [])


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


class FixedInSubjectsTest(unittest.TestCase):
    def test_reads_plain_and_backticked_subjects(self):
        text = (
            '- [x] A (fixed in `[cr159] Restore Brave\'s version name`)\n'
            '- [x] B (fixed in\n'
            '      `` [cr159][cr] Format only the files `follow-renames` and\n'
            '      `mv` change ``)\n'
        )
        self.assertEqual(
            (
                "[cr159] Restore Brave's version name",
                '[cr159][cr] Format only the files `follow-renames` and '
                '`mv` change',
            ),
            autolift.fixed_in_subjects(text),
        )


class ChromiumPathspecTest(unittest.TestCase):
    _SRC = Path('/checkout/src')
    _BRAVE = _SRC / 'brave'

    def _pathspec(self, path: str) -> str:
        return autolift.chromium_pathspec(path, self._BRAVE, self._SRC)

    def test_keeps_src_relative_paths(self):
        self.assertEqual(
            self._pathspec('chrome/browser/foo.cc'), 'chrome/browser/foo.cc'
        )

    def test_resolves_paths_relative_to_cwd(self):
        self.assertEqual(
            self._pathspec('../chrome/browser/foo.cc'), 'chrome/browser/foo.cc'
        )
        self.assertEqual(
            self._pathspec('./browser/foo.cc'), 'brave/browser/foo.cc'
        )

    def test_resolves_absolute_paths(self):
        self.assertEqual(
            self._pathspec('/checkout/src/net/http/*.cc'), 'net/http/*.cc'
        )

    def test_rejects_paths_outside_the_checkout(self):
        with self.assertRaises(ValueError):
            self._pathspec('../../other/foo.cc')


class CmdCulpritsTest(unittest.TestCase):
    def _run(self, *paths: str) -> tuple[int, mock.Mock]:
        args = mock.Mock(
            range='158.0.8117.1..158.0.8127.2', symbol=None, paths=list(paths)
        )
        with mock.patch.object(autolift.repository, 'chromium') as chromium:
            chromium.root = Path.cwd() / '..'
            chromium.run_git.return_value = ''
            with contextlib.redirect_stderr(io.StringIO()):
                returncode = autolift._cmd_culprits(args)
        return returncode, chromium

    def test_passes_cwd_relative_paths_as_src_relative(self):
        returncode, chromium = self._run('../chrome/foo.cc')
        self.assertEqual(returncode, 0)
        self.assertEqual(
            chromium.run_git.call_args.args[-2:], ('--', 'chrome/foo.cc')
        )

    def test_reports_a_git_failure_in_one_line(self):
        args = mock.Mock(
            range='158.0.8117.1..158.0.8127.2', symbol=None, paths=[]
        )
        with mock.patch.object(autolift.repository, 'chromium') as chromium:
            chromium.run_git.side_effect = subprocess.CalledProcessError(
                128, 'git', stderr="fatal: bad revision 'x'\nmore\n"
            )
            stderr = io.StringIO()
            with contextlib.redirect_stderr(stderr):
                self.assertEqual(autolift._cmd_culprits(args), 1)
        self.assertEqual(
            stderr.getvalue(), "culprits: fatal: bad revision 'x'\n"
        )


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
        args = mock.Mock(target=158, no_rebase=True, force=False, dry_run=False)
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
        args = mock.Mock(target=158, no_rebase=True, force=False, dry_run=False)
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

    def test_fix_sharing_only_the_change_id(self):
        # The culprit isn't in the checkout, so only the Change-Id its
        # quoted message carries ties the two together.
        pick = autolift.Commit(
            _M158_PICK, 'Fix the thing', [], change_ids=(_CHANGE_ID,)
        )
        fix = autolift.Commit(
            'd' * 40, 'Follow', ['e' * 40], change_ids=(_CHANGE_ID,)
        )
        self.assertEqual(
            autolift.match_next_fixes([pick], {'cr159': [fix]}),
            [autolift.NextFix(pick, 'cr159', fix)],
        )


_CHANGE_ID = 'Ibf139df91ff4e8fde5cf972d0f42c11a269a940b'
_REVERTED_CHANGE_ID = 'I' + '1' * 40

# A cherry-pick's message: the original's description is quoted, a revert's
# Change-Id included, and the cherry-pick keeps the original's own.
_CHERRY_PICK_MESSAGE = f"""\
[M157] usb: Default-disable reset config invalidation

Original change's description:
> usb: Default-disable reset config invalidation
>
> Change-Id: {_REVERTED_CHANGE_ID}

(cherry picked from commit {_MAIN})

Bug: 579988731
Change-Id: {_CHANGE_ID}
Cr-Commit-Position: refs/branch-heads/8103@{{#1027}}
"""


class ChangeIdTest(unittest.TestCase):
    def test_a_chromium_commits_own_change_id(self):
        self.assertEqual(
            autolift.Commit.change_ids_in(_CHERRY_PICK_MESSAGE), (_CHANGE_ID,)
        )

    def test_the_change_ids_a_brave_commit_quotes_for_its_culprits(self):
        quoted = '\n'.join(
            f'    {line}' if line else ''
            for line in _CHERRY_PICK_MESSAGE.splitlines()
        )
        message = (
            '[cr159] Revert the backport\n\nThe tag landed it.\n\n'
            'Chromium changes:\n'
            f'https://chromium.googlesource.com/chromium/src/+/{_MAIN}\n\n'
            f'commit {_MAIN}\nAuthor: A <a@b.c>\n\n{quoted}\n'
        )
        self.assertEqual(
            autolift.Commit.culprit_change_ids(message), (_CHANGE_ID,)
        )

    def test_a_commit_without_culprits_quotes_none(self):
        self.assertEqual(
            autolift.Commit.culprit_change_ids(
                f'IWYU fixes.\n\nChange-Id: {_CHANGE_ID}\n'
            ),
            (),
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
        '\n## For privacy review\n\n'
        '- Web Install, from\n'
        f'  [dddd](https://chromium.googlesource.com/chromium/src/+/{"d" * 40})\n'
        '- Something else\n'
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

    def test_bullets_before_a_sections_first_item_are_its_own(self):
        pick = autolift.Commit(_M158_PICK, 'Web Install', ['d' * 40])
        match = self._find_one(pick)
        self.assertEqual(match.title, 'For privacy review')
        self.assertEqual(
            match.lines,
            (
                '- Web Install, from [dddd](https://chromium.googlesource.com/'
                f'chromium/src/+/{"d" * 40})',
            ),
        )

    def test_the_matched_line_is_kept(self):
        match = self._find_one(self._FAVICON_PICK)
        self.assertEqual(
            match.lines,
            ('Upstream changed `favicon_loader_impl.mm` the same way.',),
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
    def _pick(
        self,
        fix_message: str,
        culprit: str | None = _M158_PICK[:10],
        culprit_message: str = f'Fix\n\n(cherry picked from commit {_MAIN})\n',
    ):
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
                return culprit_message
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

    def test_a_fix_quoting_the_same_change_id_is_the_fix(self):
        message = (
            self._fix_message('e' * 40)
            + f'\ncommit {"e" * 40}\n\n    Fix\n\n    Change-Id: {_CHANGE_ID}\n'
        )
        result, calls = self._pick(
            message, culprit_message=f'Fix\n\nChange-Id: {_CHANGE_ID}\n'
        )
        self.assertEqual(result, 0)
        self.assertIn(('cherry-pick', 'd' * 40), calls)

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
        '@latest-m158': autolift.Version('158.0.8128.1'),
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
        rebase_fails: bool = False,
        always: bool = True,
        base_commits: int = 0,
        since_rebase: timedelta = timedelta(hours=1),
        head: str = '',
        unpushed: int = 0,
        synced_head: str | None = None,
        conflicts: tuple[str, ...] = (),
        command=autolift._cmd_prepare,
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
            if cmd == ('rev-parse', 'HEAD'):
                return head
            if cmd == ('rev-parse', f'origin/cr{milestone}'):
                return 'origin-tip'
            if cmd[0] == 'rev-list':
                # The base's commits HEAD lacks, and the local commits origin
                # lacks; anything else finds none.
                if cmd[-1].startswith('HEAD..'):
                    return str(base_commits)
                if cmd[-1] == f'origin/cr{milestone}..cr{milestone}':
                    return str(unpushed)
                return '0'
            if cmd[0] == 'merge-tree' and conflicts:
                raise subprocess.CalledProcessError(
                    1, 'git', output='\n'.join(('tree', *conflicts))
                )
            return ''

        brave = mock.MagicMock()
        brave.is_rebase_in_progress.return_value = False
        brave.run_git.side_effect = run_git
        versions = {
            'origin/master': autolift.Version('156.0.8078.74'),
            'origin/cr157': autolift.Version('157.0.8103.15'),
            'HEAD': autolift.Version('158.0.8127.2'),
        }
        args = mock.Mock(
            target=milestone,
            no_rebase=False,
            force=False,
            always=always,
            dry_run=False,
        )
        run_root = self.run_root
        brave.root = run_root
        (run_root / f'cr{milestone}').mkdir(parents=True, exist_ok=True)
        (run_root / f'cr{milestone}' / 'lift-stopped').touch()
        if synced_head is not None:
            (run_root / f'cr{milestone}').mkdir(parents=True, exist_ok=True)
            (run_root / f'cr{milestone}' / 'synced-head').write_text(
                f'{synced_head}\n', encoding='utf-8'
            )
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
            mock.patch.object(
                autolift, '_since_rebase', return_value=since_rebase
            ),
            mock.patch.object(autolift, '_RUN_ROOT', run_root),
            mock.patch.object(
                autolift,
                'read_chromium_version_file',
                return_value=autolift.Version(checkout),
            ),
        ):
            run.side_effect = lambda cmd, **_: mock.Mock(
                returncode=int(
                    (sync_fails and cmd[:2] == ['pnpm', 'run'])
                    or (rebase_fails and 'rebase' in cmd)
                )
            )
            result = command(args)
        target = run_root / f'cr{milestone}' / 'lift-target'
        if not target.exists():
            return result, calls, run, None
        return result, calls, run, target.read_text(encoding='utf-8').strip()

    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.run_root = Path(stack.enter_context(tempfile.TemporaryDirectory()))

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

    def test_a_rebase_stopped_on_a_conflict_fails_prepare(self):
        # Brockit exits 1 when the rebase stops on a conflict it can't settle.
        result, _, run, _ = self._run(
            158, remote_exists=True, rebase_fails=True
        )
        self.assertIn('rebase', run.call_args.args[0])
        self.assertEqual(result, 1)
        self.assertFalse((self.run_root / 'cr158' / 'lift-stopped').exists())

    def test_keeps_the_commits_sync_tooling_made(self):
        result, calls, run, _ = self._run(
            158,
            remote_exists=True,
            head='synced',
            unpushed=7,
            synced_head='synced',
        )
        self.assertEqual(result, 0)
        self.assertIn(('checkout', '-B', 'cr158', 'synced'), calls)
        self.assertIn('rebase', run.call_args.args[0])
        start = self.run_root / 'cr158' / 'start'
        self.assertEqual(
            'origin-tip', start.read_text(encoding='utf-8').strip()
        )

    def test_refuses_other_unpushed_commits(self):
        result, _, run, _ = self._run(
            158,
            remote_exists=True,
            head='elsewhere',
            unpushed=7,
            synced_head='synced',
        )
        self.assertEqual(result, 1)
        run.assert_not_called()

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

    def _no_lift(self, **kwargs):
        releases = dict(self._RELEASES)
        releases['@latest-beta'] = autolift.Version('157.0.8103.30')
        return self._run(
            158,
            remote_exists=True,
            higher_exists=True,
            releases=releases,
            always=False,
            **kwargs,
        )

    def test_no_lift_and_nothing_to_rebase_is_not_worth_a_run(self):
        result, _, run, _ = self._no_lift(base_commits=0)
        self.assertEqual(result, autolift.NOT_WORTH_A_RUN)
        run.assert_not_called()

    def test_no_lift_and_a_small_recent_rebase_is_not_worth_a_run(self):
        result, _, run, _ = self._no_lift(base_commits=20)
        self.assertEqual(result, autolift.NOT_WORTH_A_RUN)
        run.assert_not_called()

    def test_a_small_rebase_that_conflicts_is_worth_a_run(self):
        result, calls, run, _ = self._no_lift(
            base_commits=1, conflicts=('build/commands/scripts/format.ts',)
        )
        self.assertEqual(result, 0)
        self.assertIn('rebase', run.call_args.args[0])
        self.assertIn(
            (
                'merge-tree',
                '--write-tree',
                '--name-only',
                '--no-messages',
                'HEAD',
                'origin/cr157',
            ),
            calls,
        )
        report = (self.run_root / 'cr158' / 'report.md').read_text(
            encoding='utf-8'
        )
        self.assertIn(
            'conflicts with origin/cr157 in 1 file(s), so it is rebased '
            'however little it is behind: build/commands/scripts/format.ts.',
            report,
        )

    def test_a_small_rebase_is_worth_it_12_hours_after_the_last(self):
        result, _, run, _ = self._no_lift(
            base_commits=1, since_rebase=timedelta(hours=13)
        )
        self.assertEqual(result, 0)
        self.assertIn('rebase', run.call_args.args[0])

    def test_more_than_20_base_commits_are_worth_a_rebase(self):
        result, _, run, _ = self._no_lift(base_commits=21)
        self.assertEqual(result, 0)
        self.assertIn('rebase', run.call_args.args[0])

    def test_a_lift_is_always_worth_a_run(self):
        result, _, run, _ = self._run(
            158, remote_exists=True, higher_exists=True, always=False
        )
        self.assertEqual(result, 0)
        self.assertIn('rebase', run.call_args.args[0])

    def test_head_branch_on_its_latest_tag_lifts_nothing(self):
        releases = dict(self._RELEASES)
        releases['@latest-m158'] = autolift.Version('158.0.8127.2')
        result, _, run, _ = self._run(
            158, remote_exists=True, releases=releases, always=False
        )
        self.assertEqual(result, autolift.NOT_WORTH_A_RUN)
        run.assert_not_called()

    def test_head_branch_skips_a_minor_bump_of_its_tag(self):
        releases = dict(self._RELEASES)
        releases['@latest-m158'] = autolift.Version('158.0.8127.7')
        result, _, run, target = self._run(
            158,
            remote_exists=True,
            releases=releases,
            always=False,
            base_commits=21,
        )
        self.assertEqual(result, 0)
        self.assertEqual(target, 'none')
        self.assertNotIn('--discard-regen-changes', run.call_args.args[0])
        report = (self.run_root / 'cr158' / 'report.md').read_text(
            encoding='utf-8'
        )
        self.assertIn(
            '158.0.8127.7 is only a minor bump of 158.0.8127.2', report
        )

    def test_head_branch_lifts_when_the_build_moves(self):
        result, _, _, target = self._run(158, remote_exists=True)
        self.assertEqual(result, 0)
        self.assertEqual(target, '@latest-for-branch')

    def _plan(self, **kwargs):
        result, calls, run, _ = self._run(
            158, remote_exists=True, command=autolift._cmd_plan, **kwargs
        )
        # It only reads: no reset, upstream, rebase or lift target.
        self.assertFalse(
            any(c[0] in ('checkout', 'branch', 'reset') for c in calls)
        )
        run.assert_not_called()
        self.assertFalse((self.run_root / 'cr158' / 'lift-target').exists())
        return result

    def test_plan_says_a_lift_is_worth_a_run(self):
        self.assertEqual(self._plan(), 0)

    def test_plan_says_a_minor_bump_is_not_worth_a_run(self):
        releases = dict(self._RELEASES)
        releases['@latest-m158'] = autolift.Version('158.0.8127.7')
        self.assertEqual(
            self._plan(releases=releases, base_commits=20),
            autolift.NOT_WORTH_A_RUN,
        )

    def test_plan_says_a_rebase_without_a_lift_is_worth_a_run(self):
        releases = dict(self._RELEASES)
        releases['@latest-m158'] = autolift.Version('158.0.8127.7')
        self.assertEqual(self._plan(releases=releases, base_commits=21), 0)

    def _advise(self, states=(), **kwargs) -> tuple[dict, mock.Mock]:
        """`advise 158`'s JSON, with the pull request's checks in `states`."""
        output = self.run_root / 'advice.json'
        checks = autolift.ci_github.PullRequestChecks(
            number=41074,
            branch='cr158',
            head='a' * 40,
            checks=tuple(
                autolift.ci_github.Check(f'check {i}', state, 'u')
                for i, state in enumerate(states)
            ),
        )

        def command(args):
            args.autofix, args.resume, args.json_output = False, False, output
            return autolift._cmd_advise(args)

        with mock.patch.object(
            autolift.ci_github, 'pr_checks', return_value=checks
        ) as pr_checks:
            result, calls, run, _ = self._run(
                158, remote_exists=True, command=command, **kwargs
            )
        self.assertEqual(result, 0)
        self.assertFalse(
            any(c[0] in ('checkout', 'branch', 'reset') for c in calls)
        )
        run.assert_not_called()
        return autolift.json.loads(
            output.read_text(encoding='utf-8')
        ), pr_checks

    def _minor_bump(self, **kwargs) -> tuple[dict, mock.Mock]:
        releases = dict(self._RELEASES)
        releases['@latest-m158'] = autolift.Version('158.0.8127.7')
        return self._advise(releases=releases, **kwargs)

    def test_advise_a_lift(self):
        advice, pr_checks = self._advise()
        self.assertEqual(advice['action'], 'lift')
        self.assertTrue(
            advice['prompt'].startswith(
                'Lift cr158 by following tools/cr/autolift/SKILL.md'
            )
        )
        self.assertIn('unattended CI run', advice['prompt'])
        pr_checks.assert_not_called()

    def test_advise_a_rebase_without_a_lift(self):
        advice, _ = self._minor_bump(base_commits=21)
        self.assertEqual(advice['action'], 'rebase')
        self.assertTrue(advice['prompt'].startswith('Lift cr158 '))

    def test_advise_an_autofix_when_a_run_isnt_worth_it_and_a_check_failed(
        self,
    ):
        State = autolift.ci_github.State
        advice, pr_checks = self._minor_bump(
            base_commits=20, states=(State.FAILURE, State.PENDING)
        )
        pr_checks.assert_called_once_with('cr158')
        self.assertEqual(advice['action'], 'autofix')
        self.assertTrue(
            advice['prompt'].startswith(
                "Fix what fails in the CI of cr158's pull request"
            )
        )
        self.assertTrue(
            any(
                r.startswith('- Not worth a lift run: no lift')
                for r in advice['reasons']
            )
        )
        self.assertIn('  - Failed: check 0', advice['reasons'])

    def test_advise_nothing_when_checks_are_only_pending(self):
        State = autolift.ci_github.State
        advice, _ = self._minor_bump(
            base_commits=20, states=(State.SUCCESS, State.PENDING)
        )
        self.assertEqual(advice['action'], 'none')
        self.assertIsNone(advice['prompt'])

    def test_advise_a_resumed_lift_without_looking(self):
        with mock.patch.object(autolift.repository, 'brave') as brave:
            advice = autolift.Advice.of(158, autofix=False, resume=True)
        brave.run_git.assert_not_called()
        self.assertEqual(advice.action, 'resume')
        self.assertTrue(
            advice.prompt.startswith(
                'Resume the lift of cr158, which an earlier run of '
                'tools/cr/autolift/SKILL.md'
            )
        )

    def test_advise_nothing_to_autofix_without_the_branch_on_github(self):
        with (
            mock.patch.object(
                autolift, '_remote_branch_exists', return_value=False
            ),
            mock.patch.object(autolift.ci_github, 'pr_checks') as pr_checks,
        ):
            advice = autolift.Advice.of(158, autofix=True, resume=False)
        self.assertEqual(advice.action, 'none')
        pr_checks.assert_not_called()

    def test_plan_says_a_new_milestone_is_worth_a_run(self):
        result, calls, run, _ = self._run(
            159, remote_exists=False, command=autolift._cmd_plan
        )
        self.assertEqual(result, 0)
        self.assertFalse(any(c[0] == 'fetch' for c in calls))
        run.assert_not_called()

    def test_head_branch_on_its_latest_tag_only_rebases(self):
        releases = dict(self._RELEASES)
        releases['@latest-m158'] = autolift.Version('158.0.8127.2')
        result, _, run, target = self._run(
            158,
            remote_exists=True,
            releases=releases,
            always=False,
            base_commits=21,
        )
        self.assertEqual(result, 0)
        # The run follows the steps without a lift: no Android build.
        self.assertEqual(target, 'none')
        cmd = run.call_args.args[0]
        self.assertIn('rebase', cmd)
        # No lift remakes the regen commits, so the rebase keeps them.
        self.assertNotIn('--discard-regen-changes', cmd)
        report = (self.run_root / 'cr158' / 'report.md').read_text(
            encoding='utf-8'
        )
        self.assertIn('No lift needed: rebase only', report)

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
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.dir = Path(stack.enter_context(tempfile.TemporaryDirectory()))
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
            env=_git_env(),
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
                env=_git_env(),
                capture_output=True,
                text=True,
            ).stdout
            if no_trim
            else self._git(*args)
        )
        with mock.patch.object(autolift.repository, 'brave', brave):
            publish = autolift.Publish(autolift.Run.cr(158))
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


class PublishWithoutRebaseTest(unittest.TestCase):
    """`publish --no-rebase`, pushing to a bare repository as `origin`."""

    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        self.run_root = root / 'runs'
        self.remote = root / 'remote.git'
        self.dir = root / 'brave'
        subprocess.run(
            ['git', 'init', '-q', '--bare', str(self.remote)],
            check=True,
            env=_git_env(),
        )
        self.dir.mkdir()
        self._git('init', '-q', '-b', 'master')
        self._git('config', 'user.email', 'a@b.c')
        self._git('config', 'user.name', 'A')
        self._git('remote', 'add', 'origin', str(self.remote))
        self._commit('base', '[cr158] Base')
        self._git('push', '-q', 'origin', 'master')
        self._git('checkout', '-q', '-b', 'cr158')
        self._commit('fix', '[cr158] Fix the build')
        self._git('push', '-q', 'origin', 'cr158')
        self._git('fetch', '-q', 'origin')
        self._git('branch', '-q', '--set-upstream-to=origin/master')
        stack.enter_context(mock.patch.dict(os.environ, {'GIT_CACHE_PATH': ''}))
        stack.enter_context(
            mock.patch.object(autolift, '_RUN_ROOT', self.run_root)
        )
        # Where `autofix` (or `prepare`) left the run.
        autolift.Publish.record_start(autolift.Run.cr(158), self._remote_tip())

    def _git(self, *args: str, cwd: Path | None = None) -> str:
        return subprocess.run(
            ['git', '-C', str(cwd or self.dir), *args],
            check=True,
            env=_git_env(),
            capture_output=True,
            text=True,
        ).stdout.strip()

    def _commit(self, name: str, subject: str) -> str:
        (self.dir / name).write_text(f'{subject}\n', encoding='utf-8')
        self._git('add', name)
        self._git('commit', '-q', '-m', subject)
        return self._git('rev-parse', 'HEAD')

    def _remote_tip(self) -> str:
        return self._git('rev-parse', 'cr158', cwd=self.remote)

    def _push(self, allow_new: bool = False, built: bool = False) -> int:
        brave = mock.MagicMock(root=self.dir)
        brave.current_branch.return_value = 'cr158'
        brave.is_rebase_in_progress.return_value = False
        brave.run_git.side_effect = lambda *args, no_trim=False: (
            subprocess.run(
                ['git', '-C', str(self.dir), *args],
                check=True,
                env=_git_env(),
                capture_output=True,
                text=True,
            ).stdout
            if no_trim
            else self._git(*args)
        )
        with (
            mock.patch.object(autolift.repository, 'brave', brave),
            # `PublishTest` covers it.
            mock.patch.object(
                autolift.Publish, '_presubmit', return_value=True
            ),
            contextlib.redirect_stdout(io.StringIO()),
        ):
            return autolift.Publish(autolift.Run.cr(158)).run(
                built=built, rebase=False, allow_new=allow_new
            )

    def _report(self) -> str:
        return (self.run_root / 'cr158' / 'report.md').read_text(
            encoding='utf-8'
        )

    def test_pushes_fixups_and_pinned_subjects(self):
        self._commit('fix', 'fixup! [cr158] Fix the build')
        head = self._commit('iwyu', '[cr158] IWYU fixes.')
        self.assertEqual(self._push(), 0)
        self.assertEqual(self._remote_tip(), head)
        self.assertIn('Pushed cr158 over', self._report())

    def test_refuses_new_commits_unless_allowed(self):
        tip = self._remote_tip()
        head = self._commit('other', '[cr158] Follow another change')
        self.assertEqual(self._push(), 1)
        self.assertEqual(self._remote_tip(), tip)
        self.assertIn('`[cr158] Follow another change`', self._report())
        self.assertEqual(self._push(allow_new=True), 0)
        self.assertEqual(self._remote_tip(), head)

    def test_picks_what_landed_and_asks_for_a_rebuild(self):
        start = self._remote_tip()
        # A teammate pushes while the run works on its fixup.
        landed = self._commit('teammate', '[cr158] A teammate fix')
        self._git('push', '-q', 'origin', 'cr158')
        self._git('reset', '-q', '--hard', start)
        self._commit('fix', 'fixup! [cr158] Fix the build')

        self.assertEqual(self._push(), autolift.Publish.AGAIN)
        self.assertEqual(self._remote_tip(), landed)
        self.assertIn('Picked what landed on GitHub', self._report())
        # The rebuild is asked for until it's done.
        self.assertEqual(self._push(), autolift.Publish.AGAIN)

        self.assertEqual(self._push(built=True), 0)
        head = self._git('rev-parse', 'HEAD')
        self.assertEqual(self._remote_tip(), head)
        self.assertEqual(
            self._git('show', f'{head}:teammate'), '[cr158] A teammate fix'
        )

    def test_nothing_to_push(self):
        self.assertEqual(self._push(), 0)
        self.assertIn('committed no fixes: nothing to publish', self._report())

    def test_without_fixes_nothing_landed_is_picked(self):
        start = self._remote_tip()
        landed = self._commit('teammate', '[cr158] A teammate fix')
        self._git('push', '-q', 'origin', 'cr158')
        self._git('reset', '-q', '--hard', start)
        self.assertEqual(self._push(), 0)
        self.assertEqual(self._git('rev-parse', 'HEAD'), start)
        self.assertEqual(self._remote_tip(), landed)

    def test_picked_tooling_alone_is_not_published(self):
        tip = self._remote_tip()
        synced = self._commit('tooling', '[cr159] Teach brockit something')
        (self.run_root / 'cr158' / 'synced-head').write_text(
            f'{synced}\n', encoding='utf-8'
        )
        self.assertEqual(self._push(), 0)
        self.assertEqual(self._remote_tip(), tip)

    def test_pushes_the_tooling_sync_tooling_picked(self):
        self._commit('tooling', '[cr159] Teach brockit something')
        synced = self._commit('ci', '[cr159] Read CI failures')
        (self.run_root / 'cr158' / 'synced-head').write_text(
            f'{synced}\n', encoding='utf-8'
        )
        head = self._commit('fix', 'fixup! [cr158] Fix the build')
        self.assertEqual(self._push(), 0)
        self.assertEqual(self._remote_tip(), head)

    def test_tooling_not_picked_this_run_is_refused(self):
        self._commit('tooling', '[cr159] Teach brockit something')
        # A marker left by another run, not on this branch.
        (self.run_root / 'cr158' / 'synced-head').write_text(
            f'{"e" * 40}\n', encoding='utf-8'
        )
        self.assertEqual(self._push(), 1)


class AutofixTest(unittest.TestCase):
    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.run_root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        stack.enter_context(
            mock.patch.object(autolift, '_RUN_ROOT', self.run_root)
        )

    def _autofix(
        self,
        unpushed: int = 0,
        checkout: str = '159.0.8138.1',
        synced: bool = False,
    ):
        calls = []

        def run_git(*cmd, **_):
            calls.append(cmd)
            if cmd[0] == 'rev-list':
                return str(unpushed)
            if cmd == ('rev-parse', 'HEAD'):
                return 'a' * 40
            return ''

        brave = mock.MagicMock()
        brave.is_rebase_in_progress.return_value = False
        brave.run_git.side_effect = run_git
        versions = {
            'origin/master': autolift.Version('157.0.8103.30'),
            'HEAD': autolift.Version('159.0.8138.1'),
        }
        with (
            mock.patch.object(autolift.repository, 'brave', brave),
            mock.patch.object(
                autolift.Version, 'from_git', side_effect=versions.get
            ),
            mock.patch.object(
                autolift,
                'read_chromium_version_file',
                return_value=autolift.Version(checkout),
            ),
            mock.patch.object(
                autolift, 'sync_chromium', return_value=0
            ) as sync,
            mock.patch.object(autolift, '_synced_tooling', return_value=synced),
            contextlib.redirect_stdout(io.StringIO()),
        ):
            result = autolift._cmd_autofix(mock.Mock(target=159, dry_run=False))
        return result, calls, sync

    def test_resets_to_github_without_a_rebase(self):
        result, calls, sync = self._autofix()
        self.assertEqual(result, 0)
        self.assertIn(('checkout', '-B', 'cr159', 'origin/cr159'), calls)
        self.assertIn(('branch', '--set-upstream-to=origin/cr158'), calls)
        sync.assert_not_called()
        target = self.run_root / 'cr159' / 'lift-target'
        self.assertEqual(target.read_text(encoding='utf-8'), 'none\n')

    def test_syncs_a_checkout_on_another_version(self):
        _, _, sync = self._autofix(checkout='159.0.8136.1')
        sync.assert_called_once_with(autolift.Run.cr(159))

    def test_refuses_to_drop_unpushed_commits(self):
        result, calls, _ = self._autofix(unpushed=2)
        self.assertEqual(result, 1)
        self.assertFalse(any(c[0] == 'checkout' for c in calls))

    def test_keeps_the_tooling_sync_tooling_picked(self):
        result, calls, _ = self._autofix(unpushed=2, synced=True)
        self.assertEqual(result, 0)
        self.assertIn(('checkout', '-B', 'cr159', 'a' * 40), calls)


class TestLeadsTest(unittest.TestCase):
    _TRACKER = '\n'.join(
        [
            '# M159 code health follow-ups',
            '## 1. Regressions',
            '### `ProfileReportGeneratorTest` filter now misses the '
            'failing test',
            '',
            'Upstream renamed `SignedInProfile` to `SyncingProfile`.',
            '',
            '- [ ] `test/filters/unit_tests.filter:933`: retarget to',
            '      `-ProfileReportGeneratorTest.SignedInProfile`',
            '### Test filters on renamed or deleted suites',
            '- [ ] `unit_tests.filter:163`: `ActorTaskListBubble*` → '
            '`GlicActivityListBubble*`',
            '- [ ] `browser_tests.filter:94`: `OtherSuite` was renamed',
            '### Unrelated',
            '- [ ] Something else',
        ]
    )
    _FILTERS = {
        'test/filters/unit_tests.filter': '\n'.join(
            [
                '# Why',
                '-ProfileReportGeneratorTest.SignedInProfileReplaceSync',
                '-GlicApiTest*.*',
            ]
        ),
        'test/filters/browser_tests.filter': '-Unrelated.Test\n',
    }

    def _leads(self, *names):
        return autolift.test_leads(list(names), self._TRACKER, self._FILTERS)

    def _lead(self, name):
        leads = self._leads(name)
        self.assertEqual(len(leads), 1)
        return leads[0]

    def test_tracker_item_and_stale_filter_of_a_renamed_test(self):
        lead = self._lead(
            'unit_tests: ProfileReportGeneratorTest.SignedInProfile'
        )
        self.assertEqual(lead.binary, 'unit_tests')
        self.assertEqual(lead.suite, 'ProfileReportGeneratorTest')
        self.assertEqual(
            lead.tracker,
            (
                '3: ### `ProfileReportGeneratorTest` filter now misses the '
                'failing test',
                '7: - [ ] `test/filters/unit_tests.filter:933`: retarget to',
            ),
        )
        self.assertEqual(
            lead.filters,
            (
                'test/filters/unit_tests.filter:2: '
                '-ProfileReportGeneratorTest.SignedInProfileReplaceSync',
            ),
        )

    def test_task_naming_the_suite_under_another_heading(self):
        lead = self._lead('All/ActorTaskListBubbleTest.Shows/0')
        self.assertEqual(
            lead.tracker,
            (
                '9: ### Test filters on renamed or deleted suites',
                '10: - [ ] `unit_tests.filter:163`: `ActorTaskListBubble*` → '
                '`GlicActivityListBubble*`',
            ),
        )

    def test_wildcard_filter_matching_the_test(self):
        lead = self._lead('GlicApiTestWithFre.Opens')
        self.assertEqual(
            lead.filters, ('test/filters/unit_tests.filter:3: -GlicApiTest*.*',)
        )
        self.assertEqual(lead.tracker, ())

    def test_unnamed_test_has_no_leads(self):
        lead = self._lead('brave_unit_tests: NothingHere.Test')
        self.assertFalse(lead.any)

    def test_repeats_and_unparsable_names_are_dropped(self):
        leads = self._leads('Suite.Test', 'Suite.Test', 'not a gtest name')
        self.assertEqual([lead.name for lead in leads], ['Suite.Test'])

    def test_filter_on_a_base_class(self):
        filters = {
            'test/filters/browser_tests.filter': '-FirstPartySets/*\n'
            '-BgFetchBrowserTest.*\n'
        }
        leads = autolift.test_leads(
            ['BgFetchCorsBrowserTest.Fetches'],
            '',
            filters,
            ancestors={
                'BgFetchCorsBrowserTest': ('BgFetchBrowserTest', 'Base')
            },
        )
        self.assertEqual(len(leads), 1)
        lead = leads[0]
        self.assertEqual(lead.filters, ())
        self.assertEqual(
            lead.inherited,
            (
                'test/filters/browser_tests.filter:2: -BgFetchBrowserTest.* '
                '(base `BgFetchBrowserTest`)',
            ),
        )
        self.assertTrue(lead.any)

    def test_filter_on_a_fixture_renamed_into_the_suite(self):
        filters = {
            'test/filters/unit_tests.filter': '-All/WithLowEntropy*Source.*\n'
        }
        leads = autolift.test_leads(
            ['All/WithLimitedEntropySource.Works/0'],
            '',
            filters,
            renames={
                'WithLimitedEntropySource': (
                    ('WithLowEntropyRandomSource', 'c' * 40),
                )
            },
        )
        self.assertEqual(len(leads), 1)
        self.assertEqual(
            leads[0].renamed,
            (
                'test/filters/unit_tests.filter:1: '
                '-All/WithLowEntropy*Source.* '
                f'(`WithLowEntropyRandomSource` in {"c" * 12})',
            ),
        )


class FixtureBasesTest(unittest.TestCase):
    def test_bases_across_lines(self):
        source = '\n'.join(
            [
                'class FooBrowserTest;',
                'class FooBrowserTest',
                '    : public ns::BarBrowserTest<std::map<int, int>>,',
                '      public ::testing::WithParamInterface<bool> {',
                ' public:',
                '};',
            ]
        )
        self.assertEqual(
            autolift.fixture_bases('FooBrowserTest', [source]),
            ('BarBrowserTest', 'WithParamInterface'),
        )

    def test_final_and_unrelated_classes(self):
        source = (
            'class FooTestX : public Other {};\n'
            'class FooTest final : public Base {};\n'
        )
        self.assertEqual(autolift.fixture_bases('FooTest', [source]), ('Base',))

    def test_ancestors_follow_bases_of_bases(self):
        sources = {
            'Sub': 'class Sub : public Mid {};',
            'Mid': 'class Mid : public Top {};',
            'Top': 'class Top : public testing::Test {};',
        }

        def defining(ref, fixtures):
            self.assertEqual(ref, '159.0.8140.1')
            return [sources[f] for f in fixtures if f in sources]

        with mock.patch.object(autolift, '_defining_sources', defining):
            self.assertEqual(
                autolift._fixture_ancestors('159.0.8140.1', ['Sub']),
                {'Sub': ('Mid', 'Top', 'Test')},
            )


class FixtureRenamesTest(unittest.TestCase):
    def test_names_removed_by_the_commit_adding_the_suite(self):
        diff = '\n'.join(
            [
                '--- a/foo_unittest.cc',
                '+++ b/foo_unittest.cc',
                '-class WithLowEntropyRandomSource : public KeptTest {',
                '+class WithLimitedEntropySource : public KeptTest {',
                '-TEST_F(WithLowEntropyRandomSource, Works) {',
                '+TEST_F(WithLimitedEntropySource, Works) {',
                '-INSTANTIATE_TEST_SUITE_P(, OtherSource, testing::Bool());',
                '+INSTANTIATE_TEST_SUITE_P(All, Untouched2, testing::Bool());',
            ]
        )
        self.assertEqual(
            autolift.fixture_renames(
                ['WithLimitedEntropySource', 'Untouched'], {'c' * 40: diff}
            ),
            {
                'WithLimitedEntropySource': (
                    ('OtherSource', 'c' * 40),
                    ('WithLowEntropyRandomSource', 'c' * 40),
                )
            },
        )


class SquashesIntoBranchTest(unittest.TestCase):
    def test_subjects(self):
        self.assertTrue(
            autolift.squashes_into_branch('fixup! [cr159] Fix the build')
        )
        self.assertTrue(
            autolift.squashes_into_branch('amend! [cr159] Fix the build')
        )
        self.assertTrue(autolift.squashes_into_branch('[cr159] IWYU fixes.'))
        self.assertTrue(
            autolift.squashes_into_branch(
                '[cr159] Disable failing upstream tests.'
            )
        )
        self.assertFalse(autolift.squashes_into_branch('[cr159] Fix the build'))


class CiFailuresTest(unittest.TestCase):
    _HEAD = 'a' * 40

    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.run_root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        stack.enter_context(
            mock.patch.object(autolift, '_RUN_ROOT', self.run_root)
        )

    def _run(self, states, found, head=_HEAD) -> tuple[int, str]:
        checks = autolift.ci_github.PullRequestChecks(
            number=41074,
            branch='cr159',
            head=self._HEAD,
            checks=tuple(
                autolift.ci_github.Check(f'check {i}', state, 'u')
                for i, state in enumerate(states)
            ),
        )
        brave = mock.MagicMock(root=self.run_root)
        brave.run_git.return_value = head
        stdout = io.StringIO()
        with (
            mock.patch.object(autolift.repository, 'brave', brave),
            mock.patch.object(
                autolift.ci_github, 'pr_checks', return_value=checks
            ) as pr_checks,
            mock.patch.object(
                autolift.ci_report, 'collect', return_value=found
            ),
            contextlib.redirect_stdout(stdout),
        ):
            result = autolift._cmd_ci_failures(mock.Mock(target=159))
        pr_checks.assert_called_once_with('cr159')
        return result, stdout.getvalue()

    def test_failed_checks(self):
        State = autolift.ci_github.State
        build = autolift.ci_failures.FailedBuild(
            'Android_Build', 'b', 'Exit code 1', autolift.ci_failures.Kind.INFRA
        )
        found = [
            autolift.ci_failures.CheckFailures(
                'check 0', 'u', 'teamcity', (build,)
            )
        ]
        result, out = self._run([State.FAILURE, State.PENDING], found)
        self.assertEqual(result, autolift.CI_FAILED)
        self.assertIn('1 failed, 1 pending of 2 checks', out)
        self.assertIn('- check 0: Android_Build: infra', out)
        self.assertIn('- Pending: check 1', out)
        details = (self.run_root / 'cr159' / 'ci-failures.md').read_text(
            encoding='utf-8'
        )
        self.assertIn('### Android_Build: infra', details)

    def test_pending_only(self):
        State = autolift.ci_github.State
        result, _ = self._run([State.SUCCESS, State.PENDING], [])
        self.assertEqual(result, autolift.CI_PENDING)

    def test_green_and_stale(self):
        State = autolift.ci_github.State
        result, out = self._run([State.SUCCESS], [], head='b' * 40)
        self.assertEqual(result, 0)
        self.assertIn(f'not HEAD ({"b" * 12})', out)


_START = '1' * 40
_LANDED = '2' * 40
_HEAD = '3' * 40
_BASE = '4' * 40
_BASE_MOVED = '5' * 40


class PublishTest(unittest.TestCase):
    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.run_root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
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
        presubmit_fails: bool = False,
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
                    return mock.Mock(returncode=1, stdout='', stderr='')
                return mock.Mock(
                    returncode=2 if pending else 0, stdout='', stderr=''
                )
            if 'rebase' in cmd:
                state.update(head='rebased', forked=state['base'], squash=False)
            if 'regen' in cmd:
                state['head'] = 'regenerated'
            if 'presubmit' in cmd:
                return mock.Mock(returncode=1 if presubmit_fails else 0)
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
            result = autolift._cmd_publish(mock.Mock(target=158, built=built))
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
        result, run = self._publish(remote=_START, head=_START)
        self.assertEqual(result, 0)
        self.assertEqual(self._pushes(), [])
        self.assertEqual(self._presubmits(run), [])

    def _presubmits(self, run) -> list[list[str]]:
        return [
            c.args[0] for c in run.call_args_list if 'presubmit' in c.args[0]
        ]

    def test_presubmit_runs_against_the_upstream_before_the_push(self):
        result, run = self._publish(remote=_START)
        self.assertEqual(result, 0)
        self.assertEqual(
            self._presubmits(run),
            [['pnpm', 'run', 'presubmit', '--base', 'origin/master']],
        )
        # Not again for the same commit.
        _, run = self._publish(remote=_START, remote_at_push=_LANDED)
        self.assertEqual(self._presubmits(run), [])

    def test_presubmit_errors_hold_the_push_back(self):
        result, _ = self._publish(remote=_START, presubmit_fails=True)
        self.assertEqual(result, 1)
        self.assertEqual(self._pushes(), [])
        self.assertFalse(
            (self.run_root / 'cr158' / 'presubmit-passed').exists()
        )

    def test_presubmit_errors_hold_a_new_branch_back(self):
        result, run = self._publish(
            remote=None, start=None, presubmit_fails=True
        )
        self.assertEqual(result, 1)
        self.assertEqual(self._pushes(), [])
        self.assertFalse(
            any('update-version-issue' in c.args[0] for c in run.call_args_list)
        )

    def test_picks_what_landed_meanwhile_then_pushes_over_it(self):
        result, _ = self._publish(remote=_LANDED)
        self.assertEqual(result, 2)
        self.assertIn(('fetch', 'origin', 'cr158'), self.calls)
        self.assertIn(
            (
                'cherry-pick',
                '--allow-empty',
                '--empty=drop',
                f'{_START}..{_LANDED}',
            ),
            self.calls,
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

    def _brockit_runs(self, run) -> list[list[str]]:
        return [
            c.args[0]
            for c in run.call_args_list
            if 'presubmit' not in c.args[0]
        ]

    def _brockit_calls(self, run) -> list[list[str]]:
        return [cmd[2:] for cmd in self._brockit_runs(run)]

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
            autolift.Publish.record_start(autolift.Run.cr(158), _START)
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
        brockit = self._brockit_runs(run)
        self.assertEqual(len(brockit), 1)
        self.assertIn('--dry-run', brockit[0])

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
        self.assertTrue(all('--dry-run' in c for c in self._brockit_runs(run)))
        self.assertEqual(len(self._pushes()), 1)

    def test_fetches_right_before_pushing(self):
        result, _ = self._publish(remote=_START)
        self.assertEqual(result, 0)
        push = self.calls.index(self._pushes()[0])
        self.assertEqual(self.calls[push - 1], ('fetch', 'origin', 'cr158'))
        self.assertFalse(any(c[0] == 'cache' for c in self.calls))

    def test_refreshes_the_git_cache_before_looking_and_pushing(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        cache = stack.enter_context(tempfile.TemporaryDirectory())
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
        # Nor on a rerun, which mustn't take the rewrite as the lease.
        result, _ = self._publish(remote=_LANDED, rewritten=True, built=True)
        self.assertEqual(result, 1)
        self.assertEqual(self._pushes(), [])

    def test_a_second_publish_pushes_over_the_first(self):
        result, _ = self._publish(remote=_START)
        self.assertEqual(result, 0)
        self.calls.clear()
        result, _ = self._publish(remote=_HEAD, head='newer')
        self.assertEqual(result, 0)
        self.assertFalse(any(c[0] == 'cherry-pick' for c in self.calls))
        self.assertEqual(
            self._pushes()[0][1], f'--force-with-lease=refs/heads/cr158:{_HEAD}'
        )

    def test_a_second_publish_of_a_new_branch_pushes_over_the_first(self):
        result, _ = self._publish(remote=None, start=None)
        self.assertEqual(result, 0)
        self.calls.clear()
        result, _ = self._publish(remote=_HEAD, head='newer', start=_BASE)
        self.assertEqual(result, 0)
        self.assertFalse(any(c[0] == 'cherry-pick' for c in self.calls))
        self.assertEqual(
            self._pushes()[0][1], f'--force-with-lease=refs/heads/cr158:{_HEAD}'
        )

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


_CRASH_LOG = """\
[18:57:49] * ✔️  6df16e5f58b [cr158] Updated strings.          terminal.py:358
Traceback (most recent call last):
  File "brockit.py", line 1932, in run
    GitHubIssue(...).create_or_update_version_issue(with_pr=False)
KeyError: 'number'

During handling of the above exception, another exception occurred:

Traceback (most recent call last):
  File "brockit.py", line 3620, in main
    upgrade.run(with_github=with_github, ack_advisory=args.ack_advisory)
subprocess.CalledProcessError: Command '['gh', 'issue', 'list']' returned \
non-zero exit status 1.
"""


class SummarizeLiftTest(unittest.TestCase):
    def test_a_request_to_continue(self):
        self.assertTrue(autolift.summarize_lift(_LIFT_LOG).resumable)
        self.assertFalse(autolift.summarize_lift(_CONTINUE_LOG).resumable)
        self.assertFalse(
            autolift.summarize_lift(
                '👋 (Address advisories and then rerun with --ack-advisory)\n'
            ).resumable
        )

    def test_a_crash_is_its_last_exception(self):
        summary = autolift.summarize_lift(_CRASH_LOG)
        self.assertEqual(
            summary.crash,
            "subprocess.CalledProcessError: Command '['gh', 'issue', 'list']' "
            'returned non-zero exit status 1.',
        )
        self.assertFalse(summary.resumable)
        self.assertIsNone(autolift.summarize_lift(_LIFT_LOG).crash)

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


_REPORT = """\
## prepare 159 (2026-10-29 16:12)

- old run

## Step 1 done: Reset and rebase (2026-10-29 16:30)

## Step 2 done: Lift (2026-10-29 18:00)

## prepare 159 (2026-11-01 12:33)

- this run

## Step 1 done: Reset and rebase (2026-11-01 12:50)

rebased

## Step 2 skipped: Lift (2026-11-01 12:51)
"""


def _status(**overrides) -> autolift.RunStatus:
    fields = {
        'milestone': 159,
        'branch': 'cr159',
        'head': 'abc [cr159] Subject',
        'origin': (0, 3),
        'in_progress': [],
        'uncommitted': 0,
        'builds': [],
        'prepared': True,
        'not_worth_it': None,
        'lift_target': '@latest-for-branch',
        'steps': {},
        'logs': [],
    }
    fields.update(overrides)
    return autolift.RunStatus(**fields)


class RunStepsTest(unittest.TestCase):
    def test_reads_only_the_current_run(self):
        self.assertEqual({1: 'done', 2: 'skipped'}, autolift.run_steps(_REPORT))

    def test_next_step_with_and_without_a_lift(self):
        steps = {1: 'done', 2: 'done', 3: 'done'}
        self.assertEqual(4, autolift.next_step(steps, lift=True))
        self.assertEqual(4, autolift.next_step(steps, lift=False))
        steps.update({4: 'done'})
        self.assertEqual(5, autolift.next_step(steps, lift=True))
        self.assertEqual(10, autolift.next_step(steps, lift=False))
        every = {n: 'done' for n in autolift.STEPS}
        self.assertIsNone(autolift.next_step(every, lift=True))


class NotWorthARunTest(unittest.TestCase):
    def test_reasons(self):
        recent, old = timedelta(hours=2), timedelta(hours=13)
        self.assertIsNone(autolift.not_worth_a_run(True, 0, recent))
        self.assertEqual(
            'no lift, and nothing to rebase',
            autolift.not_worth_a_run(False, 0, old),
        )
        self.assertEqual(
            'no lift, and only 20 base commit(s) to rebase over, 2h after the '
            'last rebase',
            autolift.not_worth_a_run(False, 20, recent),
        )
        self.assertIsNotNone(
            autolift.not_worth_a_run(False, 20, timedelta(hours=12))
        )
        self.assertIsNone(autolift.not_worth_a_run(False, 3, old))
        self.assertIsNone(autolift.not_worth_a_run(False, 21, recent))
        self.assertIsNone(autolift.not_worth_a_run(False, 1, recent, ['DEPS']))

    def test_merge_conflicts(self):
        brave = mock.MagicMock()
        with mock.patch.object(autolift.repository, 'brave', brave):
            brave.run_git.return_value = 'tree'
            self.assertEqual(autolift.merge_conflicts('origin/cr157'), [])
            brave.run_git.side_effect = subprocess.CalledProcessError(
                1, 'git', output='tree\nDEPS\npatches/a.patch\n'
            )
            self.assertEqual(
                autolift.merge_conflicts('origin/cr157'),
                ['DEPS', 'patches/a.patch'],
            )
            brave.run_git.side_effect = subprocess.CalledProcessError(
                128, 'git', output=''
            )
            with self.assertRaises(subprocess.CalledProcessError):
                autolift.merge_conflicts('origin/cr157')

    def test_status_of_a_run_not_worth_a_lift(self):
        report = (
            '## prepare 158 (2026-11-01 12:00)\n\n'
            '- Not worth a lift run: no lift, and nothing to rebase. No '
            'rebase: carry on with an autofix run of the CI; `--always` runs '
            'the lift anyway.\n'
        )
        reason = autolift.run_not_worth_it(report)
        self.assertEqual('no lift, and nothing to rebase', reason)
        text = autolift.format_status(_status(not_worth_it=reason))
        self.assertTrue(
            text.endswith(
                'Next: the "Autofix run" section, carrying on from where the '
                'report leaves it; `prepare` found nothing worth a lift run '
                '(no lift, and nothing to rebase).'
            )
        )
        self.assertIsNone(autolift.run_not_worth_it(_REPORT))


class FormatStatusTest(unittest.TestCase):
    def test_names_the_next_step(self):
        text = autolift.format_status(_status(steps={1: 'done'}))
        self.assertIn('- 3 commit(s) ahead of origin/cr159, 0 behind.', text)
        self.assertIn('- Step 1 done: Reset and rebase', text)
        self.assertTrue(text.endswith('Next: step 2, Lift.'))

    def test_in_progress_work_comes_first(self):
        text = autolift.format_status(
            _status(in_progress=['a rebase'], uncommitted=2)
        )
        self.assertIn('- In progress: a rebase', text)
        self.assertIn('- 2 uncommitted change(s).', text)
        self.assertTrue(
            text.endswith(
                'Next: finish or redo what is in progress, then step 1.'
            )
        )

    def test_before_prepare(self):
        text = autolift.format_status(
            _status(prepared=False, lift_target=None, origin=None)
        )
        self.assertIn("- origin/cr159 doesn't exist yet.", text)
        self.assertTrue(text.endswith("start at step 1."))

    def test_maintenance_run_without_a_lift(self):
        text = autolift.format_status(
            _status(
                lift_target='none',
                steps={1: 'done', 2: 'skipped', 3: 'done', 4: 'done'},
            )
        )
        self.assertIn('- No lift needed', text)
        self.assertTrue(text.endswith('Next: step 10, Wrap up.'))


class StepCommandTest(unittest.TestCase):
    def test_records_the_step_heading(self):
        with tempfile.TemporaryDirectory() as tmp:
            brave = mock.MagicMock()
            brave.current_branch.return_value = 'cr159'
            with (
                mock.patch.object(autolift.repository, 'brave', brave),
                mock.patch.object(autolift, '_RUN_ROOT', Path(tmp)),
                contextlib.redirect_stdout(io.StringIO()),
            ):
                self.assertEqual(
                    0,
                    autolift._cmd_step(
                        mock.Mock(
                            number=3, done='builds; 4 fixes', skipped=None
                        )
                    ),
                )
                self.assertEqual(
                    1,
                    autolift._cmd_step(
                        mock.Mock(number=12, done='x', skipped=None)
                    ),
                )
            report = (Path(tmp) / 'cr159' / 'report.md').read_text(
                encoding='utf-8'
            )
        self.assertRegex(report, r'^## Step 3 done: Linux build \(')
        self.assertIn('builds; 4 fixes', report)
        self.assertEqual({3: 'done'}, autolift.run_steps(report))


_LISTING = '- Fixes on newer branches: the listing.'


class LiftCommandTest(unittest.TestCase):
    def setUp(self):
        self.listed = []

    def _lift(
        self,
        log_text: str,
        returncode: int = 0,
        lift_to: str | None = '@latest-for-branch',
        continue_: bool = True,
        directory: Path | None = None,
        package_range: str | None = None,
        asked_to_continue: bool | None = None,
    ):
        """Runs `lift`, with Brockit printing `log_text`. A `--continue` run
        resumes a lift that asked to continue, unless `asked_to_continue`
        says otherwise; None keeps what the directory's runs recorded."""
        if directory is None:
            stack = contextlib.ExitStack()
            self.addCleanup(stack.close)
            directory = Path(stack.enter_context(tempfile.TemporaryDirectory()))
            if asked_to_continue is None:
                asked_to_continue = continue_
        (directory / 'cr158').mkdir(exist_ok=True)
        if lift_to is not None:
            (directory / 'cr158' / 'lift-target').write_text(f'{lift_to}\n')
        if asked_to_continue:
            (directory / 'cr158' / 'lift-stopped').touch()

        def run(_cmd, stdout, **_):
            stdout.write(log_text.encode('utf-8'))
            return mock.Mock(returncode=returncode)

        self.listed = []

        def listing(_sources, revision_range):
            self.listed.append(revision_range)
            return _LISTING

        brave = mock.MagicMock()
        brave.current_branch.return_value = 'cr158'
        with (
            mock.patch.object(autolift.repository, 'brave', brave),
            mock.patch.object(autolift, '_RUN_ROOT', directory),
            mock.patch.object(
                autolift.subprocess, 'run', side_effect=run
            ) as run_mock,
            mock.patch.object(
                autolift, '_package_range', return_value=package_range
            ),
            mock.patch.object(
                autolift.FixSources,
                'listing',
                autospec=True,
                side_effect=listing,
            ),
        ):
            result = autolift._cmd_lift(mock.Mock(continue_=continue_))
        return result, run_mock, directory / 'cr158' / 'report.md'

    def _last_section(self, report: Path) -> str:
        return report.read_text(encoding='utf-8').split('## ')[-1]

    def test_continue_reports_the_first_lift_ranges(self):
        _, _, report = self._lift(_LIFT_LOG, continue_=False)
        first = autolift.summarize_lift(_LIFT_LOG)
        self._lift(_CONTINUE_LOG, directory=report.parent.parent)
        section = self._last_section(report)
        self.assertIn(f'- Bump range: {first.bump_range}', section)
        self.assertIn(f'- Branch range: {first.branch_range}', section)

    def test_continue_reports_a_new_major_first_bump(self):
        _, _, report = self._lift(
            '💥 Done!\n',
            continue_=False,
            package_range='158.0.1.0..159.0.2.0',
        )
        self._lift(
            _CONTINUE_LOG,
            directory=report.parent.parent,
            package_range='158.0.1.0..159.0.3.0',
        )
        self.assertIn(
            '- Bump range: 158.0.1.0..159.0.2.0', self._last_section(report)
        )

    def test_continue_without_a_record_skips_the_package_range(self):
        _, _, report = self._lift(
            _CONTINUE_LOG, package_range='158.0.1.0..159.0.3.0'
        )
        self.assertIn('- Bump range: not printed', self._last_section(report))

    def test_exits_non_zero_when_brockit_exits_zero_with_work_left(self):
        result, run_mock, report = self._lift(_CONTINUE_LOG)
        self.assertEqual(result, 2)
        cmd = run_mock.call_args.args[0]
        self.assertEqual(cmd[2:], ['lift', '--infra-mode', '--continue'])
        text = report.read_bytes().decode('utf-8')
        self.assertIn('Plaster check. Please investigate it.', text)

    def test_continue_without_a_request_to_continue_runs_nothing(self):
        result, run_mock, _ = self._lift(_CONTINUE_LOG, asked_to_continue=False)
        self.assertEqual(result, 1)
        run_mock.assert_not_called()

    def test_a_crash_is_reported_with_nothing_to_resume(self):
        _, _, report = self._lift(_LIFT_LOG, continue_=False)
        directory = report.parent.parent
        result, _, _ = self._lift(_CRASH_LOG, returncode=1, directory=directory)
        self.assertEqual(result, 1)
        section = self._last_section(report)
        self.assertIn(
            '- Brockit crashed: `subprocess.CalledProcessError: Command '
            "'['gh', 'issue', 'list']' returned non-zero exit status 1.`",
            section,
        )
        self.assertIn('has nothing to resume', section)
        result, run_mock, _ = self._lift(_CONTINUE_LOG, directory=directory)
        self.assertEqual(result, 1)
        run_mock.assert_not_called()

    def test_clean_run_exits_zero(self):
        result, _, report = self._lift('💥 Done!\n')
        self.assertEqual(result, 0)
        self.assertIn(
            'Finished with nothing left to do.',
            report.read_bytes().decode('utf-8'),
        )

    def test_lifts_to_the_recorded_release(self):
        _, run_mock, _ = self._lift(
            '💥 Done!\n', lift_to='158.0.8127.4', continue_=False
        )
        self.assertEqual(
            run_mock.call_args.args[0][2:],
            ['lift', '--infra-mode', '--with-github', '--to=158.0.8127.4'],
        )

    # `FixListingTest` covers what the listing says, on real repositories.
    def test_a_minor_bump_reports_the_newer_branches_fixes(self):
        _, _, report = self._lift(_LIFT_LOG, continue_=False)
        self.assertIn(_LISTING, self._last_section(report))
        self.assertEqual(
            self.listed, [autolift.summarize_lift(_LIFT_LOG).bump_range]
        )

    def test_a_major_bump_reports_none(self):
        _, _, report = self._lift(
            '💥 Done!\n',
            continue_=False,
            package_range='158.0.1.0..159.0.2.0',
        )
        self.assertNotIn(_LISTING, self._last_section(report))
        self.assertEqual(self.listed, [])

    def test_a_dry_run_writes_no_issue(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        directory = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        (directory / 'cr158').mkdir()
        (directory / 'cr158' / 'dry-run').touch()
        _, run_mock, _ = self._lift(
            '💥 Done!\n',
            lift_to='158.0.8127.4',
            continue_=False,
            directory=directory,
        )
        self.assertEqual(
            run_mock.call_args.args[0][2:],
            ['lift', '--infra-mode', '--to=158.0.8127.4'],
        )

    def test_refuses_when_no_lift_is_needed(self):
        result, run_mock, _ = self._lift('', lift_to='none')
        self.assertEqual(result, 1)
        run_mock.assert_not_called()

    def test_refuses_before_prepare(self):
        result, run_mock, _ = self._lift('', lift_to=None)
        self.assertEqual(result, 1)
        run_mock.assert_not_called()

    def _in_progress(self, directory: Path) -> list[str]:
        brave = mock.MagicMock()
        brave.is_rebase_in_progress.return_value = False
        brave.run_git.return_value = str(directory)
        brave.root = directory
        with (
            mock.patch.object(autolift.repository, 'brave', brave),
            mock.patch.object(autolift, '_RUN_ROOT', directory),
        ):
            return autolift._in_progress(autolift.Run.cr(158))

    def test_a_stopped_lift_is_in_progress_until_it_finishes(self):
        _, _, report = self._lift(_LIFT_LOG, continue_=False)
        directory = report.parent.parent
        self.assertEqual(
            self._in_progress(directory),
            ['a `brockit lift` (`autolift.py lift --continue`)'],
        )
        self._lift(_CONTINUE_LOG, directory=directory)
        self.assertEqual(self._in_progress(directory), [])

    def test_a_continuation_file_is_not_a_lift_in_progress(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        directory = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        (directory / '.version_upgrade').write_bytes(b'left over')
        self.assertEqual(self._in_progress(directory), [])


_TAG = autolift.Version('157.0.8103.38')
_MASTER = 'cr157.x'
_BETA = 'cr157.x-on-1.100.x'


def _branch(channel: str) -> str:
    return _MASTER if channel == 'master' else f'cr157.x-on-{channel}'


class RunTest(unittest.TestCase):
    def test_cr_and_channel_bump_branches(self):
        self.assertEqual(autolift.Run.of_branch('cr158'), autolift.Run.cr(158))
        run = autolift.Run.of_branch(_BETA)
        self.assertEqual(run, autolift.Run.bump(157, '1.100.x'))
        self.assertEqual((run.milestone, run.name), (157, 'cr157.x'))
        self.assertEqual(
            autolift.Run.of_branch(_MASTER), autolift.Run.bump(157, 'master')
        )
        self.assertEqual(autolift.Run.cr(158).argument, '158')
        self.assertIsNone(autolift.Run.of_branch('cr157.x-on-1.100.y'))
        self.assertIsNone(autolift.Run.of_branch('cr157.x-on-master'))
        self.assertIsNone(autolift.Run.of_branch('master'))

    def test_a_channel_bump_argument_is_its_recorded_tag(self):
        with tempfile.TemporaryDirectory() as tmp:
            with mock.patch.object(autolift, '_RUN_ROOT', Path(tmp)):
                run = autolift.Run.bump(157, '1.100.x')
                self.assertEqual(run.argument, '<tag>')
                autolift.ChannelBump(_TAG)._record([])
                self.assertEqual(run.argument, '157.0.8103.38')

    def test_target_is_a_milestone_or_a_tag(self):
        self.assertEqual(autolift.Run.target('159'), 159)
        self.assertEqual(autolift.Run.target('157.0.8103.38'), _TAG)
        with self.assertRaises(autolift.argparse.ArgumentTypeError):
            autolift.Run.target('cr159')

    def test_channels_share_the_report_and_keep_their_own_state(self):
        with tempfile.TemporaryDirectory() as tmp:
            with mock.patch.object(autolift, '_RUN_ROOT', Path(tmp)):
                master = autolift.Run.bump(157, 'master')
                beta = autolift.Run.bump(157, '1.100.x')
                self.assertEqual(master.directory, beta.directory)
                self.assertNotEqual(master.state, beta.state)
                self.assertEqual(
                    beta.log('build-linux').name, 'build-linux-1.100.x.log'
                )
                cr = autolift.Run.cr(158)
                self.assertEqual(cr.state, cr.directory)
                self.assertEqual(cr.log('build-linux').name, 'build-linux.log')


class FixListingTest(unittest.TestCase):
    """`FixSources.listing`, on real repositories: a Chromium release branch
    with a cherry-pick, and brave-core with a newer `cr158` that fixed it.

    The fix cites the change's main-branch commit, which the Chromium checkout
    doesn't have, so only the Change-Id its message quotes ties the two.
    """

    _RANGE = '157.0.8103.30..157.0.8103.39'

    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        self.chromium = root / 'src'
        self.remote = root / 'remote.git'
        self.dir = root / 'brave'
        for repo in (self.chromium, self.dir):
            repo.mkdir()
            self._git(repo, 'init', '-q', '-b', 'main')
            self._git(repo, 'config', 'user.email', 'a@b.c')
            self._git(repo, 'config', 'user.name', 'A')
        self._commit(self.chromium, 'device_features.cc', 'BASE', 'Base')
        self._git(self.chromium, 'tag', '157.0.8103.30')
        self.pick = self._commit(
            self.chromium,
            'device_features.cc',
            'DISABLED',
            _CHERRY_PICK_MESSAGE,
        )
        self._git(self.chromium, 'tag', '157.0.8103.39')

        subprocess.run(
            ['git', 'init', '-q', '--bare', str(self.remote)],
            check=True,
            env=_git_env(),
        )
        self._git(self.dir, 'remote', 'add', 'origin', str(self.remote))
        self._commit(self.dir, 'device.yaml', 'pin\nbackport\n', 'Base')
        self._git(self.dir, 'push', '-q', 'origin', 'main:master')
        # cr158 reverts the backport, citing the main-branch commit.
        self._git(self.dir, 'checkout', '-q', '-b', 'cr158')
        self.fix = self._commit(
            self.dir,
            'device.yaml',
            'pin\n',
            self._fix_message('[cr158] Revert the backport', _CHANGE_ID),
        )
        # A revert of another change quotes this one's Change-Id with `>`.
        self._commit(
            self.dir,
            'other.yaml',
            'other\n',
            self._fix_message(
                '[cr158] Follow another change',
                'I' + '2' * 40,
                quoted=_CHANGE_ID,
            ),
        )
        self._git(self.dir, 'push', '-q', 'origin', 'cr158')
        self._git(self.dir, 'fetch', '-q', 'origin')
        self._git(self.dir, 'checkout', '-q', '-b', 'cr157.x', 'origin/master')

        def repo(path: Path) -> mock.MagicMock:
            fake = mock.MagicMock(root=path)
            fake.run_git.side_effect = lambda *args, no_trim=False: (
                subprocess.run(
                    ['git', '-C', str(path), *args],
                    check=True,
                    env=_git_env(),
                    capture_output=True,
                    text=True,
                ).stdout
                if no_trim
                else self._git(path, *args)
            )
            return fake

        stack.enter_context(
            mock.patch.object(autolift.repository, 'brave', repo(self.dir))
        )
        stack.enter_context(
            mock.patch.object(
                autolift.repository, 'chromium', repo(self.chromium)
            )
        )
        stack.enter_context(
            mock.patch.object(autolift, '_RUN_ROOT', root / 'runs')
        )

    @staticmethod
    def _git(repo: Path, *args: str) -> str:
        return subprocess.run(
            ['git', '-C', str(repo), *args],
            check=True,
            env=_git_env(),
            capture_output=True,
            text=True,
        ).stdout.strip()

    def _commit(self, repo: Path, name: str, content: str, message: str):
        (repo / name).write_text(content, encoding='utf-8')
        self._git(repo, 'add', name)
        self._git(repo, 'commit', '-q', '-m', message)
        return self._git(repo, 'rev-parse', 'HEAD')

    @staticmethod
    def _fix_message(subject: str, change_id: str, quoted: str = '') -> str:
        """A message as `git cr commit --culprit` writes it, quoting the
        culprit's own."""
        culprit = 'e' * 40
        quote = f'    > Change-Id: {quoted}\n' if quoted else ''
        return (
            f'{subject}\n\nWhy.\n\nChromium changes:\n'
            f'https://chromium.googlesource.com/chromium/src/+/{culprit}\n\n'
            f'commit {culprit}\nAuthor: A <a@b.c>\n\n    {subject}\n\n'
            f'{quote}    Change-Id: {change_id}\n'
        )

    def _listing(self) -> str:
        return autolift.FixSources(autolift.Run.bump(157, 'master')).listing(
            self._RANGE
        )

    def test_a_fix_found_through_the_change_id_alone(self):
        listing = self._listing()
        self.assertIn('(1): pick the one for an item', listing)
        self.assertIn(
            f'origin/cr158 `{self.fix[:12]} [cr158] Revert the backport` for '
            f'`{self.pick[:12]} [M157] usb: Default-disable reset config '
            'invalidation`\n',
            listing + '\n',
        )
        self.assertIn('    - `device.yaml`', listing)
        # The revert merely quoting the Change-Id is not a fix for it.
        self.assertNotIn('Follow another change', listing)

    def test_a_fix_that_changes_nothing_here(self):
        # The branch never had the backport the fix reverts.
        self._commit(self.dir, 'device.yaml', 'pin\n', 'Drop the backport')
        self.assertIn('(changes nothing here)', self._listing())

    def test_a_fix_that_conflicts_here(self):
        self._commit(self.dir, 'device.yaml', 'pin\nother\n', 'Change it')
        self.assertIn('(conflicts here)', self._listing())

    def test_a_fix_already_picked(self):
        # Onto a branch of its own, as a pick is: a copy of the fix on the same
        # parent would be the fix itself.
        self._commit(self.dir, 'version', 'lifted\n', 'Update from Chromium')
        self._git(self.dir, 'cherry-pick', self.fix)
        self.assertIn('(already on cr157.x)', self._listing())

    def test_no_fix_for_the_range(self):
        self._git(self.chromium, 'tag', '157.0.8103.40', '157.0.8103.30')
        self.assertEqual(
            autolift.FixSources(autolift.Run.bump(157, 'master')).listing(
                '157.0.8103.30..157.0.8103.40'
            ),
            "- No newer branch has a fix for this bump's cherry-picks.",
        )


class StageToolingTest(unittest.TestCase):
    def test_copies_the_tooling_once_and_runs_from_the_copy(self):
        with tempfile.TemporaryDirectory() as tmp:
            with mock.patch.object(autolift, '_RUN_ROOT', Path(tmp)):
                copy = autolift.ChannelBump.stage_tooling()
                self.assertTrue((copy / 'autolift' / 'SKILL.md').exists())
                self.assertTrue((copy / 'brockit.py').exists())
                self.assertFalse(any(copy.rglob('__pycache__')))
                # Run from the copy, it leaves it alone.
                marker = copy / 'marker'
                marker.touch()
                with mock.patch.object(
                    autolift, '__file__', str(copy / 'autolift' / 'autolift.py')
                ):
                    autolift.ChannelBump.stage_tooling()
                self.assertTrue(marker.exists())


def _channel(branch: str, name: str = 'beta') -> autolift.Channel:
    return autolift.Channel(
        name,
        branch,
        autolift.Version('157.0.8103.30'),
        autolift.Run.bump(157, branch),
    )


class ChannelStepsTest(unittest.TestCase):
    _CHANNELS = [
        _channel('master', 'nightly'),
        _channel('1.100.x'),
        _channel('1.99.x', 'release'),
    ]

    def test_highest_channel_first_with_the_review_after_it(self):
        sequence = [
            (number, run.channel if run else None)
            for number, run in autolift.ChannelBump.sequence(self._CHANNELS)
        ]
        self.assertEqual(sequence[0], (1, None))
        # No Android build: the pull requests' CI has it.
        self.assertEqual(
            sequence[1:7],
            [(n, 'master') for n in (2, 3, 4, 5, 7)] + [(8, None)],
        )
        # The lower channels are only lifted, taking the highest's fixes.
        self.assertEqual(sequence[7:9], [(2, '1.100.x'), (2, '1.99.x')])
        self.assertEqual(sequence[9:], [(9, None), (10, None), (11, None)])

    def test_steps_are_recorded_per_branch(self):
        report = (
            '## prepare 157.0.8103.38 (2026-11-03 10:00)\n\n'
            '## Step 1 done: Reset and rebase (2026-11-03 10:01)\n\n'
            + ''.join(
                f'## Step {n} done: {autolift.STEPS[n]} on '
                f'{_MASTER} (2026-11-03 11:00)\n\n'
                for n in (2, 3, 4, 5, 7)
            )
            + '## Step 8 done: Code health review (2026-11-03 12:00)\n\n'
            f'## Step 2 done: Lift on {_BETA} (2026-11-03 13:00)\n\n'
        )
        self.assertEqual(autolift.run_steps(report, _BETA), {2: 'done'})
        self.assertEqual(autolift.run_steps(report), {1: 'done', 8: 'done'})
        number, run = autolift.ChannelBump.next_step(self._CHANNELS, report)
        self.assertEqual((number, run.branch), (2, 'cr157.x-on-1.99.x'))


class ChannelBumpTest(unittest.TestCase):
    """A channel bump, on a real repository with a bare one as `origin`."""

    # (channel, Brave version, Chromium tag)
    _CHANNELS = (
        ('master', '1.101.21', '157.0.8103.30'),
        ('1.100.x', '1.100.47', '157.0.8103.30'),
        ('1.99.x', '1.99.54', '156.0.8078.80'),
    )

    def setUp(self):
        self.fix = ''
        self.presubmitted = []
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        self.run_root = root / 'runs'
        self.remote = root / 'remote.git'
        self.dir = root / 'brave'
        subprocess.run(
            ['git', 'init', '-q', '--bare', str(self.remote)],
            check=True,
            env=_git_env(),
        )
        self.dir.mkdir()
        self._git('init', '-q', '-b', 'master')
        self._git('config', 'user.email', 'a@b.c')
        self._git('config', 'user.name', 'A')
        self._git('remote', 'add', 'origin', str(self.remote))
        self._commit('base', 'Base')
        for channel, version, tag in self._CHANNELS:
            self._git('checkout', '-q', '-B', channel, 'master')
            self._package(version, tag)
            self._git('push', '-q', 'origin', channel)
        self._git('checkout', '-q', 'master')
        self._git('fetch', '-q', 'origin')
        stack.enter_context(mock.patch.dict(os.environ, {'GIT_CACHE_PATH': ''}))
        stack.enter_context(
            mock.patch.object(autolift, '_RUN_ROOT', self.run_root)
        )
        stack.enter_context(
            mock.patch.object(
                autolift.ChannelBump,
                'stage_tooling',
                return_value=Path('.autolift/tooling/cr'),
            )
        )
        brave = mock.MagicMock(root=self.dir)
        brave.is_rebase_in_progress.return_value = False
        brave.current_branch.side_effect = lambda: self._git(
            'branch', '--show-current'
        )
        brave.run_git.side_effect = lambda *args, no_trim=False: (
            subprocess.run(
                ['git', '-C', str(self.dir), *args],
                check=True,
                env=_git_env(),
                capture_output=True,
                text=True,
            ).stdout
            if no_trim
            else self._git(*args)
        )
        brave.read_file.side_effect = lambda path, commit: self._git(
            'show', f'{commit}:{path}'
        )
        stack.enter_context(
            mock.patch.object(autolift.repository, 'brave', brave)
        )
        stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
        # The Brockit calls of the last `_publish`.
        self.brockit = mock.Mock()
        # The branches `pnpm run sync` synced the checkout to, which the fake
        # one has no `pnpm` for.
        self.synced: list[str] = []

        def sync(run) -> int:
            self.assertEqual(self._git('branch', '--show-current'), run.branch)
            self.synced.append(run.branch)
            return 0

        stack.enter_context(
            mock.patch.object(autolift, 'sync_chromium', side_effect=sync)
        )

    def _git(self, *args: str, cwd: Path | None = None) -> str:
        return subprocess.run(
            ['git', '-C', str(cwd or self.dir), *args],
            check=True,
            env=_git_env(),
            capture_output=True,
            text=True,
        ).stdout.strip()

    def _commit(self, name: str, subject: str) -> str:
        (self.dir / name).write_text(f'{subject}\n', encoding='utf-8')
        self._git('add', name)
        self._git('commit', '-q', '-m', subject)
        return self._git('rev-parse', 'HEAD')

    def _package(self, version: str, tag: str) -> None:
        package = {
            'version': version,
            'config': {'projects': {'chrome': {'tag': tag}}},
        }
        (self.dir / 'package.json').write_text(
            autolift.json.dumps(package), encoding='utf-8'
        )
        self._git('add', 'package.json')
        self._git('commit', '-q', '-m', f'Brave {version} on {tag}')

    def _prepare(self, dry_run: bool = False, force: bool = False) -> int:
        return autolift._cmd_prepare(
            mock.Mock(target=_TAG, force=force, dry_run=dry_run)
        )

    def _report(self, tag: autolift.Version = _TAG) -> str:
        return (self.run_root / f'cr{tag.major}.x' / 'report.md').read_text(
            encoding='utf-8'
        )

    def _push_bump(self, channel: str, tag: str = str(_TAG)) -> str:
        """Pushes a bump of `channel` to `tag`, as a run published it."""
        version = next(v for c, v, _ in self._CHANNELS if c == channel)
        self._git('checkout', '-q', '-B', 'published', f'origin/{channel}')
        self._package(version, tag)
        self._git('push', '-q', 'origin', f'published:{_branch(channel)}')
        tip = self._git('rev-parse', 'HEAD')
        self._git('checkout', '-q', 'master')
        self._git('branch', '-q', '-D', 'published')
        self._git('fetch', '-q', 'origin')
        return tip

    def test_prepare_branches_the_channels_on_the_milestone(self):
        self.assertEqual(self._prepare(), 0)
        self.assertEqual(self._git('branch', '--show-current'), _MASTER)
        for channel in ('master', '1.100.x'):
            branch = _branch(channel)
            self.assertEqual(
                self._git(
                    'rev-parse', '--abbrev-ref', f'{branch}@{{upstream}}'
                ),
                f'origin/{channel}',
            )
            self.assertEqual(
                self._git('rev-parse', branch),
                self._git('rev-parse', f'origin/{channel}'),
            )
            target = self.run_root / 'cr157.x' / channel / 'lift-target'
            self.assertEqual(target.read_text(encoding='utf-8'), f'{_TAG}\n')
        # Release is still on M156.
        self.assertEqual(self._git('branch', '--list', '*-1.99.x'), '')
        self.assertEqual(
            [c.branch for c in autolift.ChannelBump(_TAG).recorded()],
            ['master', '1.100.x'],
        )
        self.assertIn(
            'release (1.99.x) is on 156.0.8078.80: not', self._report()
        )
        # The channels' `.gitignore` lacks the run directory: excluded
        # locally, once.
        exclude = self.dir / '.git' / 'info' / 'exclude'
        self.assertIn('/.autolift/', exclude.read_text().splitlines())
        self._git('checkout', '-q', 'master')
        self.assertEqual(self._prepare(force=True), 0)
        self.assertEqual(
            exclude.read_text().splitlines().count('/.autolift/'), 1
        )

    def _advise(self, autofix: bool = False, states: dict | None = None):
        """`Advice.of` the tag, with each bump branch's pull request's checks
        in `states` (one passed check for those it leaves out)."""
        State = autolift.ci_github.State

        def checks(branch):
            return autolift.ci_github.PullRequestChecks(
                number=1,
                branch=branch,
                head='a' * 40,
                checks=(
                    autolift.ci_github.Check(
                        'check', (states or {}).get(branch, State.SUCCESS), 'u'
                    ),
                ),
            )

        heads = self._git('for-each-ref', '--format=%(refname) %(objectname)')
        with mock.patch.object(
            autolift.ci_github, 'pr_checks', side_effect=checks
        ) as pr_checks:
            advice = autolift.Advice.of(_TAG, autofix=autofix, resume=False)
        # It only reads: no branch moved, nothing recorded.
        self.assertEqual(
            self._git('for-each-ref', '--format=%(refname) %(objectname)'),
            heads,
        )
        self.assertEqual(autolift.ChannelBump(_TAG).recorded(), [])
        return advice, [c.args[0] for c in pr_checks.call_args_list]

    def test_advise_a_new_bump(self):
        advice, checked = self._advise()
        self.assertEqual(advice.action, 'lift')
        self.assertTrue(
            advice.prompt.startswith(
                f"Bump Brave's release channels to Chromium {_TAG} "
            )
        )
        self.assertEqual(checked, [])

    def test_advise_a_rebase_of_a_bump_that_conflicts_with_its_channel(self):
        self._push_bump('master')
        self._push_bump('1.100.x')
        self._git('checkout', '-q', '-B', 'landing', 'origin/1.100.x')
        self._package('1.100.48', '157.0.8103.30')
        self._git('push', '-q', 'origin', 'landing:1.100.x')
        self._git('checkout', '-q', 'master')
        self._git('branch', '-q', '-D', 'landing')
        advice, _ = self._advise()
        self.assertEqual(advice.action, 'rebase')

    def test_advise_an_autofix_of_bumps_on_github_with_a_failed_check(self):
        for channel in ('master', '1.100.x'):
            self._push_bump(channel)
        failed = {_BETA: autolift.ci_github.State.FAILURE}
        advice, checked = self._advise(states=failed)
        self.assertEqual(checked, [_MASTER, _BETA])
        self.assertEqual(advice.action, 'autofix')
        self.assertTrue(
            advice.prompt.startswith(
                'Fix what fails in the CI of the pull requests of the channel '
                f'bump to Chromium {_TAG} '
            )
        )
        # An autofix run looks at the same pull requests.
        self.assertEqual(self._advise(autofix=True, states=failed)[1], checked)

    def test_advise_nothing_for_green_bumps_on_github(self):
        for channel in ('master', '1.100.x'):
            self._push_bump(channel)
        advice, _ = self._advise()
        self.assertEqual(advice.action, 'none')
        self.assertIsNone(advice.prompt)

    def test_advise_nothing_to_autofix_without_a_bump_on_github(self):
        advice, checked = self._advise(autofix=True)
        self.assertEqual(advice.action, 'none')
        self.assertEqual(checked, [])

    def test_advise_a_resumed_bump_from_the_staged_skill(self):
        advice = autolift.Advice.of(_TAG, autofix=True, resume=True)
        self.assertEqual(advice.action, 'resume')
        self.assertIn(
            'Resume the autofix run of the channel bump to Chromium '
            f'{_TAG}, which an earlier run of the "Autofix run" section of '
            '.autolift/tooling/cr/autolift/SKILL.md',
            advice.prompt,
        )

    def test_prepare_with_every_bump_on_github_is_an_autofix_run(self):
        for channel in ('master', '1.100.x'):
            self._push_bump(channel)
        self.assertEqual(self._prepare(), autolift.NOT_WORTH_A_RUN)
        self.assertIsNotNone(autolift.run_not_worth_it(self._report()))

    def _rerun_with_beta_conflicting(self) -> None:
        """Both bumps on GitHub, and beta's channel moved on in the file its
        bump changed; then a new run's `prepare`."""
        self._push_bump('master')
        self._push_bump('1.100.x')
        self._git('checkout', '-q', '-B', 'landing', 'origin/1.100.x')
        self._package('1.100.48', '157.0.8103.30')
        self._git('push', '-q', 'origin', 'landing:1.100.x')
        self._git('checkout', '-q', 'master')
        self._git('branch', '-q', '-D', 'landing')
        self._git('fetch', '-q', 'origin')
        self.assertEqual(self._prepare(), 0)

    def test_a_rerun_rebases_a_bump_that_conflicts_with_its_channel(self):
        self._rerun_with_beta_conflicting()
        self.assertEqual(
            {c.branch: c.action for c in autolift.ChannelBump(_TAG).recorded()},
            {'master': 'none', '1.100.x': 'rebase'},
        )
        # Beta starts from GitHub's copy, checked out, upstream its channel.
        self.assertEqual(self._git('branch', '--show-current'), _BETA)
        self.assertEqual(
            self._git('rev-parse', _BETA),
            self._git('rev-parse', f'origin/{_BETA}'),
        )
        self.assertEqual(
            self._git('rev-parse', '--abbrev-ref', f'{_BETA}@{{upstream}}'),
            'origin/1.100.x',
        )
        report = self._report()
        self.assertIn('conflicts with it in package.json', report)
        self.assertIn(
            f'{_MASTER} is on GitHub at {_TAG}, and left as it is', report
        )
        target = self.run_root / 'cr157.x' / '1.100.x' / 'lift-target'
        self.assertEqual(target.read_text(encoding='utf-8'), 'none\n')

    def test_a_rerun_works_only_on_the_bumps_it_rebases(self):
        self._rerun_with_beta_conflicting()
        sequence = autolift.ChannelBump.sequence(
            autolift.ChannelBump(_TAG).recorded()
        )
        self.assertEqual(
            {run.channel for _, run in sequence if run}, {'1.100.x'}
        )
        checked = []

        def presubmit(publish) -> bool:
            checked.append(publish.branch)
            return True

        # Beta, rebased, is the highest branch of the run: built and checked.
        self.assertEqual(
            self._publish(presubmit=presubmit, rebased=(_BETA,)),
            autolift.Publish.AGAIN,
        )
        self.assertNotIn(_MASTER, self.synced)
        self.assertEqual(checked, [])
        self.assertNotIn(f'publish {_TAG} on {_MASTER} (', self._report())

    def test_a_rebased_bump_picks_from_a_higher_one_left_as_it_is(self):
        self._rerun_with_beta_conflicting()
        self._git('checkout', '-q', _MASTER)
        self._commit('fix', 'Fix the capability element patch')
        self._git('checkout', '-q', _BETA)
        result, subjects = self._picks()
        self.assertEqual(result, 0)
        self.assertIn('Fix the capability element patch', subjects)

    def test_publish_pushes_what_a_bump_left_as_it_is_got_unrebased(self):
        self._rerun_with_beta_conflicting()
        github = self._remote(_MASTER)
        self._git('checkout', '-q', _MASTER)
        self._commit('filter', 'Disable failing upstream tests.')
        self._git('checkout', '-q', _BETA)
        # A rebase of master's bump would stop the publish.
        self.assertEqual(self._publish(rebase_fails=(_MASTER,)), 0)
        self.assertEqual(self._remote(_MASTER), self._git('rev-parse', _MASTER))
        self.assertNotEqual(self._remote(_MASTER), github)
        # Its pull request is open already.
        self.brockit.assert_not_called()

    def test_a_rerun_leaves_a_bump_that_is_fine(self):
        self._push_bump('master')
        self._push_bump('1.100.x')
        # A channel that moved by a commit that doesn't conflict, just now.
        self._land_on('1.100.x', 'unrelated', 'An unrelated change')
        self.assertEqual(self._prepare(), autolift.NOT_WORTH_A_RUN)
        self.assertIn('none needs a rebase', self._report())

    def test_an_earlier_bumps_branch_is_pushed_over(self):
        earlier = self._push_bump('master', '157.0.8103.35')
        self.assertEqual(self._prepare(), 0)
        self.assertIn(
            f'origin/{_MASTER} has an earlier bump, to 157.0.8103.35',
            self._report(),
        )
        # Reset to the channel, not to the earlier bump.
        self.assertEqual(
            self._git('rev-parse', _MASTER),
            self._git('rev-parse', 'origin/master'),
        )
        self._lift('master')
        self._lift('1.100.x')
        self.assertEqual(self._publish(), 0)
        self.assertNotEqual(
            self._git('rev-parse', f'refs/heads/{_MASTER}', cwd=self.remote),
            earlier,
        )
        self.assertEqual(
            self._git('rev-parse', f'refs/heads/{_MASTER}', cwd=self.remote),
            self._git('rev-parse', _MASTER),
        )
        # Its pull request may be merged: one is opened unless it is open.
        self.assertEqual(
            self.brockit.call_args_list,
            [mock.call('update-version-issue')] * 2,
        )

    def test_prepare_without_a_channel_on_the_milestone(self):
        tag = autolift.Version('158.0.8127.4')
        result = autolift._cmd_prepare(
            mock.Mock(target=tag, force=False, dry_run=False)
        )
        self.assertEqual(result, 1)
        self.assertIn('No channel is on M158', self._report(tag))

    def _lift(self, channel: str) -> None:
        self._git('checkout', '-q', _branch(channel))
        self._commit(
            f'version-{channel}',
            'Update from Chromium 157.0.8103.30 to Chromium 157.0.8103.38.',
        )

    def _land_on(self, channel: str, name: str, content: str) -> None:
        """Lands a commit on `channel` on GitHub, before the run starts."""
        self._git('checkout', '-q', '-B', 'landing', f'origin/{channel}')
        self._commit(name, content)
        self._git('push', '-q', 'origin', f'landing:{channel}')
        self._git('checkout', '-q', 'master')
        self._git('branch', '-q', '-D', 'landing')
        self._git('fetch', '-q', 'origin')

    def _picks(self) -> tuple[int, list[str]]:
        result = autolift._cmd_channel_picks(mock.Mock(dry_run=False))
        return result, self._git(
            'log', '--format=%s', '@{upstream}..HEAD'
        ).splitlines()

    def test_a_revert_of_what_only_the_higher_channel_had_is_left_out(self):
        # Only master got the backport, which its bump then reverts.
        self._land_on('master', 'backport', 'Backport crrev.com/c/1')
        self._prepare()
        self._lift('master')
        self._git('rm', '-q', 'backport')
        self._git('commit', '-q', '-m', 'Revert the backport')
        self._commit('fix', 'Fix the capability element patch')
        self._lift('1.100.x')
        result, subjects = self._picks()
        self.assertEqual(result, 0)
        self.assertIn('Fix the capability element patch', subjects)
        self.assertNotIn('Revert the backport', subjects)
        self.assertIn('would change nothing here', self._report())

    def test_a_fix_that_conflicts_here_is_left_out(self):
        self._land_on('master', 'shared', 'Upstream as master has it')
        self._land_on('1.100.x', 'shared', 'Upstream as beta has it')
        self._prepare()
        self._lift('master')
        self._commit('shared', 'Fix the shared file')
        self._lift('1.100.x')
        result, subjects = self._picks()
        self.assertEqual(result, 0)
        self.assertNotIn('Fix the shared file', subjects)
        self.assertIn('conflict here, left out', self._report())
        self.assertEqual(self._git('status', '--porcelain'), '')

    def test_a_fix_the_channel_in_between_left_out_reaches_the_lowest(self):
        # Release is on M157 here too, and has the file as master does;
        # beta has it otherwise, so master's fix conflicts there.
        self._git('checkout', '-q', '-B', 'landing', 'origin/1.99.x')
        self._package('1.99.54', '157.0.8103.30')
        self._git('push', '-q', 'origin', 'landing:1.99.x')
        self._git('checkout', '-q', 'master')
        self._git('branch', '-q', '-D', 'landing')
        self._land_on('master', 'shared', 'Upstream as master has it')
        self._land_on('1.99.x', 'shared', 'Upstream as master has it')
        self._land_on('1.100.x', 'shared', 'Upstream as beta has it')
        self._prepare()
        self._lift('master')
        self._commit('shared', 'Fix the shared file')
        self._lift('1.100.x')
        result, subjects = self._picks()
        self.assertEqual(result, 0)
        self.assertNotIn('Fix the shared file', subjects)
        self._lift('1.99.x')
        result, subjects = self._picks()
        self.assertEqual(result, 0)
        self.assertIn('Fix the shared file', subjects)
        self.assertIn(f'from {_MASTER}, {_BETA}:', self._report())

    def test_lower_channel_picks_the_higher_ones_fixes(self):
        self._prepare()
        self._lift('master')
        self._commit('fix', 'Fix the capability element patch')
        self._commit('iwyu', 'IWYU fixes.')
        self._commit(
            'patches',
            'Update patches from Chromium 157.0.8103.30 to Chromium '
            '157.0.8103.38.',
        )
        self._lift('1.100.x')
        picks = mock.Mock(dry_run=False)
        self.assertEqual(autolift._cmd_channel_picks(picks), 0)
        self.assertEqual(
            self._git('log', '--format=%s', '@{upstream}..HEAD').splitlines(),
            [
                'IWYU fixes.',
                'Fix the capability element patch',
                'Update from Chromium 157.0.8103.30 to Chromium 157.0.8103.38.',
            ],
        )
        head = self._git('rev-parse', 'HEAD')
        self.assertEqual(autolift._cmd_channel_picks(picks), 0)
        self.assertEqual(self._git('rev-parse', 'HEAD'), head)

    def test_lower_channel_picks_the_higher_ones_test_exclusions(self):
        self._prepare()
        self._lift('master')
        self._commit('master-filter', 'Disable failing upstream tests.')
        self._lift('1.100.x')
        self._commit('beta-filter', 'Disable failing upstream tests.')
        result, subjects = self._picks()
        self.assertEqual(result, 0)
        self.assertEqual(subjects.count('Disable failing upstream tests.'), 2)

    def test_highest_channel_has_nothing_to_pick(self):
        self._prepare()
        self.assertEqual(
            autolift._cmd_channel_picks(mock.Mock(dry_run=False)), 0
        )

    def test_lower_channels_look_for_fixes_on_the_higher_ones(self):
        self._prepare()
        bump = autolift.ChannelBump(_TAG)
        self.assertEqual(
            bump.higher_refs(autolift.Run.bump(157, '1.100.x')),
            [_MASTER, 'origin/master'],
        )
        self.assertEqual(bump.higher_refs(autolift.Run.bump(157, 'master')), [])

    def _publish(
        self,
        presubmit=lambda publish: True,
        rebased: tuple[str, ...] = (),
        rebase_fails: tuple[str, ...] = (),
        end_dry_run: bool = False,
    ) -> int:
        def rebase(publish) -> int:
            # Stands in for a rebase that changes the branches in `rebased`,
            # and stops on those in `rebase_fails`.
            if publish.branch in rebase_fails:
                return 1
            if publish.branch in rebased:
                self._commit(f'rebased-{publish.branch}', 'Rebased')
            return 0

        with (
            mock.patch.object(
                autolift.Publish, '_rebase', autospec=True, side_effect=rebase
            ),
            mock.patch.object(
                autolift.Publish,
                '_presubmit',
                autospec=True,
                side_effect=lambda publish, head: presubmit(publish),
            ),
            mock.patch.object(
                autolift.Publish, '_brockit', return_value=0
            ) as brockit,
        ):
            result = autolift._cmd_publish(
                mock.Mock(
                    target=_TAG,
                    built=False,
                    rebase=True,
                    allow_new=False,
                    end_dry_run=end_dry_run,
                )
            )
        self.brockit = brockit
        return result

    def _on_github(self, branch: str) -> bool:
        return bool(self._git('ls-remote', '--heads', 'origin', branch))

    def test_publish_pushes_every_channel_and_opens_their_pull_requests(self):
        self._prepare()
        self._lift('master')
        self._lift('1.100.x')
        self.assertEqual(self._publish(), 0)
        for channel in ('master', '1.100.x'):
            branch = _branch(channel)
            self.assertTrue(self._on_github(branch))
            self.assertEqual(
                self._git('rev-parse', f'refs/heads/{branch}', cwd=self.remote),
                self._git('rev-parse', branch),
            )
        self.assertEqual(
            self.brockit.call_args_list,
            [mock.call('update-version-issue')] * 2,
        )
        self.assertEqual(self._git('branch', '--show-current'), _MASTER)

    def _push_bumps(self) -> None:
        for channel in ('master', '1.100.x'):
            self._push_bump(channel)

    def test_autofix_resets_every_bump_on_github(self):
        self._push_bumps()
        self.assertEqual(
            autolift._cmd_autofix(mock.Mock(target=_TAG, dry_run=False)), 0
        )
        self.assertEqual(self._git('branch', '--show-current'), _MASTER)
        # Synced to the branch it checked out, whatever it was synced to.
        self.assertEqual(self.synced, [_MASTER])
        self.assertEqual(
            self._git('rev-parse', '--abbrev-ref', f'{_BETA}@{{upstream}}'),
            'origin/1.100.x',
        )
        self.assertEqual(
            [c.branch for c in autolift.ChannelBump(_TAG).recorded()],
            ['master', '1.100.x'],
        )
        target = self.run_root / 'cr157.x' / '1.100.x' / 'lift-target'
        self.assertEqual(target.read_text(encoding='utf-8'), 'none\n')

    def test_autofix_without_a_bump_on_github(self):
        self.assertEqual(
            autolift._cmd_autofix(mock.Mock(target=_TAG, dry_run=False)), 1
        )
        # An earlier bump's branch is not this bump's CI.
        self._push_bump('master', '157.0.8103.35')
        self.assertEqual(
            autolift._cmd_autofix(mock.Mock(target=_TAG, dry_run=False)), 1
        )

    def test_ci_failures_of_every_bump(self):
        self._push_bumps()
        autolift._cmd_autofix(mock.Mock(target=_TAG, dry_run=False))
        seen = []

        def report(failures, output):
            seen.append((self._git('branch', '--show-current'), output.name))
            failed = failures.run.channel == '1.100.x'
            return (autolift.CI_FAILED if failed else 0), '- summary'

        with mock.patch.object(
            autolift.PullRequestFailures,
            'report',
            autospec=True,
            side_effect=report,
        ):
            result = autolift._cmd_ci_failures(
                mock.Mock(target=_TAG, dry_run=False)
            )
        self.assertEqual(result, autolift.CI_FAILED)
        self.assertEqual(
            seen,
            [
                (_MASTER, 'ci-failures-master.md'),
                (_BETA, 'ci-failures-1.100.x.md'),
            ],
        )
        self.assertEqual(self._git('branch', '--show-current'), _MASTER)

    def test_a_dry_run_publishes_nothing(self):
        self._push_bump('master', '157.0.8103.35')
        remote_before = self._git(
            'for-each-ref', '--format=%(refname) %(objectname)', cwd=self.remote
        )
        self.assertEqual(self._prepare(dry_run=True), 0)
        self.assertIn('- Dry run:', self._report())
        self._lift('master')
        self._lift('1.100.x')
        self.assertEqual(self._publish(), 0)
        self.assertEqual(
            self._git(
                'for-each-ref',
                '--format=%(refname) %(objectname)',
                cwd=self.remote,
            ),
            remote_before,
        )
        self.brockit.assert_not_called()
        report = self._report()
        self.assertIn(f'Dry run: {_MASTER} (', report)
        self.assertIn(f'Dry run: {_BETA} (', report)
        self.assertIn('nothing pushed, and no pull request opened', report)
        # Ending it publishes the same branches, as the dry run left them.
        heads = [self._git('rev-parse', b) for b in (_MASTER, _BETA)]
        self.assertEqual(self._publish(end_dry_run=True), 0)
        self.assertFalse(autolift.Run.bump(157, 'master').dry_run)
        self.assertEqual([self._remote(b) for b in (_MASTER, _BETA)], heads)
        self.assertEqual(
            self.brockit.call_args_list,
            [mock.call('update-version-issue')] * 2,
        )
        self.assertIn('Dry run ended: publishing for real.', self._report())

    def test_a_later_run_that_is_no_dry_run_clears_it(self):
        self.assertEqual(self._prepare(dry_run=True), 0)
        self.assertEqual(self._prepare(force=True), 0)
        self.assertFalse(autolift.Run.bump(157, 'master').dry_run)

    def test_a_lower_channel_the_rebase_changes_needs_no_rebuild(self):
        self._prepare()
        self._lift('master')
        self._lift('1.100.x')
        self.assertEqual(self._publish(rebased=(_BETA,)), 0)
        self.assertTrue(self._on_github(_BETA))
        self.assertIn("a lower channel isn't built or checked", self._report())

    def test_the_highest_channel_the_rebase_changes_is_rebuilt(self):
        self._prepare()
        self._lift('master')
        self._lift('1.100.x')
        self.assertEqual(
            self._publish(rebased=(_MASTER,)), autolift.Publish.AGAIN
        )
        self.assertFalse(self._on_github(_MASTER))
        self.assertEqual(self._git('branch', '--show-current'), _MASTER)

    def _autofix_with_a_teammates_push(self) -> None:
        """An autofix run, master fixed, and a teammate's commit landing on
        beta's bump on GitHub meanwhile: `publish` picks it, which changes
        beta, so whether it asks for a rebuild shows whether beta is built."""
        self._push_bumps()
        autolift._cmd_autofix(mock.Mock(target=_TAG, dry_run=False))
        self.fix = self._commit('fix', 'fixup! Fix the capability element')
        self._git('checkout', '-q', '-B', 'teammate', f'origin/{_BETA}')
        self._commit('teammate', 'IWYU fixes.')
        self._git('push', '-q', 'origin', f'teammate:{_BETA}')
        self._git('checkout', '-q', _BETA)
        self._git('branch', '-q', '-D', 'teammate')

    def _publish_without_rebase(self, built: bool = False) -> int:
        """`publish TAG --no-rebase`, recording the branches it presubmits."""
        if not built:
            self._git('checkout', '-q', _MASTER)
        self.presubmitted = []

        def presubmit(publish, _head) -> bool:
            self.presubmitted.append(publish.branch)
            return True

        with mock.patch.object(
            autolift.Publish, '_presubmit', autospec=True, side_effect=presubmit
        ):
            return autolift._cmd_publish(
                mock.Mock(
                    target=_TAG, built=built, rebase=False, allow_new=False
                )
            )

    def _remote(self, branch: str) -> str:
        return self._git('rev-parse', f'refs/heads/{branch}', cwd=self.remote)

    def test_an_autofix_run_leaves_a_lower_channels_picks_to_the_ci(self):
        self._autofix_with_a_teammates_push()
        self._git('cherry-pick', self.fix)
        self.assertEqual(self._publish_without_rebase(), 0)
        # Pushed without a rebuild, the teammate's commit included.
        self._git('fetch', '-q', 'origin')
        self.assertEqual(self._remote(_BETA), self._git('rev-parse', _BETA))
        # The teammate's commit is picked onto the run's own.
        self.assertEqual(
            self._git('log', '-2', '--format=%s', _BETA).splitlines(),
            ['IWYU fixes.', 'fixup! Fix the capability element'],
        )
        self.assertEqual(self._remote(_MASTER), self._git('rev-parse', _MASTER))
        # Beta is left to its CI: neither synced to nor presubmitted.
        self.assertEqual(self.presubmitted, [_MASTER])
        self.assertNotIn(_BETA, self.synced)

    def test_an_autofix_run_rebuilds_a_lower_channel_for_its_own_fix(self):
        self._autofix_with_a_teammates_push()
        self._git('cherry-pick', self.fix)
        own = self._commit('own', 'fixup! Fix what only beta breaks')
        master = self._remote(_MASTER)
        self.assertEqual(self._publish_without_rebase(), autolift.Publish.AGAIN)
        # Nothing is pushed until beta, synced to, is rebuilt.
        self.assertEqual(self._remote(_MASTER), master)
        self.assertEqual(self._git('branch', '--show-current'), _BETA)
        self.assertIn(_BETA, self.synced)
        # Once it is, beta is presubmitted, and both go.
        self.assertEqual(self._publish_without_rebase(built=True), 0)
        self.assertIn(_BETA, self.presubmitted)
        self._git('fetch', '-q', 'origin')
        self.assertEqual(
            self._git('merge-base', '--is-ancestor', own, f'origin/{_BETA}'),
            '',
        )

    def _rebase_needed(self, brockit: str) -> bool | None:
        """`Publish._rebase_needed` on the checked-out branch, with a stand-in
        for the channel's Brockit."""
        script = self.dir.parent / 'brockit.py'
        script.write_text(f'import sys\n{brockit}\n', encoding='utf-8')
        run = autolift.Run.of_branch(self._git('branch', '--show-current'))
        with mock.patch.object(autolift, '_BROCKIT', script):
            return autolift.Publish(run)._rebase_needed(['rebase'])

    _OLD_BROCKIT = (
        "sys.stderr.write('brockit.py: error: unrecognized arguments: "
        "--dry-run\\n')\nsys.exit(2)"
    )

    def test_a_brockit_without_dry_run_rebases_when_the_channel_moved(self):
        self._prepare()
        self._lift('master')
        self.assertIs(self._rebase_needed(self._OLD_BROCKIT), False)
        self._land_on('master', 'landed', 'A change on master')
        self._git('checkout', '-q', _MASTER)
        self.assertIs(self._rebase_needed(self._OLD_BROCKIT), True)

    def test_a_brockit_with_dry_run_decides(self):
        self._prepare()
        self.assertIs(self._rebase_needed('sys.exit(2)'), True)
        self.assertIs(self._rebase_needed('sys.exit(0)'), False)
        self.assertIsNone(self._rebase_needed('sys.exit(1)'))

    def test_only_the_highest_channel_is_synced_and_checked(self):
        self._prepare()
        self._lift('master')
        self._lift('1.100.x')
        # Starting elsewhere, the highest channel is synced to before its
        # presubmit; the lower one is left to its CI, presubmit included.
        self._git('checkout', '-q', _BETA)
        checked = []

        def presubmit(publish) -> bool:
            checked.append((publish.branch, list(self.synced)))
            return True

        self.assertEqual(self._publish(presubmit=presubmit), 0)
        self.assertEqual(checked, [(_MASTER, [_MASTER])])
        self.assertEqual(self.synced, [_MASTER])
        self.assertTrue(self._on_github(_BETA))

    def test_publish_pushes_nothing_until_every_channel_is_ready(self):
        self._prepare()
        self._lift('master')
        self._lift('1.100.x')
        result = self._publish(rebase_fails=(_BETA,))
        self.assertEqual(result, 1)
        self.assertFalse(self._on_github(_MASTER))
        self.assertFalse(self._on_github(_BETA))
        # Left on the branch that stopped it.
        self.assertEqual(self._git('branch', '--show-current'), _BETA)


if __name__ == '__main__':
    unittest.main()
