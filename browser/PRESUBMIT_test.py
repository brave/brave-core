#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from pathlib import PurePath
import sys
import unittest
from unittest import mock

# Append paths needed to import presubmit modules and shared test mocks.
BROWSER_PATH = PurePath(__file__).parent
BRAVE_PATH = BROWSER_PATH.parent
CHROMIUM_SRC_PATH = BRAVE_PATH.parent
sys.path.append(str(BRAVE_PATH / 'script'))
sys.path.append(str(CHROMIUM_SRC_PATH))
sys.path.insert(0, str(BROWSER_PATH))

import PRESUBMIT

from PRESUBMIT_test_mocks import MockAffectedFile
from PRESUBMIT_test_mocks import MockInputApi, MockOutputApi

_DESKTOP_TAB_FEATURES = 'browser/ui/tabs/brave_tab_features.cc'
_ANDROID_TAB_FEATURES = 'browser/android/brave_tab_features.cc'
_TAB_HELPERS = 'browser/brave_tab_helpers.cc'


class CheckNoCreateForInBraveTabFeaturesTest(unittest.TestCase):
    def _Check(self, files):
        input_api = MockInputApi()
        input_api.files = files
        return PRESUBMIT.CheckNoCreateForInBraveTabFeatures(
            input_api, MockOutputApi()
        )

    def testFlagsCreateForOnDesktopAndAndroid(self):
        errors = self._Check(
            [
                MockAffectedFile(
                    _DESKTOP_TAB_FEATURES,
                    ['  FooTabHelper::CreateForWebContents(contents);'],
                ),
                MockAffectedFile(
                    _ANDROID_TAB_FEATURES,
                    ['  BarTabHelper::MaybeCreateForWebContents(contents);'],
                ),
            ]
        )

        self.assertEqual(2, len(errors))
        self.assertIn(f'{_DESKTOP_TAB_FEATURES}:1:', errors[0].message)
        self.assertIn(f'{_ANDROID_TAB_FEATURES}:1:', errors[1].message)

    def testAllowsUniquePtrMembers(self):
        errors = self._Check(
            [
                MockAffectedFile(
                    _DESKTOP_TAB_FEATURES,
                    [
                        '  foo_ = std::make_unique<FooTabFeature>(tab);',
                        '  bar_ = BarTabFeature::MaybeCreate(tab);',
                    ],
                ),
            ]
        )

        self.assertEqual(0, len(errors))

    def testAllowsAllowlistedCalls(self):
        with mock.patch.object(
            PRESUBMIT,
            '_ALLOWED_CREATE_FOR_CALLS',
            ('FooTabHelper::CreateForWebContents',),
        ):
            errors = self._Check(
                [
                    MockAffectedFile(
                        _DESKTOP_TAB_FEATURES,
                        [
                            '  FooTabHelper::CreateForWebContents('
                            'tab.GetContents());'
                        ],
                    ),
                ]
            )

        self.assertEqual(0, len(errors))

    def testIgnoresOtherFiles(self):
        errors = self._Check(
            [
                MockAffectedFile(
                    'browser/ui/tabs/other.cc',
                    ['  FooTabHelper::CreateForWebContents(web_contents);'],
                ),
            ]
        )

        self.assertEqual(0, len(errors))


class CheckNoNewTabHelpersInBraveTabHelpersTest(unittest.TestCase):
    _OLD_CONTENTS = [
        '  brave_wallet::BraveWalletTabHelper::CreateForWebContents(',
        '      web_contents);',
    ]

    def _Check(self, new_contents, old_contents=None, path=_TAB_HELPERS):
        input_api = MockInputApi()
        input_api.files = [
            MockAffectedFile(
                path,
                new_contents,
                old_contents=old_contents or self._OLD_CONTENTS,
                action='M',
            )
        ]
        return PRESUBMIT.CheckNoNewTabHelpersInBraveTabHelpers(
            input_api, MockOutputApi()
        )

    def testFlagsNewHelper(self):
        errors = self._Check(
            self._OLD_CONTENTS
            + ['  foo::FooTabHelper::MaybeCreateForWebContents(web_contents);']
        )

        self.assertEqual(1, len(errors))
        self.assertIn('foo::FooTabHelper is attached', errors[0].message)

    def testFlagsCreateIfNeeded(self):
        errors = self._Check(
            self._OLD_CONTENTS
            + ['  FooTabHelper::CreateIfNeeded(web_contents);']
        )

        self.assertEqual(1, len(errors))

    def testAllowsExistingHelperMovedOrReformatted(self):
        errors = self._Check(
            [
                '  brave_wallet::BraveWalletTabHelper::CreateForWebContents('
                'web_contents);',
            ]
        )

        self.assertEqual(0, len(errors))

    def testAllowsAllowlistedHelper(self):
        errors = self._Check(
            self._OLD_CONTENTS
            + [
                '  ephemeral_storage::EphemeralStorageTabHelper::'
                'CreateForWebContents('
            ]
        )

        self.assertEqual(0, len(errors))

    def testIgnoresUnrelatedCreateCalls(self):
        errors = self._Check(
            self._OLD_CONTENTS
            + ['  screenshot::CreatePrintPreviewExtractor(callback);']
        )

        self.assertEqual(0, len(errors))

    def testIgnoresOtherFiles(self):
        errors = self._Check(
            ['  foo::FooTabHelper::CreateForWebContents(web_contents);'],
            path='browser/other.cc',
        )

        self.assertEqual(0, len(errors))


if __name__ == '__main__':
    unittest.main()
