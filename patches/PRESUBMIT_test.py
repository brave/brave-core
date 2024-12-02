# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
import brave_chromium_utils

# pylint: disable=import-error,no-member
import PRESUBMIT

with brave_chromium_utils.sys_path("//"):
    from PRESUBMIT_test_mocks import MockFile, MockAffectedFile
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
                'name': 'empty line at start of block',
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
                'name': 'empty line at end of block',
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
                    f"Expected {case['expected_errors']} errors, got {actual_errors}"
                )


if __name__ == '__main__':
    unittest.main()
