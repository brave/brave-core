# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
import brave_chromium_utils

# pylint: disable=import-error,no-member
import PRESUBMIT

with brave_chromium_utils.sys_path("//"):
    from PRESUBMIT_test_mocks import MockAffectedFile
    from PRESUBMIT_test_mocks import MockInputApi, MockOutputApi


class PatchFileTest(unittest.TestCase):

    def testEmptyLinesInPatch(self):
        cases = [
            {
                'name': 'valid patch without empty lines',
                'contents': [
                    'diff --git a/file.cc b/file.cc',
                    '+void func() {',
                    '+  DoSomething();',
                    '+}',
                ],
                'expected_errors': 0
            },
            {
                'name': 'added single empty line',
                'contents': [
                    'diff --git a/file.cc b/file.cc',
                    '+',
                ],
                'expected_errors': 1
            },
            {
                'name': 'added multiple empty lines',
                'contents': [
                    'diff --git a/file.cc b/file.cc',
                    '+',
                    '+',
                ],
                'expected_errors': 1
            },
            {
                'name': 'removed empty line',
                'contents': [
                    'diff --git a/file.cc b/file.cc',
                    '-',
                ],
                'expected_errors': 1
            },
            {
                'name': 'empty line at start of hunk',
                'contents': [
                    'diff --git a/file.cc b/file.cc',
                    '+',
                    '+void func() {',
                    '+  DoSomething();',
                    '+}',
                ],
                'expected_errors': 1
            },
            {
                'name': 'empty line at end of hunk',
                'contents': [
                    'diff --git a/file.cc b/file.cc',
                    '+void func() {',
                    '+  DoSomething();',
                    '+}',
                    '+',
                ],
                'expected_errors': 1
            },
            {
                'name': 'empty line in middle is ok',
                'contents': [
                    'diff --git a/file.cc b/file.cc',
                    '+void func() {',
                    '+',
                    '+  DoSomething();',
                    '+}',
                ],
                'expected_errors': 0
            },
            {
                'name': 'removed empty line',
                'contents': [
                    'diff --git a/file.cc b/file.cc',
                    ' void func() {',
                    '-',
                    '   DoSomething();',
                    ' }',
                ],
                'expected_errors': 1
            },
            {
                'name': 'removed empty line in a hunk',
                'contents': [
                    'diff --git a/file.cc b/file.cc',
                    '-void func() {',
                    '-',
                    '-  DoSomething();',
                    '-}',
                ],
                'expected_errors': 0
            },
            {
                'name': 'removed empty line at hunk boundary',
                'contents': [
                    'diff --git a/file.cc b/file.cc',
                    '-void func() {',
                    '-',
                    '-  DoSomething();',
                    '-}',
                    '-',
                ],
                'expected_errors': 1
            },
        ]

        for case in cases:
            with self.subTest(name=case['name']):
                affected_file = MockAffectedFile('test.patch',
                                                 case['contents'])
                input_api = MockInputApi()
                input_api.files = [affected_file]

                results = PRESUBMIT.CheckPatchFile(input_api, MockOutputApi())

                actual_errors = len(results)
                self.assertEqual(
                    case['expected_errors'], actual_errors,
                    f"Expected {case['expected_errors']} errors,"
                    f" got {actual_errors}")


class PatchFileSourceTest(unittest.TestCase):

    def _run(self, contents, filename):
        affected_file = MockAffectedFile(filename, contents)
        input_api = MockInputApi()
        input_api.files = [affected_file]
        return PRESUBMIT.CheckPatchFileSource(input_api, MockOutputApi())

    def testFilenameMatchesConventionPasses(self):
        # base/metrics/foo.h  ->  base-metrics-foo.h.patch
        results = self._run(
            contents=['diff --git a/base/metrics/foo.h b/base/metrics/foo.h'],
            filename='base-metrics-foo.h.patch')
        self.assertEqual(0, len(results))

    def testFilenameDeepPathPasses(self):
        # chrome/browser/ui/views/frame/bar.cc
        #   -> chrome-browser-ui-views-frame-bar.cc.patch
        results = self._run(
            contents=[
                'diff --git a/chrome/browser/ui/views/frame/bar.cc'
                ' b/chrome/browser/ui/views/frame/bar.cc'
            ],
            filename='chrome-browser-ui-views-frame-bar.cc.patch')
        self.assertEqual(0, len(results))

    def testWrongFilenameFails(self):
        results = self._run(
            contents=['diff --git a/base/metrics/foo.h b/base/metrics/foo.h'],
            filename='wrong-name.patch')
        self.assertEqual(1, len(results))

    def testMissingDirectoryInFilenameFails(self):
        # Header says base/metrics/foo.h but filename omits the directory level.
        results = self._run(
            contents=['diff --git a/base/metrics/foo.h b/base/metrics/foo.h'],
            filename='base-foo.h.patch')
        self.assertEqual(1, len(results))

    def testEmptyPatchFails(self):
        results = self._run(contents=[], filename='base-foo.h.patch')
        self.assertEqual(1, len(results))

    def testMissingDiffHeaderFails(self):
        results = self._run(contents=['--- a/base/foo.h', '+++ b/base/foo.h'],
                            filename='base-foo.h.patch')
        self.assertEqual(1, len(results))

    def testNonPatchFileIgnored(self):
        results = self._run(contents=['diff --git a/base/foo.h b/base/foo.h'],
                            filename='base-foo.h.txt')
        self.assertEqual(0, len(results))


