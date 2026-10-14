#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for overlap.py."""

from __future__ import annotations

import json
from pathlib import Path
import shutil
import tempfile
import unittest

from overlap import OverrideIndex, net_zero

_LOG = '''@@@ 111
chrome/a.cc
chrome/BUILD.gn
DEPS

@@@ 222
chrome/a.cc
ui/b.ts
v8/src/c.cc
unrelated.cc
'''


class OverrideIndexTest(unittest.TestCase):
    def setUp(self):
        root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root)
        for path, text in (
            ('chromium_src/chrome/a.cc', ''),
            ('chromium_src/chrome/BUILD.gn', ''),
            ('chromium_src/PRESUBMIT.py', ''),
            ('chromium_src/chrome/PRESUBMIT_test.py', ''),
            ('chromium_src/ui/b.ts.lit_mangler.ts', ''),
            ('rewrite/chrome/a.cc.yaml', ''),
            ('rewrite/DEPS.yaml', ''),
            (
                'patches/v8/src-c.cc.patchinfo',
                json.dumps({'appliesTo': [{'path': 'src/c.cc'}]}),
            ),
        ):
            (root / path).parent.mkdir(parents=True, exist_ok=True)
            (root / path).write_text(text, encoding='utf-8', newline='')
        self.index = OverrideIndex(root)

    def test_kinds(self):
        self.assertEqual(
            {'chromium_src', 'plaster'}, self.index.kinds['chrome/a.cc']
        )
        self.assertEqual({'lit_mangler'}, self.index.kinds['ui/b.ts'])
        self.assertEqual({'patch'}, self.index.kinds['v8/src/c.cc'])

    def test_brave_presubmits_are_not_overrides(self):
        self.assertNotIn('PRESUBMIT.py', self.index.kinds)
        self.assertNotIn('chrome/PRESUBMIT_test.py', self.index.kinds)

    def test_touched_drops_noise(self):
        self.assertEqual(
            {
                'chrome/a.cc': ['111', '222'],
                'ui/b.ts': ['222'],
                'v8/src/c.cc': ['222'],
            },
            self.index.touched(_LOG),
        )

    def test_touched_by_kind(self):
        self.assertEqual(
            {'chrome/a.cc': ['111', '222']},
            self.index.touched(_LOG, 'plaster'),
        )

    def test_touched_skips_commits(self):
        self.assertEqual(
            {
                'chrome/a.cc': ['222'],
                'ui/b.ts': ['222'],
                'v8/src/c.cc': ['222'],
            },
            self.index.touched(_LOG, skip={'111'}),
        )

    def test_net_zero_pairs_a_revert_with_its_target_in_range(self):
        reland, revert, other = 'a' * 40, 'b' * 40, 'c' * 40
        outside = 'd' * 40
        log = (
            f'{revert}\x00Reverts.\n\nThis reverts commit {reland}.\n\x01\n'
            f'{other}\x00This reverts commit {outside}.\n\x01\n'
            f'{reland}\x00Reland.\n\x01\n'
        )
        self.assertEqual({'a' * 12, 'b' * 12}, net_zero(log))

    def test_format(self):
        self.assertEqual(
            '## chrome/\n'
            'chrome/a.cc (chromium_src,plaster): 111 222\n'
            '## ui/\n'
            'ui/b.ts (lit_mangler): 222\n'
            '2 overridden files, 2 commits',
            self.index.format(
                {'chrome/a.cc': ['111', '222'], 'ui/b.ts': ['222']}
            ),
        )


if __name__ == '__main__':
    unittest.main()
