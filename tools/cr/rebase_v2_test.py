#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `rebase_v2`.

This file is organised in two layers:

* Unit tests (`EntryLineParseTest`, `PatternMatchTest`, `RewritePlanTest`,
  `MessageWriterTest`) exercise the pure file-in / file-out transforms
  with no git involved.

* `RebaseV2ExecuteTest` covers `brockit.Rebase.execute(..., v2=True)`
  end-to-end against a real git tree built by `FakeChromiumRepo`, mirroring
  the integration layer used by `rebase_test.RebaseExecuteTest`.
"""

import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import brockit
import rebase_v2
from test.fake_chromium_repo import FakeChromiumRepo


class EntryLineParseTest(unittest.TestCase):
    """Tests for `rebase_v2.EntryLine.parse`."""

    def test_parse_simple_pick(self):
        entry_line = rebase_v2.EntryLine.parse(
            'pick aaa # [cr148] Feature A\n')
        self.assertEqual(entry_line.command, 'pick')
        self.assertEqual(entry_line.hash, 'aaa')
        self.assertEqual(entry_line.message, '[cr148] Feature A')
        self.assertIsNone(entry_line.subcommand)
        self.assertIsNone(entry_line.note)

    def test_parse_fixup_dash_c_multi_word_command(self):
        """`fixup -C <hash>` keeps both tokens as the command."""
        entry_line = rebase_v2.EntryLine.parse(
            'fixup -C 2c334294014 # amend! [cr149] Permit cross-host\n')
        self.assertEqual(entry_line.command, 'fixup -C')
        self.assertEqual(entry_line.hash, '2c334294014')
        self.assertEqual(entry_line.message, '[cr149] Permit cross-host')
        self.assertEqual(entry_line.subcommand, ['amend'])

    def test_parse_reassign_subcommand(self):
        """`reassign!<hash>!` is captured as two subcommand tokens."""
        entry_line = rebase_v2.EntryLine.parse(
            'pick zzz # reassign!c080270dd7e! [cr149] Fix bookmark bar.\n')
        self.assertEqual(entry_line.subcommand, ['reassign', 'c080270dd7e'])
        self.assertEqual(entry_line.message, '[cr149] Fix bookmark bar.')

    def test_parse_reassign_subcommand_hash_starting_with_digit(self):
        """`reassign!<hash>!` where `<hash>` starts with a digit must
        still capture both tokens. Regression: an earlier regex required
        every `!`-separated token to start with a letter, which silently
        dropped digit-leading hashes -- the subcommand collapsed to
        `['reassign']` and the hash leaked into `message`, causing
        `rewrite_plan` to orphan-drop the reassign."""
        entry_line = rebase_v2.EntryLine.parse(
            'pick 04d3a656bb1 # reassign!859ab9caa74! [cr150][ios] Add '
            '//brave/ios/browser/svg to visibility for //third_party/expat\n')
        self.assertEqual(entry_line.subcommand, ['reassign', '859ab9caa74'])
        self.assertEqual(
            entry_line.message,
            '[cr150][ios] Add //brave/ios/browser/svg to visibility for '
            '//third_party/expat')
        self.assertEqual(entry_line.reassign_target_hash, '859ab9caa74')

    def test_parse_trailing_empty_note(self):
        """A trailing ` # empty` marker is split into `note` and stripped
        out of the comment portion."""
        entry_line = rebase_v2.EntryLine.parse(
            'pick zzz # reassign!bbb! [cr148] Feature B # empty\n')
        self.assertEqual(entry_line.note, 'empty')
        self.assertEqual(entry_line.message, '[cr148] Feature B')
        self.assertEqual(entry_line.subcommand, ['reassign', 'bbb'])

    def test_parse_blank_line_returns_none(self):
        self.assertIsNone(rebase_v2.EntryLine.parse('\n'))
        self.assertIsNone(rebase_v2.EntryLine.parse('   \n'))

    def test_parse_pure_comment_returns_none(self):
        self.assertIsNone(rebase_v2.EntryLine.parse('# Rebase plan\n'))
        self.assertIsNone(rebase_v2.EntryLine.parse('   # indented\n'))

    def test_parse_malformed_line_raises(self):
        """A non-empty, non-comment line that doesn't fit the
        `<cmd> <hash> # <msg>` shape raises `EditorRecoverableFailure`
        so the caller can punt to the user editor."""
        with self.assertRaises(rebase_v2.EditorRecoverableFailure):
            rebase_v2.EntryLine.parse('garbage\n')
        with self.assertRaises(rebase_v2.EditorRecoverableFailure):
            rebase_v2.EntryLine.parse('pickonly # no hash\n')

    def test_entry_type_classification_and_properties(self):
        """`entry_type` is filled in by `EntryLine.__init__` during
        construction;
        the `is_*` properties are derived from it and stay mutually
        exclusive."""
        EntryType = rebase_v2.EntryType

        pinned = rebase_v2.EntryLine.parse(
            'pick aaa # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n')
        self.assertIs(pinned.entry_type, EntryType.PINNED)
        self.assertTrue(pinned.is_pinned)
        self.assertFalse(pinned.is_recyclable)
        self.assertFalse(pinned.is_reassignment)
        self.assertEqual(pinned.pinned_group, 'version')

        recyclable = rebase_v2.EntryLine.parse(
            'pick bbb # Update patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.\n')
        self.assertIs(recyclable.entry_type, EntryType.RECYCLABLE)
        self.assertTrue(recyclable.is_recyclable)

        reassign = rebase_v2.EntryLine.parse(
            'pick zzz # reassign!bbb! [cr148] Feature B\n')
        self.assertIs(reassign.entry_type, EntryType.REASSIGNMENT)
        self.assertTrue(reassign.is_reassignment)

        regular = rebase_v2.EntryLine.parse('pick ccc # [cr148] Feature A\n')
        self.assertIs(regular.entry_type, EntryType.DEFAULT)
        self.assertFalse(regular.is_pinned)
        self.assertFalse(regular.is_recyclable)
        self.assertFalse(regular.is_reassignment)

    def test_reassign_target_hash_property(self):
        """`reassign_target_hash` returns the hash from `reassign!<hash>!`
        for reassignment commits, `None` when the reassignment has no
        hash, and raises `NotImplementedError` for non-reassignment
        commits."""
        full = rebase_v2.EntryLine.parse(
            'pick zzz # reassign!c080270dd7e! [cr149] Fix bookmark bar.\n')
        self.assertEqual(full.reassign_target_hash, 'c080270dd7e')

        no_hash = rebase_v2.EntryLine.parse(
            'pick zzz # reassign! [cr149] Some subject.\n')
        self.assertTrue(no_hash.is_reassignment)
        self.assertIsNone(no_hash.reassign_target_hash)

        regular = rebase_v2.EntryLine.parse('pick aaa # [cr148] Feature A\n')
        with self.assertRaises(NotImplementedError):
            _ = regular.reassign_target_hash

    def test_fixup_of_pinned_classified_as_pinned(self):
        """A `fixup -C` whose `amend!` subject is pinned still ends up
        `entry_type=PINNED` with the right `pinned_group`. The
        autosquash marker is peeled off into `subcommand` by
        `EntryLine.parse`, so the regular pinned classifier on the bare
        subject does the work -- no special-case handling needed."""
        entry_line = rebase_v2.EntryLine.parse(
            'fixup -C bbb # amend! Update from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1\n')
        self.assertIs(entry_line.entry_type, rebase_v2.EntryType.PINNED)
        self.assertTrue(entry_line.is_pinned)
        self.assertEqual(entry_line.pinned_group, 'version')


class PatternMatchTest(unittest.TestCase):
    """Tests for the cr-tag stripper, pinned classifier, and recyclable
    detector."""

    def test_strip_cr_tag_removes_simple_tag(self):
        self.assertEqual(rebase_v2._strip_cr_tag('[cr149] Foo'), 'Foo')

    def test_strip_cr_tag_removes_chained_tags(self):
        """`[cr149][ios] ` collapses into nothing."""
        self.assertEqual(rebase_v2._strip_cr_tag('[cr149][ios] Foo'), 'Foo')
        self.assertEqual(rebase_v2._strip_cr_tag('[cr149][ios][extra] Foo'),
                         'Foo')

    def test_strip_cr_tag_leaves_unrelated_tag(self):
        """`[unrelated] ` is not a cr-tag; left in place."""
        self.assertEqual(rebase_v2._strip_cr_tag('[unrelated] Foo'),
                         '[unrelated] Foo')

    def test_strip_cr_tag_noop_on_untagged(self):
        self.assertEqual(
            rebase_v2._strip_cr_tag('Update from Chromium 1.0.0.0 to '
                                    'Chromium 1.0.0.1'),
            'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1')

    def test_strip_autosquash_prefixes_handles_stacks(self):
        """`fixup! ` / `amend! ` / `squash! ` prefixes can stack when a
        commit fixes up an already-fixup commit. `_strip_autosquash_prefixes`
        peels off any number of them at the start."""
        self.assertEqual(
            rebase_v2._strip_autosquash_prefixes(
                'fixup! Conflict-resolved patches from Chromium 1.0.0.0 '
                'to Chromium 1.0.0.1.'),
            'Conflict-resolved patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.')
        self.assertEqual(
            rebase_v2._strip_autosquash_prefixes(
                'fixup! fixup! fixup! Conflict-resolved patches from '
                'Chromium 1.0.0.0 to Chromium 1.0.0.1.'),
            'Conflict-resolved patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.')
        # Mixed prefixes also strip cleanly.
        self.assertEqual(
            rebase_v2._strip_autosquash_prefixes(
                'amend! fixup! squash! [cr149] IWYU fixes.'),
            '[cr149] IWYU fixes.')
        # Untagged subjects pass through unchanged.
        self.assertEqual(
            rebase_v2._strip_autosquash_prefixes('Just a plain subject'),
            'Just a plain subject')

    def test_pinned_match_survives_stacked_autosquash_prefixes(self):
        """A pinned subject buried under stacked `fixup!` prefixes still
        classifies via the autosquash-prefix stripping pre-pass."""
        self.assertEqual(
            rebase_v2.get_pinned_group_for_subject(
                'fixup! fixup! Conflict-resolved patches from Chromium '
                '1.0.0.0 to Chromium 1.0.0.1.'), 'conflict')
        self.assertEqual(
            rebase_v2.get_pinned_group_for_subject(
                'fixup! [cr149] IWYU fixes.'), 'iwyu')
        # Stripping is composable with `[crNNN]` -- the autosquash
        # strip happens first, then `[crNNN]`.
        self.assertEqual(
            rebase_v2.get_pinned_group_for_subject(
                'amend! [cr149] `gnrt` run for Chromium 149.0.7827.5'), 'gnrt')

    def test_pinned_patterns_match_canonical_subjects(self):
        """Every pinned pattern matches its canonical subject form, both
        tagged and untagged where applicable."""
        cases = [
            ('version', 'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1'),
            ('plaster_reruns',
             'Apply-fixed 🩹 patches from Chromium 1.0.0.0 to '
             'Chromium 1.0.0.1.'),
            ('conflict', 'Conflict-resolved patches from Chromium 1.0.0.0 to '
             'Chromium 1.0.0.1.'),
            ('gnrt', '[cr149] `gnrt` run for Chromium 149.0.7827.5'),
            ('iwyu', '[cr149] IWYU fixes.'),
            ('resource_ids', '[cr149] Bump resource_ids'),
            ('lit_mangler', '[cr149] Update Lit mangler snapshot'),
            ('disable_tests', '[cr149] Disable failing upstream tests'),
        ]
        for expected_id, subject in cases:
            with self.subTest(subject=subject):
                self.assertEqual(
                    rebase_v2.get_pinned_group_for_subject(subject),
                    expected_id)

    def test_pinned_iwyu_untagged_form_also_matches(self):
        """The `[cr*]`-tagged patterns must also match without the tag."""
        self.assertEqual(rebase_v2.get_pinned_group_for_subject('IWYU fixes.'),
                         'iwyu')
        self.assertEqual(
            rebase_v2.get_pinned_group_for_subject('Bump resource_ids'),
            'resource_ids')

    def test_pinned_rejects_near_misses(self):
        """Near-miss subjects should not match any pinned pattern."""
        self.assertIsNone(
            rebase_v2.get_pinned_group_for_subject(
                'Update from Chromium 1.0.0.0 to '
                'Chromium 1.0.0.1 (revert pending)'))
        self.assertIsNone(
            rebase_v2.get_pinned_group_for_subject('[cr149] Feature work'))
        self.assertIsNone(
            rebase_v2.get_pinned_group_for_subject(
                'Update from Chromium abc to '
                'Chromium def'))

    def test_pinned_rejects_non_four_segment_versions(self):
        """Chromium versions must be exactly four dot-separated numeric
        segments. Three-segment or five-segment shapes are rejected."""
        self.assertIsNone(
            rebase_v2.get_pinned_group_for_subject(
                'Update from Chromium 1.0.0 to '
                'Chromium 1.0.1'))
        self.assertIsNone(
            rebase_v2.get_pinned_group_for_subject(
                'Update from Chromium 1.0.0.0.0 to '
                'Chromium 1.0.0.0.1'))
        self.assertIsNone(
            rebase_v2.get_pinned_group_for_subject(
                '`gnrt` run for Chromium 149.0.7827'))

    def test_recyclable_matches_canonical(self):
        self.assertTrue(
            rebase_v2.is_recyclable('Update patches from Chromium 1.0.0.0 '
                                    'to Chromium 1.0.0.1.'))
        self.assertTrue(
            rebase_v2.is_recyclable('Updated strings for Chromium 1.0.0.1.'))

    def test_recyclable_rejects_near_misses(self):
        self.assertFalse(
            rebase_v2.is_recyclable('Update from Chromium 1.0.0.0 to '
                                    'Chromium 1.0.0.1'))
        self.assertFalse(rebase_v2.is_recyclable('[cr149] Feature work'))


class RewritePlanTest(unittest.TestCase):
    """Tests for `rebase_v2.rewrite_plan`.

    Split along the `(pinned_squashed, discard_recyclable)` flag matrix.
    """

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self._tmp_root = Path(tmp.name)

    def _todo(self, content: str) -> Path:
        path = self._tmp_root / 'git-rebase-todo'
        path.write_text(content)
        return path

    # ----- pinned_squashed=False, discard_recyclable=False ------------------

    def test_identity_transform_keeps_everything_in_order(self):
        """With no flags, `rewrite` is a no-op: the file is not parsed
        and not rewritten. Useful as a fast-path for callers that pass
        the flags through indiscriminately."""
        path = self._todo(
            'pick aaa # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'pick bbb # [cr148] Feature A\n'
            'pick ccc # Update patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.\n'
            'pick zzz # reassign!bbb! [cr148] Feature A\n')

        rebase_v2.rewrite_plan(todo_file=path)

        self.assertEqual(
            path.read_text(),
            'pick aaa # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'pick bbb # [cr148] Feature A\n'
            'pick ccc # Update patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.\n'
            'pick zzz # reassign!bbb! [cr148] Feature A\n')

    def test_identity_drops_comments_and_blanks_when_parsing(self):
        """When any flag forces a parse pass (here `discard_recyclable`),
        comments and blank lines are filtered out. With no flags set
        `rewrite` is a no-op (covered by
        `test_identity_transform_keeps_everything_in_order`)."""
        path = self._todo('# Rebase plan generated by git\n'
                          '\n'
                          'pick aaa # [cr148] Feature A\n'
                          '\n')

        rebase_v2.rewrite_plan(todo_file=path, discard_recyclable=True)

        self.assertEqual(path.read_text(), 'pick aaa # [cr148] Feature A\n')

    # ----- pinned_squashed=False, discard_recyclable=True -------------------

    def test_discard_recyclable_drops_only_recyclable_lines(self):
        """Recyclable lines are dropped; pinned, reassign, and regular
        commits stay in their arrival order, untouched."""
        path = self._todo(
            'pick aaa # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'pick bbb # [cr148] Feature A\n'
            'pick ccc # Update patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.\n'
            'pick ddd # Updated strings for Chromium 1.0.0.1.\n'
            'pick zzz # reassign!bbb! [cr148] Feature A\n')

        rebase_v2.rewrite_plan(todo_file=path, discard_recyclable=True)

        self.assertEqual(
            path.read_text(),
            'pick aaa # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'pick bbb # [cr148] Feature A\n'
            'pick zzz # reassign!bbb! [cr148] Feature A\n')

    # ----- pinned_squashed=True, discard_recyclable=False -------------------

    def test_squashed_collapses_consecutive_version_bumps(self):
        path = self._todo(
            'pick aaa # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'pick bbb # Update from Chromium 1.0.0.1 to Chromium 1.0.0.2\n'
            'pick ccc # Update from Chromium 1.0.0.2 to Chromium 1.0.0.3\n')

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        self.assertEqual(
            path.read_text(),
            'pick aaa # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'squash bbb # Update from Chromium 1.0.0.1 to Chromium 1.0.0.2\n'
            'squash ccc # Update from Chromium 1.0.0.2 to Chromium 1.0.0.3\n')

    def test_squashed_reorders_pinned_groups_by_priority(self):
        """Pinned groups emit in the priority order:
        version → plaster_reruns → conflict → gnrt → iwyu → resource_ids
        → lit_mangler → disable_tests, regardless of arrival order."""
        path = self._todo(
            'pick aaa # [cr148] Disable failing upstream tests\n'
            'pick bbb # [cr148] Update Lit mangler snapshot\n'
            'pick ccc # [cr148] Bump resource_ids\n'
            'pick ddd # [cr148] IWYU fixes.\n'
            'pick eee # [cr148] `gnrt` run for Chromium 1.0.0.1\n'
            'pick fff # Conflict-resolved patches from Chromium 1.0.0.0 '
            'to Chromium 1.0.0.1.\n'
            'pick ggg # Apply-fixed 🩹 patches from Chromium 1.0.0.0 '
            'to Chromium 1.0.0.1.\n'
            'pick hhh # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n')

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        self.assertEqual(
            path.read_text(),
            'pick hhh # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'pick ggg # Apply-fixed 🩹 patches from Chromium 1.0.0.0 '
            'to Chromium 1.0.0.1.\n'
            'pick fff # Conflict-resolved patches from Chromium 1.0.0.0 '
            'to Chromium 1.0.0.1.\n'
            'pick eee # [cr148] `gnrt` run for Chromium 1.0.0.1\n'
            'pick ddd # [cr148] IWYU fixes.\n'
            'pick ccc # [cr148] Bump resource_ids\n'
            'pick bbb # [cr148] Update Lit mangler snapshot\n'
            'pick aaa # [cr148] Disable failing upstream tests\n')

    def test_squashed_keeps_recyclable_lines_in_place(self):
        """Recyclable lines are NOT grouped or moved with pinned commits.
        They stay interleaved with regular commits in arrival order."""
        path = self._todo(
            'pick aaa # [cr148] Feature A\n'
            'pick bbb # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'pick ccc # Update patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.\n'
            'pick ddd # [cr148] Feature B\n')

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        # Pinned (`bbb`) jumps to the top; recyclable (`ccc`) and regulars
        # (`aaa`, `ddd`) keep their relative order in the tail.
        self.assertEqual(
            path.read_text(),
            'pick bbb # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'pick aaa # [cr148] Feature A\n'
            'pick ccc # Update patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.\n'
            'pick ddd # [cr148] Feature B\n')

    def test_squashed_groups_fixup_of_pinned_with_target(self):
        """A `fixup -C` whose `amend!` subject matches a pinned pattern is
        grouped with the pinned target, in arrival order."""
        path = self._todo(
            'pick aaa # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'fixup -C bbb # amend! Update from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1\n'
            'pick ccc # [cr148] Feature A\n')

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        # The fixup line is in the version group as `squash`; its original
        # `fixup -C` command is replaced by `squash` (it is no longer the
        # first commit in the group).
        self.assertEqual(
            path.read_text(),
            'pick aaa # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'squash bbb # amend! Update from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1\n'
            'pick ccc # [cr148] Feature A\n')

    def test_squashed_groups_squash_autosquash_of_pinned(self):
        """A `squash <hash> # squash! <pinned-subject>` line -- distinct
        from `fixup`/`fixup -C` but the same intent -- is also grouped with
        the pinned target. A `pick` line with the same autosquash marker
        is not (it has to be a non-`pick` autosquash command)."""
        path = self._todo(
            'pick aaa # [cr148] `gnrt` run for Chromium 1.0.0.1\n'
            'squash bbb # squash! [cr148] `gnrt` run for Chromium 1.0.0.1\n'
            'pick ccc # [cr148] Feature A\n'
            'pick ddd # squash! [cr148] `gnrt` run for Chromium 1.0.0.1\n')

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        # `bbb` (squash autosquash) groups with `aaa`; `ddd` (pick command,
        # despite the same marker) is treated as a regular pinned commit
        # and also groups with `aaa` via its message. Both non-first
        # entries in the group are forced to `squash`.
        self.assertEqual(
            path.read_text(),
            'pick aaa # [cr148] `gnrt` run for Chromium 1.0.0.1\n'
            'squash bbb # squash! [cr148] `gnrt` run for Chromium 1.0.0.1\n'
            'squash ddd # squash! [cr148] `gnrt` run for Chromium 1.0.0.1\n'
            'pick ccc # [cr148] Feature A\n')

    def test_squashed_reassign_matched_by_hash(self):
        """Reassign with hash `bbb` finds the `pick bbb` line and inserts
        itself directly above it; target becomes `squash`."""
        path = self._todo('pick aaa # [cr148] Feature A\n'
                          'pick bbb # [cr148] Feature B\n'
                          'pick zzz # reassign!bbb! [cr148] Feature B\n')

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        self.assertEqual(
            path.read_text(), 'pick aaa # [cr148] Feature A\n'
            'pick zzz # reassign!bbb! [cr148] Feature B\n'
            'squash bbb # [cr148] Feature B\n')

    def test_squashed_reassign_target_hash_starts_with_digit(self):
        """Real-world regression: a `reassign!<hash>!` where the hash
        starts with a digit (which is the case for ~6/16 short hashes on
        average) must still be matched to its target. Previously the
        subcommand regex required each token to start with a letter, so
        the hash never made it into `subcommand` -- the reassign was
        silently dropped and the target was left untouched."""
        path = self._todo(
            'pick 7606227b5da # [cr150][ios] Add backend promo provider\n'
            'pick 859ab9caa74 # [cr150][ios] Add //brave/ios/browser/svg '
            'to visibility for //third_party/expat\n'
            'pick 04d3a656bb1 # reassign!859ab9caa74! [cr150][ios] Add '
            '//brave/ios/browser/svg to visibility for '
            '//third_party/expat # empty\n'
            'pick 9c160e03412 # [cr150] wip-reassing-bug\n')

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        self.assertEqual(
            path.read_text(),
            'pick 7606227b5da # [cr150][ios] Add backend promo provider\n'
            'pick 04d3a656bb1 # reassign!859ab9caa74! [cr150][ios] Add '
            '//brave/ios/browser/svg to visibility for '
            '//third_party/expat # empty\n'
            'squash 859ab9caa74 # [cr150][ios] Add //brave/ios/browser/svg '
            'to visibility for //third_party/expat\n'
            'pick 9c160e03412 # [cr150] wip-reassing-bug\n')

    def test_squashed_reassign_falls_back_to_message_match(self):
        """Non-matching hash, matching message: the hash lookup misses,
        so the reassign falls back to matching by commit subject."""
        path = self._todo('pick aaa # [cr148] Feature A\n'
                          'pick bbb # [cr148] Feature B\n'
                          'pick zzz # reassign!xxx! [cr148] Feature B\n')

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        self.assertEqual(
            path.read_text(), 'pick aaa # [cr148] Feature A\n'
            'pick zzz # reassign!xxx! [cr148] Feature B\n'
            'squash bbb # [cr148] Feature B\n')

    def test_squashed_reassign_hash_match_wins_over_message_match(self):
        """Matching hash, non-matching message: hash lookup has priority.
        Even though the reassign's message also matches *another* commit
        in the plan, the hash-matching commit is the one that gets
        squashed."""
        path = self._todo('pick aaa # Some completely different subject\n'
                          'pick bbb # [cr148] Feature B\n'
                          'pick zzz # reassign!aaa! [cr148] Feature B\n')

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        # `aaa` matches the reassign's hash and is squashed; `bbb`
        # (whose subject matches the reassign's message) is left
        # untouched because hash-match took priority.
        self.assertEqual(
            path.read_text(), 'pick zzz # reassign!aaa! [cr148] Feature B\n'
            'squash aaa # Some completely different subject\n'
            'pick bbb # [cr148] Feature B\n')

    def test_squashed_reassign_without_hash_uses_message_match_only(self):
        """No-hash-in-subcommand (`reassign! <subject>` with a single
        `!`): the hash lookup is skipped entirely and the reassign is
        placed via message-based matching."""
        path = self._todo('pick aaa # [cr148] Feature A\n'
                          'pick bbb # [cr148] Feature B\n'
                          'pick zzz # reassign! [cr148] Feature B\n')

        # Sanity check: the parser sees only one subcommand token, so
        # `reassign_target_hash` is None and the hash-lookup branch in
        # `add_reassign_before_target` is skipped.
        reassign = rebase_v2.EntryLine.parse(
            'pick zzz # reassign! [cr148] Feature B')
        self.assertEqual(reassign.subcommand, ['reassign'])
        self.assertIsNone(reassign.reassign_target_hash)

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        self.assertEqual(
            path.read_text(), 'pick aaa # [cr148] Feature A\n'
            'pick zzz # reassign! [cr148] Feature B\n'
            'squash bbb # [cr148] Feature B\n')

    def test_squashed_reassign_to_pinned_target_is_dropped(self):
        """Reassignment of a pinned commit is not supported. The reassign
        line is treated as orphaned and silently dropped; the pinned
        commit is emitted normally."""
        path = self._todo(
            'pick aaa # [cr148] Feature A\n'
            'pick bbb # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'pick zzz # reassign!bbb! Update from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1\n')

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        self.assertEqual(
            path.read_text(),
            'pick bbb # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'pick aaa # [cr148] Feature A\n')

    def test_squashed_orphan_reassign_dropped(self):
        """A reassign whose hash and subject both miss is silently
        dropped."""
        path = self._todo(
            'pick aaa # [cr148] Feature A\n'
            'pick zzz # reassign!xxx! [cr148] Completely unrelated subject\n')

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        self.assertEqual(path.read_text(), 'pick aaa # [cr148] Feature A\n')

    def test_squashed_reassign_with_empty_suffix(self):
        """The ` # empty` suffix on a reassign must be stripped before the
        message-based lookup."""
        path = self._todo(
            'pick aaa # [cr148] Feature A\n'
            'pick bbb # [cr148] Feature B\n'
            'pick zzz # reassign!xxx! [cr148] Feature B # empty\n')

        rebase_v2.rewrite_plan(todo_file=path, pinned_squashed=True)

        self.assertEqual(
            path.read_text(), 'pick aaa # [cr148] Feature A\n'
            'pick zzz # reassign!xxx! [cr148] Feature B # empty\n'
            'squash bbb # [cr148] Feature B\n')

    # ----- pinned_squashed=True, discard_recyclable=True --------------------

    def test_combined_pinned_grouped_and_recyclable_dropped(self):
        path = self._todo(
            'pick aaa # [cr148] Feature A\n'
            'pick bbb # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'pick ccc # Update patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.\n'
            'pick ddd # [cr148] Feature B\n')

        rebase_v2.rewrite_plan(todo_file=path,
                               pinned_squashed=True,
                               discard_recyclable=True)

        self.assertEqual(
            path.read_text(),
            'pick bbb # Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'pick aaa # [cr148] Feature A\n'
            'pick ddd # [cr148] Feature B\n')

    def test_combined_reassign_orphaned_when_target_is_dropped_recyclable(
            self):
        """A reassign targeting a recyclable that gets dropped becomes
        orphaned in phase B."""
        path = self._todo(
            'pick aaa # [cr148] Feature A\n'
            'pick ccc # Updated strings for Chromium 1.0.0.1.\n'
            'pick zzz # reassign!ccc! Updated strings for Chromium 1.0.0.1.\n')

        rebase_v2.rewrite_plan(todo_file=path,
                               pinned_squashed=True,
                               discard_recyclable=True)

        self.assertEqual(path.read_text(), 'pick aaa # [cr148] Feature A\n')

    # ----- Cross-cutting -----------------------------------------------------

    def test_malformed_line_raises_editor_recoverable(self):
        """A malformed line surfaces as `EditorRecoverableFailure` --
        only when `rewrite` actually parses the file (i.e. at least one
        flag is set). The brockit dispatch catches this and punts to the
        editor with the failure reason prepended as a `#`-comment."""
        path = self._todo('pick aaa # [cr148] Feature A\n'
                          'totally malformed line without hash or hash\n')

        with self.assertRaises(rebase_v2.EditorRecoverableFailure):
            rebase_v2.rewrite_plan(todo_file=path, discard_recyclable=True)


class MessageWriterTest(unittest.TestCase):
    """Tests for `rebase_v2.MessageWriter`.

    All fixtures use git's verbose squash-editor layout: each
    `# This is the …` / `# The commit message #N will be skipped:` header
    is followed by a blank line, then the content. This matches the file
    git actually writes to `$GIT_EDITOR` during squash rebases.
    """

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self._tmp_root = Path(tmp.name)

    def _file(self, content: str) -> Path:
        path = self._tmp_root / 'COMMIT_EDITMSG'
        path.write_text(content)
        return path

    @staticmethod
    def _messages(writer: 'rebase_v2.MessageWriter') -> list:
        return [block.full_message for block in writer.blocks]

    def test_pinned_squash_writes_trailing_message(self):
        """Block 1 is a version-bump subject; block 2 is the next bump.
        Parse accepts (pinned first block); `rewrite_with_last_message`
        writes the trailing block's message back to the file."""
        path = self._file('# This is the 1st commit message:\n'
                          '\n'
                          'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
                          '\n'
                          '# This is the commit message #2:\n'
                          '\n'
                          'Update from Chromium 1.0.0.1 to Chromium 1.0.0.2\n')

        writer = rebase_v2.MessageWriter.parse(path)

        self.assertEqual(self._messages(writer), [
            'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1',
            'Update from Chromium 1.0.0.1 to Chromium 1.0.0.2',
        ])

        writer.rewrite_with_last_message()
        self.assertEqual(path.read_text(),
                         'Update from Chromium 1.0.0.1 to Chromium 1.0.0.2\n')

    def test_reassignment_squash_writes_target_message(self):
        """A `reassign!` first content line in block 1 is accepted by
        parse. The reassign block stays in `blocks`, but it's block 2
        (carrying the original commit's subject and body) that gets
        written back -- because it's the last block."""
        path = self._file('# This is the 1st commit message:\n'
                          '\n'
                          'reassign!bbb! [cr148] Feature B\n'
                          '\n'
                          '# This is the commit message #2:\n'
                          '\n'
                          '[cr148] Feature B\n'
                          '\n'
                          'Original body line.\n')

        writer = rebase_v2.MessageWriter.parse(path)

        self.assertEqual(self._messages(writer), [
            'reassign!bbb! [cr148] Feature B',
            '[cr148] Feature B\n\nOriginal body line.',
        ])

        writer.rewrite_with_last_message()
        self.assertEqual(path.read_text(),
                         '[cr148] Feature B\n\nOriginal body line.\n')

    def test_unclassifiable_first_block_raises(self):
        """Block 1 whose first content line is neither `reassign!` nor a
        pinned pattern raises `EditorRecoverableFailure`. The exception
        message names the offending content line so the caller can
        surface it to the user."""
        path = self._file('# This is the 1st commit message:\n'
                          '\n'
                          '[cr148] Some regular feature commit\n')

        with self.assertRaises(rebase_v2.EditorRecoverableFailure) as ctx:
            rebase_v2.MessageWriter.parse(path)
        self.assertIn('[cr148] Some regular feature commit',
                      str(ctx.exception))

    def test_parse_failure_wrapped_in_cannot_classify(self):
        """An empty file (no blocks parse out) also raises
        `EditorRecoverableFailure`."""
        path = self._file('')
        with self.assertRaises(rebase_v2.EditorRecoverableFailure):
            rebase_v2.MessageWriter.parse(path)

    def test_squash_marker_merges_into_previous_block(self):
        """A block whose autosquash note is `# squash! ...` is merged
        into the previous block's `full_message` rather than becoming a
        new block."""
        path = self._file(
            '# This is the 1st commit message:\n'
            '\n'
            'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            '\n'
            '# This is the commit message #2:\n'
            '\n'
            '# squash! Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            '\n'
            'Some extra detail\n')

        writer = rebase_v2.MessageWriter.parse(path)

        # Single merged block: block 1's content with the squash content
        # appended on a new line.
        self.assertEqual(self._messages(writer), [
            'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            'Some extra detail',
        ])

    def test_will_be_skipped_block_contributes_nothing(self):
        """The skipped-block header is treated as a pure delimiter; its
        contents never reach `blocks` regardless of marker."""
        path = self._file(
            '# This is the 1st commit message:\n'
            '\n'
            'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            '\n'
            '# The commit message #2 will be skipped:\n'
            '\n'
            '# fixup! Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            '\n'
            '# This is the commit message #3:\n'
            '\n'
            'Update from Chromium 1.0.0.1 to Chromium 1.0.0.2\n')

        writer = rebase_v2.MessageWriter.parse(path)

        self.assertEqual(self._messages(writer), [
            'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1',
            'Update from Chromium 1.0.0.1 to Chromium 1.0.0.2',
        ])

    def test_bare_fixup_block_is_skipped(self):
        """When the user has stacked `fixup!` commits on top of an
        already-fixup commit, git emits the marker-only `# fixup!`
        (or `# fixup! fixup! ...`) inside a regular `# This is the
        commit message #N:` header with no body. Those blocks carry
        nothing to merge -- treat them like `will be skipped:` rather
        than failing the parse."""
        path = self._file(
            '# This is a combination of 4 commits.\n'
            '# This is the 1st commit message:\n'
            '\n'
            'Conflict-resolved patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.\n'
            '\n'
            '# This is the commit message #2:\n'
            '\n'
            '# fixup! Conflict-resolved patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.\n'
            '\n'
            '# This is the commit message #3:\n'
            '\n'
            '# fixup! fixup! Conflict-resolved patches from Chromium 1.0.0.0 '
            'to Chromium 1.0.0.1.\n'
            '\n'
            '# This is the commit message #4:\n'
            '\n'
            'Conflict-resolved patches from Chromium 1.0.0.2 to '
            'Chromium 1.0.0.3.\n')

        writer = rebase_v2.MessageWriter.parse(path)

        self.assertEqual(self._messages(writer), [
            'Conflict-resolved patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.',
            'Conflict-resolved patches from Chromium 1.0.0.2 to '
            'Chromium 1.0.0.3.',
        ])

    def test_git_footer_terminates_parsing(self):
        """Anything after `# Please enter the commit message ...` is
        ignored, including stale `# This is ...` lines from git's status
        echo."""
        path = self._file(
            '# This is the 1st commit message:\n'
            '\n'
            'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1\n'
            '\n'
            '# Please enter the commit message for your changes. Lines '
            'starting\n'
            '# with \'#\' will be ignored, and an empty message aborts the '
            'commit.\n'
            '#\n'
            '# interactive rebase in progress; onto 5e0f42a4157\n')

        writer = rebase_v2.MessageWriter.parse(path)

        self.assertEqual(self._messages(writer),
                         ['Update from Chromium 1.0.0.0 to Chromium 1.0.0.1'])

    def test_hand_off_to_editor_invokes_supplied_editor(self):
        """The caller-supplied `editor` is what gets spawned, via
        `terminal.run(..., interactive=True)` so the editor inherits the
        parent tty. `hand_off_to_editor` does not consult any env var
        of its own -- editor resolution is the caller's job (typically
        `rebase_v2.get_git_editor`)."""
        path = self._file('original content\n')

        with patch.object(rebase_v2.terminal, 'run') as mock_run:
            rebase_v2.hand_off_to_editor(path, reason='', editor='nano')
        mock_run.assert_called_once_with(['nano', str(path)], interactive=True)

    def test_hand_off_to_editor_splits_multi_token_editor(self):
        """A multi-token `editor` (e.g. `code --wait`, or a flag with
        embedded whitespace inside quotes) is split via `shlex.split`
        before being passed to `terminal.run`, so each token lands in
        its own argv slot."""
        path = self._file('original content\n')

        with patch.object(rebase_v2.terminal, 'run') as mock_run:
            rebase_v2.hand_off_to_editor(path, reason='', editor='code --wait')
        mock_run.assert_called_once_with(
            ['code', '--wait', str(path)], interactive=True)

        with patch.object(rebase_v2.terminal, 'run') as mock_run:
            rebase_v2.hand_off_to_editor(
                path, reason='', editor='my_editor --feature="with space"')
        mock_run.assert_called_once_with(
            ['my_editor', '--feature=with space',
             str(path)], interactive=True)

    def test_hand_off_to_editor_prepends_reason_as_comment(self):
        """When `reason` is supplied, every line is prepended to the
        file as a `#`-comment. Lines that already start with `#` pass
        through unchanged (no double-`#`)."""
        path = self._file('original line 1\noriginal line 2\n')

        with patch.object(rebase_v2.terminal, 'run'):
            rebase_v2.hand_off_to_editor(
                path,
                reason='Cannot classify commit\nFix it manually',
                editor='vim')

        self.assertEqual(
            path.read_text(),
            '# B🚀 has been unable to handle this rebase. Investigate '
            'why, and\n'
            '# file a bug. Failure reason below.\n'
            '# Cannot classify commit\n'
            '# Fix it manually\n'
            'original line 1\n'
            'original line 2\n')

    def test_hand_off_to_editor_no_reason_leaves_file_alone(self):
        """With an empty `reason`, the file content is untouched -- only
        the editor subprocess is invoked."""
        path = self._file('original content\n')

        with patch.object(rebase_v2.terminal, 'run'):
            rebase_v2.hand_off_to_editor(path, reason='', editor='vim')

        self.assertEqual(path.read_text(), 'original content\n')

    def test_get_git_editor_raises_if_env_already_overridden(self):
        """If the resolved env var holds a brockit `--internal-rebase-*`
        dispatch, `get_git_editor` is being called too late and would
        loop on itself; raise `NotImplementedError` instead."""
        overridden = ('/path/to/vpython3 /path/to/brockit.py '
                      '--internal-rebase-v2-plan-squash-pinned')
        with patch.dict(os.environ, {'GIT_EDITOR': overridden}, clear=False):
            with self.assertRaises(NotImplementedError):
                rebase_v2.get_git_editor()


