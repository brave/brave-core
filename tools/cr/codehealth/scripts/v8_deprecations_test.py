#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for v8_deprecations.py."""

from __future__ import annotations

import unittest

from v8_deprecations import V8Deprecations

_LOG = """\
@@@ aaa [api] Deprecate Foo
diff --git a/include/v8-foo.h b/include/v8-foo.h
--- a/include/v8-foo.h
+++ b/include/v8-foo.h
@@ -1,3 +1,4 @@
 class Foo {
-  V8_DEPRECATE_SOON("Use Bar")
+  V8_DEPRECATED("Use Bar")
   void Foo();
   void Other();
   void More();
@@@ bbb [api] Unrelated
diff --git a/include/v8-bar.h b/include/v8-bar.h
--- a/include/v8-bar.h
+++ b/include/v8-bar.h
@@ -1 +1 @@
-int a;
+int b;
"""


class V8DeprecationsTest(unittest.TestCase):
    def test_revision(self) -> None:
        deps = "vars = {\n  'v8_revision': '" + 'a' * 40 + "',\n}\n"
        self.assertEqual('a' * 40, V8Deprecations.revision(deps))
        with self.assertRaises(ValueError):
            V8Deprecations.revision('vars = {}')

    def test_parse_keeps_marker_commits_with_their_declarations(self) -> None:
        self.assertEqual(
            [
                'aaa [api] Deprecate Foo\n'
                '    include/v8-foo.h\n'
                '      -  V8_DEPRECATE_SOON("Use Bar")\n'
                '      +  V8_DEPRECATED("Use Bar")\n'
                '         void Foo();\n'
                '         void Other();'
            ],
            V8Deprecations.parse(_LOG),
        )

    def test_summary_counts_the_whole_range(self) -> None:
        self.assertEqual(
            'V8 a..b: 13 commits, 0 changing deprecation markers',
            V8Deprecations.summary('a..b', 13, []),
        )


if __name__ == '__main__':
    unittest.main()
