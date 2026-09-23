#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for dotenv.py: parsing the secrets `.env` file CI writes."""

import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import dotenv


class ParseTest(unittest.TestCase):

    def test_plain_key_value_lines(self):
        self.assertEqual(
            dotenv.parse('fake_secret_key=abc123\n'
                         'other_fake_secret_key=def456\n'), {
                             'fake_secret_key': 'abc123',
                             'other_fake_secret_key': 'def456',
                         })

    def test_blank_lines_and_comments_are_skipped(self):
        self.assertEqual(
            dotenv.parse('\n# a comment\nfake_secret_key=abc123\n'
                         '\n'), {'fake_secret_key': 'abc123'})

    def test_surrounding_whitespace_is_stripped(self):
        self.assertEqual(dotenv.parse('  fake_secret_key = abc123 \n'),
                         {'fake_secret_key': 'abc123'})

    def test_matching_quotes_are_stripped(self):
        self.assertEqual(dotenv.parse('a="double"\nb=\'single\'\n'), {
            'a': 'double',
            'b': 'single',
        })

    def test_mismatched_quote_is_left_alone(self):
        self.assertEqual(dotenv.parse('a="mismatched\'\n'),
                         {'a': '"mismatched\''})

    def test_line_without_equals_is_ignored(self):
        self.assertEqual(dotenv.parse('not-a-valid-line\na=b\n'), {'a': 'b'})

    def test_empty_text_is_empty(self):
        self.assertEqual(dotenv.parse(''), {})


class ReadTest(unittest.TestCase):

    def test_missing_file_returns_empty(self):
        self.assertEqual(
            dotenv.read(Path(tempfile.mkdtemp()) / 'does-not-exist'), {})

    def test_reads_and_parses_existing_file(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        path = Path(tmp.name) / '.env'
        path.write_text('fake_secret_key=abc123\n', encoding='utf-8')

        self.assertEqual(dotenv.read(path), {'fake_secret_key': 'abc123'})

    def test_defaults_to_default_path(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        path = Path(tmp.name) / '.env'
        path.write_text('fake_secret_key=abc123\n', encoding='utf-8')

        original = dotenv.DEFAULT_PATH
        dotenv.DEFAULT_PATH = path
        try:
            self.assertEqual(dotenv.read(), {'fake_secret_key': 'abc123'})
        finally:
            dotenv.DEFAULT_PATH = original


if __name__ == '__main__':
    unittest.main()
