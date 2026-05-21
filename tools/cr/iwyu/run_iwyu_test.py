#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from run_iwyu import is_path_enabled, parse_paths_file


class ParsePathsFileTest(unittest.TestCase):

    def setUp(self):
        self._tmp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp_dir.cleanup)
        self.cfg = Path(self._tmp_dir.name) / 'paths.cfg'

    def _write(self, content: str) -> Path:
        self.cfg.write_text(content, encoding='utf-8', newline='')
        return self.cfg

    def test_empty_file(self):
        self.assertEqual(parse_paths_file(self._write('')), [])

    def test_blank_and_comment_lines_only(self):
        self._write('\n   \n# a comment\n\t# indented comment\n')
        self.assertEqual(parse_paths_file(self.cfg), [])

    def test_enable_and_disable_rules(self):
        self._write('+brave/browser/\n-brave/browser/internal/\n')
        self.assertEqual(parse_paths_file(self.cfg), [
            ('+', 'brave/browser/'),
            ('-', 'brave/browser/internal/'),
        ])

    def test_trailing_slash_is_added_when_missing(self):
        self._write('+brave/browser\n-brave/browser/internal\n')
        self.assertEqual(parse_paths_file(self.cfg), [
            ('+', 'brave/browser/'),
            ('-', 'brave/browser/internal/'),
        ])

    def test_trailing_comment_is_stripped(self):
        self._write('+brave/browser/   # enable browser\n'
                    '-brave/browser/internal/# nested disable\n')
        self.assertEqual(parse_paths_file(self.cfg), [
            ('+', 'brave/browser/'),
            ('-', 'brave/browser/internal/'),
        ])

    def test_line_that_is_only_a_comment_is_skipped(self):
        self._write('+brave/browser/\n'
                    '# -brave/browser/internal/\n'
                    '-brave/components/\n')
        self.assertEqual(parse_paths_file(self.cfg), [
            ('+', 'brave/browser/'),
            ('-', 'brave/components/'),
        ])

    def test_surrounding_whitespace_is_tolerated(self):
        self._write('   +brave/browser/   \n\t-brave/components/\t\n')
        self.assertEqual(parse_paths_file(self.cfg), [
            ('+', 'brave/browser/'),
            ('-', 'brave/components/'),
        ])

    def test_rules_preserve_file_order(self):
        self._write('+a/\n-b/\n+c/\n-d/\n')
        self.assertEqual(parse_paths_file(self.cfg), [
            ('+', 'a/'),
            ('-', 'b/'),
            ('+', 'c/'),
            ('-', 'd/'),
        ])

    def test_invalid_prefix_raises(self):
        self._write('brave/browser/\n')
        with self.assertRaisesRegex(ValueError,
                                    'must start with `\\+` or `-`'):
            parse_paths_file(self.cfg)

    def test_empty_path_after_sign_raises(self):
        self._write('+\n')
        with self.assertRaisesRegex(ValueError, 'empty path after `\\+`'):
            parse_paths_file(self.cfg)

    def test_error_line_number_matches_offending_line(self):
        self._write('+brave/browser/\n'
                    '\n'
                    '# a comment\n'
                    'oops\n')
        with self.assertRaisesRegex(ValueError, ':4:'):
            parse_paths_file(self.cfg)


class IsPathEnabledTest(unittest.TestCase):

    def test_no_rules_means_disabled(self):
        self.assertFalse(is_path_enabled('brave/browser/foo.cc', []))

    def test_unmatched_path_is_disabled(self):
        rules = [('+', 'brave/browser/')]
        self.assertFalse(is_path_enabled('brave/components/foo.cc', rules))

    def test_single_enable_rule(self):
        rules = [('+', 'brave/browser/')]
        self.assertTrue(is_path_enabled('brave/browser/foo.cc', rules))

    def test_single_disable_rule(self):
        rules = [('-', 'brave/browser/')]
        self.assertFalse(is_path_enabled('brave/browser/foo.cc', rules))

    def test_longest_prefix_wins_disable_overrides_enable(self):
        rules = [
            ('+', 'brave/browser/'),
            ('-', 'brave/browser/internal/'),
        ]
        self.assertTrue(is_path_enabled('brave/browser/foo.cc', rules))
        self.assertFalse(
            is_path_enabled('brave/browser/internal/foo.cc', rules))

    def test_longest_prefix_wins_enable_overrides_disable(self):
        rules = [
            ('-', 'brave/browser/'),
            ('+', 'brave/browser/public/'),
        ]
        self.assertFalse(is_path_enabled('brave/browser/foo.cc', rules))
        self.assertTrue(is_path_enabled('brave/browser/public/foo.cc', rules))

    def test_rule_order_does_not_matter(self):
        forward = [
            ('+', 'brave/browser/'),
            ('-', 'brave/browser/internal/'),
        ]
        reverse = list(reversed(forward))
        path = 'brave/browser/internal/foo.cc'
        self.assertEqual(is_path_enabled(path, forward),
                         is_path_enabled(path, reverse))

    def test_prefix_must_align_to_directory_boundary(self):
        # The trailing `/` is what prevents `brave/browser/` matching
        # `brave/browser_other/...`.
        rules = [('+', 'brave/browser/')]
        self.assertFalse(is_path_enabled('brave/browser_other/foo.cc', rules))

    def test_exact_directory_match_is_enabled(self):
        # Source exactly under the enabled prefix.
        rules = [('+', 'brave/browser/')]
        self.assertTrue(is_path_enabled('brave/browser/a/b/c.cc', rules))


if __name__ == '__main__':
    unittest.main()
