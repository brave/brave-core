#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for campaigns.py."""

from __future__ import annotations

from pathlib import Path
import shutil
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import campaigns
from campaigns import CampaignIndex, Commit

_LOG = '''\
@@@ aaa [ios] Automatic conversion of FooFactory
Bug: 1000001, chromium:1000002
@@@ files
ios/foo_factory.mm
ios/foo_factory.h

@@@ bbb [ios] Remove ServiceAccessType from FooFactory
Fixed: 1000003
Bug: 1000001
@@@ files
ios/foo_factory.h

@@@ ccc [ios] Automatic conversion of BarFactory (2/N)
Bug: 1000001
@@@ files
ios/bar_factory.mm
DEPS

@@@ ddd [ash] Migrate X to Y
Bug: 2000001
@@@ files
ash/x.cc
'''


_NOT_FATAL_DIFF = '''\
diff --git a/ui/a.cc b/ui/a.cc
--- a/ui/a.cc
+++ b/ui/a.cc
@@ -1,0 +2 @@
+#include "base/check.h"
@@ -88 +88,2 @@ void F() {
-    DCHECK_EQ(is_pill, shadow.is_pill());
+    CHECK_EQ(is_pill, shadow.is_pill(),
+             base::NotFatalUntil::M163);
@@ -137 +138 @@ void G() {
-  DCHECK(x) << "message";
+  CHECK(x, base::NotFatalUntil::M163) << "message";
'''


class FakeBrave:
    """The `BraveTree` fields `CampaignIndex` reads."""

    overridden = frozenset({'ios/foo_factory.mm', 'ios/bar_factory.mm', 'DEPS'})


