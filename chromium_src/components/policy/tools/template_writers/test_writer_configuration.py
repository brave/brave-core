#!/usr/bin/env python3
# Copyright 2022 The Brave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

def _merge_dicts(src, dst):
    result = dict(dst)
    for k, v in src.items():
        result[k] = \
            _merge_dicts(v, dst.get(k, {})) if isinstance(v, dict) else v
    return result

class MergeDictsTest(unittest.TestCase):
    def test_merge(self):
        self.assertEqual(
            # We actually only care about 'brave' being in the result.
            {'brave': 'Brave', "googlechrome": "Google Chrome"},
            _merge_dicts({'brave': 'Brave'}, {"googlechrome": "Google Chrome"})
        )
