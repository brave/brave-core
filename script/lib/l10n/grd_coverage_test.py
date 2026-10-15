#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# /// script
# dependencies = [
#   'lxml==4.9.3'
# ]
# ///
"""Tests for grd_coverage.py. Run with: vpython3 grd_coverage_test.py"""

import os
import sys
import unittest

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(os.path.dirname(SCRIPT_DIR)))

# pylint: disable=wrong-import-position
from lib.l10n.grd_coverage import find_unmapped


def grd(*messages):
    body = '\n'.join(
        f'<message name="{name}">{text}</message>' for name, text in messages
    )
    return (
        f'<grit><release><messages>{body}</messages></release></grit>'
    ).encode()


class FindUnmappedTest(unittest.TestCase):
    def find(self, files, mapped=(), allowlist=None):
        return find_unmapped(
            list(files), lambda path: files[path], mapped, allowlist or {}
        )

    def test_reports_unmapped_file_with_brand_name(self):
        files = {
            'a/new_strings.grd': grd(
                ('IDS_A', 'Chrome needs access'), ('IDS_B', 'Unrelated')
            )
        }
        self.assertEqual(self.find(files), {'a/new_strings.grd': ['IDS_A']})

    def test_ignores_mapped_allowlisted_and_excluded_files(self):
        text = grd(('IDS_A', 'Chrome needs access'))
        files = {
            'a/mapped.grd': text,
            'a/allowed.grd': text,
            'ios/chrome/app/strings.grd': text,
            'a/testdata/strings.grd': text,
        }
        self.assertEqual(
            self.find(files, ['a/mapped.grd'], {'a/allowed.grd': 'reason'}),
            {},
        )

    def test_ignores_text_branding_does_not_change(self):
        files = {
            'a/strings.grd': grd(
                ('IDS_EXAMPLE', '<ph name="NAME">$1<ex>Chrome</ex></ph> opens'),
                ('IDS_URL', 'Type chrome://extensions'),
                ('IDS_WALLET', 'Turn off in Google Wallet'),
            )
        }
        self.assertEqual(self.find(files), {})

    def test_ignores_non_translateable_messages(self):
        content = (
            '<grit><messages><message name="IDS_A" translateable="false">'
            'Chrome</message></messages></grit>'
        ).encode()
        self.assertEqual(self.find({'a/strings.grd': content}), {})


if __name__ == '__main__':
    unittest.main()
