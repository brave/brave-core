#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `pinned_commits`."""

import unittest

import pinned_commits


class PatternMatchTest(unittest.TestCase):
    """Tests for the cr-tag stripper, pinned classifier, and recyclable
    detector."""

    def test_strip_cr_tag_removes_simple_tag(self):
        self.assertEqual(pinned_commits.strip_cr_tag('[cr149] Foo'), 'Foo')

    def test_strip_cr_tag_removes_chained_tags(self):
        """`[cr149][ios] ` collapses into nothing."""
        self.assertEqual(pinned_commits.strip_cr_tag('[cr149][ios] Foo'), 'Foo')
        self.assertEqual(
            pinned_commits.strip_cr_tag('[cr149][ios][extra] Foo'), 'Foo'
        )

    def test_strip_cr_tag_leaves_unrelated_tag(self):
        """`[unrelated] ` is not a cr-tag; left in place."""
        self.assertEqual(
            pinned_commits.strip_cr_tag('[unrelated] Foo'), '[unrelated] Foo'
        )

    def test_strip_cr_tag_noop_on_untagged(self):
        self.assertEqual(
            pinned_commits.strip_cr_tag(
                'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1'
            ),
            'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1',
        )

    def test_strip_autosquash_prefixes_handles_stacks(self):
        """`fixup! ` / `amend! ` / `squash! ` prefixes can stack when a
        commit fixes up an already-fixup commit. `strip_autosquash_prefixes`
        peels off any number of them at the start."""
        self.assertEqual(
            pinned_commits.strip_autosquash_prefixes(
                'fixup! Conflict-resolved patches from Chromium 1.0.0.0 '
                'to Chromium 1.0.0.1.'
            ),
            'Conflict-resolved patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.',
        )
        self.assertEqual(
            pinned_commits.strip_autosquash_prefixes(
                'fixup! fixup! fixup! Conflict-resolved patches from '
                'Chromium 1.0.0.0 to Chromium 1.0.0.1.'
            ),
            'Conflict-resolved patches from Chromium 1.0.0.0 to '
            'Chromium 1.0.0.1.',
        )
        # Mixed prefixes also strip cleanly.
        self.assertEqual(
            pinned_commits.strip_autosquash_prefixes(
                'amend! fixup! squash! [cr149] IWYU fixes.'
            ),
            '[cr149] IWYU fixes.',
        )
        # Untagged subjects pass through unchanged.
        self.assertEqual(
            pinned_commits.strip_autosquash_prefixes('Just a plain subject'),
            'Just a plain subject',
        )

    def test_pinned_match_survives_stacked_autosquash_prefixes(self):
        """A pinned subject buried under stacked `fixup!` prefixes still
        classifies via the autosquash-prefix stripping pre-pass."""
        self.assertEqual(
            pinned_commits.get_pinned_group_for_subject(
                'fixup! fixup! Conflict-resolved patches from Chromium '
                '1.0.0.0 to Chromium 1.0.0.1.'
            ),
            'conflict',
        )
        self.assertEqual(
            pinned_commits.get_pinned_group_for_subject(
                'fixup! [cr149] IWYU fixes.'
            ),
            'iwyu',
        )
        # Stripping is composable with `[crNNN]` -- the autosquash
        # strip happens first, then `[crNNN]`.
        self.assertEqual(
            pinned_commits.get_pinned_group_for_subject(
                'amend! [cr149] `gnrt` run for Chromium 149.0.7827.5'
            ),
            'gnrt',
        )

    def test_pinned_patterns_match_canonical_subjects(self):
        """Every pinned pattern matches its canonical subject form, both
        tagged and untagged where applicable."""
        cases = [
            ('version', 'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1'),
            (
                'plaster_reruns',
                'Apply-fixed 🩹 patches from Chromium 1.0.0.0 to '
                'Chromium 1.0.0.1.',
            ),
            (
                'conflict',
                'Conflict-resolved patches from Chromium 1.0.0.0 to '
                'Chromium 1.0.0.1.',
            ),
            ('gnrt', '[cr149] `gnrt` run for Chromium 149.0.7827.5'),
            ('iwyu', '[cr149] IWYU fixes.'),
            ('resource_ids', '[cr149] Bump resource_ids'),
            ('lit_mangler', '[cr149] Update Lit mangler snapshot'),
            ('codehealth_tracker', '[cr149] Version CodeHealth tracker'),
            ('lift_updates', '[cr149] Version lift updates.'),
            ('disable_tests', '[cr149] Disable failing upstream tests'),
            ('disable_tests', '[cr149] Filter upstream tests as needed'),
            (
                'dead_upstream_tests',
                '[cr149] Remove dead upstream tests from filters',
            ),
        ]
        for expected_id, subject in cases:
            with self.subTest(subject=subject):
                self.assertEqual(
                    pinned_commits.get_pinned_group_for_subject(subject),
                    expected_id,
                )

    def test_pinned_iwyu_untagged_form_also_matches(self):
        """The `[cr*]`-tagged patterns must also match without the tag."""
        self.assertEqual(
            pinned_commits.get_pinned_group_for_subject('IWYU fixes.'), 'iwyu'
        )
        self.assertEqual(
            pinned_commits.get_pinned_group_for_subject('Bump resource_ids'),
            'resource_ids',
        )

    def test_pinned_rejects_near_misses(self):
        """Near-miss subjects should not match any pinned pattern."""
        self.assertIsNone(
            pinned_commits.get_pinned_group_for_subject(
                'Update from Chromium 1.0.0.0 to '
                'Chromium 1.0.0.1 (revert pending)'
            )
        )
        self.assertIsNone(
            pinned_commits.get_pinned_group_for_subject('[cr149] Feature work')
        )
        self.assertIsNone(
            pinned_commits.get_pinned_group_for_subject(
                'Update from Chromium abc to Chromium def'
            )
        )

    def test_pinned_rejects_non_four_segment_versions(self):
        """Chromium versions must be exactly four dot-separated numeric
        segments. Three-segment or five-segment shapes are rejected."""
        self.assertIsNone(
            pinned_commits.get_pinned_group_for_subject(
                'Update from Chromium 1.0.0 to Chromium 1.0.1'
            )
        )
        self.assertIsNone(
            pinned_commits.get_pinned_group_for_subject(
                'Update from Chromium 1.0.0.0.0 to Chromium 1.0.0.0.1'
            )
        )
        self.assertIsNone(
            pinned_commits.get_pinned_group_for_subject(
                '`gnrt` run for Chromium 149.0.7827'
            )
        )

    def test_recyclable_matches_canonical(self):
        self.assertTrue(
            pinned_commits.is_recyclable(
                'Update patches from Chromium 1.0.0.0 to Chromium 1.0.0.1.'
            )
        )
        self.assertTrue(
            pinned_commits.is_recyclable(
                'Updated strings for Chromium 1.0.0.1.'
            )
        )

    def test_recyclable_rejects_near_misses(self):
        self.assertFalse(
            pinned_commits.is_recyclable(
                'Update from Chromium 1.0.0.0 to Chromium 1.0.0.1'
            )
        )
        self.assertFalse(pinned_commits.is_recyclable('[cr149] Feature work'))


