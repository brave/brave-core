#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for overlap.py."""

from __future__ import annotations

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
        self.index = OverrideIndex(
            {
                'chrome/a.cc': {'chromium_src', 'plaster'},
                'chrome/BUILD.gn': {'chromium_src'},
                'ui/b.ts': {'lit_mangler'},
                'DEPS': {'plaster'},
                'v8/src/c.cc': {'patch'},
            }
        )

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
