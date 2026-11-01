#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for overlap.py."""

from __future__ import annotations

import contextlib
import json
from pathlib import Path
import tempfile
import unittest

from overlap import OverrideIndex

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
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        root = Path(stack.enter_context(tempfile.TemporaryDirectory()))
        for path, text in (
            ('chromium_src/chrome/a.cc', ''),
            ('chromium_src/chrome/BUILD.gn', ''),
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
