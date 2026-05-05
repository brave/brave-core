#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for git_cr_follow_renames.py."""

import logging
import os
import unittest
from pathlib import Path
from unittest.mock import patch

import _boot  # noqa: F401
import repository
from alias.follow_renames import (
    _collapse_renames,
    _get_chromium_renames,
    _repair_patch_files,
    _repair_plaster_files,
    cmd_follow_renames,
)
from test.fake_chromium_src import FakeChromiumSrc


class _Base(unittest.TestCase):
    """Shared fixture: a fresh fake brave-core + chromium repo per test."""

    def setUp(self) -> None:
        self._repo = FakeChromiumSrc()
        self._repo.setup()
        self.addCleanup(self._repo.cleanup)
        # chromium_src/, rewrite/ created by FakeChromiumSrc.setup()

    @property
    def _brave(self) -> Path:
        return self._repo.brave

    @property
    def _chromium(self) -> Path:
        return self._repo.chromium

    def _chromium_commit(self, rel: str, content: str) -> str:
        """Write, stage, and commit a file in chromium. Returns HEAD hash."""
        self._repo.write_and_stage_file(rel, content, self._chromium)
        return self._repo.commit(f'Add {rel}', self._chromium)

    def _chromium_rename(self, old_rel: str, new_rel: str) -> str:
        """Rename a file in chromium via git mv + commit. Returns HEAD hash."""
        new_path = self._chromium / new_rel
        new_path.parent.mkdir(parents=True, exist_ok=True)
        self._repo._run_git_command(['mv', old_rel, new_rel], self._chromium)
        return self._repo.commit(f'Rename {old_rel} -> {new_rel}',
                                 self._chromium)

    def _chromium_head(self) -> str:
        """Returns HEAD hash of the chromium repo."""
        return self._repo._run_git_command(['rev-parse', 'HEAD'],
                                           self._chromium)

    def _brave_commit(self, rel: str, content: str) -> Path:
        """Write, stage, and commit a file in brave. Returns absolute Path."""
        self._repo.write_and_stage_file(rel, content, self._brave)
        self._repo.commit(f'Add {rel}', self._brave)
        return self._brave / rel

    def _brave_write(self, rel: str, content: str) -> Path:
        """Write a file in brave without staging (for --no-git tests)."""
        path = self._brave / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding='utf-8')
        return path


# ---------------------------------------------------------------------------
# Rename parsing tests
# ---------------------------------------------------------------------------


class ParseTest(_Base):
    """_get_chromium_renames correctly parses git log output."""

    def test_rename_parsed(self) -> None:
        """A single rename commit yields one (old, new) pair."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.h', '// header\n')
        self._chromium_rename('A/foo.h', 'B/foo.h')

        renames = _get_chromium_renames(f'{before}..HEAD')

        self.assertEqual(renames, [(Path('A/foo.h'), Path('B/foo.h'))])

    def test_empty_range_returns_empty_list(self) -> None:
        """A range with no renames yields an empty list."""
        before = self._chromium_head()
        # No changes in chromium.
        renames = _get_chromium_renames(f'{before}..HEAD')
        self.assertEqual(renames, [])

    def test_bare_ref_accepted(self) -> None:
        """A bare ref (no '..') works as a single-commit reference."""
        self._chromium_commit('A/foo.h', '// header\n')
        self._chromium_rename('A/foo.h', 'B/foo.h')

        renames = _get_chromium_renames('HEAD')

        self.assertEqual(renames, [(Path('A/foo.h'), Path('B/foo.h'))])

    def test_non_rename_commit_ignored(self) -> None:
        """A range with only modifications (no renames) yields an empty list."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.h', '// v1\n')
        self._repo.write_and_stage_file('A/foo.h', '// v2\n', self._chromium)
        self._repo.commit('Modify A/foo.h', self._chromium)

        renames = _get_chromium_renames(f'{before}..HEAD')
        self.assertEqual(renames, [])

    def test_chained_renames_collapsed(self) -> None:
        """a→b→c across two commits collapses to a single a→c pair."""
        before = self._chromium_head()
        self._chromium_commit('a.txt', 'content\n')
        self._chromium_rename('a.txt', 'b.txt')
        self._chromium_rename('b.txt', 'c.txt')

        renames = _get_chromium_renames(f'{before}..HEAD')

        self.assertEqual(renames, [(Path('a.txt'), Path('c.txt'))])

    def test_round_trip_rename_dropped(self) -> None:
        """a→b→a across two commits produces an empty list (no net change)."""
        before = self._chromium_head()
        self._chromium_commit('a.json', 'content\n')
        self._chromium_rename('a.json', 'b.json')
        self._chromium_rename('b.json', 'a.json')

        renames = _get_chromium_renames(f'{before}..HEAD')

        self.assertEqual(renames, [])


