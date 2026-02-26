# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import tempfile
import unittest

import brave_chromium_utils

# pylint: disable=import-error,no-member
import PRESUBMIT

with brave_chromium_utils.sys_path("//"):
    from PRESUBMIT_test_mocks import MockAffectedFile
    from PRESUBMIT_test_mocks import MockInputApi, MockOutputApi


class ImportSpecifierTest(unittest.TestCase):

    def setUp(self):
        self._tmpdir = tempfile.mkdtemp()

    def _run_check(self, files):
        """Run the check with a files map.

        Args:
            files: dict mapping path -> lines list (affected file) or None
                   (exists on disk only, not in the changelist).
        """
        input_api = MockInputApi()
        input_api.files = []
        for path, contents in files.items():
            full = os.path.join(self._tmpdir, path)
            os.makedirs(os.path.dirname(full), exist_ok=True)
            if contents is not None:
                affected_file = MockAffectedFile(path, contents)
                affected_file.AbsoluteLocalPath = lambda p=full: p
                input_api.files.append(affected_file)
            else:
                open(full, 'w').close()
        return PRESUBMIT.CheckImportSpecifierMatchesFile(
            input_api, MockOutputApi())

    def testImportSpecifiers(self):
        # files: path -> lines (affected source) or None (on-disk only).
        cases = [
            {
                'name': 'correct .js extension',
                'files': {
                    'lib/build.js': ["import config from './config.js'"],
                    'lib/config.js': None,
                },
                'expected_errors': 0,
            },
            {
                'name': 'wrong extension: .js import but .ts file',
                'files': {
                    'lib/build.js': ["import config from './config.js'"],
                    'lib/config.ts': None,
                },
                'expected_errors': 1,
            },
            {
                'name': 'multiline import',
                'files': {
                    'lib/build.js': [
                        "import {",
                        "  getTestBinary,",
                        "  getTestsToRun,",
                        "} from './utils.js'",
                    ],
                    'lib/utils.ts': None,
                },
                'expected_errors': 1,
            },
            {
                'name': 'dynamic import()',
                'files': {
                    'lib/build.js': [
                        "const config = await import('./config.js')",
                    ],
                    'lib/config.ts': None,
                },
                'expected_errors': 1,
            },
            {
                'name': 'side-effect import, wrong extension',
                'files': {
                    'lib/build.js': ["import './setup.js'"],
                    'lib/setup.ts': None,
                },
                'expected_errors': 1,
            },
            {
                'name': 'side-effect import, correct extension',
                'files': {
                    'lib/build.js': ["import './setup.js'"],
                    'lib/setup.js': None,
                },
                'expected_errors': 0,
            },
            {
                'name': 'export { } from',
                'files': {
                    'lib/index.js': ["export { foo } from './utils.js'"],
                    'lib/utils.ts': None,
                },
                'expected_errors': 1,
            },
            {
                'name': 'export * from, correct extension',
                'files': {
                    'lib/index.js': ["export * from './utils.js'"],
                    'lib/utils.js': None,
                },
                'expected_errors': 0,
            },
            {
                'name': 'correct .ts extension',
                'files': {
                    'lib/build.ts': ["import config from './config.ts'"],
                    'lib/config.ts': None,
                },
                'expected_errors': 0,
            },
            {
                'name': 'parent directory import',
                'files': {
                    'lib/build.js': ["import config from '../config.js'"],
                    'config.ts': None,
                },
                'expected_errors': 1,
            },
            {
                'name': 'non-relative imports are ignored',
                'files': {
                    'lib/build.js': [
                        "import fs from 'fs-extra'",
                        "import path from 'path'",
                    ],
                },
                'expected_errors': 0,
            },
            {
                'name': 'multiple errors in one file',
                'files': {
                    'lib/build.js': [
                        "import config from './config.js'",
                        "import util from './util.js'",
                    ],
                    'lib/config.ts': None,
                    'lib/util.ts': None,
                },
                'expected_errors': 2,
            },
            {
                'name': 'wrong .mjs extension',
                'files': {
                    'lib/build.mjs': ["import config from './config.mjs'"],
                    'lib/config.mts': None,
                },
                'expected_errors': 1,
            },
            {
                'name': 'require() is not matched',
                'files': {
                    'lib/build.cjs': [
                        "const config = require('./config.cjs')",
                    ],
                    'lib/config.cts': None,
                },
                'expected_errors': 0,
            },
            {
                'name': 'dotted filename, wrong extension',
                'files': {
                    'lib/build.js': [
                        "import config from './config.base.js'",
                    ],
                    'lib/config.base.ts': None,
                },
                'expected_errors': 1,
            },
            {
                'name': 'dotted filename, correct extension',
                'files': {
                    'lib/build.js': [
                        "import config from './config.base.js'",
                    ],
                    'lib/config.base.js': None,
                },
                'expected_errors': 0,
            },
        ]

        for case in cases:
            with self.subTest(name=case['name']):
                self._tmpdir = tempfile.mkdtemp()
                results = self._run_check(case['files'])

                items = [i for r in results for i in r.items]
                self.assertEqual(
                    case['expected_errors'], len(items),
                    f"Expected {case['expected_errors']} errors, "
                    f"got {len(items)}: {items}")


if __name__ == '__main__':
    unittest.main()
