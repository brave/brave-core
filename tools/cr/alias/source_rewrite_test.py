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


# ---------------------------------------------------------------------------
# Integration tests: GN reference rewriting in .gn/.gni files
# ---------------------------------------------------------------------------


class UpdateGnReferencesTest(unittest.TestCase):
    """update_references rewrites quoted GN references in .gn/.gni files."""

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

    # ----- BUILD.gn move: directory rename, root references -----

    def test_build_gn_root_reference_rewritten(self):
        """`"//brave/foo"` → `"//brave/bar"` when foo's BUILD.gn moves."""
        self._write('consumer/BUILD.gn',
                    'deps = [ "//brave/components/api_request_helper" ]\n')
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/components/api_test/BUILD.gn'))
        self.assertIn('"//brave/components/api_test"',
                      self._read('consumer/BUILD.gn'))

    def test_build_gn_root_reference_with_target_rewritten(self):
        """`"//brave/foo:target"` is rewritten and target preserved."""
        self._write(
            'consumer/BUILD.gn',
            'deps = [ "//brave/components/api_request_helper:test_support" ]\n'
        )
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/components/api_test/BUILD.gn'))
        self.assertIn('"//brave/components/api_test:test_support"',
                      self._read('consumer/BUILD.gn'))

    def test_build_gn_root_reference_with_subpath_rewritten(self):
        """`"//brave/foo/sub"` is rewritten and subpath preserved."""
        self._write(
            'consumer/BUILD.gn',
            'sources = [ "//brave/components/api_request_helper/foo.h" ]\n')
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/components/api_test/BUILD.gn'))
        self.assertIn('"//brave/components/api_test/foo.h"',
                      self._read('consumer/BUILD.gn'))

    def test_build_gn_similar_prefix_not_rewritten(self):
        """`"//brave/foo_v2"` is NOT touched when only `foo` moved."""
        self._write('consumer/BUILD.gn',
                    'deps = [ "//brave/components/api_request_helper_v2" ]\n')
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/components/api_test/BUILD.gn'))
        self.assertIn('"//brave/components/api_request_helper_v2"',
                      self._read('consumer/BUILD.gn'))

    def test_build_gn_root_reference_in_gni_rewritten(self):
        """The walk applies to .gni files too, not just BUILD.gn."""
        self._write(
            'config/sources.gni',
            'shared_deps = [ "//brave/components/api_request_helper" ]\n')
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/components/api_test/BUILD.gn'))
        self.assertIn('"//brave/components/api_test"',
                      self._read('config/sources.gni'))

    # ----- BUILD.gn move: relative references -----

    def test_build_gn_relative_sibling_rewritten(self):
        """`"../api_request_helper"` from a sibling dir is rewritten."""
        self._write('components/ai_chat/BUILD.gn',
                    'deps = [ "../api_request_helper" ]\n')
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/components/api_test/BUILD.gn'))
        self.assertIn('"../api_test"',
                      self._read('components/ai_chat/BUILD.gn'))

    def test_build_gn_relative_from_parent_rewritten(self):
        """`"components/api_request_helper:foo"` from brave/BUILD.gn."""
        self._write(
            'BUILD.gn',
            'deps = [ "components/api_request_helper:test_support" ]\n')
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/components/api_test/BUILD.gn'))
        self.assertIn('"components/api_test:test_support"',
                      self._read('BUILD.gn'))

    def test_build_gn_implicit_target_renamed_in_moved_file(self):
        """In the moved BUILD.gn, the dir-name target is renamed."""
        # update_references runs after the move; seed the file at its NEW
        # location.
        self._write(
            'components/api_foo/BUILD.gn',
            'static_library("api_request_helper") {\n'
            '  sources = [ "api.cc" ]\n'
            '}\n'
            'source_set("test_support") {\n'
            '  testonly = true\n'
            '}\n')
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/components/api_foo/BUILD.gn'))
        content = self._read('components/api_foo/BUILD.gn')
        self.assertIn('static_library("api_foo")', content)
        self.assertNotIn('"api_request_helper"', content)
        # Unrelated targets are untouched.
        self.assertIn('source_set("test_support")', content)

    def test_build_gn_same_file_label_ref_renamed_in_moved_file(self):
        """In the moved BUILD.gn, `":<old_basename>"` label refs are renamed."""
        self._write(
            'components/api_foo/BUILD.gn',
            'static_library("api_request_helper") {\n'
            '}\n'
            'source_set("test_support") {\n'
            '  public_deps = [ ":api_request_helper" ]\n'
            '  deps = [ ":test_support_data" ]\n'
            '}\n')
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/components/api_foo/BUILD.gn'))
        content = self._read('components/api_foo/BUILD.gn')
        self.assertIn('static_library("api_foo")', content)
        self.assertIn(':api_foo"', content)
        self.assertNotIn('api_request_helper', content)
        # Unrelated label refs untouched.
        self.assertIn(':test_support_data"', content)

    def test_build_gn_implicit_target_not_renamed_in_other_files(self):
        """An unrelated BUILD.gn that happens to have a target / label ref
        with the same name as the old directory must NOT be touched."""
        self._write('components/api_foo/BUILD.gn',
                    'static_library("api_request_helper") {\n'
                    '}\n')
        # Unrelated BUILD.gn elsewhere with a same-named target and label.
        self._write(
            'unrelated/BUILD.gn', 'static_library("api_request_helper") {\n'
            '}\n'
            'group("entry") {\n'
            '  deps = [ ":api_request_helper" ]\n'
            '}\n')
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/components/api_foo/BUILD.gn'))
        # Moved BUILD.gn renamed.
        self.assertIn('static_library("api_foo")',
                      self._read('components/api_foo/BUILD.gn'))
        # Unrelated BUILD.gn untouched (both declaration and label ref).
        unrelated = self._read('unrelated/BUILD.gn')
        self.assertIn('static_library("api_request_helper")', unrelated)
        self.assertIn(':api_request_helper"', unrelated)

    def test_build_gn_implicit_target_no_rewrite_when_basename_unchanged(self):
        """Same-basename move (just reparenting) leaves target name alone."""
        self._write('apps/api_request_helper/BUILD.gn',
                    'static_library("api_request_helper") {\n'
                    '}\n')
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/apps/api_request_helper/BUILD.gn'))
        self.assertIn('static_library("api_request_helper")',
                      self._read('apps/api_request_helper/BUILD.gn'))

    def test_build_gn_internal_target_ref_untouched(self):
        """`":api_request_helper"` (internal target) must NOT be rewritten."""
        self._write('components/api_request_helper/BUILD.gn',
                    'public_deps = [ ":api_request_helper" ]\n')
        # Pretend the BUILD.gn is being moved (file already at new location
        # for the test would be more realistic, but token boundary on the
        # leading `"` is what we want to verify).
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/components/api_test/BUILD.gn'))
        self.assertIn('":api_request_helper"',
                      self._read('components/api_request_helper/BUILD.gn'))

    # ----- .gni file move: root reference only -----

    def test_gni_root_reference_rewritten(self):
        """`"//tools/grit/repack.gni"` is rewritten when that file moves."""
        self._write('consumer/BUILD.gn', 'import("//tools/grit/repack.gni")\n')
        update_references(Path('tools/grit/repack.gni'),
                          Path('tools/grit/new_repack.gni'))
        self.assertIn('"//tools/grit/new_repack.gni"',
                      self._read('consumer/BUILD.gn'))

    def test_gni_relative_reference_not_rewritten(self):
        """A relative .gni reference is NOT rewritten on .gni move."""
        # File is in the same dir as the (hypothetically moved) .gni,
        # so a relative reference would be `"repack.gni"`.
        self._write('tools/grit/BUILD.gn', 'import("repack.gni")\n')
        update_references(Path('tools/grit/repack.gni'),
                          Path('tools/grit/new_repack.gni'))
        self.assertIn('"repack.gni"', self._read('tools/grit/BUILD.gn'))

    # ----- C++ file move: root reference in .gn/.gni only -----

    def test_cpp_root_reference_in_build_gn_rewritten(self):
        """`"//brave/foo/bar.h"` in BUILD.gn is rewritten when bar.h moves."""
        self._write(
            'consumer/BUILD.gn', 'sources = [\n'
            '  "//brave/components/api_request_helper/api_request_helper.h"\n'
            ']\n')
        update_references(
            Path('brave/components/api_request_helper/api_request_helper.h'),
            Path('brave/components/api_test/api_test.h'))
        self.assertIn('"//brave/components/api_test/api_test.h"',
                      self._read('consumer/BUILD.gn'))

    def test_cpp_relative_reference_in_build_gn_not_rewritten(self):
        """A relative C++ source-list ref is NOT touched by the new helper.

        (Same-dir BUILD.gn is handled by _update_build_ancestors instead.)
        """
        # Sibling-dir BUILD.gn referencing the moved C++ file via a relative
        # path. The new helper must not rewrite it (per spec: only roots for
        # C++ moves). And _update_build_ancestors won't touch it either,
        # since it only walks ancestor dirs.
        self._write(
            'components/ai_chat/BUILD.gn',
            'sources = [ "../api_request_helper/api_request_helper.h" ]\n')
        update_references(
            Path('brave/components/api_request_helper/api_request_helper.h'),
            Path('brave/components/api_test/api_test.h'))
        self.assertIn('"../api_request_helper/api_request_helper.h"',
                      self._read('components/ai_chat/BUILD.gn'))

    # ----- Excluded dirs are skipped -----

    def test_excluded_out_dir_skipped(self):
        """A BUILD.gn under out/ is not rewritten."""
        self._write('out/Default/gen/BUILD.gn',
                    'deps = [ "//brave/components/api_request_helper" ]\n')
        update_references(Path('brave/components/api_request_helper/BUILD.gn'),
                          Path('brave/components/api_test/BUILD.gn'))
        self.assertIn('"//brave/components/api_request_helper"',
                      self._read('out/Default/gen/BUILD.gn'))


if __name__ == '__main__':
    unittest.main()