# ---------------------------------------------------------------------------
# _collapse_renames unit tests
# ---------------------------------------------------------------------------


class CollapseRenamesTest(unittest.TestCase):
    """Unit tests for _collapse_renames (no git needed)."""

    @staticmethod
    def _p(s: str) -> Path:
        return Path(s)

    def test_single_rename_unchanged(self) -> None:
        inp = [(self._p('a'), self._p('b'))]
        self.assertEqual(_collapse_renames(inp),
                         [(self._p('a'), self._p('b'))])

    def test_chain_collapsed(self) -> None:
        """Newest-first input a→b, b→c collapses to a→c."""
        inp = [(self._p('b'), self._p('c')), (self._p('a'), self._p('b'))]
        self.assertEqual(_collapse_renames(inp),
                         [(self._p('a'), self._p('c'))])

    def test_round_trip_dropped(self) -> None:
        """a→b→a collapses to nothing."""
        inp = [(self._p('b'), self._p('a')), (self._p('a'), self._p('b'))]
        self.assertEqual(_collapse_renames(inp), [])

    def test_independent_renames_preserved(self) -> None:
        """Two unrelated renames are both kept."""
        inp = [(self._p('x'), self._p('y')), (self._p('a'), self._p('b'))]
        result = _collapse_renames(inp)
        self.assertIn((self._p('a'), self._p('b')), result)
        self.assertIn((self._p('x'), self._p('y')), result)
        self.assertEqual(len(result), 2)


# ---------------------------------------------------------------------------
# chromium_src/ shadow-file repair tests
# ---------------------------------------------------------------------------


class ShadowFileTest(_Base):
    """_repair_chromium_src moves and patches the brave shadow file."""

    def test_shadow_h_file_moved_guard_updated(self) -> None:
        """chromium_src/.h file moves and its include guard is rewritten."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.h', '// src\n')
        self._brave_commit('chromium_src/A/foo.h',
                           ('#ifndef BRAVE_CHROMIUM_SRC_A_FOO_H_\n'
                            '#define BRAVE_CHROMIUM_SRC_A_FOO_H_\n'
                            '#endif  // BRAVE_CHROMIUM_SRC_A_FOO_H_\n'))
        self._chromium_rename('A/foo.h', 'B/foo.h')

        cmd_follow_renames([f'{before}..HEAD'])

        new_path = self._brave / 'chromium_src' / 'B' / 'foo.h'
        self.assertTrue(new_path.exists())
        self.assertFalse(
            (self._brave / 'chromium_src' / 'A' / 'foo.h').exists())
        content = new_path.read_text(encoding='utf-8')
        self.assertIn('BRAVE_CHROMIUM_SRC_B_FOO_H_', content)
        self.assertNotIn('BRAVE_CHROMIUM_SRC_A_FOO_H_', content)

    def test_shadow_include_line_updated(self) -> None:
        """#include <A/foo.h> in the shadow file becomes #include <B/foo.h>."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.h', '// src\n')
        self._brave_commit('chromium_src/A/foo.h',
                           ('#ifndef BRAVE_CHROMIUM_SRC_A_FOO_H_\n'
                            '#define BRAVE_CHROMIUM_SRC_A_FOO_H_\n'
                            '#include <A/foo.h>\n'
                            '#endif  // BRAVE_CHROMIUM_SRC_A_FOO_H_\n'))
        self._chromium_rename('A/foo.h', 'B/foo.h')

        cmd_follow_renames([f'{before}..HEAD'])

        content = (self._brave / 'chromium_src' / 'B' /
                   'foo.h').read_text(encoding='utf-8')
        self.assertIn('#include <B/foo.h>', content)
        self.assertNotIn('#include <A/foo.h>', content)

    def test_shadow_cc_moved_no_guard(self) -> None:
        """A .cc shadow file moves but no include guard is inserted."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.cc', '// impl\n')
        original = '// implementation\nvoid foo() {}\n'
        self._brave_commit('chromium_src/A/foo.cc', original)
        self._chromium_rename('A/foo.cc', 'B/foo.cc')

        cmd_follow_renames([f'{before}..HEAD'])

        new_path = self._brave / 'chromium_src' / 'B' / 'foo.cc'
        self.assertTrue(new_path.exists())
        content = new_path.read_text(encoding='utf-8')
        self.assertNotIn('#ifndef', content)

    def test_no_shadow_file_is_noop(self) -> None:
        """Chromium rename with no matching shadow file does not raise."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.h', '// src\n')
        self._chromium_rename('A/foo.h', 'B/foo.h')
        # No brave chromium_src/A/foo.h created.

        cmd_follow_renames([f'{before}..HEAD'])  # Must not raise.

        self.assertFalse(
            (self._brave / 'chromium_src' / 'B' / 'foo.h').exists())


