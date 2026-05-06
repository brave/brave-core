#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for git_cr_mv.py — all cases from spec §6.2."""

import logging
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import _boot  # noqa: F401
import repository
from alias.mv import cmd_mv
from test.fake_chromium_src import FakeChromiumSrc
from user_validation_error import UserValidationError


class _Base(unittest.TestCase):
    """Shared fixture: a fresh fake brave-core repo for every test method."""

    def setUp(self) -> None:
        self._repo = FakeChromiumSrc()
        self._repo.setup()
        self.addCleanup(self._repo.cleanup)
        # chromium_src/, rewrite/ created by FakeChromiumSrc.setup()
        # patches/ is created by FakeChromiumRepo.__init__
        # `npm run format` is not available in the fake repo; suppress it.
        self._format_mock = patch('alias.mv._run_format').start()
        self.addCleanup(patch.stopall)

    @property
    def _brave(self) -> Path:
        return self._repo.brave

    def _commit(self, rel: str, content: str) -> Path:
        """Write, stage, and commit a file in the brave repo."""
        self._repo.write_and_stage_file(rel, content, self._brave)
        self._repo.commit(f'Add {rel}', self._brave)
        return self._brave / rel

    def _write(self, rel: str, content: str) -> Path:
        """Write a file without staging (for --no-git tests)."""
        path = self._brave / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding='utf-8')
        return path


# ---------------------------------------------------------------------------
# Validation tests
# ---------------------------------------------------------------------------


class ValidationTest(_Base):
    """Error conditions that must raise UserValidationError (spec §6.2)."""

    def test_source_not_found(self) -> None:
        with self.assertRaises(UserValidationError):
            cmd_mv(['nonexistent.h', 'dest.h'])

    def test_dest_parent_missing_no_mkdir(self) -> None:
        self._commit('src.h', '// src\n')
        with self.assertRaises(UserValidationError):
            cmd_mv(['src.h', 'missing_dir/dest.h'])

    def test_dest_parent_created_with_mkdir(self) -> None:
        self._commit('src.h', '// src\n')
        cmd_mv(['--mkdir', 'src.h', 'new_dir/dest.h'])
        self.assertTrue((self._brave / 'new_dir' / 'dest.h').exists())
        self.assertFalse((self._brave / 'src.h').exists())

    def test_dest_exists_as_file(self) -> None:
        self._commit('src.h', '// src\n')
        self._commit('dest.h', '// dest\n')
        with self.assertRaises(UserValidationError):
            cmd_mv(['src.h', 'dest.h'])

    def test_cwd_outside_brave_core(self) -> None:
        saved = os.getcwd()
        try:
            os.chdir(tempfile.gettempdir())
            with self.assertRaises(UserValidationError):
                cmd_mv(['src.h', 'dest.h'])
        finally:
            os.chdir(saved)

    def test_rewrite_toml_to_outside_rewrite(self) -> None:
        self._commit('rewrite/A/foo.h.toml', '[substitution]\n')
        with self.assertRaises(UserValidationError):
            cmd_mv(['rewrite/A/foo.h.toml', 'other/foo.h.toml'])


# ---------------------------------------------------------------------------
# Include guard tests (Step 2)
# ---------------------------------------------------------------------------


