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

    def test_area_prefixed_ash_subjects_are_dropped(self) -> None:
        commits = [
            Commit('a', 'ash: Clean up reporting deps', ['1']),
            Commit('b', 'ash: Remove unused Foo', ['1']),
            Commit('c', 'chromeos: Move printing', ['1']),
        ]
        self.assertEqual([], self.index.clusters(commits))


if __name__ == '__main__':
    unittest.main()