# ---------------------------------------------------------------------------
# rewrite/ TOML and patch-file repair tests
# ---------------------------------------------------------------------------


class TomlTest(_Base):
    """_repair_plaster_files moves the TOML and deletes the patch."""

    def test_toml_moved_patch_deleted(self) -> None:
        """rewrite/A/foo.h.toml moves to rewrite/B/foo.h.toml; patch deleted."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.h', '// src\n')
        self._brave_commit('rewrite/A/foo.h.toml', '[substitution]\n')
        self._brave_commit('patches/A-foo.h.patch', 'diff\n')
        self._chromium_rename('A/foo.h', 'B/foo.h')

        cmd_follow_renames([f'{before}..HEAD'])

        self.assertTrue(
            (self._brave / 'rewrite' / 'B' / 'foo.h.toml').exists())
        self.assertFalse(
            (self._brave / 'rewrite' / 'A' / 'foo.h.toml').exists())
        self.assertFalse((self._brave / 'patches' / 'A-foo.h.patch').exists())

    def test_patchinfo_deleted_with_patch(self) -> None:
        """Sibling .patchinfo is removed when the patch is deleted."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.h', '// src\n')
        self._brave_commit('rewrite/A/foo.h.toml', '[substitution]\n')
        self._brave_commit('patches/A-foo.h.patch', 'diff\n')
        patchinfo = self._brave / 'patches' / 'A-foo.h.patchinfo'
        patchinfo.write_text('{}', encoding='utf-8')
        self._chromium_rename('A/foo.h', 'B/foo.h')

        cmd_follow_renames([f'{before}..HEAD'])

        self.assertFalse((self._brave / 'patches' / 'A-foo.h.patch').exists())
        self.assertFalse(patchinfo.exists())

    def test_missing_patch_warns_no_error(self) -> None:
        """TOML exists but patch is absent: warning logged, TOML still moves."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.h', '// src\n')
        self._brave_commit('rewrite/A/foo.h.toml', '[substitution]\n')
        # No patches/A-foo.h.patch.
        self._chromium_rename('A/foo.h', 'B/foo.h')

        with self.assertLogs(level=logging.WARNING):
            cmd_follow_renames([f'{before}..HEAD'])

        self.assertTrue(
            (self._brave / 'rewrite' / 'B' / 'foo.h.toml').exists())

    def test_no_toml_is_noop(self) -> None:
        """A Chromium rename with no TOML in rewrite/ does not raise."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.h', '// src\n')
        self._chromium_rename('A/foo.h', 'B/foo.h')

        cmd_follow_renames([f'{before}..HEAD'])  # Must not raise.


# ---------------------------------------------------------------------------
# Cross-brave-core reference update tests
# ---------------------------------------------------------------------------