_FULL_SHA = 'a' * 40
_FULL_SHA2 = 'b' * 40
_SHORT_SHA = 'abc1234'
_SHORT_SHA2 = 'def5678'


class PatchFileIndexHeaderTest(unittest.TestCase):

    def _run(self, contents, filename='foo-bar.patch'):
        affected_file = MockAffectedFile(filename, contents)
        input_api = MockInputApi()
        input_api.files = [affected_file]
        return PRESUBMIT.CheckPatchFileIndexHeader(input_api, MockOutputApi())

    def testValidFullIndexHeader(self):
        results = self._run([
            'diff --git a/foo/bar.cc b/foo/bar.cc',
            f'index {_FULL_SHA}..{_FULL_SHA2} 100644',
            '--- a/foo/bar.cc',
            '+++ b/foo/bar.cc',
            '@@ -1 +1 @@',
            '+// brave',
        ])
        self.assertEqual(0, len(results))

    def testAbbreviatedShaFails(self):
        results = self._run([
            'diff --git a/foo/bar.cc b/foo/bar.cc',
            f'index {_SHORT_SHA}..{_SHORT_SHA2} 100644',
            '--- a/foo/bar.cc',
            '+++ b/foo/bar.cc',
        ])
        self.assertEqual(1, len(results))

    def testMissingIndexLineFails(self):
        results = self._run([
            'diff --git a/foo/bar.cc b/foo/bar.cc',
            '--- a/foo/bar.cc',
            '+++ b/foo/bar.cc',
        ])
        self.assertEqual(1, len(results))

    def testEmptyPatchSkipped(self):
        results = self._run([])
        self.assertEqual(0, len(results))

    def testExecutableModeInIndexLine(self):
        results = self._run([
            'diff --git a/foo/bar.py b/foo/bar.py',
            f'index {_FULL_SHA}..{_FULL_SHA2} 100755',
            '--- a/foo/bar.py',
            '+++ b/foo/bar.py',
        ])
        self.assertEqual(0, len(results))

    def testIndexLineWithoutMode(self):
        # Some git diff outputs omit the mode when it doesn't change.
        results = self._run([
            'diff --git a/foo/bar.cc b/foo/bar.cc',
            f'index {_FULL_SHA}..{_FULL_SHA2}',
            '--- a/foo/bar.cc',
            '+++ b/foo/bar.cc',
        ])
        self.assertEqual(0, len(results))

    def testNonPatchFileIgnored(self):
        affected_file = MockAffectedFile('foo.txt', [
            'diff --git a/foo/bar.cc b/foo/bar.cc',
            f'index {_SHORT_SHA}..{_SHORT_SHA2} 100644',
        ])
        input_api = MockInputApi()
        input_api.files = [affected_file]
        results = PRESUBMIT.CheckPatchFileIndexHeader(input_api,
                                                      MockOutputApi())
        self.assertEqual(0, len(results))


class PatchFileSingleSourceTest(unittest.TestCase):

    def _run(self, contents, filename='foo-bar.patch'):
        affected_file = MockAffectedFile(filename, contents)
        input_api = MockInputApi()
        input_api.files = [affected_file]
        return PRESUBMIT.CheckPatchFileSingleSource(input_api, MockOutputApi())

    def testSingleSourcePasses(self):
        results = self._run([
            'diff --git a/foo/bar.cc b/foo/bar.cc',
            f'index {_FULL_SHA}..{_FULL_SHA2} 100644',
            '--- a/foo/bar.cc',
            '+++ b/foo/bar.cc',
            '@@ -1 +1 @@',
            '+// brave',
        ])
        self.assertEqual(0, len(results))

    def testMultipleSourcesFails(self):
        results = self._run([
            'diff --git a/foo/bar.cc b/foo/bar.cc',
            f'index {_FULL_SHA}..{_FULL_SHA2} 100644',
            '--- a/foo/bar.cc',
            '+++ b/foo/bar.cc',
            '@@ -1 +1 @@',
            '+// brave',
            'diff --git a/foo/baz.cc b/foo/baz.cc',
            f'index {_FULL_SHA}..{_FULL_SHA2} 100644',
            '--- a/foo/baz.cc',
            '+++ b/foo/baz.cc',
            '@@ -1 +1 @@',
            '+// also brave',
        ])
        self.assertEqual(1, len(results))

    def testNoDiffHeaderFails(self):
        results = self._run([
            '--- a/foo/bar.cc',
            '+++ b/foo/bar.cc',
            '@@ -1 +1 @@',
            '+// brave',
        ])
        self.assertEqual(1, len(results))

    def testEmptyPatchFails(self):
        results = self._run([])
        self.assertEqual(1, len(results))

    def testNonPatchFileIgnored(self):
        affected_file = MockAffectedFile('foo.txt', [
            'diff --git a/foo/bar.cc b/foo/bar.cc',
            'diff --git a/foo/baz.cc b/foo/baz.cc',
        ])
        input_api = MockInputApi()
        input_api.files = [affected_file]
        results = PRESUBMIT.CheckPatchFileSingleSource(input_api,
                                                       MockOutputApi())
        self.assertEqual(0, len(results))


if __name__ == '__main__':
    unittest.main()
