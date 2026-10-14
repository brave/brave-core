#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for obsolete.py."""

from __future__ import annotations

from collections.abc import Iterator
from dataclasses import dataclass, field
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

from obsolete import (
    CitedBugs,
    Git,
    LiftOverrides,
    PatchAdoptions,
    RangeCommit,
    RemovedNames,
    TestFilters,
    TestIndex,
)


class FakeGit:
    """Files by `(ref, path)`; `ref` None is the working tree."""

    def __init__(
        self,
        files: dict[tuple[str | None, str], str] | None = None,
        ref: str | None = None,
        grep: list[tuple[str, str, str]] | None = None,
        log: str = '',
    ):
        self.files_at = files or {}
        self.ref = ref
        self._grep = grep or []
        self._log = log

    def files(self, *paths: str) -> list[str]:
        return sorted(
            p
            for (ref, p) in self.files_at
            if ref is None and p.startswith(paths)
        )

    def show(self, path: str, ref: str | None = None) -> str | None:
        return self.files_at.get((ref, path))

    def grep_lines(
        self, *args: str, paths: tuple[str, ...] = ()
    ) -> Iterator[tuple[str, str, str]]:
        del args, paths  # The fake's lines are fixed.
        yield from self._grep

    def run(self, *args: str, stdin: str | None = None) -> str:
        del args, stdin  # The fake's output is fixed.
        return self._log


@dataclass
class FakeRange:
    """A `Range` over fakes, at the tags `old` and `new`."""

    brave: FakeGit = field(default_factory=FakeGit)
    chromium: FakeGit = field(default_factory=FakeGit)
    range_commits: list[RangeCommit] = field(default_factory=list)
    debt: bool = False
    old: str = 'old'
    new: str = 'new'

    def commits(self) -> list[RangeCommit]:
        return list(self.range_commits)


class CitedBugsTest(unittest.TestCase):
    def test_named(self):
        commit = RangeCommit(
            'a' * 40,
            's',
            [
                'Bug: 1234567, chromium:7654321, b:9999999',
                'Fixed: https://crbug.com/2222222',
                '> Bug: 3333333',
                'Mentions https://chromium-review.googlesource.com/'
                'c/chromium/src/+/111111',
                '> Reviewed-on: https://chromium-review.googlesource.com/'
                'c/chromium/src/+/222222',
                'Reviewed-on: https://chromium-review.googlesource.com/'
                'c/chromium/src/+/333333',
            ],
        )
        self.assertEqual(
            [
                ('crbug', '1234567', 'Bug'),
                ('crbug', '7654321', 'Bug'),
                ('crbug', '2222222', 'Fixed'),
                ('crbug', '3333333', 'quoted Bug'),
                ('CL', '222222', 'quoted CL'),
                ('CL', '333333', 'Reviewed-on'),
            ],
            list(CitedBugs(None).named(commit)),
        )

    def test_run(self):
        brave = FakeGit(
            grep=[
                ('chromium_src/a.cc', '3', '// https://crbug.com/1234567'),
                ('patches/b.patch', '9', ' // crbug.com/5555555 (context)'),
                ('patches/b.patch', '10', '+// crbug.com/2222222'),
            ],
            log='@@@ abc Brave fix\nhttps://crrev.com/c/333333\n'
            'https://issues.chromium.org/issues/4444444\n',
        )
        commits = [
            RangeCommit('1' * 40, 'Fix it', ['Fixed: 1234567']),
            RangeCommit('2' * 40, 'Revert "Fix it"', ['> Fixed: 1234567']),
            RangeCommit('3' * 40, 'Land', ['Bug: 4444444, 5555555']),
            RangeCommit(
                '4' * 40,
                'Own CL',
                [
                    'Reviewed-on: https://chromium-review.googlesource.com/'
                    'c/chromium/src/+/333333'
                ],
            ),
        ]
        lines = CitedBugs(FakeRange(brave, range_commits=commits)).run()
        self.assertEqual(
            [
                'crbug 1234567  brave: chromium_src/a.cc:3',
                f'    Fixed        {"1" * 12} Fix it (reverted in range)',
                f'    quoted Fixed {"2" * 12} Revert "Fix it"',
                'crbug 4444444  brave: commit abc Brave fix',
                f'    Bug          {"3" * 12} Land',
                '2 cited ids named in the range',
            ],
            lines,
        )

    def test_quoted_culprit_messages_are_not_citations(self) -> None:
        brave = FakeGit(
            log='@@@ abc [cr159] Follow a rename\n'
            'Chromium changes:\n'
            'commit 1111111111111111111111111111111111111111\n'
            '    Rename it\n'
            '    Bug: https://crbug.com/1234567\n'
        )
        commits = [RangeCommit('1' * 40, 'Rename it', ['Bug: 1234567'])]
        lines = CitedBugs(FakeRange(brave, range_commits=commits)).run()
        self.assertEqual(['0 cited ids named in the range'], lines)

    def test_umbrella_bugs_are_skipped(self):
        brave = FakeGit(grep=[('a.cc', '1', 'crbug.com/1234567')])
        commits = [
            RangeCommit(str(i) * 40, f'c{i}', ['Bug: 1234567'])
            for i in range(CitedBugs.UMBRELLA + 1)
        ]
        lines = CitedBugs(FakeRange(brave, range_commits=commits)).run()
        self.assertEqual(
            [
                'Umbrella bugs, skipped: 1234567 (6 commits)',
                '0 cited ids named in the range',
            ],
            lines,
        )