class ReferencesTest(_Base):
    """update_references is called for every rename regardless of artefacts."""

    def test_include_updated_even_without_shadow_file(self) -> None:
        """Brave #include of a Chromium path updates with no shadow file."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.h', '// src\n')
        self._brave_commit('other/user.cc', '#include "A/foo.h"\n')
        self._chromium_rename('A/foo.h', 'B/foo.h')

        cmd_follow_renames([f'{before}..HEAD'])

        content = (self._brave / 'other' /
                   'user.cc').read_text(encoding='utf-8')
        self.assertIn('#include "B/foo.h"', content)
        self.assertNotIn('#include "A/foo.h"', content)


# ---------------------------------------------------------------------------
# Multiple renames in a single range
# ---------------------------------------------------------------------------


class MultipleRenamesTest(_Base):
    """All renames in a rev range are processed in order."""

    def test_two_renames_both_processed(self) -> None:
        """Two separate rename commits in the range are both fully repaired."""
        before = self._chromium_head()

        # First rename: A/foo.h → B/foo.h
        self._chromium_commit('A/foo.h', '// foo\n')
        self._brave_commit('chromium_src/A/foo.h',
                           ('#ifndef BRAVE_CHROMIUM_SRC_A_FOO_H_\n'
                            '#define BRAVE_CHROMIUM_SRC_A_FOO_H_\n'
                            '#endif  // BRAVE_CHROMIUM_SRC_A_FOO_H_\n'))
        self._chromium_rename('A/foo.h', 'B/foo.h')

        # Second rename: C/bar.h → D/bar.h
        self._chromium_commit('C/bar.h', '// bar\n')
        self._brave_commit('rewrite/C/bar.h.toml', '[substitution]\n')
        self._brave_commit('patches/C-bar.h.patch', 'diff\n')
        self._chromium_rename('C/bar.h', 'D/bar.h')

        cmd_follow_renames([f'{before}..HEAD'])

        # First rename: shadow file moved.
        self.assertTrue(
            (self._brave / 'chromium_src' / 'B' / 'foo.h').exists())
        self.assertFalse(
            (self._brave / 'chromium_src' / 'A' / 'foo.h').exists())

        # Second rename: TOML moved, patch deleted.
        self.assertTrue(
            (self._brave / 'rewrite' / 'D' / 'bar.h.toml').exists())
        self.assertFalse((self._brave / 'patches' / 'C-bar.h.patch').exists())


# ---------------------------------------------------------------------------
# --no-git flag tests
# ---------------------------------------------------------------------------


class NoGitTest(_Base):
    """--no-git uses Path.rename/unlink instead of git mv/rm."""

    def test_no_git_shadow_uses_rename(self) -> None:
        """--no-git: shadow file moved via Path.rename, not staged in git."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.h', '// src\n')
        self._brave_commit('chromium_src/A/foo.h', '// shadow\n')
        self._chromium_rename('A/foo.h', 'B/foo.h')

        cmd_follow_renames(['--no-git', f'{before}..HEAD'])

        self.assertTrue(
            (self._brave / 'chromium_src' / 'B' / 'foo.h').exists())
        self.assertFalse(
            (self._brave / 'chromium_src' / 'A' / 'foo.h').exists())
        # Nothing should be staged.
        staged = repository.brave.run_git('diff', '--cached', '--name-only')
        self.assertEqual(staged, '')

    def test_no_git_toml_patch_uses_unlink(self) -> None:
        """--no-git: patch deleted with Path.unlink, no git rm."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.h', '// src\n')
        # Stage + commit the TOML but only write (don't commit) the patch
        # so that git rm would fail on it.
        self._brave_commit('rewrite/A/foo.h.toml', '[substitution]\n')
        patch_file = self._brave / 'patches' / 'A-foo.h.patch'
        patch_file.write_text('dummy\n', encoding='utf-8')
        self._chromium_rename('A/foo.h', 'B/foo.h')

        (self._brave / 'rewrite' / 'B').mkdir(parents=True, exist_ok=True)
        cmd_follow_renames(['--no-git', f'{before}..HEAD'])

        self.assertFalse(patch_file.exists())


# ---------------------------------------------------------------------------
# Subdirectory CWD — patch deletion bug
# ---------------------------------------------------------------------------


class SubdirPatchDeletionTest(_Base):
    """git rm for patch deletion must succeed when CWD is not brave root.

    _repair_plaster_files passes str(patch_file.relative_to(brave_root)) to
    `git rm`, but git runs from the Python process CWD (not brave root).  When
    CWD is a brave subdirectory the path resolves to CWD/patches/… which does
    not exist, so git rm exits 128 and raises CalledProcessError.

    Note: the bug is unreachable through cmd_follow_renames from a subdirectory
    because repository.chromium.run_git uses a CWD-relative '-C ..' path and
    fails before _repair_plaster_files is reached.  The test calls the function
    directly to keep the regression focused.
    """

    def test_patch_deleted_from_subdirectory(self) -> None:
        self._brave_commit('rewrite/A/foo.h.toml', '[substitution]\n')
        self._brave_commit('patches/A-foo.h.patch', 'diff\n')
        (self._brave / 'rewrite' / 'B').mkdir(parents=True, exist_ok=True)

        subdir = self._brave / 'chromium_src'
        subdir.mkdir(exist_ok=True)
        saved = os.getcwd()
        try:
            os.chdir(str(subdir))
            # Must not raise; patch must be deleted.
            _repair_plaster_files(Path('A/foo.h'),
                                  Path('B/foo.h'),
                                  no_git=False)
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

    def test_patch_created_at_new_location(self) -> None:
        """Plaster writes patches/B-foo.cc.patch after an upstream rename."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.cc', 'void old_func() {}\n')
        self._brave_commit('rewrite/A/foo.cc.toml', self._SUBST_TOML)
        self._brave_commit('patches/A-foo.cc.patch', 'old patch\n')
        self._chromium_rename('A/foo.cc', 'B/foo.cc')

        cmd_follow_renames([f'{before}..HEAD'])

        old_patch = self._brave / 'patches' / 'A-foo.cc.patch'
        new_patch = self._brave / 'patches' / 'B-foo.cc.patch'
        self.assertFalse(old_patch.exists())
        self.assertTrue(new_patch.exists())
        content = new_patch.read_text(encoding='utf-8')
        self.assertIn('-void old_func() {}', content)
        self.assertIn('+void new_func() {}', content)
        staged = repository.brave.run_git('diff', '--cached', '--name-only')
        self.assertIn('patches/B-foo.cc.patch', staged)

    def test_no_run_plaster_skips_patch_creation(self) -> None:
        """--no-run-plaster: no new patch file is created after TOML move."""
        before = self._chromium_head()
        self._chromium_commit('A/foo.cc', 'void old_func() {}\n')
        self._brave_commit('rewrite/A/foo.cc.toml', self._SUBST_TOML)
        self._brave_commit('patches/A-foo.cc.patch', 'old patch\n')
        self._chromium_rename('A/foo.cc', 'B/foo.cc')

        cmd_follow_renames(['--no-run-plaster', f'{before}..HEAD'])

        self.assertFalse((self._brave / 'patches' / 'B-foo.cc.patch').exists())


