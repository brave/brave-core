#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from pathlib import Path
import argparse
import contextlib
import hashlib
import io
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

    def test_original_expected_yaml_rules(self):
        """Test applying all .yaml files in the test/ folder."""
        test_folder = Path(__file__).parent / 'test/plasters'
        yaml_files = test_folder.rglob('*.yaml')

        for yaml_file in yaml_files:
            with self.subTest(yaml_file=yaml_file):
                # Deduce the path for the original file for the yaml to be
                # applied to.
                original_file_to = (yaml_file.parent / yaml_file.stem)
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

                # Copy the .yaml file to the fake Brave rewrite path.
                rewrite_path = plaster.PLASTER_FILES_PATH / yaml_file.name
                rewrite_path.parent.mkdir(parents=True, exist_ok=True)
                rewrite_path.write_text(yaml_file.read_text())

                # Use PlasterFile to apply the .yaml file to the committed file.
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
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          substitutions:
            - description: Simple test substitution
              re_pattern: 'Chromium'
              replace: 'Plaster'
        ''')

        # Use PlasterFile to apply the .yaml file to the committed file.
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

    def test_checksum_hashes_raw_bytes_without_newline_normalization(self):
        # The checksum must be over the file's raw bytes so it matches
        # build/commands/lib/calculateFileChecksum.js, which apply_patches uses
        # to validate .patchinfo entries. A text-mode read would fold CRLF into
        # LF and diverge for files with Windows line endings.
        temp_file = plaster.PLASTER_FILES_PATH / 'temp_crlf_checksum.txt'
        content = b'line one\r\nline two\r\n'
        temp_file.write_bytes(content)
        try:
            pair = plaster.PathChecksumPair(temp_file)
            self.assertEqual(pair.checksum,
                             hashlib.sha256(content).hexdigest())
            # An LF-normalized digest (what a text-mode read produced) must not
            # match, confirming the CRLF bytes are preserved.
            self.assertNotEqual(
                pair.checksum,
                hashlib.sha256(content.replace(b'\r\n', b'\n')).hexdigest())
        finally:
            temp_file.unlink()

    def test_checksum_of_empty_file(self):
        temp_file = plaster.PLASTER_FILES_PATH / 'temp_empty_checksum.txt'
        temp_file.write_bytes(b'')
        try:
            pair = plaster.PathChecksumPair(temp_file)
            self.assertEqual(pair.checksum, hashlib.sha256(b'').hexdigest())
        finally:
            temp_file.unlink()

    def test_yaml_plaster_applies(self):
        """A .yaml plaster applies its substitutions and emits patch files."""
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_yaml_plaster.idl')

        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Initial content for Chromium file.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_yaml_plaster.idl',
                                      self.fake_chromium_src.chromium)

        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('substitutions:\n'
                                '  - description: Simple yaml substitution\n'
                                "    re_pattern: 'Chromium'\n"
                                "    replace: 'Plaster'\n")

        plaster.PlasterFile(plaster_path).apply()

        self.assertEqual(
            (self.fake_chromium_src.chromium / test_file_chromium).read_text(),
            'Initial content for Plaster file.')

        # The patch file is named after the source, so applying the plaster
        # should produce both a patch and a patchinfo file.
        patchinfo = plaster.PatchinfoBuilder(plaster_path)
        self.assertTrue(patchinfo.patch.path.exists())
        self.assertTrue(patchinfo.patchinfo.path.exists())

    def test_yaml_plaster_validation_failures(self):
        """YAML plasters surface validation errors for malformed entries."""
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_yaml_validation.idl')

        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Initial content for Chromium file.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_yaml_validation.idl',
                                      self.fake_chromium_src.chromium)

        cases = [
            ('substitutions:\n'
             '  - description: Both patterns specified\n'
             "    pattern: 'Chromium'\n"
             "    re_pattern: 'Chromium'\n"
             "    replace: 'Plaster'\n",
             'Please specify either pattern or re_pattern'),
            ('substitutions:\n'
             '  - description: No pattern specified\n'
             "    replace: 'Plaster'\n", 'No pattern specified'),
            ('substitutions:\n'
             '  - description: No replace specified\n'
             "    pattern: 'Chromium'\n", 'No replace value specified'),
        ]

        for yaml_content, expected_error in cases:
            with self.subTest(error=expected_error):
                plaster_path = plaster.PLASTER_FILES_PATH / (
                    str(test_file_chromium) + '.yaml')
                plaster_path.parent.mkdir(parents=True, exist_ok=True)
                plaster_path.write_text(yaml_content)

                plaster_file = plaster.PlasterFile(plaster_path)
                with self.assertRaises(ValueError) as context:
                    plaster_file.apply()
                self.assertIn(expected_error, str(context.exception))

    def test_duplicate_yaml_keys_are_rejected(self):
        """A YAML mapping with a duplicate key surfaces as a ValueError.

        Without this guard, `yaml.safe_load` would silently keep the last
        value for the repeated key, and a typo would shadow the real
        field — applying a different patch than the author intended.
        """
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_duplicate_key.idl')

        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Content with Chromium word.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_duplicate_key.idl',
                                      self.fake_chromium_src.chromium)

        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        # `pattern` is declared twice; the second occurrence would
        # silently win without the strict loader.
        plaster_path.write_text('substitutions:\n'
                                '  - description: Duplicate pattern key\n'
                                "    pattern: 'Chromium'\n"
                                "    pattern: 'Brave'\n"
                                "    replace: 'Brave'\n")

        plaster_file = plaster.PlasterFile(plaster_path)
        with self.assertRaises(ValueError) as context:
            plaster_file.apply()
        message = str(context.exception)
        self.assertIn('Duplicate key', message)
        self.assertIn("'pattern'", message)

    def test_unknown_substitution_key_is_rejected(self):
        """Unrecognised substitution keys raise instead of being ignored."""
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_unknown_key.idl')

        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Content with Chromium word.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_unknown_key.idl',
                                      self.fake_chromium_src.chromium)

        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        # Common typo: `replacement` (the field is `replace`).
        plaster_path.write_text('substitutions:\n'
                                '  - description: Typo in key name\n'
                                "    pattern: 'Chromium'\n"
                                "    replacement: 'Brave'\n")

        plaster_file = plaster.PlasterFile(plaster_path)
        with self.assertRaises(ValueError) as context:
            plaster_file.apply()
        message = str(context.exception)
        self.assertIn('Unrecognised substitution key', message)
        self.assertIn("'replacement'", message)

    def test_check_success_multiple_up_to_date(self):
        """Test plaster check succeeds for 3 up-to-date plaster files."""
        # Create 3 source files in chromium and matching .yaml in brave/rewrite
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
            # Write the .yaml rewrite file
            rewrite_path = plaster.PLASTER_FILES_PATH / (str(src_path) +
                                                         '.yaml')
            rewrite_path.parent.mkdir(parents=True, exist_ok=True)
            rewrite_path.write_text(f'''
              substitutions:
                - description: Replace {orig} with {repl}
                  re_pattern: '{orig}'
                  replace: '{repl}'
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

    def test_check_fails_when_yaml_changed(self):
        """Test plaster check fails when there's a mismatch."""
        # Create 3 source files in chromium and matching .yaml in brave/rewrite
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
                                                         '.yaml')
            rewrite_path.parent.mkdir(parents=True, exist_ok=True)
            rewrite_path.write_text(f'''
              substitutions:
                - description: Replace {orig} with {repl}
                  re_pattern: '{orig}'
                  replace: '{repl}'
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
        # Now change one yaml file to cause a failure
        changed_path = rewrite_paths[1]
        changed_path.write_text('''
          substitutions:
            - description: Break the rule
              re_pattern: 'foo2'
              replace: 'DIFFERENT'
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
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          substitutions:
            - description: Test multiple flags in array work
              re_pattern: 'chromium'
              replace: 'Brave'
              re_flags: ['IGNORECASE', 'MULTILINE']
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
                    str(test_file_chromium) + '.yaml')
                plaster_path.parent.mkdir(parents=True, exist_ok=True)
                plaster_path.write_text(f'''
                  substitutions:
                    - description: Test invalid flag rejection
                      re_pattern: 'Chromium'
                      replace: 'Brave'
                      re_flags: ['{invalid_flag}']
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
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          substitutions:
            - description: Test empty flags list
              re_pattern: 'chromium'
              replace: 'Brave'
              re_flags: []
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
                    str(test_file_chromium) + '.yaml')
                plaster_path.parent.mkdir(parents=True, exist_ok=True)
                plaster_path.write_text(f'''
                  substitutions:
                    - description: Test invalid regex rejection
                      re_pattern: '{invalid_pattern}'
                      replace: 'Brave'
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
            # (yaml_content, expected_error_message)
            ('''
              substitutions:
                - description: Both patterns specified
                  pattern: 'Chromium'
                  re_pattern: 'Chromium'
                  replace: 'Plaster'
            ''', 'Please specify either pattern or re_pattern'),
            ('''
              substitutions:
                - description: No pattern specified
                  replace: 'Plaster'
            ''', 'No pattern specified'),
            ('''
              substitutions:
                - description: No replace specified
                  pattern: 'Chromium'
            ''', 'No replace value specified'),
        ]

        for yaml_content, expected_error in validation_cases:
            with self.subTest(error=expected_error):
                # Create a PlasterFile with invalid configuration
                plaster_path = plaster.PLASTER_FILES_PATH / (
                    str(test_file_chromium) + '.yaml')
                plaster_path.parent.mkdir(parents=True, exist_ok=True)
                plaster_path.write_text(yaml_content)

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
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          substitutions:
            - description: Replace exact pattern
              pattern: 'Chromium++'
              replace: 'Brave++'
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
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          substitutions:
            - description: Replace regex pattern
              re_pattern: 'Chromium\\w+'
              replace: 'Brave'
              count: 2
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
            str(test_file_chromium1) + '.yaml')
        plaster_path1.parent.mkdir(parents=True, exist_ok=True)
        plaster_path1.write_text('''
          substitutions:
            - description: Replace exact brackets
              pattern: '[brackets]'
              replace: '{braces}'
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
            str(test_file_chromium2) + '.yaml')
        plaster_path2.parent.mkdir(parents=True, exist_ok=True)
        plaster_path2.write_text('''
          substitutions:
            - description: Replace using regex
              re_pattern: '\\[\\w+\\]'
              replace: '{braces}'
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
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          substitutions:
            - description: Test count mismatch
              re_pattern: 'Chromium'
              replace: 'Brave'
              count: 2
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
                                (str(test_file_correct) + '.yaml'))
        plaster_path_correct.parent.mkdir(parents=True, exist_ok=True)
        plaster_path_correct.write_text('''
          substitutions:
            - description: Test default count with 1 match
              re_pattern: 'Chromium'
              replace: 'Brave'
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
                                  (str(test_file_incorrect) + '.yaml'))
        plaster_path_incorrect.parent.mkdir(parents=True, exist_ok=True)
        plaster_path_incorrect.write_text('''
          substitutions:
            - description: Test default count with 2 matches
              re_pattern: 'Chromium'
              replace: 'Brave'
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
        Test that count=0 replaces all matches (requiring at least one).
        count=0 rewrites every match but still fails if nothing matched; here
        there are three matches, so it succeeds.
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
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          substitutions:
            - description: Test count 0 replaces all
              re_pattern: 'Chromium'
              replace: 'Brave'
              count: 0
        ''')

        # Should succeed: count=0 replaces all matches, and there is at least
        # one match here.
        plaster_file = plaster.PlasterFile(plaster_path)
        plaster_file.apply()

        result = (self.fake_chromium_src.chromium /
                  test_file_chromium).read_text()
        self.assertEqual(result,
                         'Brave content with Brave and more Brave text.')

    def test_count_zero_no_match_fails(self):
        """
        Test that count=0 fails when there are no matches.

        count=0 means "one or more matches"; a pattern that matches nothing
        must fail rather than silently leave the source untouched.
        """
        test_file_chromium = Path(
            'chrome/common/extensions/api/test_count_zero_no_match.idl')

        # Write and commit a file that lacks the pattern entirely.
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'A Chromium thing.',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add test_count_zero_no_match.idl',
                                      self.fake_chromium_src.chromium)

        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          substitutions:
            - description: replace a pattern that is absent
              re_pattern: 'DoesNotAppear'
              replace: 'X'
              count: 0
        ''')

        plaster_file = plaster.PlasterFile(plaster_path)
        with self.assertRaises(plaster.PlasterApplyError) as ctx:
            plaster_file.apply()
        self.assertIn('at least one match', str(ctx.exception))

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
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          substitutions:
            - description: Replace single Chromium
              re_pattern: 'Chromium'
              replace: 'Brave'
              count: 1
            - description: Replace all browsers
              re_pattern: 'browser'
              replace: 'application'
              count: 3
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
        """Stages a chromium source, writes a plaster yaml, and applies it."""
        self.fake_chromium_src.write_and_stage_file(
            test_file, initial_content, self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit(f'Add {test_file.name}',
                                      self.fake_chromium_src.chromium)
        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file) + '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          substitutions:
            - description: Replace Chromium with Brave
              re_pattern: 'Chromium'
              replace: 'Brave'
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

    def test_needs_apply_true_after_yaml_change(self):
        """needs_apply returns True when the plaster yaml is modified."""
        test_file = Path(
            'chrome/common/extensions/api/needs_apply_yaml_change.idl')
        plaster_file = self._setup_applied_plaster(
            test_file, 'Initial Chromium content.')
        later = plaster_file.path.stat().st_mtime + 10
        plaster_file.path.write_text('''
          substitutions:
            - description: A different rule
              re_pattern: 'Brave'
              replace: 'Lion'
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

    # A source whose hunk-header function context differs depending on the
    # userdiff driver git selects: the built-in `objc` driver's xfuncname regex
    # skips the ObjC++ `Profile::~Profile()` member definition and anchors on
    # the previous C-style free function, whereas the default/`cpp` driver
    # reports the enclosing destructor. The change lands deep enough inside the
    # destructor that the hunk's leading context does not reach its opening
    # line, forcing git to search backwards for the function name.
    _DRIVER_SENSITIVE_MM = ('// Copyright.\n'
                            '#include "thing.h"\n'
                            '\n'
                            'void AssignTestingFactories(int a,\n'
                            '                            int b) {\n'
                            '  DoSomething();\n'
                            '}\n'
                            '\n'
                            'Profile::~Profile() {\n'
                            '  // Allows blocking in this scope for testing.\n'
                            '  ScopedAllowBlocking allow;\n'
                            '\n'
                            '  // Notify before destroying anything.\n'
                            '  NotifyDestroyed();\n'
                            '\n'
                            '  // Tear down the incognito profile first.\n'
                            '  otr_.reset();\n'
                            '\n'
                            '  // Shut dependencies down backward.\n'
                            '  MARKER_LINE;\n'
                            '  more_cleanup();\n'
                            '}\n')

    def test_patch_generation_ignores_ambient_git_attributes(self):
        """Generated patches do not depend on the developer's git attributes.

        The hunk-header function context is produced by whichever userdiff
        driver git selects for a source, and that selection depends on the
        ambient environment (a per-user/global `core.attributesFile`, the
        system gitattributes, git version built-ins). `.mm` files in particular
        resolve to the `objc` driver on some setups (notably Apple Git), which
        yields a different hunk header than the default driver and so makes the
        committed patch bytes differ between machines. `save_patch_if_changed`
        pins the diff to a dedicated attributes file and disables the system
        gitattributes so the output is deterministic regardless of environment.
        """
        test_file = Path('chrome/browser/profile.mm')
        self.fake_chromium_src.write_and_stage_file(
            test_file, self._DRIVER_SENSITIVE_MM,
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add profile.mm',
                                      self.fake_chromium_src.chromium)

        # Simulate a machine whose ambient git configuration routes `.mm` files
        # through the `objc` driver, e.g. via a per-user `core.attributesFile`.
        ambient_attributes = (self.fake_chromium_src.base_path /
                              'ambient_gitattributes')
        ambient_attributes.write_text('*.mm diff=objc\n')
        self.fake_chromium_src._run_git_command(
            ['config', 'core.attributesFile',
             str(ambient_attributes)], self.fake_chromium_src.chromium)

        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file) + '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('substitutions:\n'
                                '  - description: Touch the destructor body\n'
                                "    pattern: 'MARKER_LINE;'\n"
                                "    replace: 'MARKER_LINE_CHANGED;'\n")

        plaster.PlasterFile(plaster_path).apply()

        patch = plaster.PatchinfoBuilder(plaster_path).patch.path.read_text()
        hunk_header = next(line for line in patch.splitlines()
                           if line.startswith('@@'))

        # Sanity check: the ambient `objc` mapping genuinely diverges from the
        # default driver for this source, so the assertion below is meaningful.
        # Otherwise a git without a diverging `objc` driver would make this test
        # pass without exercising anything.
        objc_diff = self.fake_chromium_src._run_git_command([
            '-c', f'core.attributesFile={ambient_attributes}', 'diff',
            str(test_file)
        ], self.fake_chromium_src.chromium)
        objc_header = next(line for line in objc_diff.splitlines()
                           if line.startswith('@@'))
        self.assertIn(
            'AssignTestingFactories', objc_header,
            'ambient objc mapping is expected to anchor the hunk '
            'header on the free function; git behavior may have '
            'changed')

        # The generated patch must ignore the ambient `objc` mapping and use the
        # default driver, which reports the enclosing destructor.
        self.assertIn('Profile::~Profile()', hunk_header)
        self.assertNotIn('AssignTestingFactories', hunk_header)


class RewriterFormsTest(unittest.TestCase):
    """End-to-end tests for the substitution envelope and the `regex` rewriter.

    Rewriters apply through `PlasterFile` against a fake Chromium repo, just
    like real usage. Only the `regex` rewriter exists for now; further
    rewriters attach to the same envelope later.
    """

    def setUp(self):
        self.fake_chromium_src = FakeChromiumRepo()
        self.fake_chromium_src.setup()
        self.addCleanup(self.fake_chromium_src.cleanup)

    def _apply(self, name: str, source: str, yaml_body: str) -> str:
        """Write `source`+plaster, apply, and return the rewritten source."""
        src = Path('chrome/common/extensions/api') / name
        self.fake_chromium_src.write_and_stage_file(
            src, source, self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit(f'Add {name}',
                                      self.fake_chromium_src.chromium)
        plaster_path = plaster.PLASTER_FILES_PATH / (str(src) + '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text(yaml_body)
        plaster.PlasterFile(plaster_path).apply()
        return (self.fake_chromium_src.chromium / src).read_text()

    def _expect_value_error(self, yaml_body: str, substr: str):
        with self.assertRaises(ValueError) as ctx:
            self._apply('validation.idl', 'dummy', yaml_body)
        self.assertIn(substr, str(ctx.exception))

    # -- regex op (explicit form of the legacy bare regex) ------------------

    def test_regex_op_matches_bare_form(self):
        result = self._apply(
            'regex_op.idl', 'A Chromium thing.', 'substitutions:\n'
            '  - description: explicit regex op\n'
            '    regex:\n'
            "      re_pattern: 'Chromium'\n"
            "      replace: 'Brave'\n")
        self.assertEqual(result, 'A Brave thing.')

    def test_regex_op_honours_flags(self):
        result = self._apply(
            'regex_flags.idl', 'foo\nBAR\n', 'substitutions:\n'
            '  - description: nested regex with flags\n'
            '    regex:\n'
            "      re_pattern: '^bar$'\n"
            "      replace: 'baz'\n"
            '      re_flags: [IGNORECASE, MULTILINE]\n')
        self.assertEqual(result, 'foo\nbaz\n')

    def test_bare_regex_still_applies(self):
        result = self._apply(
            'bare.idl', 'A Chromium thing.', 'substitutions:\n'
            '  - description: legacy bare regex\n'
            "    re_pattern: 'Chromium'\n"
            "    replace: 'Brave'\n")
        self.assertEqual(result, 'A Brave thing.')

    # -- validation ---------------------------------------------------------

    def test_cannot_mix_op_and_bare_regex(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: mixed\n'
            '    regex:\n'
            "      re_pattern: 'x'\n"
            "      replace: 'y'\n"
            "    re_pattern: 'z'\n", 'Cannot mix')

    def test_unknown_regex_field_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: bad regex field\n'
            '    regex:\n'
            "      re_pattern: 'x'\n"
            "      replace: 'y'\n"
            "      bogus: 'z'\n", 'Unrecognised regex field')

    def test_regex_op_must_be_a_mapping(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: scalar regex body\n'
            "    regex: 'nope'\n", '"regex" must be a mapping')

    def test_unknown_rewriter_is_rejected(self):
        # A rewriter-shaped key (mapping body) that is not registered names an
        # unknown rewriter; the error lists the available ones.
        with self.assertRaises(ValueError) as ctx:
            self._apply(
                'unknown_rw.h', 'class C {};\n', 'substitutions:\n'
                '  - description: not a real rewriter\n'
                '    make_virtual:\n'
                '      class_name: C\n'
                '      method_name: Foo\n')
        message = str(ctx.exception)
        self.assertIn('Unknown rewriter', message)
        self.assertIn("'make_virtual'", message)
        self.assertIn('regex', message)

    def test_stray_scalar_key_is_unrecognised(self):
        # A non-mapping stray key is a bare-field typo, not a rewriter attempt,
        # so it keeps the generic "Unrecognised substitution key" error.
        self._expect_value_error(
            'substitutions:\n'
            '  - description: typo bare field\n'
            "    re_pattern: 'x'\n"
            "    replace: 'y'\n"
            '    re_flag: [DOTALL]\n', 'Unrecognised substitution key')


class RewriterRegistryTest(unittest.TestCase):
    """The `_REWRITERS` registry drives both YAML dispatch and help."""

    def test_regex_is_registered_under_its_name(self):
        self.assertIs(plaster._REWRITERS['regex'], plaster.Regex)

    def test_registry_is_read_only(self):
        with self.assertRaises(TypeError):
            plaster._REWRITERS['regex'] = plaster.Regex

    def test_every_rewriter_is_self_describing(self):
        # Each rewriter must be keyed by its own NAME and carry the metadata the
        # help system relies on, so a new rewriter can never show up blank.
        for name, cls in plaster._REWRITERS.items():
            self.assertEqual(cls.NAME, name)
            self.assertTrue(cls.SUMMARY, f'{name} is missing a SUMMARY')
            self.assertTrue(cls.help_text(), f'{name} is missing help text')


class RewritersEvalTest(unittest.TestCase):
    """Schema evaluation and access tests for plaster.RewritersEval."""

    def setUp(self):
        # load() memoises a process-wide instance; clear it so tests that
        # exercise the singleton start from a clean slate.
        plaster.RewritersEval._instance = None
        self.addCleanup(setattr, plaster.RewritersEval, '_instance', None)

    @staticmethod
    def _valid_spec() -> dict:
        """A minimal, schema-valid rewriters spec as a Python dict."""
        return {
            'ast.matcher': {
                'cxx.find_class_method_decl': {
                    'args': ['class_name', 'method_name'],
                    'template': ('kind: field_declaration\n'
                                 'has:\n'
                                 '  regex: ^{method_name}$\n'
                                 'inside:\n'
                                 '  regex: ^{class_name}$\n'),
                    'result': {
                        'node': 'field_declaration',
                    },
                },
            },
            'ast.rewriter': {
                'cxx.make_virtual': {
                    'matcher': 'cxx.find_class_method_decl',
                    'replace': {
                        're_pattern': '^',
                        'replace': 'virtual '
                    },
                    'result': {
                        'node': 'field_declaration',
                    },
                },
            },
        }

    def _eval_valid(self) -> plaster.RewritersEval:
        return plaster.RewritersEval(repr(self._valid_spec()))

    def _assert_invalid(self, mutate, expected_substr=None):
        """Apply `mutate` to a valid spec and assert it fails validation."""
        spec = self._valid_spec()
        mutate(spec)
        with self.assertRaises(plaster.RewritersSchemaError) as cm:
            plaster.RewritersEval(repr(spec))
        if expected_substr is not None:
            self.assertIn(expected_substr, str(cm.exception))

    # -- the real on-disk spec ---------------------------------------------

    def test_load_real_rewriters_file(self):
        """The shipped rewriters.pyl validates and loads.

        It ships empty for now (ops are added when they are wired in), so this
        just asserts a clean load and empty, read-only op mappings.
        """
        rewriters = plaster.RewritersEval.load()
        self.assertEqual(dict(rewriters.matchers), {})
        self.assertEqual(dict(rewriters.rewriters), {})

    def test_load_is_a_singleton(self):
        """load() reads the file once and returns the same instance."""
        first = plaster.RewritersEval.load()
        second = plaster.RewritersEval.load()
        self.assertIs(first, second)

    # -- access -------------------------------------------------------------

    def test_accessors_return_specs(self):
        rewriters = self._eval_valid()
        self.assertEqual(
            rewriters.matcher('cxx.find_class_method_decl')['args'],
            ['class_name', 'method_name'])
        self.assertEqual(
            rewriters.rewriter('cxx.make_virtual')['matcher'],
            'cxx.find_class_method_decl')

    def test_unknown_op_access_raises(self):
        rewriters = self._eval_valid()
        with self.assertRaises(plaster.RewritersSchemaError):
            rewriters.matcher('cxx.nope')
        with self.assertRaises(plaster.RewritersSchemaError):
            rewriters.rewriter('cxx.nope')

    def test_language_of(self):
        self.assertEqual(
            plaster.RewritersEval.language_of('cxx.find_class_method_decl'),
            'cpp')
        with self.assertRaises(plaster.RewritersSchemaError):
            plaster.RewritersEval.language_of('py.find_class_method_decl')

    def test_exposed_mappings_are_read_only(self):
        rewriters = self._eval_valid()
        with self.assertRaises(TypeError):
            rewriters.matchers['x'] = {}
        with self.assertRaises(TypeError):
            rewriters.rewriters['x'] = {}

    def test_valid_spec_round_trips(self):
        rewriters = self._eval_valid()
        self.assertEqual(list(rewriters.matchers),
                         ['cxx.find_class_method_decl'])
        self.assertEqual(list(rewriters.rewriters), ['cxx.make_virtual'])

    # -- top-level / parsing failures --------------------------------------

    def test_not_a_literal(self):
        with self.assertRaises(plaster.RewritersSchemaError):
            plaster.RewritersEval('this is not a literal (((')

    def test_top_level_not_a_dict(self):
        with self.assertRaises(plaster.RewritersSchemaError):
            plaster.RewritersEval('[1, 2, 3]')

    def test_present_but_empty_groups_are_valid(self):
        # A group may be present with no ops yet (as the shipped file is).
        rewriters = plaster.RewritersEval(
            "{'ast.matcher': {}, 'ast.rewriter': {}}")
        self.assertEqual(dict(rewriters.matchers), {})
        self.assertEqual(dict(rewriters.rewriters), {})

    def test_unknown_category(self):
        # schema rejects keys outside the top-level matcher/rewriter set.
        self._assert_invalid(lambda s: s.update({'mangler': {}}), 'Wrong keys')

    def test_category_not_a_mapping(self):
        self._assert_invalid(lambda s: s.__setitem__('ast.matcher', []),
                             "should be instance of 'dict'")

    # -- op id --------------------------------------------------------------

    def test_op_id_without_prefix(self):
        # An id that does not match the _OP_ID key schema is an unexpected key.
        def mutate(s):
            s['ast.matcher']['nodothere'] = s['ast.matcher'].pop(
                'cxx.find_class_method_decl')

        self._assert_invalid(mutate, 'Wrong keys')

    def test_op_id_unknown_prefix(self):

        def mutate(s):
            s['ast.matcher']['py.find_class_method_decl'] = s[
                'ast.matcher'].pop('cxx.find_class_method_decl')

        self._assert_invalid(mutate, 'Wrong keys')

    # -- matcher schema ------------------------------------------------------

    def test_matcher_missing_required_key(self):
        self._assert_invalid(
            lambda s: s['ast.matcher']['cxx.find_class_method_decl'].pop(
                'template'), 'Missing keys')

    def test_matcher_unknown_key(self):
        self._assert_invalid(
            lambda s: s['ast.matcher']['cxx.find_class_method_decl'].update(
                {'language': 'cpp'}), 'Wrong keys')

    def test_matcher_args_not_list_of_strings(self):
        self._assert_invalid(
            lambda s: s['ast.matcher']['cxx.find_class_method_decl'].
            __setitem__('args', 'class_name'), "should be instance of 'list'")

    def test_matcher_undeclared_placeholder(self):
        self._assert_invalid(
            lambda s: s['ast.matcher']
            ['cxx.find_class_method_decl'].__setitem__(
                'template', 'regex: ^{class_name}$ ^{method_name}$ ^{bogus}$'),
            'undeclared placeholder')

    def test_matcher_unused_arg(self):
        self._assert_invalid(
            lambda s: s['ast.matcher']['cxx.find_class_method_decl']['args'].
            append('unused'), 'never used')

    def test_matcher_bad_result(self):
        self._assert_invalid(
            lambda s: s['ast.matcher']['cxx.find_class_method_decl']['result'].
            pop('node'), 'Missing keys')

    # -- rewriter schema ----------------------------------------------------

    def test_rewriter_unknown_matcher_reference(self):
        self._assert_invalid(
            lambda s: s['ast.rewriter']['cxx.make_virtual'].__setitem__(
                'matcher', 'cxx.ghost'), 'unknown matcher')

    def test_rewriter_replace_missing_key(self):
        self._assert_invalid(
            lambda s: s['ast.rewriter']['cxx.make_virtual']['replace'].pop(
                're_pattern'), 'Missing keys')

    def test_rewriter_invalid_replace_regex(self):
        self._assert_invalid(
            lambda s: s['ast.rewriter']['cxx.make_virtual']['replace'].
            __setitem__('re_pattern', '(unclosed'), 'valid regular expression')

    def test_rewriter_result_node_mismatch(self):
        self._assert_invalid(
            lambda s: s['ast.rewriter']['cxx.make_virtual']['result'].
            __setitem__('node', 'declaration'), 'does not match matcher')

    def test_rewriter_unknown_key(self):
        self._assert_invalid(
            lambda s: s['ast.rewriter']['cxx.make_virtual'].update(
                {'append': '!'}), 'Wrong keys')


# ast-grep matcher templates used to build synthetic RewritersEval specs for
# the engine tests below. The shipped rewriters.pyl is empty until the ops that
# consume these land; these mirror the specs plaster will ship then, so the
# engine can be exercised end-to-end against the real binary in the meantime.
_METHOD_DECL_RULE = ('any:\n'
                     '  - kind: field_declaration\n'
                     '  - kind: declaration\n'
                     'has:\n'
                     '  kind: function_declarator\n'
                     '  stopBy: end\n'
                     '  has:\n'
                     '    field: declarator\n'
                     '    regex: ^{method_name}$\n'
                     'inside:\n'
                     '  kind: class_specifier\n'
                     '  stopBy: end\n'
                     '  has:\n'
                     '    field: name\n'
                     '    regex: ^{class_name}$\n')

_PRIVATE_SECTION_RULE = ('kind: access_specifier\n'
                         'regex: ^private$\n'
                         'inside:\n'
                         '  kind: class_specifier\n'
                         '  stopBy: end\n'
                         '  has:\n'
                         '    field: name\n'
                         '    regex: ^{class_name}$\n')

_FINAL_RULE = ('kind: virtual_specifier\n'
               'regex: ^final$\n'
               'inside:\n'
               '  kind: class_specifier\n'
               '  has:\n'
               '    field: name\n'
               '    regex: ^{class_name}$\n')

_SYNTHETIC_SPEC = {
    'ast.matcher': {
        'cxx.find_class_method_decl': {
            'args': ['class_name', 'method_name'],
            'template': _METHOD_DECL_RULE,
            'result': {
                'node': 'field_declaration'
            },
        },
        'cxx.find_class_private_section': {
            'args': ['class_name'],
            'template': _PRIVATE_SECTION_RULE,
            'result': {
                'node': 'access_specifier'
            },
        },
        'cxx.find_class_final': {
            'args': ['class_name'],
            'template': _FINAL_RULE,
            'result': {
                'node': 'virtual_specifier'
            },
        },
    },
    'ast.rewriter': {
        'cxx.make_virtual': {
            'matcher': 'cxx.find_class_method_decl',
            'replace': {
                're_pattern': '^',
                'replace': 'virtual '
            },
            'result': {
                'node': 'field_declaration'
            },
        },
        'cxx.add_friend': {
            'matcher': 'cxx.find_class_private_section',
            'replace': {
                're_pattern': '$',
                'replace': ':\\n  friend {friend_type};'
            },
            'result': {
                'node': 'access_specifier'
            },
        },
        'cxx.remove_final': {
            'matcher': 'cxx.find_class_final',
            'replace': {
                're_pattern': '^final$',
                'replace': ''
            },
            'result': {
                'node': 'virtual_specifier'
            },
        },
    },
}


class RunAstGrepTest(unittest.TestCase):
    """Integration tests for plaster.run_ast_grep (real ast-grep binary)."""

    # A small C++ source. ASCII-only, so byte offsets equal character indices.
    _SRC = 'class C {\n  void Foo();\n  void Bar();\n};\n'

    def _find(self, method_name: str, source: str) -> list[plaster.AstMatch]:
        body = _METHOD_DECL_RULE.format(class_name='C',
                                        method_name=method_name)
        return plaster.run_ast_grep(language='cpp',
                                    rule_body=body,
                                    source=source)

    def test_finds_match_with_byte_offsets(self):
        matches = self._find('Foo', self._SRC)
        self.assertEqual(len(matches), 1)
        # AstMatch is a byte range; the text is read back from the source.
        raw = self._SRC.encode('utf-8')
        m = matches[0]
        self.assertEqual(raw[m.start:m.end], b'void Foo();')
        self.assertEqual(m.length, len(b'void Foo();'))
        self.assertEqual(m.end, m.start + m.length)

    def test_no_match_returns_empty(self):
        self.assertEqual(self._find('Nope', self._SRC), [])

    def test_overloads_each_match(self):
        source = 'class C {\n  void Foo();\n  void Foo(int x);\n};\n'
        raw = source.encode('utf-8')
        matches = self._find('Foo', source)
        self.assertEqual([raw[m.start:m.end].decode() for m in matches],
                         ['void Foo();', 'void Foo(int x);'])

    def test_raises_on_bad_rule(self):
        with self.assertRaises(plaster.AstGrepError):
            plaster.run_ast_grep(language='cpp',
                                 rule_body='kind: not_a_real_kind',
                                 source='int x;\n')


class AstRewriterTest(unittest.TestCase):
    """Integration tests for plaster.AstRewriter (real ast-grep binary).

    Driven with a synthetic RewritersEval built from `_SYNTHETIC_SPEC`, since
    the shipped rewriters.pyl carries no ops yet.
    """

    _SRC = 'class C {\n  void Foo();\n  void Bar();\n};\n'

    def _rewriter(self, source: str = _SRC) -> plaster.AstRewriter:
        return plaster.AstRewriter(
            plaster.RewritersEval(repr(_SYNTHETIC_SPEC)), source)

    def test_make_virtual_single(self):
        rewriter = self._rewriter()
        count = rewriter.apply('cxx.make_virtual', {
            'class_name': 'C',
            'method_name': 'Foo'
        })
        self.assertEqual(count, 1)
        self.assertEqual(
            rewriter.content,
            'class C {\n  virtual void Foo();\n  void Bar();\n};\n')

    def test_make_virtual_destructor(self):
        # Destructors parse as `declaration` with a `destructor_name`, not the
        # `field_declaration`/`field_identifier` of a regular method.
        rewriter = self._rewriter('class C {\n public:\n  ~C();\n};\n')
        count = rewriter.apply('cxx.make_virtual', {
            'class_name': 'C',
            'method_name': '~C'
        })
        self.assertEqual(count, 1)
        self.assertEqual(rewriter.content,
                         'class C {\n public:\n  virtual ~C();\n};\n')

    def test_make_virtual_overloads_count_each(self):
        rewriter = self._rewriter(
            'class C {\n  void Foo();\n  void Foo(int x);\n};\n')
        count = rewriter.apply('cxx.make_virtual', {
            'class_name': 'C',
            'method_name': 'Foo'
        })
        self.assertEqual(count, 2)
        # Splicing from the end keeps the earlier overload's offset valid.
        self.assertEqual(
            rewriter.content, 'class C {\n  virtual void Foo();\n'
            '  virtual void Foo(int x);\n};\n')

    def test_no_match_leaves_content_unchanged(self):
        rewriter = self._rewriter()
        self.assertEqual(
            rewriter.apply('cxx.make_virtual', {
                'class_name': 'C',
                'method_name': 'Nope'
            }), 0)
        self.assertEqual(rewriter.content, self._SRC)

    def test_content_accumulates_across_calls(self):
        rewriter = self._rewriter()
        rewriter.apply('cxx.make_virtual', {
            'class_name': 'C',
            'method_name': 'Foo'
        })
        rewriter.apply('cxx.make_virtual', {
            'class_name': 'C',
            'method_name': 'Bar'
        })
        self.assertEqual(
            rewriter.content,
            'class C {\n  virtual void Foo();\n  virtual void Bar();\n};\n')

    def test_add_friend_inserts_after_private_colon(self):
        rewriter = self._rewriter(
            'class C {\n public:\n  void Foo();\n private:\n  int x_;\n};\n')
        count = rewriter.apply('cxx.add_friend', {
            'class_name': 'C',
            'friend_type': 'class BraveC'
        },
                               consume_after=':')
        self.assertEqual(count, 1)
        # The friend lands as the first private line; the `:` is not duplicated.
        self.assertEqual(
            rewriter.content, 'class C {\n public:\n  void Foo();\n'
            ' private:\n  friend class BraveC;\n  int x_;\n};\n')

    def test_add_friend_no_private_section(self):
        rewriter = self._rewriter('class C {\n public:\n  void Foo();\n};\n')
        self.assertEqual(
            rewriter.apply('cxx.add_friend', {
                'class_name': 'C',
                'friend_type': 'class BraveC'
            },
                           consume_after=':'), 0)
        self.assertEqual(rewriter.content,
                         'class C {\n public:\n  void Foo();\n};\n')

    def test_remove_final_with_base(self):
        # The class `final` is dropped (and the space before it); a method's
        # trailing `final` is left untouched.
        rewriter = self._rewriter(
            'class C final : public Base {\n  void f() final;\n};\n')
        self.assertEqual(
            rewriter.apply('cxx.remove_final', {'class_name': 'C'},
                           consume_before=' '), 1)
        self.assertEqual(rewriter.content,
                         'class C : public Base {\n  void f() final;\n};\n')

    def test_remove_final_no_base(self):
        rewriter = self._rewriter('class C final {\n};\n')
        self.assertEqual(
            rewriter.apply('cxx.remove_final', {'class_name': 'C'},
                           consume_before=' '), 1)
        self.assertEqual(rewriter.content, 'class C {\n};\n')

    def test_remove_final_absent(self):
        rewriter = self._rewriter('class C {\n};\n')
        self.assertEqual(
            rewriter.apply('cxx.remove_final', {'class_name': 'C'},
                           consume_before=' '), 0)
        self.assertEqual(rewriter.content, 'class C {\n};\n')


class HelpTest(unittest.TestCase):
    """gn-style `plaster --help [topic]` overview, categories and topic docs.

    Every case drives the real `Help` action through a parser wired like `main`
    (`--help [topic]`), so parsing and rendering are exercised together.
    """

    def _parse(self, *topic: str) -> tuple[int, str]:
        """Parse `--help [topic]`, returning its exit code and stdout.

        The parser is wired with the global options and the `Help` action the
        same way `main` does, plus a fake command registry.
        """
        apply_parser = argparse.ArgumentParser(prog='plaster apply')
        apply_parser.add_argument('--all', action='store_true')
        commands = {
            'apply': (apply_parser, 'Apply all plaster files.'),
            'check': (argparse.ArgumentParser(prog='plaster check'), 'Check.'),
        }
        parser = argparse.ArgumentParser(add_help=False)
        parser.add_argument('--verbose',
                            action='store_true',
                            help='Enable verbose logging')
        parser.add_argument('-h',
                            '--help',
                            action=plaster.Help,
                            commands=commands)

        buf = io.StringIO()
        # `Help` prints via rich `console` and, for command topics, via
        # `argparse.print_help()`; both land on stdout. It always exits.
        with contextlib.redirect_stdout(buf):
            with self.assertRaises(SystemExit) as ctx:
                parser.parse_args(['--help', *topic])
        return ctx.exception.code, buf.getvalue()

    def test_overview_lists_usage_categories_and_options(self):
        code, out = self._parse()
        self.assertEqual(code, 0)
        self.assertIn('usage:', out)
        self.assertIn('Commands', out)
        self.assertIn('apply', out)
        self.assertIn('Rewriters', out)
        self.assertIn('regex', out)
        # The global options block must survive our custom overview.
        self.assertIn('Options', out)
        self.assertIn('--verbose', out)
        self.assertIn('--help', out)

    def test_commands_category_prints_only_commands(self):
        code, out = self._parse('commands')
        self.assertEqual(code, 0)
        self.assertIn('apply', out)
        self.assertNotIn('Rewriters', out)

    def test_rewriters_category_prints_only_rewriters(self):
        code, out = self._parse('rewriters')
        self.assertEqual(code, 0)
        self.assertIn('regex', out)
        self.assertNotIn('Commands', out)

    def test_rewriter_topic_prints_its_docs(self):
        code, out = self._parse('regex')
        self.assertEqual(code, 0)
        self.assertIn('re.subn', out)
        self.assertIn('re_flags', out)

    def test_command_topic_prints_argparse_help(self):
        code, out = self._parse('apply')
        self.assertEqual(code, 0)
        self.assertIn('usage', out)
        self.assertIn('--all', out)

    def test_unknown_topic_is_an_error(self):
        code, out = self._parse('does-not-exist')
        self.assertEqual(code, 1)
        self.assertIn('Unknown help topic', out)


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
        "path": "rewrite/chrome/updater/mac/.install.sh.yaml",
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
                         'rewrite/chrome/updater/mac/.install.sh.yaml')
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
            plaster=plaster.Patchinfo.Entry(path='r.yaml', checksum='rc'),
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
                    'path': 'r.yaml',
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