class GuardTest(_Base):
    """Include-guard behaviour for moved .h files (spec §6.2 Step 2)."""

    def test_guard_updated_in_three_places(self) -> None:
        """Moving a .h with a correct guard updates #ifndef, #define, #endif."""
        self._commit('foo/bar.h', ('// Copyright\n'
                                   '#ifndef BRAVE_FOO_BAR_H_\n'
                                   '#define BRAVE_FOO_BAR_H_\n'
                                   '\n'
                                   'class Foo {};\n'
                                   '\n'
                                   '#endif  // BRAVE_FOO_BAR_H_\n'))
        cmd_mv(['--mkdir', 'foo/bar.h', 'baz/bar.h'])
        new_content = (self._brave / 'baz' /
                       'bar.h').read_text(encoding='utf-8')
        self.assertEqual(new_content.count('BRAVE_BAZ_BAR_H_'), 3)
        self.assertNotIn('BRAVE_FOO_BAR_H_', new_content)

    def test_guard_count_warning_but_file_written(self) -> None:
        """Moving a .h with 4 guard occurrences warns but writes the file."""
        # 4 occurrences: comment + ifndef + define + endif-comment
        self._commit('foo/bar.h', ('// Ref: BRAVE_FOO_BAR_H_\n'
                                   '#ifndef BRAVE_FOO_BAR_H_\n'
                                   '#define BRAVE_FOO_BAR_H_\n'
                                   '#endif  // BRAVE_FOO_BAR_H_\n'))
        with self.assertLogs(level=logging.WARNING):
            cmd_mv(['--mkdir', 'foo/bar.h', 'baz/bar.h'])
        new_content = (self._brave / 'baz' /
                       'bar.h').read_text(encoding='utf-8')
        # The replacement ran (old guard gone, new guard present).
        self.assertNotIn('BRAVE_FOO_BAR_H_', new_content)
        self.assertIn('BRAVE_BAZ_BAR_H_', new_content)

    def test_guard_inserted_when_absent(self) -> None:
        """Moving a .h with no guard inserts a complete guard block."""
        self._commit('foo/bar.h', ('// Copyright\n'
                                   '\n'
                                   'class Foo {};\n'))
        cmd_mv(['--mkdir', 'foo/bar.h', 'baz/bar.h'])
        new_content = (self._brave / 'baz' /
                       'bar.h').read_text(encoding='utf-8')
        self.assertIn('#ifndef BRAVE_BAZ_BAR_H_', new_content)
        self.assertIn('#define BRAVE_BAZ_BAR_H_', new_content)
        self.assertIn('#endif  // BRAVE_BAZ_BAR_H_', new_content)
        # Original body still present.
        self.assertIn('class Foo {};', new_content)
        # Guard comes before the class.
        guard_pos = new_content.index('#ifndef BRAVE_BAZ_BAR_H_')
        class_pos = new_content.index('class Foo {};')
        self.assertLess(guard_pos, class_pos)

    def test_cc_file_gets_no_guard(self) -> None:
        """Moving a .cc file does not touch include guards."""
        original = '// Simple source\nvoid foo() {}\n'
        self._commit('foo/bar.cc', original)
        cmd_mv(['--mkdir', 'foo/bar.cc', 'baz/bar.cc'])
        new_content = (self._brave / 'baz' /
                       'bar.cc').read_text(encoding='utf-8')
        self.assertNotIn('#ifndef', new_content)
        self.assertNotIn('#define', new_content)


# ---------------------------------------------------------------------------
# Shadow include tests (Step 3)
# ---------------------------------------------------------------------------


class ShadowIncludeTest(_Base):
    """chromium_src/ shadow-file include update (spec §6.2 Step 3)."""

    def test_shadow_include_updated(self) -> None:
        """Moving chromium_src/A/foo.h updates the upstream include."""
        self._commit('chromium_src/A/foo.h',
                     ('// Shadow\n'
                      '#ifndef BRAVE_CHROMIUM_SRC_A_FOO_H_\n'
                      '#define BRAVE_CHROMIUM_SRC_A_FOO_H_\n'
                      '\n'
                      '#include <A/foo.h>\n'
                      '\n'
                      '#endif  // BRAVE_CHROMIUM_SRC_A_FOO_H_\n'))
        cmd_mv(['--mkdir', 'chromium_src/A/foo.h', 'chromium_src/B/foo.h'])
        new_content = (self._brave / 'chromium_src' / 'B' /
                       'foo.h').read_text(encoding='utf-8')
        self.assertIn('#include <B/foo.h>', new_content)
        self.assertNotIn('#include <A/foo.h>', new_content)

    def test_no_shadow_include_when_absent(self) -> None:
        """Moving a shadow file without the upstream include is a no-op."""
        original = ('// No upstream include\n'
                    '#ifndef BRAVE_CHROMIUM_SRC_A_BAR_H_\n'
                    '#define BRAVE_CHROMIUM_SRC_A_BAR_H_\n'
                    '// code\n'
                    '#endif  // BRAVE_CHROMIUM_SRC_A_BAR_H_\n')
        self._commit('chromium_src/A/bar.h', original)
        cmd_mv(['--mkdir', 'chromium_src/A/bar.h', 'chromium_src/B/bar.h'])
        new_content = (self._brave / 'chromium_src' / 'B' /
                       'bar.h').read_text(encoding='utf-8')
        # No angle-bracket include of A/bar.h should appear.
        self.assertNotIn('#include <A/bar.h>', new_content)

    def test_non_cpp_shadow_no_content_edit(self) -> None:
        """Moving a non-C++ file under chromium_src/ does not edit content."""
        original = '# Python script\nprint("hello")\n'
        self._commit('chromium_src/A/script.py', original)
        cmd_mv([
            '--mkdir', 'chromium_src/A/script.py', 'chromium_src/B/script.py'
        ])
        new_content = (self._brave / 'chromium_src' / 'B' /
                       'script.py').read_text(encoding='utf-8')
        self.assertEqual(new_content, original)