class DerivedChangeIdTest(unittest.TestCase):
    """A pinned or recyclable commit's Change-Id follows its branch and its
    kind, not the versions its subject names."""

    _derive = staticmethod(pinned_commits.derived_change_id)

    def test_a_kind_keeps_one_change_id_per_branch(self):
        for first, second in (
            ('Version lift updates', '[cr160] Version lift updates.'),
            (
                '`gnrt` run for Chromium 160.0.8200.1.',
                '[cr160] `gnrt` run for Chromium 160.0.8210.3',
            ),
            (
                'Update from Chromium 159.0.8190.1 to Chromium 160.0.8200.1.',
                'Update from Chromium 160.0.8200.1 to Chromium 160.0.8210.3.',
            ),
            (
                'Update patches from Chromium 159.0.8190.1 to Chromium '
                '160.0.8200.1.',
                'Update patches from Chromium 160.0.8200.1 to Chromium '
                '160.0.8210.3.',
            ),
            (
                'Updated strings for Chromium 160.0.8200.1.',
                'Updated strings for Chromium 160.0.8210.3.',
            ),
        ):
            with self.subTest(first):
                change_id = self._derive('cr160', first)
                self.assertRegex(change_id, r'^I[0-9a-f]{40}$')
                self.assertEqual(change_id, self._derive('cr160', second))
                self.assertNotEqual(change_id, self._derive('cr161', first))

    def test_kinds_have_change_ids_of_their_own(self):
        subjects = (
            'Version lift updates',
            'IWYU fixes.',
            '`gnrt` run for Chromium 160.0.8200.1.',
            'Update patches from Chromium 159.0.8190.1 to Chromium '
            '160.0.8200.1.',
            'Updated strings for Chromium 160.0.8200.1.',
        )
        change_ids = {self._derive('cr160', subject) for subject in subjects}
        self.assertEqual(len(change_ids), len(subjects))

    def test_none_for_other_commits_or_branches(self):
        for branch, subject in (
            ('master', 'Version lift updates'),
            ('cr160+fix', 'Version lift updates'),
            # Channel bumps, whose branch names a later bump reuses.
            ('cr160.x', 'Disable failing upstream tests.'),
            ('cr160.x-on-1.100.x', 'Disable failing upstream tests.'),
            ('cr160', '[cr160] Feature work'),
            ('cr160', 'fixup! Version lift updates'),
            ('cr160', 'reassign!abc123! Version lift updates'),
        ):
            with self.subTest(branch=branch, subject=subject):
                self.assertIsNone(self._derive(branch, subject))


if __name__ == '__main__':
    unittest.main()