class PatchAdoptionsTest(unittest.TestCase):
    _PATCH = (
        'diff --git a/a/x.cc b/a/x.cc\n'
        'index 1..2 100644\n'
        '--- a/a/x.cc\n'
        '+++ b/a/x.cc\n'
        '@@ -1,2 +1,3 @@\n'
        ' context();\n'
        '+UpstreamAddsThisLine(argument);\n'
        '+BraveOnlyLine(argument_two);\n'
        '+}\n'
        'diff --git a/a/y.cc b/a/y.cc\n'
        '+AlreadyThereAtBothTags(arg);\n'
        '-MovedLineInsideTheHunk(arg);\n'
        '+MovedLineInsideTheHunk(arg);\n'
        'diff --git a/a/z.cc b/a/z.cc\n'
        '+AlsoThereAtBothTags(arg);\n'
        '+  BRAVE_Z_HOOK(arg);\n'
    )

    _CHROMIUM = {
        ('old', 'a/x.cc'): 'context();\n',
        ('new', 'a/x.cc'): 'context();\n  UpstreamAddsThisLine(argument);\n',
        ('old', 'a/y.cc'): 'AlreadyThereAtBothTags(arg);\n',
        ('new', 'a/y.cc'): 'AlreadyThereAtBothTags(arg);\n',
        ('old', 'a/z.cc'): 'AlsoThereAtBothTags(arg);\n',
        ('new', 'a/z.cc'): 'AlsoThereAtBothTags(arg);\n',
    }

    def test_run(self):
        """A patch that also adds a line naming Brave (`a/z.cc`) still does
        something, even when upstream has its other lines."""
        brave = FakeGit({(None, 'patches/a.patch'): self._PATCH})
        self.assertEqual(
            [
                'adopted (0 -> 1)\tpatches/a.patch\ta/x.cc\t'
                '+UpstreamAddsThisLine(argument);',
                '0 patched files outside chromium/src skipped',
                '1 patched files already upstream',
                '    (list them with --debt)',
            ],
            PatchAdoptions(FakeRange(brave, FakeGit(self._CHROMIUM))).run(),
        )

    def test_already_upstream_is_listed_with_debt(self):
        brave = FakeGit({(None, 'patches/a.patch'): self._PATCH})
        lines = PatchAdoptions(
            FakeRange(brave, FakeGit(self._CHROMIUM), debt=True)
        ).run()
        self.assertEqual(
            [
                '1 patched files already upstream',
                'already upstream\tpatches/a.patch\ta/y.cc\t'
                'all 1 added lines, e.g. +AlreadyThereAtBothTags(arg);',
            ],
            lines[-2:],
        )

    def test_subdirectories_and_missing_files(self):
        brave = FakeGit(
            {
                (
                    None,
                    'patches/v8/a.patch',
                ): 'diff --git a/b.cc b/b.cc\n+Lines(x, y);\n'
            }
        )
        self.assertEqual(
            [
                '1 patched files outside chromium/src skipped',
                '0 patched files already upstream',
            ],
            PatchAdoptions(FakeRange(brave)).run(),
        )


