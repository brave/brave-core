#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from pathlib import Path, PurePath
import json
import sys
import tempfile
import unittest

# Append paths needed to import presubmit modules and shared test mocks.
BRAVE_PATH = PurePath(__file__).parent
CHROMIUM_SRC_PATH = BRAVE_PATH.parent
sys.path.append(str(BRAVE_PATH / 'script'))
sys.path.append(str(CHROMIUM_SRC_PATH))
sys.path.append(str(BRAVE_PATH / 'vendor' / 'depot_tools'))

import PRESUBMIT  # noqa: E402

from PRESUBMIT_test_mocks import MockAffectedFile  # noqa: E402
from PRESUBMIT_test_mocks import MockInputApi, MockOutputApi  # noqa: E402


class CheckTypeScriptSuppressionsHaveReasonsTest(unittest.TestCase):
    def testFlagsSuppressionsWithoutReason(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile('brave/foo.ts', ['// @ts-expect-error']),
            MockAffectedFile('brave/bar.tsx', ['// @ts-ignore']),
        ]

        errors = PRESUBMIT.CheckTypeScriptSuppressionsHaveReasons(
            input_api, MockOutputApi()
        )

        self.assertEqual(1, len(errors))
        self.assertEqual(2, len(errors[0].items))
        self.assertIn('brave/foo.ts:1: // @ts-expect-error', errors[0].items)
        self.assertIn('brave/bar.tsx:1: // @ts-ignore', errors[0].items)

    def testAllowsSuppressionsWithReason(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile(
                'brave/foo.ts',
                ['// @ts-expect-error: This will be fixed in v148.'],
            ),
        ]

        errors = PRESUBMIT.CheckTypeScriptSuppressionsHaveReasons(
            input_api, MockOutputApi()
        )

        self.assertEqual(0, len(errors))

    def testWarnsForTsIgnoreWithReason(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile(
                'brave/bar.tsx',
                ['// @ts-ignore because upstream typing is incorrect'],
            ),
        ]

        errors = PRESUBMIT.CheckTypeScriptSuppressionsHaveReasons(
            input_api, MockOutputApi()
        )

        self.assertEqual(1, len(errors))
        self.assertIn(
            'Educational guideline for @ts-ignore usage.', errors[0].message
        )
        self.assertEqual(1, len(errors[0].items))
        self.assertIn(
            'brave/bar.tsx:1: // @ts-ignore because upstream typing is '
            'incorrect',
            errors[0].items,
        )

    def testIgnoresNonTargetExtensions(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile('brave/foo.jsx', ['// @ts-expect-error']),
        ]

        errors = PRESUBMIT.CheckTypeScriptSuppressionsHaveReasons(
            input_api, MockOutputApi()
        )

        self.assertEqual(0, len(errors))

    def testIgnoresUnknownTsSuppressionAnnotations(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile('brave/foo.ts', ['// @ts-ignore-error']),
        ]

        errors = PRESUBMIT.CheckTypeScriptSuppressionsHaveReasons(
            input_api, MockOutputApi()
        )

        self.assertEqual(0, len(errors))


class CheckJson5ParseErrorsTest(unittest.TestCase):
    def testAcceptsJson5WithCommentsAndTrailingCommas(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile(
                'brave/build/mappings.json5',
                [
                    '// libc++ private headers -> public facades',
                    '{',
                    '  "include": [',
                    '    [ "<__algorithm/sort.h>", "private",',
                    '      "<algorithm>", "public" ],  /* trailing comma OK */',
                    '  ],',
                    '}',
                ],
            ),
        ]

        errors = PRESUBMIT.CheckJson5ParseErrors(input_api, MockOutputApi())

        self.assertEqual([], errors)

    def testReportsInvalidJson5(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile(
                'brave/build/broken.json5',
                [
                    '{',
                    '  "key": "value"',
                    '  "missing_comma": true',
                    '}',
                ],
            ),
        ]

        errors = PRESUBMIT.CheckJson5ParseErrors(input_api, MockOutputApi())

        self.assertEqual(1, len(errors))
        self.assertIn(
            'brave/build/broken.json5 could not be parsed', errors[0].message
        )

    def testIgnoresNonJson5Files(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile(
                'brave/build/mappings.json', ['{ "not-json5": true,, }']
            ),
            MockAffectedFile('brave/foo.ts', ['const x = 1;']),
        ]

        errors = PRESUBMIT.CheckJson5ParseErrors(input_api, MockOutputApi())

        self.assertEqual([], errors)

    def testIgnoresDeletedFiles(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile('brave/build/deleted.json5', [], action='D'),
        ]

        errors = PRESUBMIT.CheckJson5ParseErrors(input_api, MockOutputApi())

        self.assertEqual([], errors)


class CheckNoCommittedSecretsFilesTest(unittest.TestCase):
    def testFlagsASecretsGniFile(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile(
                'out/linux-x64-asan-brave/secrets.gni',
                ['fake_secret_key = "abc123"'],
            ),
        ]

        errors = PRESUBMIT.CheckNoCommittedSecretsFiles(
            input_api, MockOutputApi()
        )

        self.assertEqual(1, len(errors))
        self.assertIn('secrets.gni', errors[0].message)

    def testIgnoresOtherGniFiles(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile(
                'brave/build/args/brave_defaults.gni', ['is_asan = true']
            ),
        ]

        errors = PRESUBMIT.CheckNoCommittedSecretsFiles(
            input_api, MockOutputApi()
        )

        self.assertEqual([], errors)

    def testIgnoresDeletedSecretsFiles(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile(
                'out/linux-x64-asan-brave/secrets.gni', [], action='D'
            ),
        ]

        errors = PRESUBMIT.CheckNoCommittedSecretsFiles(
            input_api, MockOutputApi()
        )

        self.assertEqual([], errors)