# ---------------------------------------------------------------------------
# Cross-brave-core reference rewriting tests (Step 4)
# ---------------------------------------------------------------------------


class ReferencesTest(_Base):
    """Reference rewriting across brave-core (spec §6.2 Step 4)."""

    def _setup_source_file(self) -> None:
        """Create and commit the file that will be moved."""
        self._commit('foo/bar.h', '// moved header\n')

    def test_quoted_include_in_h_and_cc_updated(self) -> None:
        self._setup_source_file()
        # Files that reference the moved file.
        self._commit('other/user.h', '#include "brave/foo/bar.h"\n')
        self._commit('other/user.cc', '#include "brave/foo/bar.h"\n')

        cmd_mv(['--mkdir', 'foo/bar.h', 'baz/bar.h'])

        h_content = (self._brave / 'other' /
                     'user.h').read_text(encoding='utf-8')
        cc_content = (self._brave / 'other' /
                      'user.cc').read_text(encoding='utf-8')
        self.assertIn('#include "brave/baz/bar.h"', h_content)
        self.assertNotIn('#include "brave/foo/bar.h"', h_content)
        self.assertIn('#include "brave/baz/bar.h"', cc_content)
        self.assertNotIn('#include "brave/foo/bar.h"', cc_content)

    def test_import_in_mm_updated(self) -> None:
        self._setup_source_file()
        self._commit('other/user.mm', '#import "brave/foo/bar.h"\n')

        cmd_mv(['--mkdir', 'foo/bar.h', 'baz/bar.h'])

        mm_content = (self._brave / 'other' /
                      'user.mm').read_text(encoding='utf-8')
        self.assertIn('#import "brave/baz/bar.h"', mm_content)
        self.assertNotIn('#import "brave/foo/bar.h"', mm_content)

    def test_comment_reference_updated(self) -> None:
        self._setup_source_file()
        self._commit('other/user.cc',
                     '// See brave/foo/bar.h for details\nvoid f() {}\n')

        cmd_mv(['--mkdir', 'foo/bar.h', 'baz/bar.h'])

        cc_content = (self._brave / 'other' /
                      'user.cc').read_text(encoding='utf-8')
        self.assertIn('brave/baz/bar.h', cc_content)
        self.assertNotIn('brave/foo/bar.h', cc_content)

    def test_build_gn_entry_updated(self) -> None:
        self._setup_source_file()
        # BUILD.gn at brave_root — entry uses path relative to brave_root.
        self._commit('BUILD.gn', 'sources = [\n  "foo/bar.h",\n]\n')

        cmd_mv(['--mkdir', 'foo/bar.h', 'baz/bar.h'])

        build_content = (self._brave / 'BUILD.gn').read_text(encoding='utf-8')
        self.assertIn('"baz/bar.h"', build_content)
        self.assertNotIn('"foo/bar.h"', build_content)

    def test_angle_bracket_chromium_include_untouched(self) -> None:
        self._setup_source_file()
        self._commit('other/user.cc', '#include <base/feature_list.h>\n')

        cmd_mv(['--mkdir', 'foo/bar.h', 'baz/bar.h'])

        cc_content = (self._brave / 'other' /
                      'user.cc').read_text(encoding='utf-8')
        self.assertIn('#include <base/feature_list.h>', cc_content)

    def test_directory_move_rewrites_gn_references(self) -> None:
        """Moving a directory rewrites root and relative GN references in
        unrelated BUILD.gn files."""
        self._commit(
            'components/api_request_helper/BUILD.gn',
            'static_library("api_request_helper") {\n'
            '  sources = [ "api_request_helper.cc" ]\n'
            '}\n'
            'source_set("test_support") {\n'
            '  public_deps = [ ":api_request_helper" ]\n'
            '}\n')
        self._commit('components/api_request_helper/api_request_helper.cc',
                     '// impl\n')
        # Consumer at an unrelated dir uses a root reference.
        self._commit(
            'browser/BUILD.gn',
            'deps = [ "//brave/components/api_request_helper:test_support" ]\n'
        )
        # Sibling under components/ uses a relative reference.
        self._commit('components/ai_chat/BUILD.gn',
                     'deps = [ "../api_request_helper" ]\n')

        cmd_mv([
            '--mkdir', 'components/api_request_helper', 'components/api_test'
        ])

        browser_content = (self._brave / 'browser' /
                           'BUILD.gn').read_text(encoding='utf-8')
        self.assertIn('"//brave/components/api_test:test_support"',
                      browser_content)
        self.assertNotIn('api_request_helper', browser_content)

        sibling_content = (self._brave / 'components' / 'ai_chat' /
                           'BUILD.gn').read_text(encoding='utf-8')
        self.assertIn('"../api_test"', sibling_content)
        self.assertNotIn('api_request_helper', sibling_content)

        # The moved BUILD.gn's directory-name target is renamed too — both
        # the declaration and same-file label refs. The `api_request_helper.cc`
        # source-list entry is a stable same-dir reference and must remain.
        moved_content = (self._brave / 'components' / 'api_test' /
                         'BUILD.gn').read_text(encoding='utf-8')
        self.assertIn('static_library("api_test")', moved_content)
        self.assertIn(':api_test"', moved_content)
        self.assertNotIn('"api_request_helper"', moved_content)
        self.assertNotIn(':api_request_helper"', moved_content)