_OLD_TESTS = """\
TEST_F(FooTest, Renamed) {
IN_PROC_BROWSER_TEST_F(MAYBE_BarTest,
                       Split) {
TEST_F(ParamTest, Becomes) {
TEST_P(LosesParam, Case) {
INSTANTIATE_TEST_SUITE_P(
    All,
    LosesParam,
TEST_F(FlakyTest, DISABLED_Flaky) {
TEST_F(OldSuite, PreloadedProductionPins) {
FRIEND_TEST_ALL_PREFIXES(FriendOnly, Test);
""".splitlines()

_NEW_TESTS = """\
TEST_F(FooTest, NewName) {
TEST_F(OtherTest, Renamed) {
IN_PROC_BROWSER_TEST_F(MAYBE_BarTest,
                       Split) {
TEST_P(ParamTest, Becomes) {
TEST_F(LosesParam, Case) {
TEST_F(FlakyTest, Flaky) {
TEST_F(OldSuite, PreloadedProductionPins) {
TEST_F(KeyPinningState, PreloadedProductionPins) {
""".splitlines()


class TestFiltersTest(unittest.TestCase):
    def setUp(self):
        self.filters = TestFilters(
            FakeRange(), TestIndex(_OLD_TESTS), TestIndex(_NEW_TESTS)
        )

    def _check(self, text: str) -> list[str]:
        return self.filters.check(TestFilters.parse('f:1', text))

    def test_index(self):
        index = TestIndex(_OLD_TESTS)
        self.assertEqual({('F', False)}, index.suites['BarTest']['Split'])
        self.assertEqual({('F', True)}, index.suites['FlakyTest']['Flaky'])
        self.assertEqual({'All'}, index.prefixes['LosesParam'])
        self.assertNotIn('FriendOnly', index.suites)

    def test_index_split_instantiation_without_prefix(self) -> None:
        index = TestIndex(
            [
                'INSTANTIATE_TEST_SUITE_P(',
                '    ,',
                '    GlicApiActorTest,',
                '    testing::Values(1));',
            ]
        )
        self.assertEqual({''}, index.prefixes['GlicApiActorTest'])

    def test_index_at_a_tag(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'a_unittest.cc').write_text(
                'TEST_F(FooTest,\n       Bar) {}\nint TEST_ignored;\n',
                encoding='utf-8',
                newline='',
            )
            env = {
                'GIT_AUTHOR_NAME': 't',
                'GIT_AUTHOR_EMAIL': 't@t',
                'GIT_COMMITTER_NAME': 't',
                'GIT_COMMITTER_EMAIL': 't@t',
                'HOME': tmp,
                'PATH': '/usr/bin:/bin',
            }
            for args in (
                ['init', '-q'],
                ['add', '.'],
                ['commit', '-q', '-m', 'x'],
                ['tag', 't'],
            ):
                subprocess.run(['git', '-C', tmp, *args], check=True, env=env)
            index = TestIndex.at(Git(root), 't')
        self.assertEqual({('F', False)}, index.suites['FooTest']['Bar'])

    def test_parse(self):
        entry = TestFilters.parse('f:1', '-All/Suite.Test/0  # comment')
        self.assertEqual(
            ('Suite', 'Test', True), (entry.suite, entry.test, entry.prefixed)
        )
        entry = TestFilters.parse('f:1', 'Typed/1.Test')
        self.assertEqual(('Typed', False), (entry.suite, entry.prefixed))
        self.assertIsNone(TestFilters.parse('f:1', '# -Suite.Test'))
        self.assertIsNone(TestFilters.parse('f:1', '-*.Test'))
        self.assertIsNone(TestFilters.parse('f:1', ''))

    def test_checks(self):
        self.assertEqual(
            ['gone in range; now in OtherTest'], self._check('-FooTest.Renamed')
        )
        self.assertEqual([], self._check('-BarTest.Split'))
        self.assertEqual(
            ['became TEST_P: the filter no longer applies'],
            self._check('-ParamTest.Becomes'),
        )
        self.assertEqual(
            ['became TEST_F: the prefixed entry matches nothing'],
            self._check('-All/LosesParam.Case/0'),
        )
        self.assertEqual(
            ['DISABLED_ dropped upstream'], self._check('-FlakyTest.Flaky')
        )
        self.assertEqual(
            ['copied into new KeyPinningState'],
            self._check('-OldSuite.PreloadedProductionPins'),
        )
        self.assertEqual(
            ['matches nothing at either tag'], self._check('-Gone.Test')
        )
        self.assertEqual([], self._check('-FooTest.*'))

    def test_new_subclass_suite_of_a_filtered_fixture(self):
        brave = FakeGit({(None, 'test/filters/a.filter'): '-FooTest.*\n'})
        log = (
            '@@@ 18d83271faee\n'
            'diff --git a/a_unittest.cc b/a_unittest.cc\n'
            '+++ b/a_unittest.cc\n'
            '@@ -1,0 +2,3 @@\n'
            '+class NewVisualTest\n'
            '+    : public ns::FooTest {\n'
            '+class Helper : public FooTest {};\n'
        )
        filters = TestFilters(
            FakeRange(brave, FakeGit(log=log)),
            TestIndex(_OLD_TESTS),
            TestIndex(_NEW_TESTS + ['TEST_F(NewVisualTest, Case) {']),
        )
        self.assertIn(
            'test/filters/a.filter:1\t-FooTest.*\tnew subclass suite '
            'NewVisualTest (18d83271faee)',
            filters.run(),
        )
        self.assertFalse(any('Helper' in line for line in filters.run()))

    def test_debt_is_counted(self):
        brave = FakeGit(
            {(None, 'test/filters/a.filter'): '-Gone.Test\n-FooTest.Renamed\n'}
        )
        filters = TestFilters(
            FakeRange(brave), TestIndex(_OLD_TESTS), TestIndex(_NEW_TESTS)
        )
        self.assertEqual(
            [
                'test/filters/a.filter:2\t-FooTest.Renamed\t'
                'gone in range; now in OtherTest',
                '1 entries match nothing at either tag',
                '    1 in test/filters/a.filter',
                '    (list them with --debt)',
            ],
            filters.run(),
        )


