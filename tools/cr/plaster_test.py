#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from pathlib import Path
import hashlib
import json
import os
import time

import plaster

from test.fake_chromium_repo import FakeChromiumRepo


class PlasterTest(unittest.TestCase):

    def setUp(self):
        """Set up a fake Chromium repository for testing."""
        self.fake_chromium_src = FakeChromiumRepo()
        self.fake_chromium_src.setup()
        self.addCleanup(self.fake_chromium_src.cleanup)

    def test_original_expected_toml_rules(self):
        """Test applying all .toml files in the test/ folder."""
        test_folder = Path(__file__).parent / 'test/plasters'
        toml_files = test_folder.rglob('*.toml')

        for toml_file in toml_files:
            with self.subTest(toml_file=toml_file):
                # Deduce the path for the original file for the toml to be
                # applied to.
                original_file_to = (toml_file.parent / toml_file.stem)
                original_file_from = original_file_to.with_stem(
                    original_file_to.stem + '-original')
                expected_file = original_file_to.with_stem(
                    original_file_to.stem + '-expected')

                # Commit the original file to the fake Chromium repository.
                self.fake_chromium_src.write_and_stage_file(
                    relative_path=original_file_to.name,
                    content=original_file_from.read_text(),
                    repo_path=self.fake_chromium_src.chromium)
                self.fake_chromium_src.commit(
                    commit_message=f'Add {original_file_from.name}',
                    repo_path=self.fake_chromium_src.chromium)

                # Copy the .toml file to the fake Brave rewrite path.
                rewrite_path = plaster.PLASTER_FILES_PATH / toml_file.name
                rewrite_path.parent.mkdir(parents=True, exist_ok=True)
                rewrite_path.write_text(toml_file.read_text())

                # Use PlasterFile to apply the .toml file to the committed file.
                plaster_file = plaster.PlasterFile(rewrite_path)
                plaster_file.apply()

                # Check that the committed file matches the expected file.
                committed_file_path = (self.fake_chromium_src.chromium /
                                       original_file_to.name)
                self.assertEqual(
                    committed_file_path.read_text(), expected_file.read_text(),
                    f'File {committed_file_path} does not match '
                    f'expected {expected_file}')

    def test_plaster_patchinfo_creation(self):
        """Test the patchinfo creation mechanics."""
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_file1.idl')

        # Write and commit files to respective repositories
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Initial content for Chromium file.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_file1.idl',
                                      self.fake_chromium_src.chromium)

        # Create a PlasterFile instance for the test file
        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.toml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          [[substitution]]
          description = 'Simple test substitution'
          re_pattern = 'Chromium'
          replace = 'Plaster'
        ''')

        # Use PlasterFile to apply the .toml file to the committed file.
        plaster_file = plaster.PlasterFile(plaster_path)
        plaster_file.apply()

        self.assertEqual(
            (self.fake_chromium_src.chromium / test_file_chromium).read_text(),
            'Initial content for Plaster file.')

        # Checking for the creation of the patchinfo file
        patchinfo = plaster.PatchinfoBuilder(plaster_path)
        patchinfo_from_disk = json.loads(
            patchinfo.patchinfo.path.read_text(encoding='utf-8'))

        self.assertEqual(patchinfo_from_disk['schemaVersion'], 1)
        self.assertEqual(
            patchinfo_from_disk['patchChecksum'],
            hashlib.sha256(
                patchinfo.patch.path.read_text().encode()).hexdigest())
        self.assertEqual(patchinfo_from_disk['appliesTo'][0]['path'],
                         str(test_file_chromium))
        self.assertEqual(
            patchinfo_from_disk['appliesTo'][0]['checksum'],
            hashlib.sha256(
                (self.fake_chromium_src.chromium /
                 test_file_chromium).read_text().encode()).hexdigest())
        # PatchinfoBuilder normalizes plaster path to be relative to brave root.
        self.assertEqual(patchinfo_from_disk['plaster']['path'],
                         str(patchinfo.plaster_file))
        self.assertEqual(
            patchinfo_from_disk['plaster']['checksum'],
            hashlib.sha256(plaster_path.read_text().encode()).hexdigest())

        self.assertEqual(patchinfo_from_disk['patchChecksum'],
                         patchinfo.patch.checksum)
        self.assertEqual(patchinfo_from_disk['appliesTo'][0]['path'],
                         str(patchinfo.source))
        self.assertEqual(patchinfo_from_disk['appliesTo'][0]['checksum'],
                         patchinfo.source_with_checksum.checksum)
        self.assertEqual(patchinfo_from_disk['plaster']['path'],
                         str(patchinfo.plaster_file))
        self.assertEqual(patchinfo_from_disk['plaster']['checksum'],
                         patchinfo.plaster_checksum)

        # Use a temp file for this test
        temp_file = plaster.PLASTER_FILES_PATH / 'temp_save_if_changed.txt'
        temp_file.write_text('foo')
        pair = plaster.PathChecksumPair(temp_file)
        mtime_before = temp_file.stat().st_mtime
        # Should not write if content is the same
        time.sleep(0.01)  # Ensure mtime can change if written
        pair.save_if_changed('foo')
        mtime_after = temp_file.stat().st_mtime
        self.assertEqual(mtime_before, mtime_after)

        # Should write if content is different
        time.sleep(0.01)
        pair.save_if_changed('bar')
        mtime_changed = temp_file.stat().st_mtime
        self.assertNotEqual(mtime_after, mtime_changed)
        self.assertEqual(temp_file.read_text(), 'bar')
        temp_file.unlink()

    def test_check_success_multiple_up_to_date(self):
        """Test plaster check succeeds for 3 up-to-date plaster files."""
        # Create 3 source files in chromium and matching .toml in brave/rewrite
        files = [
            ('chrome/common/extensions/api/test_file1.idl', 'foo1', 'bar1'),
            ('chrome/common/extensions/api/test_file2.idl', 'foo2', 'bar2'),
            ('chrome/common/extensions/api/test_file3.idl', 'foo3', 'bar3'),
        ]
        for rel_path, orig, repl in files:
            # Write and commit the original file
            src_path = Path(rel_path)
            self.fake_chromium_src.write_and_stage_file(
                src_path, f'Initial {orig} content.',
                self.fake_chromium_src.chromium)
            self.fake_chromium_src.commit(f'Add {src_path}',
                                          self.fake_chromium_src.chromium)
            # Write the .toml rewrite file
            rewrite_path = plaster.PLASTER_FILES_PATH / (str(src_path) +
                                                         '.toml')
            rewrite_path.parent.mkdir(parents=True, exist_ok=True)
            rewrite_path.write_text(f'''
              [[substitution]]
              description = 'Replace {orig} with {repl}'
              re_pattern = '{orig}'
              replace = '{repl}'
            ''')
            # Apply the rewrite so files are up-to-date
            plaster_file = plaster.PlasterFile(rewrite_path)
            plaster_file.apply()
        # Now check should succeed without raising.
        class DummyArgs:

            def __init__(self):
                self.infra_mode = False
                self.verbose = False

        args = DummyArgs()
        self.assertEqual(plaster.check(args), 0)

    def test_check_fails_when_toml_changed(self):
        """Test plaster check fails when there's a mismatch."""
        # Create 3 source files in chromium and matching .toml in brave/rewrite
        files = [
            ('chrome/common/extensions/api/test_file1.idl', 'foo1', 'bar1'),
            ('chrome/common/extensions/api/test_file2.idl', 'foo2', 'bar2'),
            ('chrome/common/extensions/api/test_file3.idl', 'foo3', 'bar3'),
        ]
        rewrite_paths = []
        for rel_path, orig, repl in files:
            src_path = Path(rel_path)
            self.fake_chromium_src.write_and_stage_file(
                src_path, f'Initial {orig} content.',
                self.fake_chromium_src.chromium)
            self.fake_chromium_src.commit(f'Add {src_path}',
                                          self.fake_chromium_src.chromium)
            rewrite_path = plaster.PLASTER_FILES_PATH / (str(src_path) +
                                                         '.toml')
            rewrite_path.parent.mkdir(parents=True, exist_ok=True)
            rewrite_path.write_text(f'''
              [[substitution]]
              description = 'Replace {orig} with {repl}'
              re_pattern = '{orig}'
              replace = '{repl}'
            ''')
            plaster_file = plaster.PlasterFile(rewrite_path)
            plaster_file.apply()
            rewrite_paths.append(rewrite_path)

        # Check should succeed first
        class DummyArgs:

            def __init__(self):
                self.infra_mode = False
                self.verbose = False

        args = DummyArgs()
        self.assertEqual(plaster.check(args), 0)
        # Now change one toml file to cause a failure
        changed_path = rewrite_paths[1]
        changed_path.write_text('''
          [[substitution]]
          description = 'Break the rule'
          re_pattern = 'foo2'
          replace = 'DIFFERENT'
        ''')
        # Now check should raise PlasterFileNeedsRegen with the path included.
        with self.assertRaises(plaster.PlasterFileNeedsRegen) as context:
            plaster.check(args)
        self.assertIn(str(changed_path), str(context.exception))

    def test_regex_flags_array_works(self):
        """Test that multiple flags in array are passed through correctly."""
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_flags_array.idl')

        # Write and commit file with mixed case content
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Content with CHROMIUM word.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_flags_array.idl',
                                      self.fake_chromium_src.chromium)

        # Create a PlasterFile with multiple flags in array
        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.toml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          [[substitution]]
          description = 'Test multiple flags in array work'
          re_pattern = 'chromium'
          replace = 'Brave'
          re_flags = ['IGNORECASE', 'MULTILINE']
        ''')

        plaster_file = plaster.PlasterFile(plaster_path)
        plaster_file.apply()

        # Should match uppercase CHROMIUM due to IGNORECASE flag
        result = (self.fake_chromium_src.chromium /
                  test_file_chromium).read_text()
        self.assertEqual(result, 'Content with Brave word.')

    def test_regex_flags_invalid_cases_fail(self):
        """Test that invalid regex flags (nonexistent, lowercase, etc.) raise
           ValueError."""
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_invalid_flags.idl')

        # Write and commit file
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Content with Chromium word.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_invalid_flags.idl',
                                      self.fake_chromium_src.chromium)

        # Test various invalid flag cases
        invalid_flags_cases = [
            'INVALID_FLAG',  # Nonexistent flag
            'ignorecase',  # Lowercase (should be IGNORECASE)
            'fake_flag',  # Another nonexistent flag
            'NOTREAL'  # Another invalid flag
        ]

        for invalid_flag in invalid_flags_cases:
            with self.subTest(flag=invalid_flag):
                # Create a PlasterFile with invalid flag
                plaster_path = plaster.PLASTER_FILES_PATH / (
                    str(test_file_chromium) + '.toml')
                plaster_path.parent.mkdir(parents=True, exist_ok=True)
                plaster_path.write_text(f'''
                  [[substitution]]
                  description = 'Test invalid flag rejection'
                  re_pattern = 'Chromium'
                  replace = 'Brave'
                  re_flags = ['{invalid_flag}']
                ''')

                plaster_file = plaster.PlasterFile(plaster_path)
                with self.assertRaises(ValueError) as context:
                    plaster_file.apply()

                self.assertIn(f'Invalid re flag specified: {invalid_flag}',
                              str(context.exception))

    def test_regex_flags_empty_list_works(self):
        """Test that empty flag list works (no flags applied)."""
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_no_flags.idl')

        # Write and commit file with mixed case content
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Content with CHROMIUM and chromium words.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_no_flags.idl',
                                      self.fake_chromium_src.chromium)

        # Create a PlasterFile with empty flags list
        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.toml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          [[substitution]]
          description = 'Test empty flags list'
          re_pattern = 'chromium'
          replace = 'Brave'
          re_flags = []
        ''')

        plaster_file = plaster.PlasterFile(plaster_path)
        plaster_file.apply()

        # Should only match lowercase 'chromium', not uppercase 'CHROMIUM'
        result = (self.fake_chromium_src.chromium /
                  test_file_chromium).read_text()
        self.assertEqual(result, 'Content with CHROMIUM and Brave words.')

    def test_invalid_regex_fails(self):
        """Test that invalid regex patterns raise PlasterApplyError."""
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_invalid_regex.idl')

        # Write and commit file
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Content with Chromium word.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_invalid_regex.idl',
                                      self.fake_chromium_src.chromium)

        # Test various invalid regex patterns
        invalid_patterns = [
            '[',  # Unclosed bracket
            '(?P<',  # Incomplete named group
            '(?P<name',  # Incomplete named group
            '*',  # Nothing to repeat
            '(?',  # Incomplete group
        ]

        for invalid_pattern in invalid_patterns:
            with self.subTest(pattern=invalid_pattern):
                # Create a PlasterFile with invalid regex
                plaster_path = plaster.PLASTER_FILES_PATH / (
                    str(test_file_chromium) + '.toml')
                plaster_path.parent.mkdir(parents=True, exist_ok=True)
                plaster_path.write_text(f'''
                  [[substitution]]
                  description = 'Test invalid regex rejection'
                  re_pattern = '{invalid_pattern}'
                  replace = 'Brave'
                ''')

                plaster_file = plaster.PlasterFile(plaster_path)
                with self.assertRaises(plaster.PlasterApplyError) as context:
                    plaster_file.apply()
                message = str(context.exception)
                self.assertIn('Invalid regex:', message)
                self.assertIn(str(plaster_path), message)

    def test_pattern_validation_failures(self):
        """Test various pattern validation failures."""
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_validation.idl')

        # Write and commit file to chromium repository
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Initial content for Chromium file.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_validation.idl',
                                      self.fake_chromium_src.chromium)

        # Test various invalid validation cases
        validation_cases = [
            # (toml_content, expected_error_message)
            ('''
              [[substitution]]
              description = 'Both patterns specified'
              pattern = 'Chromium'
              re_pattern = 'Chromium'
              replace = 'Plaster'
            ''', 'Please specify either pattern or re_pattern'),
            ('''
              [[substitution]]
              description = 'No pattern specified'
              replace = 'Plaster'
            ''', 'No pattern specified'),
            ('''
              [[substitution]]
              description = 'No replace specified'
              pattern = 'Chromium'
            ''', 'No replace value specified'),
        ]

        for toml_content, expected_error in validation_cases:
            with self.subTest(error=expected_error):
                # Create a PlasterFile with invalid configuration
                plaster_path = plaster.PLASTER_FILES_PATH / (
                    str(test_file_chromium) + '.toml')
                plaster_path.parent.mkdir(parents=True, exist_ok=True)
                plaster_path.write_text(toml_content)

                plaster_file = plaster.PlasterFile(plaster_path)
                with self.assertRaises(ValueError) as context:
                    plaster_file.apply()

                self.assertIn(expected_error, str(context.exception))

    def test_pattern_exact_match_works(self):
        """Test that using pattern (exact string match) works correctly."""
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_file1.idl')

        # Write and commit file to chromium repository
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium,
            'Initial content with Chromium and Chromium++ text.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_file1.idl',
                                      self.fake_chromium_src.chromium)

        # Create a PlasterFile using pattern (should escape special regex chars)
        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.toml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          [[substitution]]
          description = 'Replace exact pattern'
          pattern = 'Chromium++'
          replace = 'Brave++'
        ''')

        # Apply the plaster file
        plaster_file = plaster.PlasterFile(plaster_path)
        plaster_file.apply()

        # Verify that only the exact match was replaced, not partial matches
        result = (self.fake_chromium_src.chromium /
                  test_file_chromium).read_text()
        self.assertEqual(result,
                         'Initial content with Chromium and Brave++ text.')

    def test_re_pattern_regex_match_works(self):
        """Test that using re_pattern (regex match) works correctly."""
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_file1.idl')

        # Write and commit file to chromium repository
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium,
            'Initial content with Chromium123 and ChromiumABC text.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_file1.idl',
                                      self.fake_chromium_src.chromium)

        # Create a PlasterFile using re_pattern for regex matching
        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.toml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          [[substitution]]
          description = 'Replace regex pattern'
          re_pattern = 'Chromium\\w+'
          replace = 'Brave'
          count = 2
        ''')

        # Apply the plaster file
        plaster_file = plaster.PlasterFile(plaster_path)
        plaster_file.apply()

        # Verify that regex matches were replaced
        result = (self.fake_chromium_src.chromium /
                  test_file_chromium).read_text()
        self.assertEqual(result, 'Initial content with Brave and Brave text.')

    def test_pattern_vs_re_pattern_behavior_difference(self):
        """
        Test that pattern and re_pattern behave differently for special chars.
        """

        # Test 1: pattern (exact match, should escape regex chars)
        test_file_chromium1 = Path(
            'chrome/common/extensions/api/test_file_pattern.idl')
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium1, 'Text with [brackets] and (parentheses).',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_file_pattern.idl',
                                      self.fake_chromium_src.chromium)

        plaster_path1 = plaster.PLASTER_FILES_PATH / (
            str(test_file_chromium1) + '.toml')
        plaster_path1.parent.mkdir(parents=True, exist_ok=True)
        plaster_path1.write_text('''
          [[substitution]]
          description = 'Replace exact brackets'
          pattern = '[brackets]'
          replace = '{braces}'
        ''')

        plaster_file1 = plaster.PlasterFile(plaster_path1)
        plaster_file1.apply()

        result1 = (self.fake_chromium_src.chromium /
                   test_file_chromium1).read_text()
        self.assertEqual(result1, 'Text with {braces} and (parentheses).')

        # Test 2: re_pattern (regex match, brackets are character class)
        test_file_chromium2 = Path(
            'chrome/common/extensions/api/test_file_re_pattern.idl')
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium2, 'Text with [brackets] and (parentheses).',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_file_re_pattern.idl',
                                      self.fake_chromium_src.chromium)

        plaster_path2 = plaster.PLASTER_FILES_PATH / (
            str(test_file_chromium2) + '.toml')
        plaster_path2.parent.mkdir(parents=True, exist_ok=True)
        plaster_path2.write_text('''
          [[substitution]]
          description = 'Replace using regex'
          re_pattern = '\\[\\w+\\]'
          replace = '{braces}'
        ''')

        plaster_file2 = plaster.PlasterFile(plaster_path2)
        plaster_file2.apply()

        result2 = (self.fake_chromium_src.chromium /
                   test_file_chromium2).read_text()
        self.assertEqual(result2, 'Text with {braces} and (parentheses).')

    def test_count_mismatch_fails(self):
        """Test that count mismatch raises PlasterApplyError."""
        # Test case: more matches than expected
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_file1.idl')

        # Write and commit file with 3 matches but expect only 2
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Chromium and Chromium and Chromium word.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_file1.idl',
                                      self.fake_chromium_src.chromium)

        # Create a PlasterFile with count=2 but 3 matches exist
        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.toml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          [[substitution]]
          description = 'Test count mismatch'
          re_pattern = 'Chromium'
          replace = 'Brave'
          count = 2
        ''')

        # Should fail because there are 3 matches but count expects 2
        plaster_file = plaster.PlasterFile(plaster_path)
        with self.assertRaises(plaster.PlasterApplyError) as context:
            plaster_file.apply()
        message = str(context.exception)
        self.assertIn('Unexpected number of matches (3 vs 2)', message)
        self.assertIn(str(plaster_path), message)

    def test_default_count(self):
        """Test default count=1 behavior."""
        # Test case 1: Correct - exactly 1 match (should succeed)
        test_file_correct = Path(
            'chrome/common/extensions/api/test_default_count_correct.idl')

        self.fake_chromium_src.write_and_stage_file(
            test_file_correct, 'Content with single Chromium word.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_default_count_correct.idl',
                                      self.fake_chromium_src.chromium)

        plaster_path_correct = (plaster.PLASTER_FILES_PATH /
                                (str(test_file_correct) + '.toml'))
        plaster_path_correct.parent.mkdir(parents=True, exist_ok=True)
        plaster_path_correct.write_text('''
          [[substitution]]
          description = 'Test default count with 1 match'
          re_pattern = 'Chromium'
          replace = 'Brave'
        ''')

        # Should succeed because there's exactly 1 match (matches default)
        plaster_file_correct = plaster.PlasterFile(plaster_path_correct)
        plaster_file_correct.apply()

        result_correct = (self.fake_chromium_src.chromium /
                          test_file_correct).read_text()
        self.assertEqual(result_correct, 'Content with single Brave word.')

        # Test case 2: Incorrect - 2 matches (should fail)
        test_file_incorrect = Path(
            'chrome/common/extensions/api/test_default_count_incorrect.idl')

        self.fake_chromium_src.write_and_stage_file(
            test_file_incorrect, 'Chromium browser and Chromium app.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_default_count_incorrect.idl',
                                      self.fake_chromium_src.chromium)

        plaster_path_incorrect = (plaster.PLASTER_FILES_PATH /
                                  (str(test_file_incorrect) + '.toml'))
        plaster_path_incorrect.parent.mkdir(parents=True, exist_ok=True)
        plaster_path_incorrect.write_text('''
          [[substitution]]
          description = 'Test default count with 2 matches'
          re_pattern = 'Chromium'
          replace = 'Brave'
        ''')

        # Should fail because there are 2 matches but default expects 1
        plaster_file_incorrect = plaster.PlasterFile(plaster_path_incorrect)
        with self.assertRaises(plaster.PlasterApplyError) as context:
            plaster_file_incorrect.apply()
        message = str(context.exception)
        self.assertIn('Unexpected number of matches (2 vs 1)', message)
        self.assertIn(str(plaster_path_incorrect), message)

    def test_count_zero_replaces_all(self):
        """
        Test that count=0 replaces all matches and bypasses count validation.
        Note: All substitutions now behave as count=0 since count=0 is
        always passed to subn.
        """
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_count_zero_all.idl')

        # Write and commit file with multiple matches
        original_content = ('Chromium content with Chromium and more '
                            'Chromium text.')
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, original_content,
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_count_zero_all.idl',
                                      self.fake_chromium_src.chromium)

        # Create a PlasterFile with count=0 (replace all)
        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.toml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          [[substitution]]
          description = 'Test count 0 replaces all'
          re_pattern = 'Chromium'
          replace = 'Brave'
          count = 0
        ''')

        # Should succeed because count=0 means replace all and bypass validation
        plaster_file = plaster.PlasterFile(plaster_path)
        plaster_file.apply()

        result = (self.fake_chromium_src.chromium /
                  test_file_chromium).read_text()
        self.assertEqual(result,
                         'Brave content with Brave and more Brave text.')

    def test_count_explicit_values_work(self):
        """Test that explicit count values work correctly for validation.
           Note: All substitutions now replace all matches due to count=0
           in subn, but validation still checks if actual matches equal
           expected count."""
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_explicit_counts.idl')

        # Write and commit file with patterns for multiple substitutions
        original_content = ('Chromium browser and Firefox browser and '
                            'Safari browser.')
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, original_content,
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_explicit_counts.idl',
                                      self.fake_chromium_src.chromium)

        # Create a PlasterFile with multiple substitutions using different count
        # strategies - these counts must match the actual number of
        # matches for validation
        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.toml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          [[substitution]]
          description = 'Replace single Chromium'
          re_pattern = 'Chromium'
          replace = 'Brave'
          count = 1

          [[substitution]]
          description = 'Replace all browsers'
          re_pattern = 'browser'
          replace = 'application'
          count = 3
        ''')

        # Should succeed - first substitution finds 1 Chromium
        # (matches count=1),
        # second finds 3 "browser" instances (matches count=3)
        plaster_file = plaster.PlasterFile(plaster_path)
        plaster_file.apply()

        result = (self.fake_chromium_src.chromium /
                  test_file_chromium).read_text()
        self.assertEqual(
            result,
            'Brave application and Firefox application and Safari application.'
        )

    def _setup_applied_plaster(self, test_file: Path,
                               initial_content: str) -> plaster.PlasterFile:
        """Stages a chromium source, writes a plaster toml, and applies it."""
        self.fake_chromium_src.write_and_stage_file(
            test_file, initial_content, self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit(f'Add {test_file.name}',
                                      self.fake_chromium_src.chromium)
        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file) + '.toml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          [[substitution]]
          description = 'Replace Chromium with Brave'
          re_pattern = 'Chromium'
          replace = 'Brave'
        ''')
        plaster_file = plaster.PlasterFile(plaster_path)
        plaster_file.apply()
        return plaster_file

    def test_needs_apply_false_after_fresh_apply(self):
        """needs_apply returns False immediately after a successful apply."""
        plaster_file = self._setup_applied_plaster(
            Path('chrome/common/extensions/api/needs_apply_fresh.idl'),
            'Initial Chromium content.')
        self.assertFalse(plaster_file.needs_apply())

    def test_needs_apply_true_after_source_change(self):
        """needs_apply returns True when the source file is modified."""
        test_file = Path(
            'chrome/common/extensions/api/needs_apply_source_change.idl')
        plaster_file = self._setup_applied_plaster(
            test_file, 'Initial Chromium content.')
        source_path = self.fake_chromium_src.chromium / test_file
        later = source_path.stat().st_mtime + 10
        source_path.write_text('Tampered Brave content.')
        os.utime(source_path, (later, later))
        self.assertTrue(plaster_file.needs_apply())

    def test_needs_apply_true_after_toml_change(self):
        """needs_apply returns True when the plaster toml is modified."""
        test_file = Path(
            'chrome/common/extensions/api/needs_apply_toml_change.idl')
        plaster_file = self._setup_applied_plaster(
            test_file, 'Initial Chromium content.')
        later = plaster_file.path.stat().st_mtime + 10
        plaster_file.path.write_text('''
          [[substitution]]
          description = 'A different rule'
          re_pattern = 'Brave'
          replace = 'Lion'
        ''')
        os.utime(plaster_file.path, (later, later))
        self.assertTrue(plaster_file.needs_apply())

    def test_needs_apply_true_when_patchinfo_missing(self):
        """needs_apply returns True when the patchinfo file does not exist."""
        plaster_file = self._setup_applied_plaster(
            Path('chrome/common/extensions/api/needs_apply_no_patchinfo.idl'),
            'Initial Chromium content.')
        plaster.PatchinfoBuilder(plaster_file.path).patchinfo.path.unlink()
        self.assertTrue(plaster_file.needs_apply())

    def test_needs_apply_true_when_patchinfo_missing_fields(self):
        """needs_apply returns True when patchinfo lacks required pairs."""
        plaster_file = self._setup_applied_plaster(
            Path('chrome/common/extensions/api/needs_apply_incomplete_info.idl'
                 ), 'Initial Chromium content.')
        patchinfo_path = plaster.PatchinfoBuilder(
            plaster_file.path).patchinfo.path
        # Schema-only patchinfo: no appliesTo, plaster, or patchChecksum.
        patchinfo_path.write_text('{"schemaVersion": 1}',
                                  encoding='utf-8',
                                  newline='')
        # Backdate patchinfo so the mtime check trips and the field check
        # runs.
        older = patchinfo_path.stat().st_mtime - 100
        os.utime(patchinfo_path, (older, older))
        self.assertTrue(plaster_file.needs_apply())

    def test_needs_apply_true_after_patch_change(self):
        """needs_apply returns True when the patch file is modified."""
        test_file = Path(
            'chrome/common/extensions/api/needs_apply_patch_change.idl')
        plaster_file = self._setup_applied_plaster(
            test_file, 'Initial Chromium content.')
        patch_path = plaster.PatchinfoBuilder(plaster_file.path).patch.path
        later = patch_path.stat().st_mtime + 10
        patch_path.write_text('tampered patch contents\n')
        os.utime(patch_path, (later, later))
        self.assertTrue(plaster_file.needs_apply())

    def test_needs_apply_true_when_patch_missing(self):
        """needs_apply returns True when the patch file does not exist."""
        plaster_file = self._setup_applied_plaster(
            Path('chrome/common/extensions/api/needs_apply_no_patch.idl'),
            'Initial Chromium content.')
        plaster.PatchinfoBuilder(plaster_file.path).patch.path.unlink()
        self.assertTrue(plaster_file.needs_apply())

    def test_needs_apply_true_when_source_missing(self):
        """needs_apply returns True when the chromium source is missing."""
        test_file = Path(
            'chrome/common/extensions/api/needs_apply_no_source.idl')
        plaster_file = self._setup_applied_plaster(
            test_file, 'Initial Chromium content.')
        (self.fake_chromium_src.chromium / test_file).unlink()
        self.assertTrue(plaster_file.needs_apply())

    def test_needs_apply_false_when_stale_mtime_but_checksums_match(self):
        """needs_apply returns False when mtime is stale but checksums match.
        """
        plaster_file = self._setup_applied_plaster(
            Path('chrome/common/extensions/api/needs_apply_stale_mtime.idl'),
            'Initial Chromium content.')
        patchinfo_path = plaster.PatchinfoBuilder(
            plaster_file.path).patchinfo.path
        older = patchinfo_path.stat().st_mtime - 100
        os.utime(patchinfo_path, (older, older))
        self.assertFalse(plaster_file.needs_apply())


class PatchinfoTest(unittest.TestCase):
    """Tests for Patchinfo and Patchinfo.parse."""

    _VALID_JSON = '''{
      "schemaVersion": 1,
      "patchChecksum": "cff50e7ef57149c15b3550204e6ed5beadb14d9ede81a0d1d9e0c2fa89a3708f",
      "appliesTo": [
        {
          "path": "chrome/updater/mac/.install.sh",
          "checksum": "aef9cc2e118501527bab9f46a652ba8c28009ea46771af219de45a680a4157ad"
        }
      ],
      "plaster": {
        "path": "rewrite/chrome/updater/mac/.install.sh.toml",
        "checksum": "c8335aa0242a7f4426bb8e870239585063d20c2e3c1bfbe07a3060f003bd2a31"
      }
    }'''

    def test_from_json_happy_path(self):
        """from_json populates every field from a well-formed patchinfo."""
        info = plaster.Patchinfo.from_json(self._VALID_JSON)
        self.assertIsNotNone(info)
        self.assertEqual(info.schema_version, 1)
        self.assertEqual(
            info.patch_checksum,
            'cff50e7ef57149c15b3550204e6ed5beadb14d9ede81a0d1d9e0c2fa89a3708f')
        self.assertIsInstance(info.applies_to, plaster.Patchinfo.Entry)
        self.assertEqual(info.applies_to.path,
                         'chrome/updater/mac/.install.sh')
        self.assertEqual(
            info.applies_to.checksum,
            'aef9cc2e118501527bab9f46a652ba8c28009ea46771af219de45a680a4157ad')
        self.assertIsInstance(info.plaster, plaster.Patchinfo.Entry)
        self.assertEqual(info.plaster.path,
                         'rewrite/chrome/updater/mac/.install.sh.toml')
        self.assertEqual(
            info.plaster.checksum,
            'c8335aa0242a7f4426bb8e870239585063d20c2e3c1bfbe07a3060f003bd2a31')

    def test_from_json_rejects_invalid_input(self):
        """from_json returns None for malformed JSON, wrong types, or missing
        required fields."""
        valid_entry = {'path': 'x', 'checksum': 'h'}
        valid_plaster = {'path': 'p', 'checksum': 'hp'}
        base = {
            'schemaVersion': 1,
            'patchChecksum': 'a',
            'appliesTo': [valid_entry],
            'plaster': valid_plaster,
        }

        cases: list[tuple[str, str]] = [
            ('not JSON at all', 'not json'),
            ('list root', '[]'),
            ('int root', '42'),
            ('string root', '"hello"'),
            ('missing schemaVersion',
             json.dumps({
                 k: v
                 for k, v in base.items() if k != 'schemaVersion'
             })),
            ('wrong schemaVersion type',
             json.dumps({
                 **base, 'schemaVersion': '1'
             })),
            ('missing patchChecksum',
             json.dumps({
                 k: v
                 for k, v in base.items() if k != 'patchChecksum'
             })),
            ('wrong patchChecksum type',
             json.dumps({
                 **base, 'patchChecksum': 123
             })),
            ('missing appliesTo',
             json.dumps({
                 k: v
                 for k, v in base.items() if k != 'appliesTo'
             })),
            ('empty appliesTo', json.dumps({
                **base, 'appliesTo': []
            })),
            ('more than one appliesTo entry',
             json.dumps({
                 **base, 'appliesTo': [valid_entry, valid_entry]
             })),
            ('non-list appliesTo',
             json.dumps({
                 **base, 'appliesTo': valid_entry
             })),
            ('appliesTo entry missing path',
             json.dumps({
                 **base, 'appliesTo': [{
                     'checksum': 'h'
                 }]
             })),
            ('appliesTo entry missing checksum',
             json.dumps({
                 **base, 'appliesTo': [{
                     'path': 'x'
                 }]
             })),
            ('appliesTo entry wrong path type',
             json.dumps({
                 **base, 'appliesTo': [{
                     'path': 1,
                     'checksum': 'h'
                 }]
             })),
            ('missing plaster',
             json.dumps({
                 k: v
                 for k, v in base.items() if k != 'plaster'
             })),
            ('non-dict plaster', json.dumps({
                **base, 'plaster': 'oops'
            })),
            ('plaster missing path',
             json.dumps({
                 **base, 'plaster': {
                     'checksum': 'hp'
                 }
             })),
            ('plaster missing checksum',
             json.dumps({
                 **base, 'plaster': {
                     'path': 'p'
                 }
             })),
            ('plaster wrong checksum type',
             json.dumps({
                 **base, 'plaster': {
                     'path': 'p',
                     'checksum': 7
                 }
             })),
        ]

        for name, content in cases:
            with self.subTest(case=name):
                self.assertIsNone(plaster.Patchinfo.from_json(content))

    def test_parsed_instance_is_frozen(self):
        """Patchinfo and Patchinfo.Entry both reject attribute assignment."""
        from dataclasses import FrozenInstanceError
        info = plaster.Patchinfo.from_json(self._VALID_JSON)
        self.assertIsNotNone(info)
        with self.assertRaises(FrozenInstanceError):
            info.schema_version = 99
        with self.assertRaises(FrozenInstanceError):
            info.applies_to.path = 'other'

    def test_parsed_instance_equality_and_hashable(self):
        """Two patchinfos parsed from identical JSON compare equal and hash
        equal."""
        a = plaster.Patchinfo.from_json(self._VALID_JSON)
        b = plaster.Patchinfo.from_json(self._VALID_JSON)
        self.assertEqual(a, b)
        self.assertEqual(hash(a), hash(b))

    def test_to_json_matches_schema(self):
        """to_json emits exactly the keys/structure of a .patchinfo file."""
        info = plaster.Patchinfo(
            schema_version=1,
            patch_checksum='pc',
            applies_to=plaster.Patchinfo.Entry(path='src.cc', checksum='sc'),
            plaster=plaster.Patchinfo.Entry(path='r.toml', checksum='rc'),
        )
        self.assertEqual(
            json.loads(info.to_json()), {
                'schemaVersion': 1,
                'patchChecksum': 'pc',
                'appliesTo': [{
                    'path': 'src.cc',
                    'checksum': 'sc'
                }],
                'plaster': {
                    'path': 'r.toml',
                    'checksum': 'rc'
                },
            })

    def test_json_roundtrip(self):
        """from_json(x.to_json()) == x for any well-formed Patchinfo."""
        original = plaster.Patchinfo.from_json(self._VALID_JSON)
        self.assertIsNotNone(original)
        roundtripped = plaster.Patchinfo.from_json(original.to_json())
        self.assertEqual(original, roundtripped)


if __name__ == '__main__':
    unittest.main()
