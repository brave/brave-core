#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for source_rewrite.py — walk-filter rules and update_references."""

import unittest
from pathlib import Path

import _boot  # noqa: F401
from alias.source_rewrite import (
    _is_path_excluded,
    _should_walk_dir,
    update_references,
)
from test.fake_chromium_src import FakeChromiumSrc

# ---------------------------------------------------------------------------
# Unit tests: _is_path_excluded
# ---------------------------------------------------------------------------


class IsPathExcludedTest(unittest.TestCase):
    """_is_path_excluded applies +/- rules in longest-first order."""

    # --- explicitly excluded paths ---

    def test_top_level_excluded_dir(self):
        self.assertTrue(_is_path_excluded('third_party'))

    def test_subpath_of_excluded_dir(self):
        self.assertTrue(_is_path_excluded('third_party/foo'))

    def test_deep_subpath_of_excluded_dir(self):
        self.assertTrue(_is_path_excluded('third_party/foo/bar.h'))

    def test_out_excluded(self):
        self.assertTrue(_is_path_excluded('out'))

    def test_out_subpath_excluded(self):
        self.assertTrue(_is_path_excluded('out/Default'))

    def test_node_modules_excluded(self):
        self.assertTrue(_is_path_excluded('node_modules'))

    def test_vendor_excluded(self):
        self.assertTrue(_is_path_excluded('vendor'))

    def test_tools_crates_vendor_excluded(self):
        self.assertTrue(_is_path_excluded('tools/crates/vendor'))

    # --- bare-name patterns match any depth ---

    def test_bare_pycache_at_root(self):
        self.assertTrue(_is_path_excluded('__pycache__'))

    def test_bare_pycache_nested(self):
        self.assertTrue(_is_path_excluded('foo/__pycache__'))

    def test_bare_pycache_deep(self):
        self.assertTrue(_is_path_excluded('foo/__pycache__/bar'))

    def test_bare_git_nested(self):
        self.assertTrue(_is_path_excluded('some/path/.git'))

    # --- '+' override: third_party/blink is explicitly included ---

    def test_blink_not_excluded(self):
        self.assertFalse(_is_path_excluded('third_party/blink'))

    def test_blink_subpath_not_excluded(self):
        self.assertFalse(_is_path_excluded('third_party/blink/renderer'))

    def test_blink_deep_not_excluded(self):
        self.assertFalse(_is_path_excluded('third_party/blink/renderer/core'))

    def test_devtools_frontend_not_excluded(self):
        self.assertFalse(_is_path_excluded('third_party/devtools-frontend'))

    def test_devtools_frontend_subpath_not_excluded(self):
        self.assertFalse(
            _is_path_excluded('third_party/devtools-frontend/src'))

    # --- normal paths: not excluded ---

    def test_normal_path_not_excluded(self):
        self.assertFalse(_is_path_excluded('browser'))

    def test_normal_subpath_not_excluded(self):
        self.assertFalse(_is_path_excluded('browser/core/foo.cc'))

    def test_tools_not_excluded(self):
        self.assertFalse(_is_path_excluded('tools'))

    def test_tools_crates_not_excluded(self):
        # Only tools/crates/vendor is excluded, not tools/crates itself.
        self.assertFalse(_is_path_excluded('tools/crates'))


# ---------------------------------------------------------------------------
# Unit tests: _should_walk_dir
# ---------------------------------------------------------------------------


class ShouldWalkDirTest(unittest.TestCase):
    """_should_walk_dir returns True iff os.walk should descend into the dir."""

    def test_normal_dir_walked(self):
        self.assertTrue(_should_walk_dir('browser'))

    def test_out_not_walked(self):
        self.assertFalse(_should_walk_dir('out'))

    def test_node_modules_not_walked(self):
        self.assertFalse(_should_walk_dir('node_modules'))

    def test_vendor_not_walked(self):
        self.assertFalse(_should_walk_dir('vendor'))

    def test_pycache_not_walked(self):
        self.assertFalse(_should_walk_dir('__pycache__'))

    def test_tools_crates_vendor_not_walked(self):
        self.assertFalse(_should_walk_dir('tools/crates/vendor'))

    def test_third_party_walked_for_blink(self):
        # third_party is excluded, but +third_party/blink requires descending.
        self.assertTrue(_should_walk_dir('third_party'))

    def test_third_party_foo_not_walked(self):
        # No '+' rule inside third_party/foo.
        self.assertFalse(_should_walk_dir('third_party/foo'))

    def test_third_party_blink_walked(self):
        self.assertTrue(_should_walk_dir('third_party/blink'))

    def test_third_party_blink_subdir_walked(self):
        self.assertTrue(_should_walk_dir('third_party/blink/renderer'))


# ---------------------------------------------------------------------------
# Integration tests: update_references respects the walk filter
# ---------------------------------------------------------------------------


class UpdateReferencesFilterTest(unittest.TestCase):
    """update_references skips excluded dirs and processes included ones."""

    def setUp(self):
        self._repo = FakeChromiumSrc()
        self._repo.setup()
        self.addCleanup(self._repo.cleanup)

    def _write(self, rel: str, content: str) -> Path:
        path = self._repo.brave / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding='utf-8')
        return path

    def _read(self, rel: str) -> str:
        return (self._repo.brave / rel).read_text(encoding='utf-8')

    def test_normal_file_updated(self):
        """A file outside excluded dirs has its #include updated."""
        self._write('browser/test.cc', '#include "A/foo.h"\n')
        update_references(Path('A/foo.h'), Path('B/foo.h'))
        self.assertIn('#include "B/foo.h"', self._read('browser/test.cc'))

    def test_third_party_file_not_updated(self):
        """A file inside third_party (non-blink) is skipped."""
        self._write('third_party/foo/test.cc', '#include "A/foo.h"\n')
        update_references(Path('A/foo.h'), Path('B/foo.h'))
        self.assertIn('#include "A/foo.h"',
                      self._read('third_party/foo/test.cc'))

    def test_blink_file_updated(self):
        """A file inside third_party/blink IS updated (+blink override)."""
        self._write('third_party/blink/renderer/test.cc',
                    '#include "A/foo.h"\n')
        update_references(Path('A/foo.h'), Path('B/foo.h'))
        self.assertIn('#include "B/foo.h"',
                      self._read('third_party/blink/renderer/test.cc'))

    def test_out_file_not_updated(self):
        """A file inside out/ is skipped."""
        self._write('out/Default/gen/test.cc', '#include "A/foo.h"\n')
        update_references(Path('A/foo.h'), Path('B/foo.h'))
        self.assertIn('#include "A/foo.h"',
                      self._read('out/Default/gen/test.cc'))


if __name__ == '__main__':
    unittest.main()