class RemovedNamesTest(unittest.TestCase):
    def test_api_names(self):
        api = RemovedNames._API
        for name in (
            'OriginalProfileSupplier',
            'kChromeUntrustedPrefix',
            'TreatAsPublicAddress',
        ):
            self.assertTrue(api.fullmatch(name), name)
        for name in (
            'has_initialized',
            'REMOVING',
            'Bluetooth',
            'mOnResetListener',
        ):
            self.assertFalse(api.fullmatch(name), name)

    def test_comment_mentions_are_not_uses(self):
        brave = FakeGit(
            grep=[
                (
                    'a.java',
                    '20',
                    "   * Replaces upstream's OldSupplier, removed",
                ),
                ('b.cc', '3', '  // GoneThing is gone'),
                ('c.py', '1', '# GoneThing'),
                ('d.cc', '7', '  GoneThing x = MakeGoneThing();'),
            ]
        )
        uses, comments = RemovedNames(FakeRange(brave))._code_locations(
            {'OldSupplier', 'GoneThing'}
        )
        self.assertEqual({'GoneThing': ['d.cc:7']}, dict(uses))
        self.assertEqual({'OldSupplier': ['a.java:20']}, dict(comments))

    def test_headers(self):
        brave = FakeGit(
            {
                (None, 'chromium_src/a/b.h'): (
                    '// Copyright (c) 2026\n'
                    'inline constexpr char kKept[] = "";\n'
                    'inline constexpr char kDeletedInRange[] = "";\n'
                    'inline constexpr char kNeverUpstream[] = "";\n'
                    'inline constexpr char kBraveOwn[] = "";\n'
                ),
                (
                    None,
                    'chromium_src/a/c.h',
                ): '#include <a/c.h>\nint kAnything;\n',
            }
        )
        chromium = FakeGit(
            {
                ('old', 'a/b.h'): 'kKept kDeletedInRange',
                ('new', 'a/b.h'): 'kKept',
                ('old', 'a/c.h'): '',
                ('new', 'a/c.h'): '',
            }
        )
        self.assertEqual(
            [
                'header\tchromium_src/a/b.h\t'
                '1 deleted in range: kDeletedInRange',
                'header\tchromium_src/a/b.h\t1 not upstream: kNeverUpstream',
            ],
            RemovedNames(FakeRange(brave, chromium)).headers(),
        )

    def test_rows_predating_the_range_are_counted(self):
        removed = RemovedNames(FakeRange())
        with (
            mock.patch.object(removed, 'files', return_value=[]),
            mock.patch.object(
                removed,
                'headers',
                return_value=[
                    'header\ta.h\t1 deleted in range: kA',
                    'header\ta.h\t1 not upstream: kB',
                ],
            ),
            mock.patch.object(
                removed,
                'features',
                return_value=['feature\tf.inc\tGone\tmissing at both'],
            ),
            mock.patch.object(removed, 'names', return_value=[]),
        ):
            self.assertEqual(
                [
                    'header\ta.h\t1 deleted in range: kA',
                    '2 header names and features missing at both tags',
                    '    (list them with --debt)',
                ],
                removed.run(),
            )


