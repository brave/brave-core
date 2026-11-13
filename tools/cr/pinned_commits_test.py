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


if __name__ == '__main__':
    unittest.main()