# ---------------------------------------------------------------------------
# Plaster TOML handling tests (Step 5)
# ---------------------------------------------------------------------------


class PlasterTest(_Base):
    """Plaster TOML and patch-file handling (spec §6.2 Step 5)."""

    def test_patch_deleted_on_toml_move(self) -> None:
        """Moving rewrite/A/foo.h.toml deletes the corresponding patch."""
        self._commit('rewrite/A/foo.h.toml', '[substitution]\n')
        self._commit('patches/A-foo.h.patch', 'diff --git a/foo\n')

        cmd_mv(['--mkdir', 'rewrite/A/foo.h.toml', 'rewrite/B/foo.h.toml'])

        self.assertFalse((self._brave / 'patches' / 'A-foo.h.patch').exists())
        self.assertTrue(
            (self._brave / 'rewrite' / 'B' / 'foo.h.toml').exists())

    def test_patchinfo_deleted_with_patch(self) -> None:
        """Sibling .patchinfo is removed when the patch is deleted."""
        self._commit('rewrite/A/foo.h.toml', '[substitution]\n')
        self._commit('patches/A-foo.h.patch', 'diff --git a/foo\n')
        patchinfo = self._brave / 'patches' / 'A-foo.h.patchinfo'
        patchinfo.write_text('{}', encoding='utf-8')

        cmd_mv(['--mkdir', 'rewrite/A/foo.h.toml', 'rewrite/B/foo.h.toml'])

        self.assertFalse((self._brave / 'patches' / 'A-foo.h.patch').exists())
        self.assertFalse(patchinfo.exists())

    def test_missing_patch_warns_no_error(self) -> None:
        """Moving a TOML whose patch is absent warns but does not raise."""
        self._commit('rewrite/A/foo.h.toml', '[substitution]\n')
        # No patches/A-foo.h.patch created.
        with self.assertLogs(level=logging.WARNING):
            cmd_mv(['--mkdir', 'rewrite/A/foo.h.toml', 'rewrite/B/foo.h.toml'])
        # Command completed; TOML is at new location.
        self.assertTrue(
            (self._brave / 'rewrite' / 'B' / 'foo.h.toml').exists())

    def test_directory_move_handles_all_tomls(self) -> None:
        """Moving a rewrite/ directory moves all TOMLs and deletes patches."""
        self._commit('rewrite/A/foo.h.toml', '[substitution]\n')
        self._commit('rewrite/A/bar.h.toml', '[substitution]\n')
        self._commit('patches/A-foo.h.patch', 'diff\n')
        self._commit('patches/A-bar.h.patch', 'diff\n')

        cmd_mv(['rewrite/A', 'rewrite/B'])

        self.assertFalse((self._brave / 'patches' / 'A-foo.h.patch').exists())
        self.assertFalse((self._brave / 'patches' / 'A-bar.h.patch').exists())
        self.assertTrue(
            (self._brave / 'rewrite' / 'B' / 'foo.h.toml').exists())
        self.assertTrue(
            (self._brave / 'rewrite' / 'B' / 'bar.h.toml').exists())