class CampaignIndexTest(unittest.TestCase):
    def setUp(self) -> None:
        root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root)
        files = {
            'chromium_src/ios/foo_factory.mm': '#include "ios/foo_factory.h"\n',
            'chromium_src/ios/bar_factory.mm': (
                '#include <ios/bar_factory.mm>\n'
            ),
            'x.cc': '#include "ios/foo_factory.h"\n',
        }
        for path, text in files.items():
            (root / path).parent.mkdir(parents=True, exist_ok=True)
            (root / path).write_text(text, encoding='utf-8', newline='')
        brave = SimpleNamespace(root=root, run_git=lambda *args: 'x.cc\0')
        self.enterContext(
            mock.patch.object(campaigns.repository, 'brave', brave)
        )
        self.index = CampaignIndex('o', 'n', FakeBrave())
        self.diffs: dict[str, str] = {}
        self.enterContext(
            mock.patch.object(
                self.index,
                '_diff',
                side_effect=lambda sha: self.diffs.get(sha, ''),
            )
        )

    def test_not_fatal_files(self) -> None:
        self.diffs['a'] = _NOT_FATAL_DIFF
        # A fatal `CHECK` changes what a release build does.
        self.diffs['b'] = _NOT_FATAL_DIFF.replace(
            ',\n+             base::NotFatalUntil::M163', ''
        )
        self.diffs['c'] = (
            _NOT_FATAL_DIFF
            + _NOT_FATAL_DIFF.replace('ui/a.cc', 'ui/b.cc')
            + '@@ -9 +9 @@\n-  Foo();\n+  Bar();\n'
        )
        self.assertEqual(
            {'ui/a.cc'}, self.index.not_fatal_files(Commit('a', 's'))
        )
        self.assertEqual(set(), self.index.not_fatal_files(Commit('b', 's')))
        self.assertEqual(
            {'ui/a.cc'}, self.index.not_fatal_files(Commit('c', 's'))
        )
        self.assertEqual(set(), self.index.not_fatal_files(Commit('d', 's')))

    def test_not_fatal_files_drop_out_of_a_cluster(self) -> None:
        self.diffs['aaa'] = _NOT_FATAL_DIFF.replace(
            'ui/a.cc', 'ios/foo_factory.mm'
        )
        clusters = self.index.clusters(CampaignIndex.parse(_LOG))
        self.assertEqual(['ios/bar_factory.mm'], clusters[0].overridden)
        self.assertFalse(clusters[0].not_fatal_only)

    def test_not_fatal_only_clusters_are_skipped(self) -> None:
        for sha, paths in (
            ('aaa', ('ios/foo_factory.mm', 'ios/foo_factory.h')),
            ('bbb', ('ios/foo_factory.h',)),
            ('ccc', ('ios/bar_factory.mm',)),
        ):
            self.diffs[sha] = ''.join(
                _NOT_FATAL_DIFF.replace('ui/a.cc', p) for p in paths
            )
        report = self.index.report(
            self.index.clusters(CampaignIndex.parse(_LOG))
        )
        self.assertIn('## 0 clusters touching what Brave overrides', report)
        self.assertIn(
            'Skipped, only `DCHECK` -> `CHECK(..., NotFatalUntil)`: '
            'crbug.com/1000001 (3)',
            report,
        )
        self.assertIn('## 0 other campaigns', report)

    def test_a_webui_override_importing_its_original_is_not_whole(self) -> None:
        root = campaigns.repository.brave.root
        for path, text in {
            'chromium_src/ui/app.ts': "export * from './app-chromium.js'\n",
            'chromium_src/ui/item.html.ts': (
                "import {x} from './item-chromium.html.js'\n"
            ),
            'chromium_src/ui/full.ts': (
                "import {y} from './other-chromium.js'\n"
            ),
        }.items():
            (root / path).parent.mkdir(parents=True, exist_ok=True)
            (root / path).write_text(text, encoding='utf-8')
        self.assertFalse(self.index._whole_file('ui/app.ts'))
        self.assertFalse(self.index._whole_file('ui/item.html.ts'))
        self.assertTrue(self.index._whole_file('ui/full.ts'))

    def test_parse(self) -> None:
        commits = CampaignIndex.parse(_LOG)
        self.assertEqual(['aaa', 'bbb', 'ccc', 'ddd'], [c.sha for c in commits])
        self.assertEqual(['1000001', '1000002'], commits[0].bugs)
        self.assertEqual(['1000003', '1000001'], commits[1].bugs)
        self.assertEqual(['ios/bar_factory.mm', 'DEPS'], commits[2].files)

    def test_clusters(self) -> None:
        clusters = self.index.clusters(CampaignIndex.parse(_LOG))
        self.assertEqual(1, len(clusters))
        cluster = clusters[0]
        self.assertEqual(['1000001'], cluster.bugs)
        self.assertEqual(3, len(cluster.commits))
        # Whole-file replacements first; DEPS is noise.
        self.assertEqual(
            ['ios/foo_factory.mm *', 'ios/bar_factory.mm'], cluster.overridden
        )
        self.assertEqual(['ios/foo_factory.h'], cluster.included)
        self.assertEqual(['bbb'], [c.sha for c in cluster.by_hand])

    def test_report(self) -> None:
        report = self.index.report(
            self.index.clusters(CampaignIndex.parse(_LOG))
        )
        self.assertIn('crbug.com/1000001: 3 commits', report)
        self.assertIn('Brave overrides: ios/foo_factory.mm *', report)
        self.assertIn('by hand, on a header Brave includes: bbb', report)
        self.assertNotIn('2000001', report)

    def test_small_and_non_campaign_clusters_are_dropped(self) -> None:
        commits = [
            Commit('a', 'Add a thing', ['1']),
            Commit('b', 'Fix a thing', ['1']),
            Commit('c', 'Tweak a thing', ['1']),
            Commit('d', 'Migrate X', ['2']),
            Commit('e', 'Migrate Y', ['2']),
        ]
        self.assertEqual([], self.index.clusters(commits))

    def test_area_prefixed_subjects_count_as_campaign_verbs(self) -> None:
        commits = [
            Commit('a', 'Media: Use JniUniquePtr in Foo', ['1']),
            Commit('b', 'Autofill: Use JniUniquePtr in Bar', ['1']),
            Commit('c', '[android] content/browser: Use JniRawPtr', ['1']),
        ]
        clusters = self.index.clusters(commits)
        self.assertEqual(['1'], [b for c in clusters for b in c.bugs])

    def test_reverts_are_not_shown_first(self) -> None:
        commits = [
            Commit('a', 'Revert "Migrate Foo to Bar"', ['1']),
            Commit('b', 'Migrate Foo to Bar', ['1']),
            Commit('c', 'Migrate Baz to Bar', ['1']),
        ]
        clusters = self.index.clusters(commits)
        self.assertEqual(['b', 'c', 'a'], [c.sha for c in clusters[0].commits])
        self.assertIn('(3): b Migrate Foo to Bar', self.index.report(clusters))

    def test_area_prefixed_ash_subjects_are_dropped(self) -> None:
        commits = [
            Commit('a', 'ash: Clean up reporting deps', ['1']),
            Commit('b', 'ash: Remove unused Foo', ['1']),
            Commit('c', 'chromeos: Move printing', ['1']),
        ]
        self.assertEqual([], self.index.clusters(commits))


if __name__ == '__main__':
    unittest.main()