class CheckTodoBugReferencesTest(unittest.TestCase):
    def _Check(self, line):
        input_api = MockInputApi()
        input_api.files = [MockAffectedFile('brave/foo.cc', [line])]
        # CheckTodoBugReferences is wrapped by @override_check, which injects
        # the _original_check argument at runtime; pylint can't see that.
        # pylint: disable=no-value-for-parameter
        return PRESUBMIT.CheckTodoBugReferences(input_api, MockOutputApi())

    def testAllowsGithubIssueUrl(self):
        line = (
            '// TODO(https://github.com/brave/brave-browser/issues/123): '
            'fix this'
        )
        self.assertEqual([], self._Check(line))

    def testAllowsGithubIssueUrlWithoutScheme(self):
        line = '// TODO(github.com/brave/brave-browser/issues/123): fix this'
        self.assertEqual([], self._Check(line))

    def testAllowsBraveDevBugLink(self):
        self.assertEqual([], self._Check('// TODO(brave.dev/bug/55738): fix'))

    def testAllowsBraveDevShortBugLink(self):
        self.assertEqual([], self._Check('// TODO(brave.dev/b/55738): fix'))

    def testAllowsBraveDevBugLinkWithScheme(self):
        line = '// TODO(https://brave.dev/bug/55738): fix this'
        self.assertEqual([], self._Check(line))

    def testFlagsTodoWithoutBugReference(self):
        warnings = self._Check('// TODO(bbondy): fix this')

        self.assertEqual(1, len(warnings))
        self.assertEqual(1, len(warnings[0].items))
        self.assertIn('brave/foo.cc:1', warnings[0].items[0])

    def testFlagsBraveDevLinkWithoutId(self):
        self.assertEqual(1, len(self._Check('// TODO(brave.dev/bug/): fix')))

    def testIgnoresLinesWithoutTodo(self):
        self.assertEqual([], self._Check('// just a comment, no todo here'))


def _PackageJson(chromium_tag, brave_version='1.0.0'):
    """Returns package.json lines with the given Chromium tag."""
    return json.dumps(
        {
            'version': brave_version,
            'config': {'projects': {'chrome': {'tag': chromium_tag}}},
        },
        indent=2,
    ).splitlines()


class CheckSystemNetworkContextCallersTest(unittest.TestCase):
    def setUp(self):
        self._find_callers = PRESUBMIT._FindSystemNetworkContextCallers
        self._callers = {'chrome/browser/a.cc', 'chrome/browser/b.cc'}
        PRESUBMIT._FindSystemNetworkContextCallers = lambda _: self._callers
        self._chromium_bump = MockAffectedFile(
            'package.json',
            _PackageJson('160.0.8181.0'),
            old_contents=_PackageJson('159.0.8154.18'),
            action='M',
        )

    def tearDown(self):
        PRESUBMIT._FindSystemNetworkContextCallers = self._find_callers

    def _Check(self, known, files=None, requested=False, all_brave=False):
        input_api = MockInputApi()
        input_api.PRESUBMIT_ALL_BRAVE = all_brave
        input_api.PRESUBMIT_CHECK_SYSTEM_NETWORK_CONTEXT = requested
        input_api.files = [self._chromium_bump] if files is None else files
        with tempfile.TemporaryDirectory() as tmp:
            input_api.presubmit_local_path = tmp
            callers_file = (
                Path(tmp) / PRESUBMIT._SYSTEM_NETWORK_CONTEXT_CALLERS_FILE
            )
            callers_file.parent.mkdir(parents=True)
            callers_file.write_text(
                '# A comment.\n' + ''.join(f'{c}\n' for c in known),
                encoding='utf-8',
                newline='',
            )
            return PRESUBMIT.CheckSystemNetworkContextCallers(
                input_api, MockOutputApi()
            )

    def testAcceptsListedCallers(self):
        self.assertEqual([], self._Check(self._callers))

    def testWarnsAboutNewCallerOnChromiumBump(self):
        results = self._Check({'chrome/browser/a.cc'})

        self.assertEqual(1, len(results))
        self.assertEqual('warning', results[0].type)
        self.assertEqual(['chrome/browser/b.cc'], results[0].items)

    def testNotesRemovedCaller(self):
        results = self._Check(self._callers | {'chrome/browser/c.cc'})

        self.assertEqual(1, len(results))
        self.assertEqual('notify', results[0].type)
        self.assertEqual(['chrome/browser/c.cc'], results[0].items)

    def testRunsWhenRequested(self):
        results = self._Check({'chrome/browser/a.cc'}, files=[], requested=True)

        self.assertEqual(1, len(results))
        self.assertEqual(['chrome/browser/b.cc'], results[0].items)

    def testSkipsWithoutPackageJsonChange(self):
        files = [MockAffectedFile('brave/foo.cc', ['int x;'])]

        self.assertEqual([], self._Check(set(), files=files))

    def testSkipsPackageJsonChangeWithoutChromiumBump(self):
        files = [
            MockAffectedFile(
                'package.json',
                _PackageJson('159.0.8154.18', brave_version='1.0.1'),
                old_contents=_PackageJson('159.0.8154.18'),
                action='M',
            )
        ]

        self.assertEqual([], self._Check(set(), files=files))

    def testSkipsOnFullPresubmit(self):
        self.assertEqual([], self._Check(set(), files=[], all_brave=True))

    def testSkipsWithoutUpstreamCheckout(self):
        PRESUBMIT._FindSystemNetworkContextCallers = lambda _: None

        self.assertEqual([], self._Check(set()))


if __name__ == '__main__':
    unittest.main()