# ---------------------------------------------------------------------------
# CWD-relative argument tests
# ---------------------------------------------------------------------------


class CwdRelativeTest(_Base):
    """Source and destination resolved relative to CWD (spec §6.2)."""

    def test_paths_resolved_from_subdirectory(self) -> None:
        """Invoking from a subdirectory resolves paths relative to that dir."""
        self._commit('foo/bar.h', '// header\n')
        (self._brave / 'baz').mkdir(exist_ok=True)

        saved = os.getcwd()
        try:
            os.chdir(str(self._brave / 'foo'))
            cmd_mv(['bar.h', '../baz/bar.h'])
        finally:
            os.chdir(saved)

        self.assertTrue((self._brave / 'baz' / 'bar.h').exists())
        self.assertFalse((self._brave / 'foo' / 'bar.h').exists())


# ---------------------------------------------------------------------------
# --no-git flag tests
# ---------------------------------------------------------------------------


class NoGitTest(_Base):
    """--no-git flag uses Path.rename/unlink instead of git mv/rm."""

    def test_no_git_uses_rename_not_staged(self) -> None:
        """--no-git moves the file without staging the change."""
        self._commit('src.h', '// content\n')

        cmd_mv(['--no-git', 'src.h', 'dst.h'])

        self.assertTrue((self._brave / 'dst.h').exists())
        self.assertFalse((self._brave / 'src.h').exists())
        # Nothing should be staged.
        staged = repository.brave.run_git('diff', '--cached', '--name-only')
        self.assertEqual(staged, '')

    def test_no_git_patch_deletion_uses_unlink(self) -> None:
        """--no-git deletes the patch with Path.unlink() rather than git rm."""
        self._commit('rewrite/A/foo.h.toml', '[substitution]\n')
        # Patch written but not committed — git rm would fail on it.
        patch_file = self._brave / 'patches' / 'A-foo.h.patch'
        patch_file.write_text('dummy\n', encoding='utf-8')

        (self._brave / 'rewrite' / 'B').mkdir(parents=True, exist_ok=True)
        cmd_mv(['--no-git', 'rewrite/A/foo.h.toml', 'rewrite/B/foo.h.toml'])

        self.assertFalse(patch_file.exists())


# ---------------------------------------------------------------------------
# Subdirectory CWD — patch deletion bug
# ---------------------------------------------------------------------------


class SubdirPatchDeletionTest(_Base):
    """git rm for patch deletion must succeed when CWD is not brave root."""

    def test_patch_deleted_from_subdirectory(self) -> None:
        """Moving a TOML from a brave subdirectory deletes the patch file.

        Regression for the bug where _step5_plaster passed a brave-root-relative
        path to `git rm` but git ran from CWD (a subdirectory), causing git rm
        to look for the file relative to the subdirectory and fail.
        """
        self._commit('rewrite/A/foo.h.toml', '[substitution]\n')
        self._commit('patches/A-foo.h.patch', 'diff --git a/foo\n')
        subdir = self._brave / 'chromium_src'
        subdir.mkdir(exist_ok=True)

        saved = os.getcwd()
        try:
            os.chdir(str(subdir))
            # Must not raise; patch must be deleted.
            cmd_mv([
                '--mkdir', '../rewrite/A/foo.h.toml', '../rewrite/B/foo.h.toml'
            ])
        finally:
            os.chdir(saved)

        self.assertTrue(
            (self._brave / 'rewrite' / 'B' / 'foo.h.toml').exists())
        self.assertFalse((self._brave / 'patches' / 'A-foo.h.patch').exists())