class RebaseV2ExecuteTest(unittest.TestCase):
    """End-to-end tests for `Rebase.execute(..., v2=True)`."""

    def setUp(self):
        self.repo = FakeChromiumRepo()
        self.repo.setup()
        self.addCleanup(self.repo.cleanup)
        self.repo.create_brave_remote()
        self._commit_counter = 0

    def _seed_bump_branch(self) -> dict:
        """Builds a small branch: v100 → v101 (master) → feature → v102
        (master continues). Returns the relevant hashes."""
        brave = self.repo.brave
        v100 = self.repo.update_brave_version('1.0.0.0')
        v101 = self.repo.update_brave_version('1.0.0.1')
        self.repo._run_git_command(['checkout', '-b', 'feature-branch'], brave)
        self.repo.write_and_stage_file('feature.txt', 'feature content\n',
                                       brave)
        feature = self.repo.commit('Add brave-only feature.txt', brave)
        self.repo._run_git_command(
            ['push', '--set-upstream', 'origin', 'feature-branch'], brave)
        self.repo._run_git_command(['checkout', 'master'], brave)
        v102 = self.repo.update_brave_version('1.0.0.2')
        self.repo._run_git_command(['checkout', 'feature-branch'], brave)
        return {'v100': v100, 'v101': v101, 'v102': v102, 'feature': feature}

    def _git_log_subjects(self, ref: str = 'HEAD') -> list:
        out = self.repo._run_git_command(
            ['log', '--reverse', '--format=%s', ref], self.repo.brave)
        return out.splitlines() if out else []

    def _commit_with_file(self, message: str) -> str:
        """Adds a small unique file and commits it -- a real change so
        `--empty=drop` doesn't strip it."""
        self._commit_counter += 1
        self.repo.write_and_stage_file(f'gen-{self._commit_counter}.txt',
                                       f'content {self._commit_counter}\n',
                                       self.repo.brave)
        return self.repo.commit(message, self.repo.brave)

    def test_v2_no_flags_is_a_passthrough_rebase(self):
        """`--v2` alone: no plan transform, history preserved on the new
        target."""
        scenario = self._seed_bump_branch()

        brockit.Rebase().execute(from_ref=scenario['v101'],
                                 to_ref=scenario['v102'],
                                 recommit=False,
                                 discard_regen_changes=False,
                                 squash_minor_bumps=False,
                                 v2=True)

        self.assertEqual(self._git_log_subjects()[-1],
                         'Add brave-only feature.txt')
        parent = self.repo._run_git_command(['rev-parse', 'HEAD^'],
                                            self.repo.brave)
        self.assertEqual(parent, scenario['v102'])

    def test_v2_discard_regen_changes_drops_recyclable(self):
        scenario = self._seed_bump_branch()
        self._commit_with_file(
            'Update patches from Chromium 1.0.0.1 to Chromium 1.0.0.2.')
        self._commit_with_file('Updated strings for Chromium 1.0.0.2.')
        self._commit_with_file('[cr148] Another brave-only feature.')

        brockit.Rebase().execute(from_ref=scenario['v101'],
                                 to_ref=scenario['v102'],
                                 recommit=False,
                                 discard_regen_changes=True,
                                 squash_minor_bumps=False,
                                 v2=True)

        subjects = self._git_log_subjects(scenario['v102'] + '..HEAD')
        self.assertEqual(subjects, [
            'Add brave-only feature.txt',
            '[cr148] Another brave-only feature.',
        ])

    def test_v2_squash_minor_bumps_collapses_pinned(self):
        """v2 squash groups version bumps at the top and keeps the
        trailing message. Regular commits stay below in arrival order."""
        scenario = self._seed_bump_branch()
        self._commit_with_file(
            'Update from Chromium 1.0.0.2 to Chromium 1.0.0.3')
        self._commit_with_file('[cr148] Some unrelated feature commit')
        self._commit_with_file(
            'Update from Chromium 1.0.0.3 to Chromium 1.0.0.4')

        brockit.Rebase().execute(from_ref=scenario['v101'],
                                 to_ref=scenario['v102'],
                                 recommit=False,
                                 discard_regen_changes=False,
                                 squash_minor_bumps=True,
                                 v2=True)

        subjects = self._git_log_subjects(scenario['v102'] + '..HEAD')
        # Version bumps fold into the trailing message; the original
        # feature commit and the unrelated feature stay in arrival order.
        self.assertEqual(subjects, [
            'Update from Chromium 1.0.0.3 to Chromium 1.0.0.4',
            'Add brave-only feature.txt',
            '[cr148] Some unrelated feature commit',
        ])

    def _commit_fixup(self, target: str = 'HEAD') -> str:
        """Stages a unique gen-N.txt and commits it with `git commit
        --fixup=<target>`. Mirrors what `git commit --fixup` produces in
        the wild: a commit whose subject is `fixup! <target subject>`
        and whose body is empty. Stacking a second call (with the
        previous fixup as target) yields a `fixup! fixup! <subject>`
        subject -- the marker-only blocks that broke `MsgBlock.parse`."""
        self._commit_counter += 1
        self.repo.write_and_stage_file(f'gen-{self._commit_counter}.txt',
                                       f'fixup {self._commit_counter}\n',
                                       self.repo.brave)
        self.repo._run_git_command(['commit', f'--fixup={target}'],
                                   self.repo.brave)
        return self.repo._run_git_command(['rev-parse', 'HEAD'],
                                          self.repo.brave)

    def test_v2_squash_minor_bumps_with_stacked_fixup_commits(self):
        """Regression: `git commit --fixup=<conflict-resolved commit>` (and
        a stacked fixup of that fixup) produces commits whose message is
        only the autosquash marker -- `fixup! …` and `fixup! fixup! …`
        with empty bodies. After `--autosquash` chains them next to the
        pinned target and v2's `squash_minor_bumps` collapses the whole
        group, git emits those as marker-only `# fixup!` blocks inside
        the squash editor file. `MsgBlock.parse` used to raise
        `EditorRecoverableFailure` for them; the rebase must instead
        complete and keep the latest pinned subject."""
        scenario = self._seed_bump_branch()
        first_pinned = self._commit_with_file(
            'Conflict-resolved patches from Chromium 1.0.0.1 to '
            'Chromium 1.0.0.2.')
        inner_fixup = self._commit_fixup(target=first_pinned)
        self._commit_fixup(target=inner_fixup)
        self._commit_with_file(
            'Conflict-resolved patches from Chromium 1.0.0.2 to '
            'Chromium 1.0.0.3.')

        brockit.Rebase().execute(from_ref=scenario['v101'],
                                 to_ref=scenario['v102'],
                                 recommit=False,
                                 discard_regen_changes=False,
                                 squash_minor_bumps=True,
                                 v2=True)

        subjects = self._git_log_subjects(scenario['v102'] + '..HEAD')
        self.assertEqual(subjects, [
            'Conflict-resolved patches from Chromium 1.0.0.2 to '
            'Chromium 1.0.0.3.',
            'Add brave-only feature.txt',
        ])

    def test_v2_recommit_amends_first_commit(self):
        """`--recommit` is reused from v1 -- v2 doesn't change its
        semantics, just confirms it still works when `v2=True`."""
        scenario = self._seed_bump_branch()
        before = self.repo._run_git_command(['rev-parse', 'HEAD'],
                                            self.repo.brave)

        brockit.Rebase().execute(from_ref=scenario['v101'],
                                 to_ref=scenario['v102'],
                                 recommit=True,
                                 discard_regen_changes=False,
                                 squash_minor_bumps=False,
                                 v2=True)

        after = self.repo._run_git_command(['rev-parse', 'HEAD'],
                                           self.repo.brave)
        self.assertNotEqual(after, before)
        self.assertEqual(self._git_log_subjects()[-1],
                         'Add brave-only feature.txt')


if __name__ == '__main__':
    unittest.main()