class LiftOverridesTest(unittest.TestCase):
    def test_overrides_the_range_does_not_touch_are_counted(self):
        brave = FakeGit(log='chromium_src/a.cc\nchromium_src/b.cc\n')
        overrides = LiftOverrides(FakeRange(brave))
        touching = {'added\tchromium_src/a.cc': ['abc Change a']}
        with mock.patch.object(
            overrides, '_touching', side_effect=lambda l: touching.get(l, [])
        ):
            self.assertEqual(
                [
                    'added\tchromium_src/a.cc\n    abc Change a',
                    '1 lift overrides no range commit touches',
                    '    (list them with --debt)',
                ],
                overrides.run(),
            )

    def test_diffs_from_the_branch_upstream(self):
        class UpstreamGit(FakeGit):
            def __init__(self, upstream):
                super().__init__()
                self.upstream = upstream
                self.spans = []

            def run(self, *args, stdin=None):
                if args[0] == 'rev-parse':
                    if self.upstream is None:
                        raise subprocess.CalledProcessError(128, args)
                    return f'{self.upstream}\n'
                self.spans += [a for a in args if '...' in a]
                return ''

        for upstream, base in (
            ('origin/cr158', 'origin/cr158'),
            (None, 'origin/master'),
        ):
            brave = UpstreamGit(upstream)
            LiftOverrides(FakeRange(brave)).run()
            self.assertEqual([f'{base}...HEAD'] * 2, brave.spans)


if __name__ == '__main__':
    unittest.main()