# ---------------------------------------------------------------------------
# Plaster apply-after-move tests
# ---------------------------------------------------------------------------


class PlasterApplyTest(_Base):
    """plaster.apply() is called after each TOML move by default."""

    _SUBST_TOML = ('[[substitution]]\n'
                   'description = "Replace old_func"\n'
                   'pattern = "old_func"\n'
                   'replace = "new_func"\n')

    def _commit_chromium(self, rel: str, content: str) -> None:
        self._repo.write_and_stage_file(rel, content, self._repo.chromium)
        self._repo.commit(f'Add {rel}', self._repo.chromium)

    def test_patch_created_at_new_location(self) -> None:
        """After TOML move, plaster writes a new patch at
        patches/B-foo.cc.patch."""
        self._commit_chromium('B/foo.cc', 'void old_func() {}\n')
        self._commit('rewrite/A/foo.cc.toml', self._SUBST_TOML)
        self._commit('patches/A-foo.cc.patch', 'old patch\n')

        cmd_mv(['--mkdir', 'rewrite/A/foo.cc.toml', 'rewrite/B/foo.cc.toml'])

        old_patch = self._brave / 'patches' / 'A-foo.cc.patch'
        new_patch = self._brave / 'patches' / 'B-foo.cc.patch'
        self.assertFalse(old_patch.exists())
        self.assertTrue(new_patch.exists())
        content = new_patch.read_text(encoding='utf-8')
        self.assertIn('-void old_func() {}', content)
        self.assertIn('+void new_func() {}', content)
        staged = repository.brave.run_git('diff', '--cached', '--name-only')
        self.assertIn('patches/B-foo.cc.patch', staged)

    def test_no_upstream_source_logs_warning(self) -> None:
        """Warning is logged when the chromium source for the new path is
        absent."""
        # No chromium file at B/foo.cc — plaster cannot read the source.
        self._commit('rewrite/A/foo.cc.toml', self._SUBST_TOML)
        self._commit('patches/A-foo.cc.patch', 'old patch\n')

        with self.assertLogs(level=logging.WARNING) as cm:
            cmd_mv(
                ['--mkdir', 'rewrite/A/foo.cc.toml', 'rewrite/B/foo.cc.toml'])

        self.assertTrue(any('plaster failed' in msg for msg in cm.output))
        self.assertTrue(
            (self._brave / 'rewrite' / 'B' / 'foo.cc.toml').exists())
        self.assertFalse((self._brave / 'patches' / 'B-foo.cc.patch').exists())

    def test_no_run_plaster_skips_patch_creation(self) -> None:
        """--no-run-plaster: no new patch file is created after TOML move."""
        self._commit_chromium('B/foo.cc', 'void old_func() {}\n')
        self._commit('rewrite/A/foo.cc.toml', self._SUBST_TOML)
        self._commit('patches/A-foo.cc.patch', 'old patch\n')

        cmd_mv([
            '--mkdir', '--no-run-plaster', 'rewrite/A/foo.cc.toml',
            'rewrite/B/foo.cc.toml'
        ])

        self.assertFalse((self._brave / 'patches' / 'B-foo.cc.patch').exists())


# ---------------------------------------------------------------------------
# Format step
# ---------------------------------------------------------------------------


class FormatTest(_Base):
    """`npm run format` runs after a successful move unless --no-format."""

    def test_format_called_by_default(self) -> None:
        self._commit('foo/bar.h', '// header\n')
        cmd_mv(['--mkdir', 'foo/bar.h', 'baz/bar.h'])
        self._format_mock.assert_called_once()

    def test_format_skipped_with_no_format_flag(self) -> None:
        self._commit('foo/bar.h', '// header\n')
        cmd_mv(['--mkdir', '--no-format', 'foo/bar.h', 'baz/bar.h'])
        self._format_mock.assert_not_called()


if __name__ == '__main__':
    unittest.main()
