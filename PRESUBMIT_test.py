#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from pathlib import PurePath
import sys
import unittest

# Append paths needed to import presubmit modules and shared test mocks.
BRAVE_PATH = PurePath(__file__).parent
CHROMIUM_SRC_PATH = BRAVE_PATH.parent
sys.path.append(str(BRAVE_PATH / 'script'))
sys.path.append(str(CHROMIUM_SRC_PATH))

import PRESUBMIT

from PRESUBMIT_test_mocks import MockAffectedFile
from PRESUBMIT_test_mocks import MockInputApi, MockOutputApi


class CheckTypeScriptSuppressionsHaveReasonsTest(unittest.TestCase):

    def testFlagsSuppressionsWithoutReason(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile('brave/foo.ts', ['// @ts-expect-error']),
            MockAffectedFile('brave/bar.tsx', ['// @ts-ignore']),
        ]

        errors = PRESUBMIT.CheckTypeScriptSuppressionsHaveReasons(
            input_api, MockOutputApi())

        self.assertEqual(1, len(errors))
        self.assertEqual(2, len(errors[0].items))
        self.assertIn('brave/foo.ts:1: // @ts-expect-error', errors[0].items)
        self.assertIn('brave/bar.tsx:1: // @ts-ignore', errors[0].items)

    def testAllowsSuppressionsWithReason(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile(
                'brave/foo.ts',
                ['// @ts-expect-error: This will be fixed in v148.']),
        ]

        errors = PRESUBMIT.CheckTypeScriptSuppressionsHaveReasons(
            input_api, MockOutputApi())

        self.assertEqual(0, len(errors))

    def testWarnsForTsIgnoreWithReason(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile(
                'brave/bar.tsx',
                ['// @ts-ignore because upstream typing is incorrect']),
        ]

        errors = PRESUBMIT.CheckTypeScriptSuppressionsHaveReasons(
            input_api, MockOutputApi())

        self.assertEqual(1, len(errors))
        self.assertIn('Educational guideline for @ts-ignore usage.',
                      errors[0].message)
        self.assertEqual(1, len(errors[0].items))
        self.assertIn(
            'brave/bar.tsx:1: // @ts-ignore because upstream typing is '
            'incorrect', errors[0].items)

    def testIgnoresNonTargetExtensions(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile('brave/foo.jsx', ['// @ts-expect-error']),
        ]

        errors = PRESUBMIT.CheckTypeScriptSuppressionsHaveReasons(
            input_api, MockOutputApi())

        self.assertEqual(0, len(errors))

    def testIgnoresUnknownTsSuppressionAnnotations(self):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile('brave/foo.ts', ['// @ts-ignore-error']),
        ]

        errors = PRESUBMIT.CheckTypeScriptSuppressionsHaveReasons(
            input_api, MockOutputApi())

        self.assertEqual(0, len(errors))


if __name__ == '__main__':
    unittest.main()