# ---------------------------------------------------------------------------
# .patch file repair tests
# ---------------------------------------------------------------------------


class PatchFileRepairTest(_Base):
    """_repair_patch_files renames .patch files and attempts re-apply."""

    _OLD_REL = 'A/foo.cc'
    _NEW_REL = 'B/foo.cc'
    _FILE_CONTENT = 'int x = 1;\n'
    # Real brave patch format with diff --git header and index line.
    _PATCH_TEMPLATE = ('diff --git a/{path} b/{path}\n'
                       'index abc1234..def5678 100644\n'
                       '--- a/{path}\n'
                       '+++ b/{path}\n'
                       '@@ -1 +1 @@\n'
                       '-int x = 1;\n'
                       '+int x = 2;\n')

    def _setup_rename(self) -> str:
        """Commits file + patch in their repos, renames in chromium."""
        before = self._chromium_head()
        self._chromium_commit(self._OLD_REL, self._FILE_CONTENT)
        patch_content = self._PATCH_TEMPLATE.format(path=self._OLD_REL)
        self._brave_commit('patches/A-foo.cc.patch', patch_content)
        self._chromium_rename(self._OLD_REL, self._NEW_REL)
        return before

    def test_patch_renamed_to_new_path(self) -> None:
        """Patch file is renamed from the old chromium name to the new one."""
        before = self._setup_rename()

        cmd_follow_renames([f'{before}..HEAD'])

        self.assertFalse((self._brave / 'patches' / 'A-foo.cc.patch').exists())
        self.assertTrue((self._brave / 'patches' / 'B-foo.cc.patch').exists())

    def test_patch_headers_updated(self) -> None:
        """diff --git, --- and +++ headers are rewritten to the new path."""
        before = self._setup_rename()

        cmd_follow_renames([f'{before}..HEAD'])

        content = (self._brave / 'patches' /
                   'B-foo.cc.patch').read_text(encoding='utf-8')
        self.assertIn('diff --git a/B/foo.cc b/B/foo.cc', content)
        self.assertIn('--- a/B/foo.cc', content)
        self.assertIn('+++ b/B/foo.cc', content)
        self.assertNotIn('A/foo.cc', content)

    def test_patch_applied_and_unstaged(self) -> None:
        """Patch is applied to the working tree but not left staged."""
        before = self._setup_rename()

        cmd_follow_renames([f'{before}..HEAD'])

        # Working tree has the patched content.
        content = (self._chromium / 'B' / 'foo.cc').read_text(encoding='utf-8')
        self.assertIn('int x = 2;', content)
        # The change must not be staged in chromium.
        staged = repository.chromium.run_git('diff', '--cached', '--name-only')
        self.assertEqual(staged, '')

    def test_no_patch_is_noop(self) -> None:
        """No patch for the renamed file → step is a no-op, no error."""
        before = self._chromium_head()
        self._chromium_commit(self._OLD_REL, self._FILE_CONTENT)
        self._chromium_rename(self._OLD_REL, self._NEW_REL)

        cmd_follow_renames([f'{before}..HEAD'])  # Must not raise.

        self.assertFalse((self._brave / 'patches' / 'B-foo.cc.patch').exists())

    def test_failed_apply_warns(self) -> None:
        """A patch whose context doesn't match the file logs a warning."""
        before = self._chromium_head()
        self._chromium_commit(self._OLD_REL, 'completely_different\n')
        patch_content = self._PATCH_TEMPLATE.format(path=self._OLD_REL)
        self._brave_commit('patches/A-foo.cc.patch', patch_content)
        self._chromium_rename(self._OLD_REL, self._NEW_REL)

        with self.assertLogs(level=logging.WARNING):
            cmd_follow_renames([f'{before}..HEAD'])

        # Patch is still renamed even though apply failed.
        self.assertTrue((self._brave / 'patches' / 'B-foo.cc.patch').exists())

    def test_no_git_renames_without_staging(self) -> None:
        """--no-git moves the patch via Path.rename; nothing is staged."""
        before = self._setup_rename()

        cmd_follow_renames(['--no-git', f'{before}..HEAD'])

        self.assertFalse((self._brave / 'patches' / 'A-foo.cc.patch').exists())
        self.assertTrue((self._brave / 'patches' / 'B-foo.cc.patch').exists())
        staged = repository.brave.run_git('diff', '--cached', '--name-only')
        self.assertNotIn('B-foo.cc.patch', staged)

    def test_plaster_patch_not_double_processed(self) -> None:
        """Patch deleted by _repair_plaster_files is not re-renamed here."""
        _SUBST_TOML = ('[[substitution]]\n'
                       'description = "test"\n'
                       'pattern = "old_func"\n'
                       'replace = "new_func"\n')
        before = self._chromium_head()
        self._chromium_commit(self._OLD_REL, 'void old_func() {}\n')
        self._brave_commit('rewrite/A/foo.cc.toml', _SUBST_TOML)
        self._brave_commit('patches/A-foo.cc.patch', 'stale patch\n')
        self._chromium_rename(self._OLD_REL, self._NEW_REL)

        cmd_follow_renames([f'{before}..HEAD'])

        # Plaster created a proper patch; _repair_patch_files must not
        # overwrite it with the stale content.
        new_patch = self._brave / 'patches' / 'B-foo.cc.patch'
        self.assertTrue(new_patch.exists())
        content = new_patch.read_text(encoding='utf-8')
        self.assertNotEqual(content, 'stale patch\n')

    def test_repair_patch_files_direct_call(self) -> None:
        """_repair_patch_files can be called directly."""
        self._chromium_commit(self._OLD_REL, self._FILE_CONTENT)
        patch_content = self._PATCH_TEMPLATE.format(path=self._OLD_REL)
        self._brave_commit('patches/A-foo.cc.patch', patch_content)
        self._chromium_rename(self._OLD_REL, self._NEW_REL)

        _repair_patch_files(Path(self._OLD_REL),
                            Path(self._NEW_REL),
                            no_git=False)

        self.assertFalse((self._brave / 'patches' / 'A-foo.cc.patch').exists())
        self.assertTrue((self._brave / 'patches' / 'B-foo.cc.patch').exists())


if __name__ == '__main__':
    unittest.main()
