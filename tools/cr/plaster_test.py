#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from pathlib import Path
from unittest import mock
import argparse
import contextlib
import copy
import dataclasses
import hashlib
import io
import json
import os
import time

import plaster
import repository

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
              regex:
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
            hashlib.sha256(patchinfo.patch.path.read_bytes()).hexdigest())
        self.assertEqual(patchinfo_from_disk['appliesTo'][0]['path'],
                         str(test_file_chromium))
        self.assertEqual(
            patchinfo_from_disk['appliesTo'][0]['checksum'],
            hashlib.sha256((self.fake_chromium_src.chromium /
                            test_file_chromium).read_bytes()).hexdigest())
        # PatchinfoBuilder normalizes plaster path to be relative to brave root.
        self.assertEqual(patchinfo_from_disk['plaster']['path'],
                         str(patchinfo.plaster_file))
        self.assertEqual(patchinfo_from_disk['plaster']['checksum'],
                         hashlib.sha256(plaster_path.read_bytes()).hexdigest())

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

    def test_save_patch_pins_diff_algorithm_and_attributes(self):
        """save_patch_if_changed must pin the diff algorithm and attributes.

        Regression test: a user's own `.gitconfig` (`diff.algorithm`) or
        gitattributes could otherwise make the same substitution produce
        different patch bytes than what CI generates, failing presubmit.
        """
        test_file_chromium = Path('chrome/common/pin_test.cc')
        self.fake_chromium_src.write_and_stage_file(
            test_file_chromium, 'Initial content for Chromium file.\n',
            self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit('Add pin_test.cc',
                                      self.fake_chromium_src.chromium)

        plaster_path = plaster.PLASTER_FILES_PATH / (str(test_file_chromium) +
                                                     '.yaml')
        plaster_path.parent.mkdir(parents=True, exist_ok=True)
        plaster_path.write_text('''
          substitutions:
            - description: Simple test substitution
              regex:
                re_pattern: 'Chromium'
                replace: 'Plaster'
        ''')

        plaster_file = plaster.PlasterFile(plaster_path)
        with mock.patch.object(
                repository.Repository,
                'run_git',
                autospec=True,
                side_effect=repository.Repository.run_git) as run_git_mock:
            plaster_file.apply()

        # `autospec` includes the bound `self` as the first positional arg.
        diff_calls = [
            call for call in run_git_mock.call_args_list
            if 'diff' in call.args[1:]
        ]
        self.assertEqual(len(diff_calls), 1)
        diff_args = diff_calls[0].args[1:]
        pinned_options = set(zip(diff_args, diff_args[1:]))
        self.assertIn(('-c', 'diff.algorithm=histogram'), pinned_options)
        self.assertIn(
            ('-c',
             f'core.attributesFile={plaster.PLASTER_GITATTRIBUTES_PATH}'),
            pinned_options)
        self.assertEqual(
            diff_calls[0].kwargs.get('env', {}).get('GIT_ATTR_NOSYSTEM'), '1')

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
                                '    regex:\n'
                                "      re_pattern: 'Chromium'\n"
                                "      replace: 'Plaster'\n")

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
             '    regex:\n'
             "      pattern: 'Chromium'\n"
             "      re_pattern: 'Chromium'\n"
             "      replace: 'Plaster'\n",
             'Please specify either pattern or re_pattern'),
            ('substitutions:\n'
             '  - description: No pattern specified\n'
             '    regex:\n'
             "      replace: 'Plaster'\n", 'No pattern specified'),
            ('substitutions:\n'
             '  - description: No replace specified\n'
             '    regex:\n'
             "      pattern: 'Chromium'\n", 'No replace value specified'),
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
                  regex:
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
                  regex:
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
              regex:
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
              regex:
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
                      regex:
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
              regex:
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
                      regex:
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
                  regex:
                    pattern: 'Chromium'
                    re_pattern: 'Chromium'
                    replace: 'Plaster'
            ''', 'Please specify either pattern or re_pattern'),
            ('''
              substitutions:
                - description: No pattern specified
                  regex:
                    replace: 'Plaster'
            ''', 'No pattern specified'),
            ('''
              substitutions:
                - description: No replace specified
                  regex:
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
              regex:
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
              regex:
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
              regex:
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
              regex:
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
              regex:
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
              regex:
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
              regex:
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
              regex:
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
              regex:
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
              regex:
                re_pattern: 'Chromium'
                replace: 'Brave'
              count: 1
            - description: Replace all browsers
              regex:
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
              regex:
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
              regex:
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
                                '    regex:\n'
                                "      pattern: 'MARKER_LINE;'\n"
                                "      replace: 'MARKER_LINE_CHANGED;'\n")

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
    """End-to-end tests for the substitution envelope and its rewriters.

    Rewriters apply through `PlasterFile` against a fake Chromium repo, just
    like real usage; `make_virtual` runs the real ast-grep binary. Further
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

    def _expect_value_error(self,
                            yaml_body: str,
                            substr: str,
                            name: str = 'validation.cc'):
        # Defaults to a C++ target, since most callers check how a cxx
        # rewriter validates its own body -- which it only gets to do once the
        # name has resolved to it, so the target has to be one it serves. Pass
        # a `name` outside that namespace to test the resolution itself.
        with self.assertRaises(ValueError) as ctx:
            self._apply(name, 'dummy', yaml_body)
        self.assertIn(substr, str(ctx.exception))

    # -- regex op -----------------------------------------------------------

    def test_regex_op_applies(self):
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

    def test_bare_regex_is_rejected(self):
        # The bare regex form (regex fields directly on the item, without a
        # `regex:` key) is no longer supported: with no rewriter key it is an
        # entry that names no rewriter.
        self._expect_value_error(
            'substitutions:\n'
            '  - description: legacy bare regex\n'
            "    re_pattern: 'Chromium'\n"
            "    replace: 'Brave'\n", 'Unrecognised substitution key')

    # -- make_virtual op (real ast-grep binary) -----------------------------

    def test_make_virtual_op(self):
        result = self._apply(
            'virt.h', 'class C {\n  void Foo();\n};\n', 'substitutions:\n'
            '  - description: make Foo virtual\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: Foo\n')
        self.assertEqual(result, 'class C {\n  virtual void Foo();\n};\n')

    def test_make_virtual_on_inline_defined_method(self):
        # A method defined inline (with a body, not just declared) parses as
        # a function_definition, not a field_declaration -- the real bug this
        # covers: `UpdateContent` in tab_hover_card_bubble_view.cc is defined
        # this way, and make_virtual used to find nothing to match.
        source = ('class C {\n'
                  ' public:\n'
                  '  void UpdateContent(const Data* data) {\n'
                  '    Apply(data);\n'
                  '  }\n'
                  '};\n')
        result = self._apply(
            'inline_defined.h', source, 'substitutions:\n'
            '  - description: make the inline-defined method virtual\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: UpdateContent\n')
        self.assertEqual(
            result,
            source.replace('  void UpdateContent',
                           '  virtual void UpdateContent'))

    def test_make_virtual_on_inline_defined_method_qualified_nested_class(
            self):
        # The real fixture: an out-of-line nested class (needing the fully
        # qualified `class_name`, per the earlier fix) whose method is also
        # defined inline (needing this fix), together.
        source = ('class Outer::Inner : public views::View {\n'
                  ' public:\n'
                  '  void UpdateContent(const Data* data) {\n'
                  '    Apply(data);\n'
                  '  }\n'
                  '};\n')
        result = self._apply(
            'inline_defined_nested.h', source, 'substitutions:\n'
            '  - description: make the inline-defined nested method virtual\n'
            '    make_virtual:\n'
            '      class_name: Outer::Inner\n'
            '      method_name: UpdateContent\n')
        self.assertEqual(
            result,
            source.replace('  void UpdateContent',
                           '  virtual void UpdateContent'))

    def test_make_virtual_after_leading_attribute(self):
        # `virtual` must land after a leading attribute, not before it.
        result = self._apply(
            'attr.h', 'class C {\n  [[nodiscard]] bool Foo();\n};\n',
            'substitutions:\n'
            '  - description: make Foo virtual, keeping the attribute first\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: Foo\n')
        self.assertEqual(
            result, 'class C {\n  [[nodiscard]] virtual bool Foo();\n};\n')

    def test_make_virtual_after_multiple_leading_attributes(self):
        result = self._apply(
            'attrs.h',
            'class C {\n  [[nodiscard]] [[maybe_unused]] bool Foo();\n};\n',
            'substitutions:\n'
            '  - description: keep both attributes before virtual\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: Foo\n')
        self.assertEqual(
            result, 'class C {\n'
            '  [[nodiscard]] [[maybe_unused]] virtual bool Foo();\n};\n')

    def test_make_virtual_destructor_quoted(self):
        result = self._apply(
            'dtor.h', 'class C {\n public:\n  ~C();\n};\n', 'substitutions:\n'
            '  - description: make the destructor virtual\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            "      method_name: '~C'\n")
        self.assertEqual(result, 'class C {\n public:\n  virtual ~C();\n};\n')

    def test_make_virtual_overloads_need_count(self):
        result = self._apply(
            'ovl.h', 'class C {\n  void Foo();\n  void Foo(int x);\n};\n',
            'substitutions:\n'
            '  - description: make both overloads virtual\n'
            '    count: 2\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: Foo\n')
        self.assertEqual(
            result, 'class C {\n  virtual void Foo();\n'
            '  virtual void Foo(int x);\n};\n')

    def test_make_virtual_count_mismatch_fails(self):
        # Two overloads match, but the default count is 1.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'ovl2.h', 'class C {\n  void Foo();\n  void Foo(int x);\n};\n',
                'substitutions:\n'
                '  - description: forgot the count\n'
                '    make_virtual:\n'
                '      class_name: C\n'
                '      method_name: Foo\n')

    def test_make_virtual_skips_defined_function_template_overload(self):
        # Real-world overload set: a plain overload plus a function-template
        # overload (constrained with `requires`) that forwards to it. `virtual`
        # is illegal on a function template, so only the plain overload should
        # match -- exercising whether make_virtual can tell the two apart.
        source = (
            'class C {\n'
            ' public:\n'
            '  void AddTabRecursive(ScopedTab tab,\n'
            '                       size_t index,\n'
            '                       std::optional<tab_groups::TabGroupId> new_group_id,\n'
            '                       bool new_pinned_state);\n'
            '\n'
            '  template <typename T>\n'
            '    requires std::derived_from<T, TabInterface>\n'
            '  void AddTabRecursive(std::unique_ptr<T> tab,\n'
            '                       size_t index,\n'
            '                       std::optional<tab_groups::TabGroupId> new_group_id,\n'
            '                       bool new_pinned_state) {\n'
            '    AddTabRecursive(ScopedTab(tab.release()), index, new_group_id,\n'
            '                    new_pinned_state);\n'
            '  }\n'
            '};\n')
        result = self._apply(
            'template_overload.h', source, 'substitutions:\n'
            '  - description: make the non-template overload virtual\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: AddTabRecursive\n')
        expected = source.replace(
            '  void AddTabRecursive(ScopedTab tab,',
            '  virtual void AddTabRecursive(ScopedTab tab,')
        self.assertEqual(result, expected)

    def test_make_virtual_cannot_exclude_declaration_only_function_template(
            self):
        # Same overload set, but the template overload is a bare declaration
        # (no body), matching the same `field_declaration`/`declaration` shape
        # the matcher looks for. make_virtual has no way to distinguish a
        # function template from an ordinary method here, so it counts 2
        # matches for a 1-match default and refuses to apply. It fails closed
        # rather than incorrectly stamping `virtual` on the template, but
        # there is currently no arg to select just the non-template overload.
        source = (
            'class C {\n'
            ' public:\n'
            '  void AddTabRecursive(ScopedTab tab, size_t index);\n'
            '\n'
            '  template <typename T>\n'
            '    requires std::derived_from<T, TabInterface>\n'
            '  void AddTabRecursive(std::unique_ptr<T> tab, size_t index);\n'
            '};\n')
        with self.assertRaises(plaster.PlasterApplyError) as ctx:
            self._apply(
                'template_overload_decl.h', source, 'substitutions:\n'
                '  - description: make the non-template overload virtual\n'
                '    make_virtual:\n'
                '      class_name: C\n'
                '      method_name: AddTabRecursive\n')
        self.assertIn('Unexpected number of matches (2 vs 1)',
                      str(ctx.exception))

    def test_make_virtual_on_out_of_line_nested_class_fully_qualified(self):
        # As with add_friend, a class defined out-of-line as a nested class
        # (`class Outer::Inner`) has a qualified `name` field, so
        # `class_name` must be spelled fully qualified to match it.
        result = self._apply(
            'nested_virt.h', 'class Outer::Inner : public Base {\n'
            ' public:\n'
            '  void Foo();\n'
            '};\n', 'substitutions:\n'
            '  - description: make Foo virtual on the nested class\n'
            '    make_virtual:\n'
            '      class_name: Outer::Inner\n'
            '      method_name: Foo\n')
        self.assertEqual(
            result, 'class Outer::Inner : public Base {\n'
            ' public:\n'
            '  virtual void Foo();\n'
            '};\n')

    def test_make_virtual_bare_name_does_not_match_out_of_line_nested_class(
            self):
        with self.assertRaises(plaster.PlasterApplyError) as ctx:
            self._apply(
                'nested_virt_bare.h', 'class Outer::Inner : public Base {\n'
                ' public:\n'
                '  void Foo();\n'
                '};\n', 'substitutions:\n'
                '  - description: bare name does not match\n'
                '    make_virtual:\n'
                '      class_name: Inner\n'
                '      method_name: Foo\n')
        self.assertIn('Unexpected number of matches (0 vs 1)',
                      str(ctx.exception))

    def test_make_virtual_unknown_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: typo arg\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_nam: Foo\n', 'Unrecognised make_virtual arg')

    def test_make_virtual_missing_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: missing arg\n'
            '    make_virtual:\n'
            '      class_name: C\n', 'make_virtual requires arg')

    # -- add_friend op (real ast-grep binary) -------------------------------

    def test_add_friend_op(self):
        result = self._apply(
            'friend.h',
            'class C {\n public:\n  void Foo();\n private:\n  int x_;\n};\n',
            'substitutions:\n'
            '  - description: friend the Brave subclass\n'
            '    add_friend:\n'
            '      class_name: C\n'
            '      friend_type: class BraveC\n')
        self.assertEqual(
            result, 'class C {\n public:\n  void Foo();\n'
            ' private:\n  friend class BraveC;\n  int x_;\n};\n')

    def test_add_friend_inserts_into_first_private_section_only(self):
        # A class may reopen `private:` more than once. The friend must land in
        # the first private section only; the later one is left untouched.
        result = self._apply(
            'twoprivate.h', 'class C {\n public:\n  void Foo();\n'
            ' private:\n  int x_;\n'
            ' private:\n  int y_;\n};\n', 'substitutions:\n'
            '  - description: friend the Brave subclass\n'
            '    add_friend:\n'
            '      class_name: C\n'
            '      friend_type: class BraveC\n')
        self.assertEqual(
            result, 'class C {\n public:\n  void Foo();\n'
            ' private:\n  friend class BraveC;\n  int x_;\n'
            ' private:\n  int y_;\n};\n')

    def test_add_friend_list_into_first_private_section_only(self):
        # The same holds for a list of friends: all of them go into the first
        # private section, in the order listed, and the later one is untouched.
        result = self._apply(
            'twoprivate_list.h', 'class C {\n public:\n  void Foo();\n'
            ' private:\n  int x_;\n'
            ' private:\n  int y_;\n};\n', 'substitutions:\n'
            '  - description: friend the Brave subclass and its test\n'
            '    add_friend:\n'
            '      class_name: C\n'
            '      friend_type:\n'
            '        - class BraveC\n'
            '        - class BraveCTest\n')
        self.assertEqual(
            result, 'class C {\n public:\n  void Foo();\n'
            ' private:\n  friend class BraveC;\n  friend class BraveCTest;\n'
            '  int x_;\n private:\n  int y_;\n};\n')

    def test_add_friend_ignores_nested_class_private_after(self):
        # A nested class has its own private section. When befriending the outer
        # class, the friend must land in the outer class's private section, never
        # the nested one -- here the nested `private:` comes *after* the outer's.
        result = self._apply(
            'nested_after.h', 'class Foo {\n private:\n  int x_;\n'
            '  class Bar {\n   private:\n    int y_;\n  };\n};\n',
            'substitutions:\n'
            '  - description: friend the Brave subclass of the outer class\n'
            '    add_friend:\n'
            '      class_name: Foo\n'
            '      friend_type: class BraveFoo\n')
        self.assertEqual(
            result, 'class Foo {\n private:\n  friend class BraveFoo;\n'
            '  int x_;\n'
            '  class Bar {\n   private:\n    int y_;\n  };\n};\n')

    def test_add_friend_ignores_nested_class_private_before(self):
        # As above, but the nested class (and so its `private:`) appears *before*
        # the outer class's own private section. The nested section is earlier in
        # source order, so this guards against naively taking the first match.
        result = self._apply(
            'nested_before.h', 'class Foo {\n public:\n'
            '  class Bar {\n   private:\n    int y_;\n  };\n'
            ' private:\n  int x_;\n};\n', 'substitutions:\n'
            '  - description: friend the Brave subclass of the outer class\n'
            '    add_friend:\n'
            '      class_name: Foo\n'
            '      friend_type: class BraveFoo\n')
        self.assertEqual(
            result, 'class Foo {\n public:\n'
            '  class Bar {\n   private:\n    int y_;\n  };\n'
            ' private:\n  friend class BraveFoo;\n  int x_;\n};\n')

    def test_add_friend_bare_name_does_not_match_out_of_line_nested_class(
            self):
        # A nested class defined out-of-line spells its class-head with its
        # enclosing class as a qualifier (`class Outer::Inner : public Base`),
        # so the class_specifier's `name` field is a qualified_identifier
        # ("Outer::Inner"), not a bare identifier ("Inner"). A bare
        # `class_name` does *not* match this -- it must be spelled fully
        # qualified (see the test below) -- e.g. TabHoverCardBubbleView::
        # TabCardView in
        # chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc.
        with self.assertRaises(plaster.PlasterApplyError) as ctx:
            self._apply(
                'out_of_line_nested.h', 'class Outer::Inner : public Base {\n'
                ' public:\n'
                '  void Foo();\n'
                '\n'
                ' private:\n'
                '  int x_;\n'
                '};\n', 'substitutions:\n'
                '  - description: friend the Brave subclass\n'
                '    add_friend:\n'
                '      class_name: Inner\n'
                '      friend_type: class BraveInner\n')
        self.assertIn('Unexpected number of matches (0 vs 1)',
                      str(ctx.exception))

    def test_add_friend_on_out_of_line_nested_class_fully_qualified(self):
        # `class_name` must be spelled fully qualified (`Outer::Inner`) to
        # match an out-of-line nested class definition; this also
        # disambiguates it from an unrelated same-named top-level class.
        result = self._apply(
            'out_of_line_nested_qualified.h', 'class Inner {\n'
            ' private:\n'
            '  int unrelated_;\n'
            '};\n'
            '\n'
            'class Outer::Inner : public Base {\n'
            ' public:\n'
            '  void Foo();\n'
            '\n'
            ' private:\n'
            '  int x_;\n'
            '};\n', 'substitutions:\n'
            '  - description: friend the Brave subclass of the nested class\n'
            '    add_friend:\n'
            '      class_name: Outer::Inner\n'
            '      friend_type: class BraveInner\n')
        self.assertEqual(
            result, 'class Inner {\n'
            ' private:\n'
            '  int unrelated_;\n'
            '};\n'
            '\n'
            'class Outer::Inner : public Base {\n'
            ' public:\n'
            '  void Foo();\n'
            '\n'
            ' private:\n'
            '  friend class BraveInner;\n'
            '  int x_;\n'
            '};\n')

    def test_add_friend_bare_name_ignores_out_of_line_nested_class(self):
        # A bare `class_name` matching an unrelated top-level class is
        # unaffected by a same-named out-of-line nested class elsewhere in
        # the file -- the qualified name never matches the bare regex, so
        # there is no ambiguity to resolve.
        result = self._apply(
            'bare_name_top_level_only.h', 'class Inner {\n'
            ' private:\n'
            '  int unrelated_;\n'
            '};\n'
            '\n'
            'class Outer::Inner : public Base {\n'
            ' private:\n'
            '  int x_;\n'
            '};\n', 'substitutions:\n'
            '  - description: friend the top-level class only\n'
            '    add_friend:\n'
            '      class_name: Inner\n'
            '      friend_type: class BraveInner\n')
        self.assertEqual(
            result, 'class Inner {\n'
            ' private:\n'
            '  friend class BraveInner;\n'
            '  int unrelated_;\n'
            '};\n'
            '\n'
            'class Outer::Inner : public Base {\n'
            ' private:\n'
            '  int x_;\n'
            '};\n')

    def test_add_friend_no_private_section_fails(self):
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'nofriend.h', 'class C {\n public:\n  void Foo();\n};\n',
                'substitutions:\n'
                '  - description: no private section to insert into\n'
                '    add_friend:\n'
                '      class_name: C\n'
                '      friend_type: class BraveC\n')

    def test_add_friend_unknown_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: typo arg\n'
            '    add_friend:\n'
            '      class_name: C\n'
            '      freind: class BraveC\n', 'Unrecognised add_friend arg')

    def test_add_friend_list_inserts_in_authored_order(self):
        # A list-valued `friend_type` befriends several types in one entry. Each
        # is inserted once into the single private section, so no `count:` is
        # needed; they land in the source in the order listed here even though
        # each is inserted as the first line.
        result = self._apply(
            'friends.h',
            'class C {\n public:\n  void Foo();\n private:\n  int x_;\n};\n',
            'substitutions:\n'
            '  - description: friend the Brave subclass and its test\n'
            '    add_friend:\n'
            '      class_name: C\n'
            '      friend_type:\n'
            '        - class BraveC\n'
            '        - class BraveCTest\n')
        self.assertEqual(
            result, 'class C {\n public:\n  void Foo();\n'
            ' private:\n  friend class BraveC;\n  friend class BraveCTest;\n'
            '  int x_;\n};\n')

    def test_add_friend_list_fails_when_private_section_absent(self):
        # Each friend is validated on its own: with no private section every
        # per-friend operation matches nothing and fails.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'friends2.h', 'class C {\n public:\n  void Foo();\n};\n',
                'substitutions:\n'
                '  - description: no private section for the friends\n'
                '    add_friend:\n'
                '      class_name: C\n'
                '      friend_type:\n'
                '        - class BraveC\n'
                '        - class BraveCTest\n')

    def test_add_friend_friend_type_must_be_string_or_list(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: bad friend_type\n'
            '    add_friend:\n'
            '      class_name: C\n'
            '      friend_type: 42\n',
            'friend_type` must be a string or a non-empty list')

    def test_add_friend_friend_type_empty_list_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: empty friend list\n'
            '    add_friend:\n'
            '      class_name: C\n'
            '      friend_type: []\n',
            'friend_type` must be a string or a non-empty list')

    def test_add_friend_missing_class_name_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: missing class\n'
            '    add_friend:\n'
            '      friend_type: class BraveC\n', 'add_friend requires arg')

    # -- drop_final op (real ast-grep binary) -----------------------------

    def test_drop_final_op(self):
        result = self._apply(
            'final.h', 'class C final : public Base {\n};\n',
            'substitutions:\n'
            '  - description: drop final so Brave can subclass\n'
            '    drop_final:\n'
            '      class_name: C\n')
        self.assertEqual(result, 'class C : public Base {\n};\n')

    def test_drop_final_absent_fails(self):
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'nofinal.h', 'class C {\n};\n', 'substitutions:\n'
                '  - description: nothing to remove\n'
                '    drop_final:\n'
                '      class_name: C\n')

    def test_drop_final_missing_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: missing arg\n'
            '    drop_final: {}\n', 'drop_final requires arg')

    # -- preempt_function_impl op (real ast-grep binary) --------------------------

    def test_preempt_function_impl_return_if(self):
        result = self._apply(
            'guard.cc', 'void C::Foo() {\n  Upstream();\n}\n',
            'substitutions:\n'
            '  - description: skip upstream when Brave has it disabled\n'
            '    preempt_function_impl:\n'
            '      function_name: C::Foo\n'
            "      return_if: '!Enabled()'\n")
        self.assertEqual(
            result, 'void C::Foo() {\n  if (!Enabled()) return;\n'
            '  Upstream();\n}\n')

    def test_preempt_function_impl_free_function(self):
        # A free function is named without a class qualifier -- just the bare
        # declarator name.
        result = self._apply(
            'free.cc', 'void FreeFunc(int x) {\n  Upstream(x);\n}\n',
            'substitutions:\n'
            '  - description: guard a free function\n'
            '    preempt_function_impl:\n'
            '      function_name: FreeFunc\n'
            "      return_if: '!Enabled()'\n")
        self.assertEqual(
            result, 'void FreeFunc(int x) {\n  if (!Enabled()) return;\n'
            '  Upstream(x);\n}\n')

    def test_preempt_function_impl_templated_multiline_free_function(self):
        # Reproduces CreateHorizontalTabStripRegionView: a free function whose
        # templated return type sits on its own line, above the declarator.
        source = ('std::unique_ptr<TabStripRegionView> '
                  'CreateHorizontalTabStripRegionView(\n'
                  '    BrowserView* browser_view) {\n'
                  '  return std::make_unique<Old>(browser_view);\n}\n')
        result = self._apply(
            'multiline.cc', source, 'substitutions:\n'
            '  - description: guard a templated multiline free function\n'
            '    preempt_function_impl:\n'
            '      function_name: CreateHorizontalTabStripRegionView\n'
            '      code: |-\n'
            '        if (!Enabled()) {\n'
            '          return std::make_unique<Brave>(browser_view);\n'
            '        }\n')
        self.assertEqual(
            result,
            source.replace(
                '{\n  return std::make_unique<Old>', '{\n  if (!Enabled()) {\n'
                '    return std::make_unique<Brave>(browser_view);\n  }\n'
                '  return std::make_unique<Old>'))

    def test_preempt_function_impl_ignores_forward_declaration(self):
        # A forward declaration is a bodyless `declaration`, not a
        # `function_definition`, so the matcher skips it (count stays 1) and the
        # guard lands only in the definition.
        source = ('void FreeFunc(int x);\n\n'
                  'void FreeFunc(int x) {\n  Upstream(x);\n}\n')
        result = self._apply(
            'forward.cc', source, 'substitutions:\n'
            '  - description: guard the definition, not the declaration\n'
            '    preempt_function_impl:\n'
            '      function_name: FreeFunc\n'
            "      return_if: '!Enabled()'\n")
        self.assertEqual(
            result,
            source.replace('{\n  Upstream(x);',
                           '{\n  if (!Enabled()) return;\n  Upstream(x);'))

    def test_preempt_function_impl_anonymous_namespace_function(self):
        # A function inside an anonymous namespace is named by its bare
        # declarator -- the enclosing `namespace {}` is contextual, not part of
        # the name.
        source = ('namespace {\n\n'
                  'bool ShouldProceed(int x) {\n  return x > 0;\n}\n\n'
                  '}  // namespace\n')
        result = self._apply(
            'anon.cc', source, 'substitutions:\n'
            '  - description: guard a function in an anonymous namespace\n'
            '    preempt_function_impl:\n'
            '      function_name: ShouldProceed\n'
            "      return_if: '!Enabled()'\n")
        self.assertEqual(
            result,
            source.replace('{\n  return x > 0;',
                           '{\n  if (!Enabled()) return;\n  return x > 0;'))

    def test_preempt_function_impl_return_if_with_value(self):
        result = self._apply(
            'guard_value.cc', 'bool C::IsVisible() {\n  return real_;\n}\n',
            'substitutions:\n'
            '  - description: force the answer for Brave\n'
            '    preempt_function_impl:\n'
            '      function_name: C::IsVisible\n'
            "      return_if: 'true'\n"
            "      return_value: 'false'\n")
        self.assertEqual(
            result, 'bool C::IsVisible() {\n  if (true) return false;\n'
            '  return real_;\n}\n')

    def test_preempt_function_impl_code_block(self):
        # The `code` block is authored flush-left; the engine indents the whole
        # block to the body's first-statement level (two spaces).
        result = self._apply(
            'code.cc', 'void C::Pin(int id) {\n  Upstream();\n}\n',
            'substitutions:\n'
            '  - description: pin Brave actions before the upstream body\n'
            '    preempt_function_impl:\n'
            '      function_name: C::Pin\n'
            '      code: |-\n'
            '        if (PinBraveAction(id)) {\n'
            '          return;\n'
            '        }\n')
        self.assertEqual(
            result, 'void C::Pin(int id) {\n  if (PinBraveAction(id)) {\n'
            '    return;\n  }\n  Upstream();\n}\n')

    def test_preempt_function_impl_code_block_blank_line_not_indented(self):
        # A blank line inside the block stays empty -- no trailing whitespace.
        result = self._apply(
            'code_blank.cc', 'void C::Pin(int id) {\n  Upstream();\n}\n',
            'substitutions:\n'
            '  - description: two guarded statements split by a blank line\n'
            '    preempt_function_impl:\n'
            '      function_name: C::Pin\n'
            '      code: |-\n'
            '        Prepare(id);\n'
            '\n'
            '        Track(id);\n')
        self.assertEqual(
            result, 'void C::Pin(int id) {\n  Prepare(id);\n\n'
            '  Track(id);\n  Upstream();\n}\n')

    def test_preempt_function_impl_survives_braced_default_argument(self):
        # A `= {}` default argument and a multi-line signature both defeat a
        # naive `\\(.*?{` regex, which would stop at the argument's brace. The
        # AST matcher lands on the real body brace regardless.
        source = ('void C::Tricky(const Options& opts = {},\n'
                  '               int flags = 0) {\n  Upstream();\n}\n')
        result = self._apply(
            'tricky.cc', source, 'substitutions:\n'
            '  - description: guard a function with a braced default arg\n'
            '    preempt_function_impl:\n'
            '      function_name: C::Tricky\n'
            "      return_if: '!ok'\n")
        self.assertEqual(
            result,
            source.replace('{\n  Upstream();', '{\n  if (!ok) return;\n'
                           '  Upstream();'))

    def test_preempt_function_impl_constructor(self):
        # A constructor has no return type, so the matcher's `return_type`
        # capture cannot resolve for it. This op never asks for that capture,
        # so the rewrite still applies -- captures are resolved lazily.
        result = self._apply(
            'ctor.cc', 'C::C() : x_(1) {\n  Init();\n}\n', 'substitutions:\n'
            '  - description: skip upstream init when Brave owns it\n'
            '    preempt_function_impl:\n'
            '      function_name: C::C\n'
            "      return_if: 'BraveOwnsInit()'\n")
        self.assertEqual(
            result, 'C::C() : x_(1) {\n'
            '  if (BraveOwnsInit()) return;\n  Init();\n}\n')

    def test_preempt_function_impl_targets_named_function_only(self):
        # Only the named function's body is touched, not a sibling in the same
        # file.
        result = self._apply(
            'siblings.cc',
            'void C::A() {\n  a();\n}\n\nvoid C::B() {\n  b();\n}\n',
            'substitutions:\n'
            '  - description: guard only B\n'
            '    preempt_function_impl:\n'
            '      function_name: C::B\n'
            "      return_if: 'g()'\n")
        self.assertEqual(
            result, 'void C::A() {\n  a();\n}\n\n'
            'void C::B() {\n  if (g()) return;\n  b();\n}\n')

    def test_preempt_function_impl_nested_class_out_of_line_method(self):
        # A method of a nested class defined out-of-line is declared with its
        # full enclosing scope (`Outer::Inner::Method`), not just
        # `Inner::Method` -- the same qualification rule as `add_friend` and
        # `make_virtual` on the nested class itself.
        result = self._apply(
            'nested_method.cc', 'void Outer::Inner::Method(int x) {\n'
            '  Upstream(x);\n}\n', 'substitutions:\n'
            '  - description: guard a nested class out-of-line method\n'
            '    preempt_function_impl:\n'
            '      function_name: Outer::Inner::Method\n'
            "      return_if: '!Enabled()'\n")
        self.assertEqual(
            result, 'void Outer::Inner::Method(int x) {\n'
            '  if (!Enabled()) return;\n  Upstream(x);\n}\n')

    def test_preempt_function_impl_partial_qualification_fails(self):
        # `Inner::Method` (missing the `Outer::` scope) does not match --
        # the declarator's full qualified text must be given, not a suffix.
        with self.assertRaises(plaster.PlasterApplyError) as ctx:
            self._apply(
                'nested_method_partial.cc',
                'void Outer::Inner::Method(int x) {\n'
                '  Upstream(x);\n}\n', 'substitutions:\n'
                '  - description: partial qualification does not match\n'
                '    preempt_function_impl:\n'
                '      function_name: Inner::Method\n'
                "      return_if: '!Enabled()'\n")
        self.assertIn('Unexpected number of matches (0 vs 1)',
                      str(ctx.exception))

    def test_preempt_function_impl_overloads_need_count(self):
        result = self._apply(
            'overloads.cc',
            'void C::F() {\n  a();\n}\n\nvoid C::F(int x) {\n  b();\n}\n',
            'substitutions:\n'
            '  - description: guard both overloads\n'
            '    count: 2\n'
            '    preempt_function_impl:\n'
            '      function_name: C::F\n'
            "      return_if: 'g()'\n")
        self.assertEqual(
            result, 'void C::F() {\n  if (g()) return;\n  a();\n}\n\n'
            'void C::F(int x) {\n  if (g()) return;\n  b();\n}\n')

    def test_preempt_function_impl_overload_count_mismatch_fails(self):
        # Two overloads match, but the default count is 1.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'overloads_bad.cc',
                'void C::F() {\n  a();\n}\n\nvoid C::F(int x) {\n  b();\n}\n',
                'substitutions:\n'
                '  - description: forgot the count\n'
                '    preempt_function_impl:\n'
                '      function_name: C::F\n'
                "      return_if: 'g()'\n")

    def test_preempt_function_impl_absent_function_fails(self):
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'missing.cc', 'void C::Foo() {\n}\n', 'substitutions:\n'
                '  - description: no such function\n'
                '    preempt_function_impl:\n'
                '      function_name: C::Nope\n'
                "      return_if: 'g()'\n")

    def test_preempt_function_impl_both_modes_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: two modes\n'
            '    preempt_function_impl:\n'
            '      function_name: C::F\n'
            '      code: x;\n'
            "      return_if: 'g()'\n", 'exactly one of `code` or `return_if`')

    def test_preempt_function_impl_no_mode_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: no mode\n'
            '    preempt_function_impl:\n'
            '      function_name: C::F\n',
            'exactly one of `code` or `return_if`')

    def test_preempt_function_impl_return_value_requires_return_if(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: return_value with code\n'
            '    preempt_function_impl:\n'
            '      function_name: C::F\n'
            '      code: x;\n'
            "      return_value: 'false'\n",
            '`return_value` is only valid with `return_if`')

    def test_preempt_function_impl_unknown_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: typo arg\n'
            '    preempt_function_impl:\n'
            '      function_name: C::F\n'
            '      cod: x;\n', 'Unrecognised preempt_function_impl arg')

    def test_preempt_function_impl_missing_function_name_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: missing function_name\n'
            '    preempt_function_impl:\n'
            "      return_if: 'g()'\n",
            'preempt_function_impl `function_name` must be a non-empty string')

    # -- rename_class op (real ast-grep binary) -----------------------------

    # `count` is omitted throughout: rename_class defaults to `count: 0` (one or
    # more), unlike every other rewriter (which defaults to exactly one).

    def test_rename_class_renames_declaration_and_type_uses(self):
        # The class declaration and a type-position use are both renamed; a
        # different class that merely shares a prefix is left alone.
        result = self._apply(
            'rename.h',
            'class Foo {\n};\n\nclass FooBar {\n  Foo* foo_;\n};\n',
            'substitutions:\n'
            '  - description: rename Foo to Foo_ChromiumImpl\n'
            '    rename_class:\n'
            '      class_name: Foo\n'
            '      rename: Foo_ChromiumImpl\n')
        self.assertEqual(
            result, 'class Foo_ChromiumImpl {\n};\n\n'
            'class FooBar {\n  Foo_ChromiumImpl* foo_;\n};\n')

    def test_rename_class_renames_qualifiers_and_ctor(self):
        # The `Foo::` qualifier and the out-of-line constructor name are both
        # renamed.
        result = self._apply(
            'rename_ctor.cc', 'Foo::Foo() {}\nvoid Foo::Bar() {}\n',
            'substitutions:\n'
            '  - description: rename Foo\n'
            '    rename_class:\n'
            '      class_name: Foo\n'
            '      rename: Foo_ChromiumImpl\n')
        self.assertEqual(
            result, 'Foo_ChromiumImpl::Foo_ChromiumImpl() {}\n'
            'void Foo_ChromiumImpl::Bar() {}\n')

    def test_rename_class_renames_forwarding_constructor(self):
        # A forwarding (delegating) constructor names the class again in its
        # member initializer list -- `: Foo(...)` -- which tree-sitter parses as
        # a field_identifier, not the identifier/type_identifier the other uses
        # are. That token must be renamed along with the rest (the real-world
        # miss was `CookieMonster::CookieMonster(...) : CookieMonster(...)`).
        result = self._apply(
            'rename_fwd_ctor.cc', 'Foo::Foo(int a)\n'
            '    : Foo(base::PassKey<Foo>(), a) {}\n'
            '\n'
            'Foo::Foo(base::PassKey<Foo>, int a) {}\n', 'substitutions:\n'
            '  - description: rename Foo, forwarding ctor included\n'
            '    rename_class:\n'
            '      class_name: Foo\n'
            '      rename: Foo_ChromiumImpl\n')
        self.assertEqual(
            result, 'Foo_ChromiumImpl::Foo_ChromiumImpl(int a)\n'
            '    : Foo_ChromiumImpl(base::PassKey<Foo_ChromiumImpl>(), a) {}\n'
            '\n'
            'Foo_ChromiumImpl::Foo_ChromiumImpl('
            'base::PassKey<Foo_ChromiumImpl>, int a) {}\n')

    def test_rename_class_leaves_member_access_in_initializer_untouched(self):
        # The forwarding-ctor fix only claims the *name* slot of a member
        # initializer (`: Foo(...)`). A same-spelled member access passed as an
        # argument (`other.Foo`) is not that slot and must survive, or the fix
        # would over-match member accesses that merely share the class name.
        result = self._apply(
            'rename_init_member.cc', 'Foo::Foo(const Other& other)\n'
            '    : value_(other.Foo) {}\n', 'substitutions:\n'
            '  - description: rename Foo but keep the member access\n'
            '    rename_class:\n'
            '      class_name: Foo\n'
            '      rename: Foo_ChromiumImpl\n')
        self.assertEqual(
            result, 'Foo_ChromiumImpl::Foo_ChromiumImpl(const Other& other)\n'
            '    : value_(other.Foo) {}\n')

    def test_rename_class_leaves_string_literal_untouched(self):
        # The whole point of matching AST identifier nodes: a same-spelled
        # string literal survives. The exact-quote-adjacent form (`"Foo"`) is
        # the case a `#define`/regex token rename would wrongly rewrite.
        result = self._apply(
            'rename_str.cc', 'void Foo::Run() {\n  Register("Foo");\n}\n',
            'substitutions:\n'
            '  - description: rename Foo but keep the string\n'
            '    rename_class:\n'
            '      class_name: Foo\n'
            '      rename: Foo_ChromiumImpl\n')
        self.assertEqual(
            result, 'void Foo_ChromiumImpl::Run() {\n  Register("Foo");\n}\n')

    def test_rename_class_leaves_token_inside_a_larger_string_untouched(self):
        # `Foo` sits in the *middle* of a string, not adjacent to a quote -- the
        # case a `(?<!")...(?!")` regex guard would still rewrite, but an AST
        # match never can (the whole literal is one string node).
        result = self._apply(
            'rename_midstr.cc',
            'void Foo::Log() {\n  LOG(INFO) << "start Foo done";\n}\n',
            'substitutions:\n'
            '  - description: rename Foo, keep it inside the message string\n'
            '    rename_class:\n'
            '      class_name: Foo\n'
            '      rename: Foo_ChromiumImpl\n')
        self.assertEqual(
            result, 'void Foo_ChromiumImpl::Log() {\n'
            '  LOG(INFO) << "start Foo done";\n}\n')

    def test_rename_class_leaves_line_and_block_comments_untouched(self):
        # Neither a `//` line comment nor a `/* */` block comment mentioning the
        # name is rewritten -- comments are not identifier nodes.
        result = self._apply(
            'rename_comment.cc', '// Foo does things.\n'
            '/* Foo again, and Foo. */\n'
            'void Foo::Run() {\n}\n', 'substitutions:\n'
            '  - description: rename Foo but keep both comments\n'
            '    rename_class:\n'
            '      class_name: Foo\n'
            '      rename: Foo_ChromiumImpl\n')
        self.assertEqual(
            result, '// Foo does things.\n'
            '/* Foo again, and Foo. */\n'
            'void Foo_ChromiumImpl::Run() {\n}\n')

    def test_rename_class_default_count_renames_all_occurrences(self):
        # With the default `count: 0`, omitting `count` renames every occurrence
        # (here four tokens across two lines) instead of demanding exactly one.
        result = self._apply(
            'rename_many.cc', 'Foo::Foo() {}\nvoid Foo::Bar() {}\n',
            'substitutions:\n'
            '  - description: rename Foo everywhere, no count needed\n'
            '    rename_class:\n'
            '      class_name: Foo\n'
            '      rename: Foo_ChromiumImpl\n')
        self.assertEqual(
            result, 'Foo_ChromiumImpl::Foo_ChromiumImpl() {}\n'
            'void Foo_ChromiumImpl::Bar() {}\n')

    def test_rename_class_explicit_count_still_asserts_exact(self):
        # An explicit `count` overrides the default and asserts an exact number;
        # here two tokens match the stated two.
        result = self._apply(
            'rename_exact.h', 'class Foo {\n  Foo* self_;\n};\n',
            'substitutions:\n'
            '  - description: rename the two Foo tokens\n'
            '    count: 2\n'
            '    rename_class:\n'
            '      class_name: Foo\n'
            '      rename: Foo_ChromiumImpl\n')
        self.assertEqual(
            result,
            'class Foo_ChromiumImpl {\n  Foo_ChromiumImpl* self_;\n};\n')

    def test_rename_class_explicit_count_mismatch_fails(self):
        # Two tokens match, but the entry asserts one.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'rename_bad_count.h', 'class Foo {\n  Foo* self_;\n};\n',
                'substitutions:\n'
                '  - description: wrong explicit count\n'
                '    count: 1\n'
                '    rename_class:\n'
                '      class_name: Foo\n'
                '      rename: Foo_ChromiumImpl\n')

    def test_rename_class_absent_fails(self):
        # The default `count: 0` is "one or more", so zero matches still fails.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'rename_absent.h', 'class Bar {\n};\n', 'substitutions:\n'
                '  - description: no such class\n'
                '    rename_class:\n'
                '      class_name: Foo\n'
                '      rename: Foo_ChromiumImpl\n')

    def test_rename_class_unknown_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: typo arg\n'
            '    rename_class:\n'
            '      class_name: Foo\n'
            '      renam: Bar\n', 'Unrecognised rename_class arg')

    def test_rename_class_missing_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: missing rename\n'
            '    rename_class:\n'
            '      class_name: Foo\n', 'rename_class requires arg')

    # -- add_to_protected op (real ast-grep binary) -----------------

    def test_add_to_protected_creates_section_before_private(self):
        # No existing protected section: a fresh `protected:` is created just
        # before `private:`.
        result = self._apply(
            'prot_new.h',
            'class C {\n public:\n  void Foo();\n private:\n  int x_;\n};\n',
            'substitutions:\n'
            '  - description: add a protected hook\n'
            '    add_to_protected:\n'
            '      class_name: C\n'
            '      code: virtual void Bar() = 0;\n')
        self.assertEqual(
            result, 'class C {\n public:\n  void Foo();\n'
            ' protected:\n  virtual void Bar() = 0;\n\n'
            ' private:\n  int x_;\n};\n')

    def test_add_to_protected_reuses_existing_protected(self):
        # An existing protected section is reused: the code becomes its first
        # line, and no second protected section is created before private.
        result = self._apply(
            'prot_reuse.h', 'class C {\n protected:\n  void Existing();\n'
            ' private:\n  int x_;\n};\n', 'substitutions:\n'
            '  - description: reuse the protected section\n'
            '    add_to_protected:\n'
            '      class_name: C\n'
            '      code: virtual void Bar() = 0;\n')
        self.assertEqual(
            result, 'class C {\n protected:\n  virtual void Bar() = 0;\n'
            '  void Existing();\n private:\n  int x_;\n};\n')

    def test_add_to_protected_scoped_to_named_class(self):
        # Only the named class is touched; a sibling class with its own private
        # section is left alone (the matchers are scoped by class_name).
        result = self._apply(
            'prot_scope.h', 'class C {\n private:\n  int c_;\n};\n\n'
            'class D {\n private:\n  int d_;\n};\n', 'substitutions:\n'
            '  - description: add only to C\n'
            '    add_to_protected:\n'
            '      class_name: C\n'
            '      code: virtual void Bar() = 0;\n')
        self.assertEqual(
            result, 'class C {\n protected:\n  virtual void Bar() = 0;\n\n'
            ' private:\n  int c_;\n};\n\n'
            'class D {\n private:\n  int d_;\n};\n')

    def test_add_to_protected_multiline_code(self):
        # A multi-line `code` block inserts several declarations, each indented
        # to the member level.
        result = self._apply(
            'prot_multi.h', 'class C {\n private:\n  int x_;\n};\n',
            'substitutions:\n'
            '  - description: add two protected hooks\n'
            '    add_to_protected:\n'
            '      class_name: C\n'
            '      code: |-\n'
            '        virtual void A() = 0;\n'
            '        virtual void B() = 0;\n')
        self.assertEqual(
            result, 'class C {\n'
            ' protected:\n  virtual void A() = 0;\n  virtual void B() = 0;\n\n'
            ' private:\n  int x_;\n};\n')

    def test_add_to_protected_indents_flush_left_nested_code(self):
        # A flush-left `code` block is indented to the member level (two spaces),
        # and its own relative indentation is preserved (the nested `DoStuff();`
        # ends up two spaces deeper) -- like preempt_function_impl's `code`.
        result = self._apply(
            'prot_nested.h', 'class C {\n private:\n  int x_;\n};\n',
            'substitutions:\n'
            '  - description: add a hook with an inline body\n'
            '    add_to_protected:\n'
            '      class_name: C\n'
            '      code: |-\n'
            '        void OnFoo() {\n'
            '          DoStuff();\n'
            '        }\n')
        self.assertEqual(
            result, 'class C {\n'
            ' protected:\n  void OnFoo() {\n    DoStuff();\n  }\n\n'
            ' private:\n  int x_;\n};\n')

    def test_add_to_protected_blank_line_in_code_stays_empty(self):
        # A blank line inside the `code` block stays empty -- no trailing
        # whitespace from the member-level indentation.
        result = self._apply(
            'prot_blank.h', 'class C {\n private:\n  int x_;\n};\n',
            'substitutions:\n'
            '  - description: two declarations split by a blank line\n'
            '    add_to_protected:\n'
            '      class_name: C\n'
            '      code: |-\n'
            '        virtual void A() = 0;\n'
            '\n'
            '        virtual void B() = 0;\n')
        self.assertEqual(
            result, 'class C {\n'
            ' protected:\n  virtual void A() = 0;\n\n  virtual void B() = 0;\n\n'
            ' private:\n  int x_;\n};\n')

    def test_add_to_protected_reused_section_indents_code(self):
        # Indentation is applied the same way when reusing an existing protected
        # section: the multi-line block lands at the member level above the
        # existing members.
        result = self._apply(
            'prot_reuse_multi.h',
            'class C {\n protected:\n  void Existing();\n'
            ' private:\n  int x_;\n};\n', 'substitutions:\n'
            '  - description: add two hooks to the existing protected section\n'
            '    add_to_protected:\n'
            '      class_name: C\n'
            '      code: |-\n'
            '        void A();\n'
            '        void B();\n')
        self.assertEqual(
            result, 'class C {\n protected:\n  void A();\n  void B();\n'
            '  void Existing();\n private:\n  int x_;\n};\n')

    def test_add_to_protected_no_anchor_fails(self):
        # A class with neither a protected nor a private section has nothing to
        # anchor on.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'prot_none.h', 'class C {\n public:\n  void Foo();\n};\n',
                'substitutions:\n'
                '  - description: no anchor\n'
                '    add_to_protected:\n'
                '      class_name: C\n'
                '      code: virtual void Bar() = 0;\n')

    def test_add_to_protected_unknown_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: typo arg\n'
            '    add_to_protected:\n'
            '      class_name: C\n'
            '      cod: virtual void Bar() = 0;\n',
            'Unrecognised add_to_protected arg')

    def test_add_to_protected_missing_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: missing code\n'
            '    add_to_protected:\n'
            '      class_name: C\n', 'add_to_protected requires arg')

    def test_add_to_protected_explicit_count_one_ok(self):
        # An explicit `count: 1` is the only count accepted; it applies normally.
        result = self._apply(
            'prot_count_ok.h', 'class C {\n private:\n  int x_;\n};\n',
            'substitutions:\n'
            '  - description: explicit count of one\n'
            '    count: 1\n'
            '    add_to_protected:\n'
            '      class_name: C\n'
            '      code: virtual void Bar() = 0;\n')
        self.assertEqual(
            result, 'class C {\n protected:\n  virtual void Bar() = 0;\n\n'
            ' private:\n  int x_;\n};\n')

    def test_add_to_protected_count_other_than_one_rejected(self):
        # It always adds exactly once, so any other count is a config error.
        self._expect_value_error(
            'substitutions:\n'
            '  - description: bogus count\n'
            '    count: 2\n'
            '    add_to_protected:\n'
            '      class_name: C\n'
            '      code: virtual void Bar() = 0;\n',
            'does not accept a count other than 1')

    def test_add_to_protected_creates_section_in_nested_class(self):
        # A nested class is indented to its own column: the new protected
        # section and its member follow the nested class's indentation, not the
        # top-level one.
        result = self._apply(
            'prot_nested_class.h', 'class Outer {\n public:\n  class C {\n'
            '   public:\n    void Foo();\n   private:\n    int x_;\n'
            '  };\n};\n', 'substitutions:\n'
            '  - description: add a protected hook to the nested class\n'
            '    add_to_protected:\n'
            '      class_name: C\n'
            '      code: virtual void Bar() = 0;\n')
        self.assertEqual(
            result, 'class Outer {\n public:\n  class C {\n'
            '   public:\n    void Foo();\n'
            '   protected:\n    virtual void Bar() = 0;\n\n'
            '   private:\n    int x_;\n  };\n};\n')

    def test_add_to_protected_reuses_section_in_nested_class(self):
        # Reusing an existing protected section in a nested class indents the
        # inserted member to the nested member column.
        result = self._apply(
            'prot_nested_reuse.h', 'class Outer {\n public:\n  class C {\n'
            '   protected:\n    void Existing();\n   private:\n    int x_;\n'
            '  };\n};\n', 'substitutions:\n'
            '  - description: reuse the nested protected section\n'
            '    add_to_protected:\n'
            '      class_name: C\n'
            '      code: virtual void Bar() = 0;\n')
        self.assertEqual(
            result, 'class Outer {\n public:\n  class C {\n'
            '   protected:\n    virtual void Bar() = 0;\n    void Existing();\n'
            '   private:\n    int x_;\n  };\n};\n')

    # -- add_to_public op (real ast-grep binary) --------------------

    def test_add_to_public_appends_before_following_section(self):
        # The code becomes the last public member, right before the private
        # section that follows public -- not the first public line.
        result = self._apply(
            'pub_before_priv.h',
            'class C {\n public:\n  void Foo();\n private:\n  int x_;\n};\n',
            'substitutions:\n'
            '  - description: expose a public hook\n'
            '    add_to_public:\n'
            '      class_name: C\n'
            '      code: virtual void Bar();\n')
        self.assertEqual(
            result, 'class C {\n public:\n  void Foo();\n'
            '  virtual void Bar();\n\n private:\n  int x_;\n};\n')

    def test_add_to_public_appends_before_first_following_section(self):
        # With both a protected and a private section, the append lands before
        # the first one after public (protected) -- the end of public.
        result = self._apply(
            'pub_before_prot.h',
            'class C {\n public:\n  void Foo();\n protected:\n  void P();\n'
            ' private:\n  int x_;\n};\n', 'substitutions:\n'
            '  - description: expose a public hook\n'
            '    add_to_public:\n'
            '      class_name: C\n'
            '      code: virtual void Bar();\n')
        self.assertEqual(
            result, 'class C {\n public:\n  void Foo();\n'
            '  virtual void Bar();\n\n protected:\n  void P();\n'
            ' private:\n  int x_;\n};\n')

    def test_add_to_public_appends_before_closing_brace(self):
        # A pure interface (only a public section): with no following specifier
        # to anchor on, the code lands just before the class's closing brace.
        result = self._apply(
            'pub_only.h', 'class C {\n public:\n  void Foo();\n};\n',
            'substitutions:\n'
            '  - description: expose a public hook\n'
            '    add_to_public:\n'
            '      class_name: C\n'
            '      code: virtual void Bar();\n')
        self.assertEqual(
            result, 'class C {\n public:\n  void Foo();\n'
            '  virtual void Bar();\n};\n')

    def test_add_to_public_reordered_sections_use_closing_brace(self):
        # `public:` is the last section (private declared first): nothing
        # follows public, so the append falls back to the closing brace.
        result = self._apply(
            'pub_last.h',
            'class C {\n private:\n  int x_;\n public:\n  void Foo();\n};\n',
            'substitutions:\n'
            '  - description: expose a public hook\n'
            '    add_to_public:\n'
            '      class_name: C\n'
            '      code: virtual void Bar();\n')
        self.assertEqual(
            result,
            'class C {\n private:\n  int x_;\n public:\n  void Foo();\n'
            '  virtual void Bar();\n};\n')

    def test_add_to_public_scoped_to_named_class(self):
        # Only the named class is touched; a sibling class is left alone.
        result = self._apply(
            'pub_scope.h', 'class C {\n public:\n  void Foo();\n'
            ' private:\n  int c_;\n};\n\n'
            'class D {\n public:\n  void Baz();\n private:\n  int d_;\n};\n',
            'substitutions:\n'
            '  - description: add only to C\n'
            '    add_to_public:\n'
            '      class_name: C\n'
            '      code: virtual void Bar();\n')
        self.assertEqual(
            result, 'class C {\n public:\n  void Foo();\n'
            '  virtual void Bar();\n\n private:\n  int c_;\n};\n\n'
            'class D {\n public:\n  void Baz();\n private:\n  int d_;\n};\n')

    def test_add_to_public_multiline_code(self):
        # A multi-line `code` block appends several declarations, each indented
        # to the member level.
        result = self._apply(
            'pub_multi.h',
            'class C {\n public:\n  void Foo();\n private:\n  int x_;\n};\n',
            'substitutions:\n'
            '  - description: add two public hooks\n'
            '    add_to_public:\n'
            '      class_name: C\n'
            '      code: |-\n'
            '        virtual void A();\n'
            '        virtual void B();\n')
        self.assertEqual(
            result, 'class C {\n public:\n  void Foo();\n'
            '  virtual void A();\n  virtual void B();\n\n'
            ' private:\n  int x_;\n};\n')

    def test_add_to_public_first_of_multiple_public_sections(self):
        # A reopened `public:` appends to the *first* public section (the append
        # anchors on the first following section, here the private one).
        result = self._apply(
            'pub_reopen.h', 'class C {\n public:\n  void A();\n'
            ' private:\n  int x_;\n public:\n  void B();\n};\n',
            'substitutions:\n'
            '  - description: append to the first public section\n'
            '    add_to_public:\n'
            '      class_name: C\n'
            '      code: virtual void Bar();\n')
        self.assertEqual(
            result, 'class C {\n public:\n  void A();\n'
            '  virtual void Bar();\n\n private:\n  int x_;\n public:\n'
            '  void B();\n};\n')

    def test_add_to_public_no_public_fails(self):
        # A class with no public section has nothing to append to.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'pub_none.h', 'class C {\n private:\n  int x_;\n};\n',
                'substitutions:\n'
                '  - description: no public section\n'
                '    add_to_public:\n'
                '      class_name: C\n'
                '      code: virtual void Bar();\n')

    def test_add_to_public_unknown_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: typo arg\n'
            '    add_to_public:\n'
            '      class_name: C\n'
            '      cod: virtual void Bar();\n',
            'Unrecognised add_to_public arg')

    def test_add_to_public_missing_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: missing code\n'
            '    add_to_public:\n'
            '      class_name: C\n', 'add_to_public requires arg')

    def test_add_to_public_count_other_than_one_rejected(self):
        # It always adds exactly once, so any other count is a config error.
        self._expect_value_error(
            'substitutions:\n'
            '  - description: bogus count\n'
            '    count: 2\n'
            '    add_to_public:\n'
            '      class_name: C\n'
            '      code: virtual void Bar();\n',
            'does not accept a count other than 1')

    def test_add_to_public_appends_before_section_in_nested_class(self):
        # A nested class is indented to its own column: the appended member sits
        # at the nested member column and the following section keeps its own.
        result = self._apply(
            'pub_nested_section.h', 'class Outer {\n public:\n  class C {\n'
            '   public:\n    void Foo();\n   private:\n    int x_;\n'
            '  };\n};\n', 'substitutions:\n'
            '  - description: expose a public hook on the nested class\n'
            '    add_to_public:\n'
            '      class_name: C\n'
            '      code: virtual void Bar();\n')
        self.assertEqual(
            result, 'class Outer {\n public:\n  class C {\n'
            '   public:\n    void Foo();\n    virtual void Bar();\n\n'
            '   private:\n    int x_;\n  };\n};\n')

    def test_add_to_public_appends_before_close_in_nested_class(self):
        # A nested pure-interface class: the appended member sits at the nested
        # member column and the closing brace keeps its own indentation.
        result = self._apply(
            'pub_nested_close.h', 'class Outer {\n public:\n  class C {\n'
            '   public:\n    void Foo();\n  };\n};\n', 'substitutions:\n'
            '  - description: expose a public hook on the nested interface\n'
            '    add_to_public:\n'
            '      class_name: C\n'
            '      code: virtual void Bar();\n')
        self.assertEqual(
            result, 'class Outer {\n public:\n  class C {\n'
            '   public:\n    void Foo();\n    virtual void Bar();\n'
            '  };\n};\n')

    def test_add_to_public_multiline_indents_uniformly_when_nested(self):
        # Every appended line lands at the same nested member column -- the
        # first line is not indented differently from the rest.
        result = self._apply(
            'pub_nested_multi.h', 'class Outer {\n public:\n  class C {\n'
            '   public:\n    void Foo();\n   private:\n    int x_;\n'
            '  };\n};\n', 'substitutions:\n'
            '  - description: add two nested public hooks\n'
            '    add_to_public:\n'
            '      class_name: C\n'
            '      code: |-\n'
            '        virtual void A();\n'
            '        virtual void B();\n')
        self.assertEqual(
            result, 'class Outer {\n public:\n  class C {\n'
            '   public:\n    void Foo();\n'
            '    virtual void A();\n    virtual void B();\n\n'
            '   private:\n    int x_;\n  };\n};\n')

    # -- add_enum_entries op (real ast-grep binary) --------------------------
    #
    # The sources below are cut down from the upstream enums our `rewrite/`
    # plasters extend today, so each case shows the rewriter covering one of
    # them.

    def test_add_enum_entries_repoints_the_max_value(self):
        # chrome/browser/ui/page_action/page_action_model.h: the entries are
        # appended to the nested `Property` enum, at its own column, and
        # kMaxValue is re-pointed at the last one so PropertySet's EnumSet range
        # covers them.
        result = self._apply(
            'enum_property.h', 'class PageActionModelInterface {\n public:\n'
            '  enum class Property {\n'
            '    kShowRequested,\n'
            '    kOverrideBackgroundColor,\n'
            '    kMaxValue = kOverrideBackgroundColor,\n'
            '  };\n};\n', 'substitutions:\n'
            '  - description: add the Brave properties\n'
            '    add_enum_entries:\n'
            '      enum_name: Property\n'
            '      max_value: kMaxValue\n'
            '      entries:\n'
            '        - kAlwaysShowLabel\n'
            '        - kOverrideChipColors\n'
            '        - kOverrideBorder\n')
        self.assertEqual(
            result, 'class PageActionModelInterface {\n public:\n'
            '  enum class Property {\n'
            '    kShowRequested,\n'
            '    kOverrideBackgroundColor,\n'
            '    kAlwaysShowLabel,\n'
            '    kOverrideChipColors,\n'
            '    kOverrideBorder,\n'
            '    kMaxValue = kOverrideBorder,\n'
            '  };\n};\n')

    def test_add_enum_entries_repoints_a_max_value_of_any_name(self):
        # components/sync/base/user_selectable_type.h: the max value is spelled
        # kLastType, and carries no trailing comma of its own -- neither matters
        # to the insertion, which re-emits the entry in place.
        result = self._apply(
            'enum_last_type.h', 'enum class UserSelectableType {\n'
            '  kBookmarks,\n'
            '  kFirstType = kBookmarks,\n'
            '\n'
            '  kCookies,\n'
            '  kLastType = kCookies\n'
            '};\n', 'substitutions:\n'
            '  - description: append kAIChat as the new kLastType\n'
            '    add_enum_entries:\n'
            '      enum_name: UserSelectableType\n'
            '      max_value: kLastType\n'
            '      entries: kAIChat\n')
        self.assertEqual(
            result, 'enum class UserSelectableType {\n'
            '  kBookmarks,\n'
            '  kFirstType = kBookmarks,\n'
            '\n'
            '  kCookies,\n'
            '  kAIChat,\n'
            '  kLastType = kAIChat\n'
            '};\n')

    def test_add_enum_entries_accepts_entries_carrying_values(self):
        # components/permissions/request_type.h: the Brave entries include
        # aliases of their own. The max value is re-pointed at the last entry
        # added -- kBraveMaxValue, which its alias makes equal to kBraveCardano.
        result = self._apply(
            'enum_request_type.h', 'enum class RequestType {\n'
            '  kStorageAccess,\n'
            '  kWindowManagement,\n'
            '  kMaxValue = kWindowManagement,\n'
            '};\n', 'substitutions:\n'
            '  - description: append the Brave request types\n'
            '    add_enum_entries:\n'
            '      enum_name: RequestType\n'
            '      max_value: kMaxValue\n'
            '      entries:\n'
            '        - kWidevine\n'
            '        - kBraveCardano\n'
            '        - kBraveMinValue = kWidevine\n'
            '        - kBraveMaxValue = kBraveCardano\n')
        self.assertEqual(
            result, 'enum class RequestType {\n'
            '  kStorageAccess,\n'
            '  kWindowManagement,\n'
            '  kWidevine,\n'
            '  kBraveCardano,\n'
            '  kBraveMinValue = kWidevine,\n'
            '  kBraveMaxValue = kBraveCardano,\n'
            '  kMaxValue = kBraveMaxValue,\n'
            '};\n')

    def test_add_enum_entries_leaves_a_valueless_max_value_alone(self):
        # An entry that carries no value of its own follows on from whatever
        # precedes it, so inserting before it already moves it along; it is
        # re-emitted untouched rather than pointed at the new last key.
        result = self._apply(
            'enum_count.h', 'enum class Kind {\n'
            '  kA,\n'
            '  kCount,\n'
            '};\n', 'substitutions:\n'
            '  - description: add kB before the count\n'
            '    add_enum_entries:\n'
            '      enum_name: Kind\n'
            '      max_value: kCount\n'
            '      entries: kB\n')
        self.assertEqual(
            result, 'enum class Kind {\n'
            '  kA,\n'
            '  kB,\n'
            '  kCount,\n'
            '};\n')

    def test_add_enum_entries_appends_without_a_max_value(self):
        # crypto/signature_verifier.h: an unscoped enum with no max-value entry,
        # so the entries simply follow the last one. The comment above that entry
        # is not where the insertion goes.
        result = self._apply(
            'enum_signature.h', 'class SignatureVerifier {\n public:\n'
            '  enum SignatureAlgorithm {\n'
            '    RSA_PKCS1_SHA1,\n'
            '    // This is RSA-PSS with SHA-256 as both signing hash and MGF-1\n'
            '    // hash.\n'
            '    RSA_PSS_SHA256,\n'
            '  };\n};\n', 'substitutions:\n'
            '  - description: add ECDSA_SHA384 for downstream switches\n'
            '    add_enum_entries:\n'
            '      enum_name: SignatureAlgorithm\n'
            '      entries: ECDSA_SHA384\n')
        self.assertEqual(
            result, 'class SignatureVerifier {\n public:\n'
            '  enum SignatureAlgorithm {\n'
            '    RSA_PKCS1_SHA1,\n'
            '    // This is RSA-PSS with SHA-256 as both signing hash and MGF-1\n'
            '    // hash.\n'
            '    RSA_PSS_SHA256,\n'
            '    ECDSA_SHA384,\n'
            '  };\n};\n')

    def test_add_enum_entries_appends_without_a_max_value_or_a_separator(self):
        # The same enum, with the trailing comma its last entry is free to omit
        # while nothing follows it. Appending after that entry adds the comma the
        # new key now requires, and the comment above is still not the anchor.
        result = self._apply(
            'enum_signature_bare.h', 'class SignatureVerifier {\n public:\n'
            '  enum SignatureAlgorithm {\n'
            '    RSA_PKCS1_SHA1,\n'
            '    // This is RSA-PSS with SHA-256 as both signing hash and MGF-1\n'
            '    // hash.\n'
            '    RSA_PSS_SHA256\n'
            '  };\n};\n', 'substitutions:\n'
            '  - description: add ECDSA_SHA384 for downstream switches\n'
            '    add_enum_entries:\n'
            '      enum_name: SignatureAlgorithm\n'
            '      entries: ECDSA_SHA384\n')
        self.assertEqual(
            result, 'class SignatureVerifier {\n public:\n'
            '  enum SignatureAlgorithm {\n'
            '    RSA_PKCS1_SHA1,\n'
            '    // This is RSA-PSS with SHA-256 as both signing hash and MGF-1\n'
            '    // hash.\n'
            '    RSA_PSS_SHA256,\n'
            '    ECDSA_SHA384,\n'
            '  };\n};\n')

    def test_add_enum_entries_ignores_a_comment_after_the_last_entry(self):
        # A comment trailing the last entry is not an entry: the entries still go
        # after that entry, where a match on the body's closing brace would put
        # them after the comment instead.
        result = self._apply(
            'enum_trailing_comment.h', 'enum class OriginFilter {\n'
            '  kPublic = 0,\n'
            '  kValidTestOriginForTesting,\n'
            '  // NOTE(crbug.com/481255908): Remove the placeholder filter.\n'
            '};\n', 'substitutions:\n'
            '  - description: add the Brave origins\n'
            '    add_enum_entries:\n'
            '      enum_name: OriginFilter\n'
            '      entries:\n'
            '        - kBraveSearch\n'
            '        - kBraveTalk\n')
        self.assertEqual(
            result, 'enum class OriginFilter {\n'
            '  kPublic = 0,\n'
            '  kValidTestOriginForTesting,\n'
            '  kBraveSearch,\n'
            '  kBraveTalk,\n'
            '  // NOTE(crbug.com/481255908): Remove the placeholder filter.\n'
            '};\n')

    def test_add_enum_entries_separates_a_bare_last_entry(self):
        # The upstream last entry carries no comma, since nothing followed it.
        # Appending after it adds the one the new entries now require.
        result = self._apply(
            'enum_bare_last.h', 'enum class Kind {\n'
            '  kA,\n'
            '  kB\n'
            '};\n', 'substitutions:\n'
            '  - description: append two kinds\n'
            '    add_enum_entries:\n'
            '      enum_name: Kind\n'
            '      entries:\n'
            '        - kC\n'
            '        - kD\n')
        self.assertEqual(
            result, 'enum class Kind {\n'
            '  kA,\n'
            '  kB,\n'
            '  kC,\n'
            '  kD,\n'
            '};\n')

    def test_add_enum_entries_scoped_to_named_enum(self):
        # Only the named enum is extended; a sibling enum is left alone.
        result = self._apply(
            'enum_scope.h', 'enum class Kind {\n  kA,\n};\n\n'
            'enum class Other {\n  kA,\n};\n', 'substitutions:\n'
            '  - description: add only to Kind\n'
            '    add_enum_entries:\n'
            '      enum_name: Kind\n'
            '      entries: kB\n')
        self.assertEqual(
            result, 'enum class Kind {\n  kA,\n  kB,\n};\n\n'
            'enum class Other {\n  kA,\n};\n')

    def test_add_enum_entries_extends_an_enum_with_a_base_clause(self):
        # An underlying type on the enum head does not get in the way of the
        # body's entries.
        result = self._apply(
            'enum_base.h', 'enum class Kind : int {\n  kA,\n};\n',
            'substitutions:\n'
            '  - description: add kB\n'
            '    add_enum_entries:\n'
            '      enum_name: Kind\n'
            '      entries: kB\n')
        self.assertEqual(result, 'enum class Kind : int {\n  kA,\n  kB,\n};\n')

    def test_add_enum_entries_two_enums_of_the_same_name_fail(self):
        # Two enums share the name, so which one to extend is ambiguous: the
        # count check flags it rather than extending both.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'enum_ambiguous.h', 'enum class Kind {\n  kA,\n};\n'
                'class C {\n public:\n  enum class Kind {\n    kX,\n  };\n};\n',
                'substitutions:\n'
                '  - description: ambiguous enum name\n'
                '    add_enum_entries:\n'
                '      enum_name: Kind\n'
                '      entries: kB\n')

    def test_add_enum_entries_max_value_not_last_fails(self):
        # The declared max value is no longer the enum's last entry, so the
        # premise of the insertion no longer holds. The error names the entry
        # that is last, so the next reader knows what changed upstream.
        with self.assertRaises(plaster.PlasterApplyError) as ctx:
            self._apply(
                'enum_moved_max_value.h', 'enum class Kind {\n'
                '  kA,\n'
                '  kMaxValue = kA,\n'
                '  kExtra,\n'
                '};\n', 'substitutions:\n'
                '  - description: the max value is not last anymore\n'
                '    add_enum_entries:\n'
                '      enum_name: Kind\n'
                '      max_value: kMaxValue\n'
                '      entries: kB\n')
        self.assertIn(
            'Enum `Kind` ends in `kExtra`, not in the declared `max_value` '
            'entry `kMaxValue`', str(ctx.exception))

    def test_add_enum_entries_absent_enum_fails(self):
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'enum_absent.h', 'enum class Kind {\n  kA,\n};\n',
                'substitutions:\n'
                '  - description: no such enum\n'
                '    add_enum_entries:\n'
                '      enum_name: Missing\n'
                '      entries: kB\n')

    def test_add_enum_entries_empty_enum_fails(self):
        # There is no last entry to anchor on.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'enum_empty.h', 'enum class Kind {};\n', 'substitutions:\n'
                '  - description: nothing to append to\n'
                '    add_enum_entries:\n'
                '      enum_name: Kind\n'
                '      entries: kB\n')

    def test_add_enum_entries_unknown_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: typo arg\n'
            '    add_enum_entries:\n'
            '      enum_name: Kind\n'
            '      key: kB\n', 'Unrecognised add_enum_entries arg')

    def test_add_enum_entries_missing_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: missing entries\n'
            '    add_enum_entries:\n'
            '      enum_name: Kind\n', 'add_enum_entries requires arg')

    def test_add_enum_entries_empty_entry_list_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: no entries to add\n'
            '    add_enum_entries:\n'
            '      enum_name: Kind\n'
            '      entries: []\n',
            'add_enum_entries `entries` must be a string or a '
            'non-empty list of strings')

    def test_add_enum_entries_entry_with_trailing_comma_rejected(self):
        # The separators are the rewriter's to add, so an authored one is a
        # mistake rather than something to pass through.
        self._expect_value_error(
            'substitutions:\n'
            '  - description: comma in the key\n'
            '    add_enum_entries:\n'
            '      enum_name: Kind\n'
            "      entries: 'kB,'\n",
            'add_enum_entries `entries` items must be an '
            'entry name')

    def test_add_enum_entries_max_value_must_name_an_entry(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: the max value is not a name\n'
            '    add_enum_entries:\n'
            '      enum_name: Kind\n'
            '      max_value: kMaxValue = kA\n'
            '      entries: kB\n',
            'add_enum_entries `max_value` must be an entry name')

    def test_add_enum_entries_count_other_than_one_rejected(self):
        # It always adds the entries once, so any other count is a config error.
        self._expect_value_error(
            'substitutions:\n'
            '  - description: bogus count\n'
            '    count: 2\n'
            '    add_enum_entries:\n'
            '      enum_name: Kind\n'
            '      entries: kB\n', 'does not accept a count other than 1')

    # -- after_function_impl op (real ast-grep binary) -------------------------

    def test_after_function_impl_void(self):
        # A void function: the body is wrapped in a bare IIFE lambda and the
        # appended code runs after it, unconditionally.
        result = self._apply(
            'append.cc', 'void C::Foo() {\n  Upstream();\n}\n',
            'substitutions:\n'
            '  - description: always run Brave code after the body\n'
            '    after_function_impl:\n'
            '      function_name: C::Foo\n'
            '      code: |-\n'
            '        RecordBraveMetric();\n')
        self.assertEqual(
            result,
            'void C::Foo() {\n  [&]() -> void {\n  Upstream();\n  }();\n'
            '  RecordBraveMetric();\n}\n')

    def test_after_function_impl_captures_result(self):
        # A non-void function: `result_var` binds the wrapped body's value so
        # the appended code can use it and own the final return.
        result = self._apply(
            'append_result.cc', 'int C::Compute() {\n  return real_;\n}\n',
            'substitutions:\n'
            '  - description: adjust the computed value for Brave\n'
            '    after_function_impl:\n'
            '      function_name: C::Compute\n'
            '      result_var: score\n'
            '      code: |-\n'
            '        return BraveAdjust(score);\n')
        self.assertEqual(
            result, 'int C::Compute() {\n  int score = [&]() -> int {\n'
            '  return real_;\n  }();\n  return BraveAdjust(score);\n}\n')

    def test_after_function_impl_reference_return_type(self):
        # The lambda and the result variable both state the return type exactly
        # as upstream spells it. `const` leads the type as a separate node and
        # the `&` hangs off the declarator, so neither is part of the `type`
        # field -- a naive capture would yield `std::vector<int>`, silently
        # turning the reference return into a copy.
        result = self._apply(
            'append_ref.cc',
            'const std::vector<int>& C::Items() const {\n  return items_;\n}\n',
            'substitutions:\n'
            '  - description: let Brave filter the returned items\n'
            '    after_function_impl:\n'
            '      function_name: C::Items\n'
            '      result_var: items\n'
            '      code: |-\n'
            '        return BraveFilter(items);\n')
        self.assertEqual(
            result, 'const std::vector<int>& C::Items() const {\n'
            '  const std::vector<int>& items = [&]()'
            ' -> const std::vector<int>& {\n'
            '  return items_;\n  }();\n  return BraveFilter(items);\n}\n')

    def test_after_function_impl_pointer_return_type(self):
        # The `*` lives on the declarator, not the type field.
        result = self._apply(
            'append_ptr.cc', 'Widget* C::GetWidget() {\n  return w_;\n}\n',
            'substitutions:\n'
            '  - description: fall back to the Brave widget\n'
            '    after_function_impl:\n'
            '      function_name: C::GetWidget\n'
            '      result_var: widget\n'
            '      code: |-\n'
            '        return widget ? widget : BraveWidget();\n')
        self.assertEqual(
            result, 'Widget* C::GetWidget() {\n'
            '  Widget* widget = [&]() -> Widget* {\n'
            '  return w_;\n  }();\n'
            '  return widget ? widget : BraveWidget();\n}\n')

    def test_after_function_impl_trailing_return_type(self):
        # With a trailing return type the `type` field is a bare `auto`, so the
        # capture takes the trailing type instead -- its first candidate.
        result = self._apply(
            'append_trailing.cc',
            'auto C::Name() -> const char* {\n  return name_;\n}\n',
            'substitutions:\n'
            '  - description: let Brave rename\n'
            '    after_function_impl:\n'
            '      function_name: C::Name\n'
            '      result_var: name\n'
            '      code: |-\n'
            '        return BraveName(name);\n')
        self.assertEqual(
            result, 'auto C::Name() -> const char* {\n'
            '  const char* name = [&]() -> const char* {\n'
            '  return name_;\n  }();\n  return BraveName(name);\n}\n')

    def test_after_function_impl_multiline_signature_return_type(self):
        # A return type split across lines is spliced into generated code as a
        # single expression, so its whitespace collapses to single spaces.
        result = self._apply(
            'append_multiline_sig.cc',
            'const std::map<int, std::string>&\nC::Map() const {\n'
            '  return m_;\n}\n', 'substitutions:\n'
            '  - description: let Brave extend the map\n'
            '    after_function_impl:\n'
            '      function_name: C::Map\n'
            '      result_var: m\n'
            '      code: |-\n'
            '        return BraveMap(m);\n')
        self.assertEqual(
            result, 'const std::map<int, std::string>&\nC::Map() const {\n'
            '  const std::map<int, std::string>& m = [&]()'
            ' -> const std::map<int, std::string>& {\n'
            '  return m_;\n  }();\n  return BraveMap(m);\n}\n')

    def test_after_function_impl_constructor(self):
        # A constructor has no return type to read, so the capture falls
        # through to `void` -- which is what its wrapped body returns.
        result = self._apply(
            'append_ctor.cc', 'C::C() : x_(1) {\n  Init();\n}\n',
            'substitutions:\n'
            '  - description: run Brave setup after the upstream body\n'
            '    after_function_impl:\n'
            '      function_name: C::C\n'
            '      code: |-\n'
            '        BraveInit();\n')
        self.assertEqual(
            result, 'C::C() : x_(1) {\n  [&]() -> void {\n  Init();\n  }();\n'
            '  BraveInit();\n}\n')

    def test_after_function_impl_wraps_early_returns(self):
        # Every `return` in the upstream body only returns from the lambda, so
        # the appended code still runs. The body's own lines are untouched.
        source = ('void C::Foo() {\n'
                  '  if (!ready_) {\n'
                  '    return;\n'
                  '  }\n'
                  '  Work();\n'
                  '}\n')
        result = self._apply(
            'append_early.cc', source, 'substitutions:\n'
            '  - description: guarantee cleanup runs\n'
            '    after_function_impl:\n'
            '      function_name: C::Foo\n'
            '      code: |-\n'
            '        BraveCleanup();\n')
        self.assertEqual(
            result, 'void C::Foo() {\n  [&]() -> void {\n'
            '  if (!ready_) {\n    return;\n  }\n  Work();\n  }();\n'
            '  BraveCleanup();\n}\n')

    def test_after_function_impl_free_function(self):
        result = self._apply(
            'append_free.cc', 'void FreeFunc(int x) {\n  Upstream(x);\n}\n',
            'substitutions:\n'
            '  - description: append to a free function\n'
            '    after_function_impl:\n'
            '      function_name: FreeFunc\n'
            '      code: |-\n'
            '        AfterFree(x);\n')
        self.assertEqual(
            result,
            'void FreeFunc(int x) {\n  [&]() -> void {\n  Upstream(x);\n'
            '  }();\n  AfterFree(x);\n}\n')

    def test_after_function_impl_multiline_code_indented(self):
        # A multi-line `code` block is authored flush-left and indented to the
        # body level as a whole; a blank line stays empty.
        result = self._apply(
            'append_block.cc', 'void C::Foo() {\n  Upstream();\n}\n',
            'substitutions:\n'
            '  - description: append a two-statement block\n'
            '    after_function_impl:\n'
            '      function_name: C::Foo\n'
            '      code: |-\n'
            '        Prepare();\n'
            '\n'
            '        Track();\n')
        self.assertEqual(
            result,
            'void C::Foo() {\n  [&]() -> void {\n  Upstream();\n  }();\n'
            '  Prepare();\n\n  Track();\n}\n')

    def test_after_function_impl_nested_class_out_of_line_method(self):
        # As with preempt_function_impl, a method of a nested class defined
        # out-of-line must be named with its full enclosing scope.
        result = self._apply(
            'append_nested_method.cc', 'void Outer::Inner::Method(int x) {\n'
            '  Upstream(x);\n}\n', 'substitutions:\n'
            '  - description: append after a nested class method\n'
            '    after_function_impl:\n'
            '      function_name: Outer::Inner::Method\n'
            '      code: |-\n'
            '        AfterNested(x);\n')
        self.assertEqual(
            result, 'void Outer::Inner::Method(int x) {\n  [&]() -> void {\n'
            '  Upstream(x);\n  }();\n  AfterNested(x);\n}\n')

    def test_after_function_impl_partial_qualification_fails(self):
        with self.assertRaises(plaster.PlasterApplyError) as ctx:
            self._apply(
                'append_nested_method_partial.cc',
                'void Outer::Inner::Method(int x) {\n'
                '  Upstream(x);\n}\n', 'substitutions:\n'
                '  - description: partial qualification does not match\n'
                '    after_function_impl:\n'
                '      function_name: Inner::Method\n'
                '      code: |-\n'
                '        AfterNested(x);\n')
        self.assertIn('Unexpected number of matches (0 vs 1)',
                      str(ctx.exception))

    def test_after_function_impl_targets_named_function_only(self):
        # Only the named function's body is wrapped, not a sibling.
        result = self._apply(
            'append_siblings.cc',
            'void C::A() {\n  a();\n}\n\nvoid C::B() {\n  b();\n}\n',
            'substitutions:\n'
            '  - description: append only to B\n'
            '    after_function_impl:\n'
            '      function_name: C::B\n'
            '      code: |-\n'
            '        after_b();\n')
        self.assertEqual(
            result, 'void C::A() {\n  a();\n}\n\n'
            'void C::B() {\n  [&]() -> void {\n  b();\n  }();\n'
            '  after_b();\n}\n')

    def test_after_function_impl_overloads_need_count(self):
        result = self._apply(
            'append_overloads.cc',
            'void C::F() {\n  a();\n}\n\nvoid C::F(int x) {\n  b();\n}\n',
            'substitutions:\n'
            '  - description: append to both overloads\n'
            '    count: 2\n'
            '    after_function_impl:\n'
            '      function_name: C::F\n'
            '      code: |-\n'
            '        done();\n')
        self.assertEqual(
            result, 'void C::F() {\n  [&]() -> void {\n  a();\n  }();\n'
            '  done();\n}\n\n'
            'void C::F(int x) {\n  [&]() -> void {\n  b();\n  }();\n'
            '  done();\n}\n')

    def test_after_function_impl_overload_count_mismatch_fails(self):
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'append_overloads_bad.cc',
                'void C::F() {\n  a();\n}\n\nvoid C::F(int x) {\n  b();\n}\n',
                'substitutions:\n'
                '  - description: forgot the count\n'
                '    after_function_impl:\n'
                '      function_name: C::F\n'
                '      code: |-\n'
                '        done();\n')

    def test_after_function_impl_absent_function_fails(self):
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'append_missing.cc', 'void C::Foo() {\n}\n', 'substitutions:\n'
                '  - description: no such function\n'
                '    after_function_impl:\n'
                '      function_name: C::Nope\n'
                '      code: |-\n'
                '        x();\n')

    def test_after_function_impl_missing_code_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: missing code\n'
            '    after_function_impl:\n'
            '      function_name: C::F\n',
            'after_function_impl `code` must be a non-empty string')

    def test_after_function_impl_missing_function_name_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: missing function_name\n'
            '    after_function_impl:\n'
            '      code: x();\n',
            'after_function_impl `function_name` must be a non-empty string')

    def test_after_function_impl_empty_result_var_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: empty result_var\n'
            '    after_function_impl:\n'
            '      function_name: C::F\n'
            '      result_var: \'\'\n'
            '      code: x();\n',
            'after_function_impl `result_var` must be a non-empty string')

    def test_after_function_impl_unknown_arg_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: typo arg\n'
            '    after_function_impl:\n'
            '      function_name: C::F\n'
            '      cod: x();\n', 'Unrecognised after_function_impl arg')

    # -- export-macro classes (real ast-grep binary) ----------------------
    #
    # tree-sitter cannot parse `class MACRO_EXPORT Name`, so the engine blanks
    # the macro before matching. These confirm the AST rewriters reach a class
    # declared with an export macro while leaving the macro itself in place.

    def test_drop_final_on_export_macro_class(self):
        result = self._apply(
            'exp_final.h',
            'class MODULES_EXPORT C final : public Base {\n};\n',
            'blank_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: drop final on an exported class\n'
            '    drop_final:\n'
            '      class_name: C\n')
        self.assertEqual(result,
                         'class MODULES_EXPORT C : public Base {\n};\n')

    def test_make_virtual_on_export_macro_class(self):
        result = self._apply(
            'exp_virt.h', 'class MODULES_EXPORT C {\n  void Foo();\n};\n',
            'blank_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: make Foo virtual on an exported class\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: Foo\n')
        self.assertEqual(
            result, 'class MODULES_EXPORT C {\n  virtual void Foo();\n};\n')

    def test_add_friend_on_parenthesised_export_macro_class(self):
        # The parenthesised `COMPONENT_EXPORT(FOO)` form is blanked too.
        result = self._apply(
            'exp_friend.h',
            'class COMPONENT_EXPORT(FOO) C {\n private:\n  int x_;\n};\n',
            'blank_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: friend an exported class\n'
            '    add_friend:\n'
            '      class_name: C\n'
            '      friend_type: class BraveC\n')
        self.assertEqual(
            result, 'class COMPONENT_EXPORT(FOO) C {\n'
            ' private:\n  friend class BraveC;\n  int x_;\n};\n')

    # -- classes with base-list preprocessor conditionals -----------------
    #
    # A `#if` in the base-specifier list stops tree-sitter from resolving the
    # class; the engine blanks it before matching. These confirm the AST
    # rewriters reach such a class while the conditional survives in the output.

    _AURA_CLASS = ('class C : public A\n'
                   '#if defined(USE_AURA)\n'
                   '    ,\n'
                   '         public D\n'
                   '#endif  // defined(USE_AURA)\n'
                   '{\n'
                   ' public:\n'
                   '  void Foo();\n'
                   ' private:\n'
                   '  int x_;\n'
                   '};\n')

    def test_make_virtual_through_base_list_conditional(self):
        result = self._apply(
            'aura_virt.h', self._AURA_CLASS,
            'blank_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: make Foo virtual despite the aura base\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: Foo\n')
        self.assertEqual(
            result,
            self._AURA_CLASS.replace('  void Foo();', '  virtual void Foo();'))

    def test_add_friend_through_base_list_conditional(self):
        result = self._apply(
            'aura_friend.h', self._AURA_CLASS,
            'blank_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: friend despite the aura base\n'
            '    add_friend:\n'
            '      class_name: C\n'
            '      friend_type: class BraveC\n')
        self.assertEqual(
            result,
            self._AURA_CLASS.replace(' private:\n',
                                     ' private:\n  friend class BraveC;\n'))

    # -- classes wrapped in Views METADATA_HEADER/BEGIN_METADATA/END_METADATA
    #
    # METADATA_HEADER(Name, Base) (in the class body) and
    # BEGIN_METADATA(Name, Base) ... END_METADATA (right after it, at
    # namespace scope) are bare macro calls with no trailing `;`; tree-sitter
    # turns the call -- and, for BEGIN_METADATA, everything after it -- into
    # one ERROR node. This mirrors the real bug: an unrelated class's
    # BEGIN_METADATA/END_METADATA sitting just before the target class broke
    # every AST rewriter's ability to reach it.

    # The leading comment lines and blank line before the class are load-
    # bearing for the repro: tree-sitter's error recovery only cascades all
    # the way to `class Outer::Inner` with this exact shape ahead of it
    # (confirmed empirically -- dropping them, or the constructor's member
    # initializer, makes it recover locally instead, same as it does for
    # `blank_macros_for_ast_parsing`'s export-macro/conditional cases).
    _METADATA_CLASS = ('BEGIN_METADATA(Unrelated, views::View)\n'
                       'END_METADATA\n'
                       '\n'
                       '// Outer::Inner\n'
                       '// ----------------------------------------------\n'
                       'class Outer::Inner : public views::View {\n'
                       '  METADATA_HEADER(Inner, views::View)\n'
                       '\n'
                       ' public:\n'
                       '  explicit Inner(Outer* bubble_view)\n'
                       '      : bubble_view_(bubble_view) {\n'
                       '    CHECK(bubble_view_);\n'
                       '  }\n'
                       '\n'
                       '  void Foo();\n'
                       '\n'
                       ' private:\n'
                       '  int x_;\n'
                       '};\n'
                       '\n'
                       'BEGIN_METADATA(Inner, views::View)\n'
                       'END_METADATA\n')

    def test_add_friend_reaches_class_after_begin_metadata(self):
        result = self._apply(
            'metadata_friend.h', self._METADATA_CLASS,
            'blank_metadata_header_macros: true\n'
            'substitutions:\n'
            '  - description: friend the Brave subclass past BEGIN_METADATA\n'
            '    add_friend:\n'
            '      class_name: Outer::Inner\n'
            '      friend_type: class BraveInner\n')
        self.assertEqual(
            result,
            self._METADATA_CLASS.replace(
                ' private:\n', ' private:\n  friend class BraveInner;\n'))

    def test_make_virtual_reaches_class_after_begin_metadata(self):
        result = self._apply(
            'metadata_virt.h', self._METADATA_CLASS,
            'blank_metadata_header_macros: true\n'
            'substitutions:\n'
            '  - description: make Foo virtual past BEGIN_METADATA\n'
            '    make_virtual:\n'
            '      class_name: Outer::Inner\n'
            '      method_name: Foo\n')
        self.assertEqual(
            result,
            self._METADATA_CLASS.replace('  void Foo();',
                                         '  virtual void Foo();'))

    def test_rename_class_renames_name_inside_metadata_macros(self):
        # The blanking preserves `name`'s byte offset exactly so the edit,
        # which is always spliced onto the real (unblanked) source, lands on
        # the literal `Inner` inside METADATA_HEADER and BEGIN_METADATA too --
        # not just the class declaration itself.
        result = self._apply(
            'metadata_rename.h', self._METADATA_CLASS,
            'blank_metadata_header_macros: true\n'
            'substitutions:\n'
            '  - description: rename Inner\n'
            '    rename_class:\n'
            '      class_name: Inner\n'
            '      rename: Inner_ChromiumImpl\n')
        expected = self._METADATA_CLASS
        for old, new in (
            ('class Outer::Inner :', 'class Outer::Inner_ChromiumImpl :'),
            ('explicit Inner(', 'explicit Inner_ChromiumImpl('),
            ('METADATA_HEADER(Inner,', 'METADATA_HEADER(Inner_ChromiumImpl,'),
            ('BEGIN_METADATA(Inner,', 'BEGIN_METADATA(Inner_ChromiumImpl,'),
        ):
            expected = expected.replace(old, new)
        self.assertEqual(result, expected)

    def test_metadata_header_flag_off_by_default_fails(self):
        # Without the flag, BEGIN_METADATA breaks tree-sitter's parse of
        # everything after it, so add_friend finds nothing to friend.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'metadata_no_flag.h', self._METADATA_CLASS, 'substitutions:\n'
                '  - description: no flag, so BEGIN_METADATA breaks parsing\n'
                '    add_friend:\n'
                '      class_name: Outer::Inner\n'
                '      friend_type: class BraveInner\n')

    def test_metadata_header_flag_must_be_boolean(self):
        self._expect_value_error(
            'blank_metadata_header_macros: yes please\n'
            'substitutions:\n'
            '  - description: bad flag type\n'
            '    drop_final:\n'
            '      class_name: C\n',
            '`blank_metadata_header_macros` must be a boolean')

    def test_metadata_header_flag_rejected_for_non_cxx_source(self):
        self._expect_value_error(
            'blank_metadata_header_macros: true\n'
            'substitutions:\n'
            '  - description: flag on a non-C++ source\n'
            '    regex:\n'
            "      re_pattern: 'x'\n"
            "      replace: 'y'\n",
            '`blank_metadata_header_macros` is only supported for C++ '
            'sources',
            name='validation.idl')

    def test_metadata_header_flag_allowed_for_cxx_source(self):
        result = self._apply(
            'metadata_cxx_flag.h', 'A Chromium thing.\n',
            'blank_metadata_header_macros: true\n'
            'substitutions:\n'
            '  - description: flag on a C++ source\n'
            '    regex:\n'
            "      re_pattern: 'Chromium'\n"
            "      replace: 'Brave'\n")
        self.assertEqual(result, 'A Brave thing.\n')

    def test_metadata_header_flag_independent_of_other_blank_flags(self):
        # Enabling the other two blanking passes must not also enable this
        # one -- BEGIN_METADATA still breaks the parse.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'metadata_other_flags.h', self._METADATA_CLASS,
                'blank_macros_for_ast_parsing: true\n'
                'blank_string_adjacent_macros_for_ast_parsing: true\n'
                'substitutions:\n'
                '  - description: wrong flags for this construct\n'
                '    add_friend:\n'
                '      class_name: Outer::Inner\n'
                '      friend_type: class BraveInner\n')

    # -- functions with a macro-adjacent string literal -------------------
    #
    # A bare macro touching a string literal (only valid post-preprocessing,
    # e.g. Skia's `STRINGIZE(SK_MILESTONE)` version string idiom) drops into
    # a tree-sitter error node that can swallow everything up to the next
    # construct it resyncs on. Regression coverage for a real bug: this once
    # made `after_function_impl` on the *first* function wrap the *second*
    # function's body too.

    _VERSION_STRING_FUNCTION = (
        'base::DictValue C::GetClientInfo() {\n'
        '  base::DictValue dict;\n'
        '  dict.Set("graphics_backend",\n'
        '           std::string("Skia/" STRINGIZE(SK_MILESTONE) " " '
        'SKIA_COMMIT_HASH));\n'
        '  return dict;\n'
        '}\n'
        '\n'
        'base::ListValue C::GetLogMessages() {\n'
        '  return GetLogs();\n'
        '}\n')

    def test_after_function_impl_unaffected_by_later_macro_adjacent_string(
            self):
        # Without the fix, `after_function_impl` on `GetClientInfo` would
        # wrap `GetLogMessages` too, since the STRINGIZE construct inside
        # `GetClientInfo` throws tree-sitter's parse off. Confirm it now stays
        # scoped to the target function's own body.
        result = self._apply(
            'version_string.cc', self._VERSION_STRING_FUNCTION,
            'blank_string_adjacent_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: report the executable path after the body\n'
            '    after_function_impl:\n'
            '      function_name: C::GetClientInfo\n'
            '      result_var: dict\n'
            '      code: |-\n'
            '        return dict;\n')
        self.assertEqual(
            result, 'base::DictValue C::GetClientInfo() {\n'
            '  base::DictValue dict = [&]() -> base::DictValue {\n'
            '  base::DictValue dict;\n'
            '  dict.Set("graphics_backend",\n'
            '           std::string("Skia/" STRINGIZE(SK_MILESTONE) " " '
            'SKIA_COMMIT_HASH));\n'
            '  return dict;\n'
            '  }();\n'
            '  return dict;\n'
            '}\n'
            '\n'
            'base::ListValue C::GetLogMessages() {\n'
            '  return GetLogs();\n'
            '}\n')

    def test_make_virtual_unaffected_by_macro_adjacent_string_in_sibling(self):
        # A macro-adjacent string literal in one method must not stop a
        # rewriter from correctly reaching a *different* method in the same
        # class.
        result = self._apply(
            'version_string.h', 'class C {\n'
            ' public:\n'
            '  void GetClientInfo() {\n'
            '    Log("Skia/" STRINGIZE(SK_MILESTONE) " " SKIA_COMMIT_HASH);\n'
            '  }\n'
            '  void Foo();\n'
            '};\n', 'blank_string_adjacent_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: make Foo virtual\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: Foo\n')
        self.assertEqual(
            result, 'class C {\n'
            ' public:\n'
            '  void GetClientInfo() {\n'
            '    Log("Skia/" STRINGIZE(SK_MILESTONE) " " SKIA_COMMIT_HASH);\n'
            '  }\n'
            '  virtual void Foo();\n'
            '};\n')

    def test_blanking_is_off_by_default(self):
        # Without `blank_macros_for_ast_parsing`, the export-macro class is
        # unparseable, so the rewriter matches nothing and the apply fails.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'no_blank.h', 'class MODULES_EXPORT C final {\n};\n',
                'substitutions:\n'
                '  - description: no blanking, so final is invisible\n'
                '    drop_final:\n'
                '      class_name: C\n')

    def test_blank_flag_false_does_not_blank(self):
        # Explicit `false` behaves like the default: still unparseable.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'blank_false.h', 'class MODULES_EXPORT C final {\n};\n',
                'blank_macros_for_ast_parsing: false\n'
                'substitutions:\n'
                '  - description: blanking explicitly off\n'
                '    drop_final:\n'
                '      class_name: C\n')

    def test_blank_flag_must_be_boolean(self):
        self._expect_value_error(
            'blank_macros_for_ast_parsing: yes please\n'
            'substitutions:\n'
            '  - description: bad flag type\n'
            '    drop_final:\n'
            '      class_name: C\n',
            '`blank_macros_for_ast_parsing` must be a boolean')

    def test_unknown_top_level_key_rejected(self):
        self._expect_value_error(
            'blank_macros: true\n'
            'substitutions:\n'
            '  - description: typo in the top-level flag\n'
            '    drop_final:\n'
            '      class_name: C\n', 'Unrecognised top-level plaster key')

    def test_blank_flag_rejected_for_non_cxx_source(self):
        # The flag only applies to C++ files, so it is rejected on a `.idl`
        # target.
        self._expect_value_error(
            'blank_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: flag on a non-C++ source\n'
            '    regex:\n'
            "      re_pattern: 'x'\n"
            "      replace: 'y'\n",
            '`blank_macros_for_ast_parsing` is only supported for C++ sources',
            name='validation.idl')

    def test_blank_flag_allowed_for_cxx_source(self):
        # The same flag is accepted for a `.h` source (here with no AST work to
        # do, so it just applies the regex).
        result = self._apply(
            'cxx_flag.h', 'A Chromium thing.\n',
            'blank_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: flag on a C++ source\n'
            '    regex:\n'
            "      re_pattern: 'Chromium'\n"
            "      replace: 'Brave'\n")
        self.assertEqual(result, 'A Brave thing.\n')

    # -- `blank_string_adjacent_macros_for_ast_parsing` (separate flag) ----
    #
    # A distinct opt-in from `blank_macros_for_ast_parsing`, with the same
    # validation shape, plus coverage that the two are independent end to end
    # (not just at the `CxxMacrosEraser.erase` unit level above).

    _VERSION_STRING_CLASS = ('class C {\n'
                             ' public:\n'
                             '  void Bar() { Log("v" STRINGIZE(V)); }\n'
                             '  void Foo();\n'
                             '};\n')

    def test_string_adjacent_flag_off_by_default(self):
        # `after_function_impl` needs `GetClientInfo`'s function_definition
        # node intact to find its body; on this fixture, without the flag,
        # the STRINGIZE construct's error node swallows enough of it that the
        # query comes up empty (0 matches) rather than finding *a* match.
        # (On the real, larger file this bug was found in, tree-sitter's
        # error recovery instead re-synced onto a much later, wrong
        # `compound_statement` -- still broken, just a different symptom.)
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'no_string_blank.cc', self._VERSION_STRING_FUNCTION,
                'substitutions:\n'
                '  - description: report the executable path after the body\n'
                '    after_function_impl:\n'
                '      function_name: C::GetClientInfo\n'
                '      result_var: dict\n'
                '      code: |-\n'
                '        return dict;\n')

    def test_string_adjacent_flag_must_be_boolean(self):
        self._expect_value_error(
            'blank_string_adjacent_macros_for_ast_parsing: yes please\n'
            'substitutions:\n'
            '  - description: bad flag type\n'
            '    drop_final:\n'
            '      class_name: C\n',
            '`blank_string_adjacent_macros_for_ast_parsing` must be a boolean')

    def test_string_adjacent_flag_rejected_for_non_cxx_source(self):
        self._expect_value_error(
            'blank_string_adjacent_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: flag on a non-C++ source\n'
            '    regex:\n'
            "      re_pattern: 'x'\n"
            "      replace: 'y'\n",
            '`blank_string_adjacent_macros_for_ast_parsing` is only '
            'supported for C++ sources',
            name='validation.idl')

    def test_macros_flag_alone_does_not_enable_string_adjacent_pass(self):
        # Setting `blank_macros_for_ast_parsing` must not also enable the
        # separate string-adjacent pass this construct needs: still fails.
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'macros_only.cc', self._VERSION_STRING_FUNCTION,
                'blank_macros_for_ast_parsing: true\n'
                'substitutions:\n'
                '  - description: wrong flag for this construct\n'
                '    after_function_impl:\n'
                '      function_name: C::GetClientInfo\n'
                '      result_var: dict\n'
                '      code: |-\n'
                '        return dict;\n')

    def test_string_adjacent_flag_alone_fixes_the_match(self):
        result = self._apply(
            'string_adjacent_only.cc', self._VERSION_STRING_FUNCTION,
            'blank_string_adjacent_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: report the executable path after the body\n'
            '    after_function_impl:\n'
            '      function_name: C::GetClientInfo\n'
            '      result_var: dict\n'
            '      code: |-\n'
            '        return dict;\n')
        # Correctly scoped: the wrap closes right after GetClientInfo's own
        # `return dict;`, well before GetLogMessages even starts.
        self.assertNotIn('GetLogMessages', result[:result.index('}();')])

    def test_string_adjacent_flag_alone_reaches_sibling_method(self):
        # A construct simple enough that tree-sitter handles it fine even
        # without the flag; this only confirms the flag does not itself
        # break normal operation on it.
        result = self._apply(
            'string_adjacent_only.h', self._VERSION_STRING_CLASS,
            'blank_string_adjacent_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: make Foo virtual\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: Foo\n')
        self.assertEqual(
            result,
            self._VERSION_STRING_CLASS.replace('void Foo();',
                                               'virtual void Foo();'))

    def test_both_blank_flags_together(self):
        # The two flags compose: an export-macro class *and* a
        # STRINGIZE-guarded method in the same file, both reached in one pass.
        result = self._apply(
            'both_flags.h', 'class MODULES_EXPORT C {\n'
            ' public:\n'
            '  void Bar() { Log("v" STRINGIZE(V)); }\n'
            '  void Foo();\n'
            '};\n', 'blank_macros_for_ast_parsing: true\n'
            'blank_string_adjacent_macros_for_ast_parsing: true\n'
            'substitutions:\n'
            '  - description: make Foo virtual on an exported, STRINGIZE-using class\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: Foo\n')
        self.assertEqual(
            result, 'class MODULES_EXPORT C {\n'
            ' public:\n'
            '  void Bar() { Log("v" STRINGIZE(V)); }\n'
            '  virtual void Foo();\n'
            '};\n')

    def test_ast_rewriter_rejected_for_non_cxx_source(self):
        # AST rewriters belong to the `cxx` namespace, which a `.idl` target
        # is not in, so the name resolves to nothing usable here.
        self._expect_value_error(
            'substitutions:\n'
            '  - description: AST rewriter on a non-C++ source\n'
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: Foo\n',
            'the `make_virtual` rewriter is not available for this source, '
            'which is not in any namespace it serves (cxx)',
            name='validation.idl')

    def test_regex_rewriter_allowed_for_non_cxx_source(self):
        # Text rewriters are language-agnostic, so they work on any source.
        result = self._apply(
            'plain.idl', 'A Chromium thing.\n', 'substitutions:\n'
            '  - description: regex on a non-C++ source\n'
            '    regex:\n'
            "      re_pattern: 'Chromium'\n"
            "      replace: 'Brave'\n")
        self.assertEqual(result, 'A Brave thing.\n')

    # -- js.set_blink_runtime_enabled_feature_state op (real ast-grep) ------
    #
    # Targets `runtime_enabled_features.json5`, parsed with ast-grep's `js`
    # grammar. The rewriter is composite: per entry, it works out at apply
    # time whether to add the field or override the one already there.

    _FEATURE_YAML = ('substitutions:\n'
                     '  - description: Ship MyFeature disabled.\n'
                     '    set_blink_runtime_enabled_feature_state:\n'
                     '      feature_name: MyFeature\n'
                     '      value: disabled\n')

    def test_blink_runtime_enabled_feature_state_adds_a_missing_field(self):
        result = self._apply(
            'runtime_enabled_features.json5', '[\n'
            '  {\n'
            '    name: "MyFeature",\n'
            '    status: "stable",\n'
            '  },\n'
            '];\n', self._FEATURE_YAML)
        self.assertEqual(
            result, '[\n'
            '  {\n'
            '    name: "MyFeature",\n'
            '    base_feature_status: "disabled",  '
            '// feature state is enforced via plaster rewrite.\n'
            '    status: "stable",\n'
            '  },\n'
            '];\n')

    def test_blink_runtime_enabled_feature_state_overrides_an_existing_field(
            self):
        # Upstream puts the field last, by the origin-trial keys rather than
        # by `name`; it is overridden where it stands, not moved.
        result = self._apply(
            'runtime_enabled_features.json5', '[\n'
            '  {\n'
            '    name: "MyFeature",\n'
            '    origin_trial_feature_name: "MyFeature",\n'
            '    base_feature_status: "enabled",\n'
            '    copied_from_base_feature_if: "overridden",\n'
            '  },\n'
            '];\n', self._FEATURE_YAML)
        self.assertEqual(
            result, '[\n'
            '  {\n'
            '    name: "MyFeature",\n'
            '    origin_trial_feature_name: "MyFeature",\n'
            '    base_feature_status: "disabled",  '
            '// feature state is enforced via plaster rewrite.\n'
            '    copied_from_base_feature_if: "overridden",\n'
            '  },\n'
            '];\n')

    def test_blink_runtime_enabled_feature_state_survives_a_leading_comment(
            self):
        # A comment before `name` -- common upstream -- must not defeat the
        # match, as a positional `{name: "...", $$$REST}` pattern would.
        result = self._apply(
            'runtime_enabled_features.json5', '[\n'
            '  {\n'
            '    // PARAKEET ad serving runtime flag/JS API.\n'
            '    name: "MyFeature",\n'
            '    status: "stable",\n'
            '  },\n'
            '];\n', self._FEATURE_YAML)
        self.assertIn(
            '    name: "MyFeature",\n'
            '    base_feature_status: "disabled",  '
            '// feature state is enforced via plaster rewrite.\n', result)

    def test_blink_runtime_enabled_feature_state_ignores_a_longer_name_field(
            self):
        # `origin_trial_feature_name` ends in the same characters as `name`
        # and often carries the same value; the field must land after the
        # real `name`, not after that one.
        result = self._apply(
            'runtime_enabled_features.json5', '[\n'
            '  {\n'
            '    name: "MyFeature",\n'
            '    origin_trial_feature_name: "MyFeature",\n'
            '  },\n'
            '];\n', self._FEATURE_YAML)
        self.assertEqual(
            result, '[\n'
            '  {\n'
            '    name: "MyFeature",\n'
            '    base_feature_status: "disabled",  '
            '// feature state is enforced via plaster rewrite.\n'
            '    origin_trial_feature_name: "MyFeature",\n'
            '  },\n'
            '];\n')

    def test_blink_runtime_enabled_feature_state_leaves_sibling_entries_alone(
            self):
        result = self._apply(
            'runtime_enabled_features.json5', '[\n'
            '  {\n'
            '    name: "OtherFeature",\n'
            '    status: "stable",\n'
            '  },\n'
            '  {\n'
            '    name: "MyFeature",\n'
            '    status: "stable",\n'
            '  },\n'
            '];\n', self._FEATURE_YAML)
        self.assertEqual(
            result, '[\n'
            '  {\n'
            '    name: "OtherFeature",\n'
            '    status: "stable",\n'
            '  },\n'
            '  {\n'
            '    name: "MyFeature",\n'
            '    base_feature_status: "disabled",  '
            '// feature state is enforced via plaster rewrite.\n'
            '    status: "stable",\n'
            '  },\n'
            '];\n')

    def test_blink_runtime_enabled_feature_state_reapplies_unchanged(self):
        # Rerunning over already-migrated text takes the override branch and
        # must not stack up a second comment.
        once = self._apply(
            'runtime_enabled_features.json5', '[\n'
            '  {\n'
            '    name: "MyFeature",\n'
            '    status: "stable",\n'
            '  },\n'
            '];\n', self._FEATURE_YAML)
        twice = self._apply('runtime_enabled_features.json5', once,
                            self._FEATURE_YAML)
        self.assertEqual(once, twice)
        self.assertEqual(
            twice.count('// feature state is enforced via plaster rewrite.'),
            1)

    def test_blink_runtime_enabled_feature_state_unknown_feature_fails(self):
        with self.assertRaises(plaster.PlasterApplyError):
            self._apply(
                'runtime_enabled_features.json5', '[\n'
                '  {\n'
                '    name: "OtherFeature",\n'
                '    status: "stable",\n'
                '  },\n'
                '];\n', self._FEATURE_YAML)

    def test_blink_runtime_enabled_feature_state_count_other_than_one_rejected(
            self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: bogus count\n'
            '    count: 2\n'
            '    set_blink_runtime_enabled_feature_state:\n'
            '      feature_name: MyFeature\n'
            '      value: disabled\n',
            'does not accept a count other than 1',
            name='validation.json5')

    # -- validation ---------------------------------------------------------

    def test_two_op_keys_rejected(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: two ops\n'
            '    regex:\n'
            "      re_pattern: 'x'\n"
            "      replace: 'y'\n"
            '    make_virtual:\n'
            '      class_name: C\n'
            '      method_name: Foo\n', 'Only one rewriter')

    def test_stray_field_alongside_rewriter_rejected(self):
        # A stray item-level field next to a rewriter key is an unrecognised
        # key for that rewriter.
        self._expect_value_error(
            'substitutions:\n'
            '  - description: stray field\n'
            '    regex:\n'
            "      re_pattern: 'x'\n"
            "      replace: 'y'\n"
            "    re_pattern: 'z'\n", 'Unrecognised key(s) for the "regex"')

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
                '    make_final:\n'
                '      class_name: C\n')
        message = str(ctx.exception)
        self.assertIn('Unknown rewriter', message)
        self.assertIn("'make_final'", message)
        self.assertIn('make_virtual', message)

    def test_stray_scalar_key_is_unrecognised(self):
        # Non-mapping stray keys name no rewriter, so they get the generic
        # "Unrecognised substitution key" error rather than the unknown-rewriter
        # one (which is reserved for mapping-valued keys).
        self._expect_value_error(
            'substitutions:\n'
            '  - description: stray fields\n'
            "    re_pattern: 'x'\n"
            "    replace: 'y'\n"
            '    re_flag: [DOTALL]\n', 'Unrecognised substitution key')


class RegexMacroDispatchTest(unittest.TestCase):
    """End-to-end tests for dispatching a `regex_macro:`-style substitution
    key (e.g. `set_feature_flag_default_state:`) to `RegexMacro`.

    Every `regex_macro` op declared in `rewriters.pyl` is handled by the same
    `RegexMacro` class (see `_regex_macro_rewriters`), so these tests exercise
    that generic dispatch/validation path via the one macro currently shipped
    (`set_feature_flag_default_state`), rather than the macro's own regex --
    that is `ToggleBaseFeatureDefaultStateTest`'s job (renamed
    `OverrideFeatureDefaultStateTest`).
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

    def test_macro_op_applies(self):
        result = self._apply(
            'feature.cc',
            'BASE_FEATURE(kFoo, base::FEATURE_ENABLED_BY_DEFAULT);',
            'substitutions:\n'
            '  - description: Ship kFoo disabled.\n'
            '    set_feature_flag_default_state:\n'
            '      feature_name: kFoo\n'
            '      value: base::FEATURE_DISABLED_BY_DEFAULT\n')
        self.assertEqual(
            result, '// kFoo feature state is enforced via plaster rewrite.\n'
            'BASE_FEATURE(kFoo, base::FEATURE_DISABLED_BY_DEFAULT);')

    def test_registered_under_its_bare_name(self):
        # The YAML key is the op id with its `cxx.` prefix stripped, and the
        # prefix becomes the namespace it is registered under.
        self.assertIn('set_feature_flag_default_state', plaster._REWRITERS)
        cls = plaster._REWRITERS.resolve('set_feature_flag_default_state',
                                         'cxx')
        self.assertTrue(issubclass(cls, plaster.RegexMacro))
        self.assertEqual(cls.OP_ID, 'cxx.set_feature_flag_default_state')
        self.assertEqual(cls.namespace(), 'cxx')

    def test_help_text_lists_every_input(self):
        # `plaster --help <macro>` must document each input, not just the
        # macro's own top-level `description`.
        cls = plaster._REWRITERS.resolve('set_feature_flag_default_state',
                                         'cxx')
        help_text = cls.help_text()
        self.assertIn('Fields:', help_text)
        self.assertIn('feature_name', help_text)
        self.assertIn('value', help_text)
        spec = plaster.RewritersEval.load().regex_macro(cls.OP_ID)
        for entry in spec['inputs']:
            self.assertIn(entry['name'], help_text)
            self.assertIn(entry['description'], help_text)

    def test_missing_required_arg_is_rejected(self):
        spec = 'substitutions:\n' \
              '  - description: d\n' \
              '    set_feature_flag_default_state:\n' \
              '      feature_name: kFoo\n'
        with self.assertRaises(ValueError) as cm:
            plaster.Substitution.from_yaml(spec, namespace='cxx')
        self.assertIn('requires arg(s): value', str(cm.exception))

    def test_unknown_arg_is_rejected(self):
        spec = ('substitutions:\n'
                '  - description: d\n'
                '    set_feature_flag_default_state:\n'
                '      feature_name: kFoo\n'
                '      value: base::FEATURE_DISABLED_BY_DEFAULT\n'
                '      bogus: x\n')
        with self.assertRaises(ValueError) as cm:
            plaster.Substitution.from_yaml(spec, namespace='cxx')
        self.assertIn("Unrecognised set_feature_flag_default_state arg(s)",
                      str(cm.exception))
        self.assertIn("'bogus'", str(cm.exception))

    def test_non_string_arg_is_rejected(self):
        spec = ('substitutions:\n'
                '  - description: d\n'
                '    set_feature_flag_default_state:\n'
                '      feature_name: kFoo\n'
                '      value: 1\n')
        with self.assertRaises(ValueError) as cm:
            plaster.Substitution.from_yaml(spec, namespace='cxx')
        self.assertIn('must be a string', str(cm.exception))

    def test_unknown_macro_name_is_unrecognised(self):
        # A name that is not a rewriter and not a declared regex macro falls
        # through to the same "unrecognised" path as any other bad key.
        self._expect_value_error(
            'substitutions:\n'
            '  - description: d\n'
            '    not_a_real_macro:\n'
            '      feature_name: kFoo\n', 'Unknown rewriter')

    def test_only_one_rewriter_allowed_alongside_a_macro(self):
        self._expect_value_error(
            'substitutions:\n'
            '  - description: d\n'
            "    regex:\n"
            "      re_pattern: 'a'\n"
            "      replace: 'b'\n"
            '    set_feature_flag_default_state:\n'
            '      feature_name: kFoo\n'
            '      value: base::FEATURE_DISABLED_BY_DEFAULT\n',
            'Only one rewriter allowed per entry')

    def test_still_matches_and_applies_when_value_already_set(self):
        # The macro always matches -- and so always reports a `count:` of 1,
        # never 0 -- even when the current value already equals the one
        # being set, so the substitution can never silently stop applying as
        # upstream's own default happens to converge on ours. The comment it
        # inserts is what makes this rerun visible in the diff.
        result = self._apply(
            'already_set.cc',
            'BASE_FEATURE(kFoo, base::FEATURE_DISABLED_BY_DEFAULT);',
            'substitutions:\n'
            '  - description: Ship kFoo disabled.\n'
            '    set_feature_flag_default_state:\n'
            '      feature_name: kFoo\n'
            '      value: base::FEATURE_DISABLED_BY_DEFAULT\n')
        self.assertEqual(
            result, '// kFoo feature state is enforced via plaster rewrite.\n'
            'BASE_FEATURE(kFoo, base::FEATURE_DISABLED_BY_DEFAULT);')

    def test_rejected_on_a_source_outside_every_namespace_it_serves(self):
        # The name belongs to `cxx` alone, and a `.idl` target is not in it.
        with self.assertRaises(ValueError) as cm:
            self._apply(
                'feature.idl', 'irrelevant', 'substitutions:\n'
                '  - description: Ship kFoo disabled.\n'
                '    set_feature_flag_default_state:\n'
                '      feature_name: kFoo\n'
                '      value: base::FEATURE_DISABLED_BY_DEFAULT\n')
        self.assertIn(
            'the `set_feature_flag_default_state` rewriter is not available '
            'for this source, which is not in any namespace it serves '
            '(cxx)', str(cm.exception))

    def _expect_value_error(self, yaml_body: str, substr: str):
        with self.assertRaises(ValueError) as ctx:
            self._apply('validation.cc', 'dummy', yaml_body)
        self.assertIn(substr, str(ctx.exception))


class RegexMacroHelpTest(unittest.TestCase):
    """Unit tests for `_regex_macro_help`, independent of the shipped macro.
    """

    @staticmethod
    def _spec(description: str) -> dict:
        return {
            'description': description,
            'inputs': [
                {
                    'name': 'foo',
                    'description': 'The foo to use.'
                },
                {
                    'name': 'bar',
                    'description': 'The bar to use.'
                },
            ],
        }

    def test_fields_inserted_before_example_code_block(self):
        help_text = plaster._regex_macro_help(
            self._spec('Does a thing.\n\n```cpp\ncode here\n```\n'))
        fields_index = help_text.index('Fields:')
        example_index = help_text.index('```cpp')
        self.assertLess(fields_index, example_index)
        self.assertIn('- `foo` — The foo to use.', help_text)
        self.assertIn('- `bar` — The bar to use.', help_text)

    def test_fields_appended_when_no_code_block(self):
        help_text = plaster._regex_macro_help(self._spec('Does a thing.'))
        self.assertTrue(help_text.startswith('Does a thing.'))
        self.assertIn('Fields:', help_text)
        self.assertIn('- `foo` — The foo to use.', help_text)
        self.assertIn('- `bar` — The bar to use.', help_text)

    def test_field_order_matches_declared_inputs(self):
        help_text = plaster._regex_macro_help(self._spec('Does a thing.'))
        self.assertLess(help_text.index('`foo`'), help_text.index('`bar`'))


class RewriterNamespaceTest(unittest.TestCase):
    """`RewriterNamespace` binds op-id prefixes, target suffixes and grammars.

    It is the one table a new language is added to, so these check both that
    the shipped entries are coherent and that a plaster's target resolves to
    the namespace claiming its suffix.
    """

    # -- the shipped table ---------------------------------------------------

    def test_every_namespace_is_indexed_under_its_own_name(self):
        for name, namespace in plaster._NAMESPACE_BY_NAME.items():
            self.assertEqual(namespace.name, name)

    def test_a_source_namespace_claims_suffixes(self):
        # Everything but the global namespace describes a real kind of
        # source, so it must say which targets it covers. Whether it also
        # names a grammar is a separate question -- see below.
        for name, namespace in plaster._NAMESPACE_BY_NAME.items():
            if name == plaster._GLOBAL_NAMESPACE:
                continue
            self.assertTrue(namespace.suffixes, f'{name} claims no suffixes')

    def test_a_grammar_is_optional_and_independent_of_suffixes(self):
        # A namespace names a grammar only when ast-grep can parse it. A
        # language it cannot parse (GN, say) is still a perfectly good
        # namespace with its own suffixes -- it just hosts text ops only. So
        # claiming suffixes must not be taken to imply having a grammar.
        for name, namespace in plaster._NAMESPACE_BY_NAME.items():
            if namespace.ast_grep_language is None:
                continue
            self.assertTrue(
                namespace.ast_grep_language,
                f'{name} has an empty grammar id; use None to '
                f'mean "ast-grep cannot parse this"')

    def test_the_global_namespace_has_no_grammar_and_no_suffixes(self):
        # `all` describes what a rewriter can be used on, not a kind of
        # source: nothing is in it, and its ops are never parsed.
        namespace = plaster._NAMESPACE_BY_NAME[plaster._GLOBAL_NAMESPACE]
        self.assertIsNone(namespace.ast_grep_language)
        self.assertEqual(namespace.suffixes, frozenset())

    def test_no_suffix_is_claimed_by_two_namespaces(self):
        # `_NAMESPACE_BY_SUFFIX` is built by flattening, so a suffix claimed
        # twice would silently resolve to whichever namespace came last.
        claimed = [
            suffix for namespace in plaster._NAMESPACES
            for suffix in namespace.suffixes
        ]
        self.assertCountEqual(claimed, set(claimed))

    def test_suffix_index_covers_every_declared_suffix(self):
        for namespace in plaster._NAMESPACES:
            for suffix in namespace.suffixes:
                self.assertIs(plaster._NAMESPACE_BY_SUFFIX[suffix], namespace)

    # -- resolving a plaster's target ----------------------------------------

    def test_cxx_targets_resolve_to_the_cxx_namespace(self):
        for name in ('foo.cc.yaml', 'foo.h.yaml', 'foo.mm.yaml',
                     'foo.cpp.yaml'):
            self.assertEqual(
                plaster._namespace_of_source(Path('rewrite/dir') / name),
                'cxx', name)

    def test_unclaimed_suffix_resolves_to_no_namespace(self):
        self.assertIsNone(
            plaster._namespace_of_source(Path('rewrite/foo.idl.yaml')))
        self.assertIsNone(
            plaster._namespace_of_source(Path('rewrite/foo.grd.yaml')))

    def test_only_the_suffix_before_yaml_decides(self):
        # A `.cc` earlier in the name must not make a `.idl` target C++.
        self.assertIsNone(
            plaster._namespace_of_source(Path('rewrite/foo.cc.idl.yaml')))

    def test_is_cxx_source_agrees_with_the_namespace(self):
        self.assertTrue(plaster._is_cxx_source(Path('rewrite/foo.cc.yaml')))
        self.assertFalse(plaster._is_cxx_source(Path('rewrite/foo.idl.yaml')))

    def test_no_target_resolves_to_the_global_namespace(self):
        # `all` is a fallback, never something a target is in, so no suffix
        # may lead to it.
        self.assertNotIn(
            plaster._GLOBAL_NAMESPACE,
            {ns.name
             for ns in plaster._NAMESPACE_BY_SUFFIX.values()})


class RewriterRegistryTest(unittest.TestCase):
    """`RewriterRegistry` drives both YAML dispatch and help.

    The shipped registry (`_REWRITERS`) is checked for the invariants every
    rewriter must hold; the namespace machinery itself is exercised against
    purpose-built registries, so the cases stay stable as real rewriters come
    and go.
    """

    @staticmethod
    def _rewriter(name: str,
                  namespace: str,
                  summary: str = 's') -> type[plaster.Rewriter]:
        """A throwaway rewriter class with the given name and namespace."""
        return type(
            f'{namespace.capitalize()}{name}Rewriter',
            (plaster.AllRegexRewriter, ), {
                'NAME': name,
                'SUMMARY': summary,
                'HELP': 'help',
                'namespace': classmethod(lambda cls, ns=namespace: ns),
            })

    # -- the shipped registry ------------------------------------------------

    def test_regex_is_registered_in_the_global_namespace(self):
        self.assertEqual(plaster.AllRegexRewriter.namespace(),
                         plaster._GLOBAL_NAMESPACE)
        self.assertEqual(dict(plaster._REWRITERS.candidates('regex')),
                         {plaster._GLOBAL_NAMESPACE: plaster.AllRegexRewriter})

    def test_a_global_rewriter_resolves_for_any_target(self):
        for target in ('cxx', None):
            self.assertIs(plaster._REWRITERS.resolve('regex', target),
                          plaster.AllRegexRewriter)

    def test_candidates_are_read_only(self):
        with self.assertRaises(TypeError):
            plaster._REWRITERS.candidates('regex')[
                plaster._GLOBAL_NAMESPACE] = None

    def test_every_rewriter_is_self_describing(self):
        # Each rewriter must be keyed by its own NAME, agree with the
        # namespace it is filed under, and carry the metadata the help system
        # relies on, so a new rewriter can never show up blank or misfiled.
        for name in plaster._REWRITERS.names:
            for namespace, cls in plaster._REWRITERS.candidates(name).items():
                label = f'{name} ({namespace})'
                self.assertEqual(cls.NAME, name)
                self.assertEqual(cls.namespace(), namespace)
                self.assertTrue(cls.SUMMARY, f'{label} is missing a SUMMARY')
                self.assertTrue(cls.help_text(), f'{label} has no help text')

    def test_every_rewriter_names_a_known_namespace(self):
        known = set(plaster._NAMESPACE_BY_NAME)
        for name in plaster._REWRITERS.names:
            for namespace in plaster._REWRITERS.candidates(name):
                self.assertIn(
                    namespace, known,
                    f'{name} is filed under an unregistered namespace')

    # -- resolution ----------------------------------------------------------

    def test_resolve_prefers_an_exact_namespace_match(self):
        cxx = self._rewriter('shared', 'cxx')
        js = self._rewriter('shared', 'js')
        registry = plaster.RewriterRegistry(cxx, js)
        self.assertIs(registry.resolve('shared', 'cxx'), cxx)
        self.assertIs(registry.resolve('shared', 'js'), js)

    def test_resolve_falls_back_to_the_global_namespace(self):
        # A global rewriter reads the target as text, so it fits a target in
        # any namespace -- and one in no namespace at all.
        globally = self._rewriter('anywhere', plaster._GLOBAL_NAMESPACE)
        registry = plaster.RewriterRegistry(globally)
        self.assertIs(registry.resolve('anywhere', 'cxx'), globally)
        self.assertIs(registry.resolve('anywhere', None), globally)

    def test_exact_match_wins_over_the_global_fallback(self):
        cxx = self._rewriter('shared', 'cxx')
        globally = self._rewriter('shared', plaster._GLOBAL_NAMESPACE)
        registry = plaster.RewriterRegistry(cxx, globally)
        self.assertIs(registry.resolve('shared', 'cxx'), cxx)
        self.assertIs(registry.resolve('shared', 'js'), globally)

    def test_resolve_returns_none_for_a_namespace_it_does_not_serve(self):
        registry = plaster.RewriterRegistry(self._rewriter('cxx_only', 'cxx'))
        self.assertIsNone(registry.resolve('cxx_only', 'js'))
        self.assertIsNone(registry.resolve('cxx_only', None))

    def test_same_name_in_one_namespace_is_rejected(self):
        # Two rewriters sharing a name *and* a namespace would silently
        # shadow one another, so building the registry fails outright.
        with self.assertRaises(AssertionError) as ctx:
            plaster.RewriterRegistry(self._rewriter('clash', 'cxx'),
                                     self._rewriter('clash', 'cxx'))
        self.assertIn('registered twice', str(ctx.exception))
        self.assertIn("'cxx'", str(ctx.exception))

    def test_names_are_reported_once_regardless_of_namespace(self):
        registry = plaster.RewriterRegistry(
            self._rewriter('shared', 'cxx'), self._rewriter('shared', 'js'),
            self._rewriter('solo', plaster._GLOBAL_NAMESPACE))
        self.assertEqual(sorted(registry.names), ['shared', 'solo'])
        self.assertIn('shared', registry)
        self.assertNotIn('absent', registry)

    # -- help grouping -------------------------------------------------------

    def test_by_namespace_groups_and_sorts_with_the_global_one_last(self):
        registry = plaster.RewriterRegistry(
            self._rewriter('shared', 'js'), self._rewriter('shared', 'cxx'),
            self._rewriter('alpha', 'cxx'),
            self._rewriter('solo', plaster._GLOBAL_NAMESPACE))
        grouped = [(namespace, [rewriter.NAME for rewriter in rewriters])
                   for namespace, rewriters in registry.by_namespace()]
        self.assertEqual(grouped, [
            ('cxx', ['alpha', 'shared']),
            ('js', ['shared']),
            (plaster._GLOBAL_NAMESPACE, ['solo']),
        ])


class AstGrepCompositionTest(unittest.TestCase):
    """Frontend rewriters compose into `Operation`s fed to the engine.

    These exercise the parse -> `operations()` seam directly (no ast-grep run),
    against the shipped rewriters.pyl so `declared_inputs()` reads real specs.
    """

    def test_declared_inputs_read_from_spec(self):
        # The accepted arg keys come from the op spec, not a duplicated class
        # constant.
        self.assertEqual(plaster.CxxMakeVirtualRewriter.declared_inputs(),
                         frozenset({'class_name', 'method_name'}))
        self.assertEqual(plaster.CxxDropFinalRewriter.declared_inputs(),
                         frozenset({'class_name'}))

    def test_flat_rewriter_expands_to_one_operation(self):
        rewriter = plaster.CxxMakeVirtualRewriter.parse(
            {
                'class_name': 'C',
                'method_name': 'Foo'
            }, description='d')
        self.assertEqual(rewriter.operations(1), [
            plaster.Operation('cxx.make_virtual', {
                'class_name': 'C',
                'method_name': 'Foo'
            })
        ])

    def test_add_friend_single_expands_to_one_operation(self):
        rewriter = plaster.CxxAddFriendRewriter.parse(
            {
                'class_name': 'C',
                'friend_type': 'class BraveC'
            },
            description='d')
        self.assertEqual(rewriter.operations(1), [
            plaster.Operation('cxx.add_friend', {
                'class_name': 'C',
                'friend_type': 'class BraveC'
            })
        ])

    def test_add_friend_list_expands_reversed_to_preserve_order(self):
        # Each insertion goes to the top of the private section, so the ops are
        # emitted in reverse of the authored list to land them in order.
        rewriter = plaster.CxxAddFriendRewriter.parse(
            {
                'class_name': 'C',
                'friend_type': ['class BraveC', 'class BraveCTest'],
            },
            description='d')
        self.assertEqual(rewriter.operations(1), [
            plaster.Operation('cxx.add_friend', {
                'class_name': 'C',
                'friend_type': 'class BraveCTest'
            }),
            plaster.Operation('cxx.add_friend', {
                'class_name': 'C',
                'friend_type': 'class BraveC'
            }),
        ])


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
                    'inputs': ['class_name', 'method_name'],
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
        """The shipped rewriters.pyl validates and exposes its ops."""
        rewriters = plaster.RewritersEval.load()
        self.assertIn('cxx.find_class_method_decl', rewriters.matchers)
        for op in ('cxx.make_virtual', 'cxx.add_friend', 'cxx.drop_final'):
            self.assertIn(op, rewriters.rewriters)

    def test_load_is_a_singleton(self):
        """load() reads the file once and returns the same instance."""
        first = plaster.RewritersEval.load()
        second = plaster.RewritersEval.load()
        self.assertIs(first, second)

    # -- access -------------------------------------------------------------

    def test_accessors_return_specs(self):
        rewriters = self._eval_valid()
        self.assertEqual(
            rewriters.matcher('cxx.find_class_method_decl')['result']['node'],
            'field_declaration')
        self.assertEqual(
            rewriters.rewriter('cxx.make_virtual')['matcher'],
            'cxx.find_class_method_decl')
        self.assertEqual(
            rewriters.rewriter('cxx.make_virtual')['inputs'],
            ['class_name', 'method_name'])

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

    def test_ast_op_in_the_global_namespace_is_rejected(self):
        # `all` is a known namespace, so the op id itself is well-formed --
        # but it names no grammar, and an ast op cannot be parsed without one.
        def mutate(s):
            s['ast.matcher']['all.find_class_method_decl'] = s[
                'ast.matcher'].pop('cxx.find_class_method_decl')

        self._assert_invalid(mutate, 'names no grammar to parse with')

    def test_ast_rewriter_in_the_global_namespace_is_rejected(self):

        def mutate(s):
            s['ast.rewriter']['all.make_virtual'] = s['ast.rewriter'].pop(
                'cxx.make_virtual')

        self._assert_invalid(mutate, 'names no grammar to parse with')

    def test_ast_op_in_an_unparseable_source_namespace_is_rejected(self):
        # The same rejection, for the other reason a namespace can lack a
        # grammar: a real kind of source, with its own suffixes, that
        # ast-grep has no parser for. Text ops there are fine; ast ops are
        # not, and must fail at load rather than at invocation.
        unparseable = plaster.RewriterNamespace(name='gn',
                                                ast_grep_language=None,
                                                suffixes=frozenset(
                                                    {'.gn', '.gni'}))
        namespaces = dict(plaster._NAMESPACE_BY_NAME) | {'gn': unparseable}
        self.addCleanup(setattr, plaster, '_NAMESPACE_BY_NAME',
                        plaster._NAMESPACE_BY_NAME)
        plaster._NAMESPACE_BY_NAME = namespaces

        def mutate(s):
            s['ast.matcher']['gn.find_class_method_decl'] = s[
                'ast.matcher'].pop('cxx.find_class_method_decl')

        self._assert_invalid(mutate, 'names no grammar to parse with')

    # -- matcher schema ------------------------------------------------------

    def test_matcher_missing_required_key(self):
        self._assert_invalid(
            lambda s: s['ast.matcher']['cxx.find_class_method_decl'].pop(
                'template'), 'Missing keys')

    def test_matcher_unknown_key(self):
        self._assert_invalid(
            lambda s: s['ast.matcher']['cxx.find_class_method_decl'].update(
                {'language': 'cpp'}), 'Wrong keys')

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

    def test_rewriter_inputs_not_list_of_strings(self):
        self._assert_invalid(
            lambda s: s['ast.rewriter']['cxx.make_virtual'].__setitem__(
                'inputs', 'class_name'), "should be instance of 'list'")

    def test_rewriter_undeclared_input(self):
        # The templates reference `method_name`, but it is dropped from the
        # declared `inputs`, so the op's interface no longer covers them.
        self._assert_invalid(
            lambda s: s['ast.rewriter']['cxx.make_virtual'].__setitem__(
                'inputs', ['class_name']), 'undeclared input')

    def test_rewriter_unused_input(self):
        self._assert_invalid(
            lambda s: s['ast.rewriter']['cxx.make_virtual']['inputs'].append(
                'unused'), 'never used')

    def test_rewriter_replace_consume_tokens_are_optional(self):
        # `consume_before` / `consume_after` are optional; adding them keeps the
        # spec valid (and they must be strings).
        spec = self._valid_spec()
        spec['ast.rewriter']['cxx.make_virtual']['replace'].update({
            'consume_before': ' ',
            'consume_after': ':',
        })
        rewriters = plaster.RewritersEval(repr(spec))
        replace = rewriters.rewriter('cxx.make_virtual')['replace']
        self.assertEqual(replace['consume_before'], ' ')
        self.assertEqual(replace['consume_after'], ':')

    def test_rewriter_consume_placeholder_must_be_declared(self):
        # A `{placeholder}` used only in a consume token is an input like any
        # other, so it must appear in `inputs`.
        self._assert_invalid(
            lambda s: s['ast.rewriter']['cxx.make_virtual']['replace'].
            __setitem__('consume_before', '{indent}'), 'undeclared input')

    def test_rewriter_consume_placeholder_declared_is_valid(self):
        # Declaring the consume token's placeholder in `inputs` makes it valid.
        spec = self._valid_spec()
        spec['ast.rewriter']['cxx.make_virtual']['replace'][
            'consume_before'] = '{indent}'
        spec['ast.rewriter']['cxx.make_virtual']['inputs'].append('indent')
        rewriters = plaster.RewritersEval(repr(spec))
        self.assertEqual(
            rewriters.rewriter('cxx.make_virtual')['replace']
            ['consume_before'], '{indent}')

    def test_rewriter_first_match_is_optional_bool(self):
        # `first_match` is an optional flag; when present it must be a bool and
        # is exposed on the rewriter spec.
        spec = self._valid_spec()
        spec['ast.rewriter']['cxx.make_virtual']['first_match'] = True
        rewriters = plaster.RewritersEval(repr(spec))
        self.assertIs(
            rewriters.rewriter('cxx.make_virtual')['first_match'], True)

    def test_rewriter_first_match_must_be_bool(self):
        self._assert_invalid(lambda spec: spec['ast.rewriter'][
            'cxx.make_virtual'].update({'first_match': 'yes'}))

    # -- matcher captures ---------------------------------------------------

    # The candidate list `_with_capture` gives a capture by default.
    _RET_CANDIDATES = ({'text': 'RET'}, )

    @staticmethod
    def _with_capture(spec: dict, candidates=_RET_CANDIDATES) -> dict:
        """Bind `$RET` in the matcher template and expose it as a capture."""
        matcher = spec['ast.matcher']['cxx.find_class_method_decl']
        matcher['template'] += 'has:\n  field: type\n  pattern: $RET\n'
        matcher['result']['captures'] = {'return_type': list(candidates)}
        return spec

    def test_rewriter_may_use_matcher_capture(self):
        # A capture is not an input: the rewriter names it in `replace` without
        # declaring it, and the engine fills it per match.
        spec = self._with_capture(self._valid_spec())
        spec['ast.rewriter']['cxx.make_virtual']['replace']['replace'] = (
            'virtual {return_type} ')
        rewriters = plaster.RewritersEval(repr(spec))
        self.assertEqual(
            rewriters.matcher('cxx.find_class_method_decl')['result']
            ['captures']['return_type'], [{
                'text': 'RET'
            }])

    def test_matcher_capture_may_go_unused(self):
        # Matchers are shared, so a capture one rewriter needs is dead weight
        # to another. Unlike an input, that is not an error.
        rewriters = plaster.RewritersEval(
            repr(self._with_capture(self._valid_spec())))
        self.assertIn(
            'return_type',
            rewriters.matcher('cxx.find_class_method_decl')['result']
            ['captures'])

    def test_matcher_capture_reads_unbound_metavariable(self):
        # `$NOPE` appears in no template, so the capture could never resolve.
        self._assert_invalid(
            lambda s: self._with_capture(s, [{
                'text': 'NOPE'
            }]), 'never binds')

    def test_matcher_capture_span_reads_unbound_metavariable(self):
        self._assert_invalid(
            lambda s: self._with_capture(s, [{
                'span': ['RET', 'NOPE']
            }]), 'never binds')

    def test_matcher_capture_span_needs_two_metavariables(self):
        self._assert_invalid(
            lambda s: self._with_capture(s, [{
                'span': ['RET']
            }]))

    def test_matcher_capture_needs_a_candidate(self):
        self._assert_invalid(lambda s: self._with_capture(s, []))

    def test_matcher_capture_candidate_needs_known_kind(self):
        self._assert_invalid(
            lambda s: self._with_capture(s, [{
                'node': 'RET'
            }]))

    def test_matcher_capture_literal_candidate(self):
        # A literal reads no metavariable, so it always resolves -- it is the
        # way to give a capture a fallback for code that binds nothing.
        rewriters = plaster.RewritersEval(
            repr(
                self._with_capture(self._valid_spec(), [{
                    'text': 'RET'
                }, {
                    'literal': 'void'
                }])))
        self.assertEqual(
            rewriters.matcher('cxx.find_class_method_decl')['result']
            ['captures']['return_type'][-1], {'literal': 'void'})

    def test_matcher_capture_literal_must_be_non_empty(self):
        self._assert_invalid(
            lambda s: self._with_capture(s, [{
                'literal': ''
            }]))

    def test_matcher_capture_metavariable_must_be_upper_case(self):
        self._assert_invalid(
            lambda s: self._with_capture(s, [{
                'text': 'ret'
            }]))

    def test_rewriter_input_may_not_shadow_a_capture(self):
        # If both could fill `{return_type}`, which wins would be invisible at
        # the call site.
        def mutate(spec):
            self._with_capture(spec)
            rewriter = spec['ast.rewriter']['cxx.make_virtual']
            rewriter['inputs'].append('return_type')
            rewriter['replace']['replace'] = 'virtual {return_type} '

        self._assert_invalid(mutate, 'shadow')

    # -- optional inputs (`when_set`) ---------------------------------------

    def test_rewriter_when_set_expands_an_optional_input(self):
        spec = self._valid_spec()
        rewriter = spec['ast.rewriter']['cxx.make_virtual']
        rewriter['inputs'].append('result_var')
        rewriter['when_set'] = {'result_var': 'auto {result_var} = '}
        rewriter['replace']['replace'] = '{result_var}virtual '
        rewriters = plaster.RewritersEval(repr(spec))
        self.assertEqual(
            rewriters.rewriter('cxx.make_virtual')['when_set'],
            {'result_var': 'auto {result_var} = '})

    def test_rewriter_when_set_input_must_be_declared(self):
        self._assert_invalid(
            lambda s: s['ast.rewriter']['cxx.make_virtual'].update(
                {'when_set': {
                    'nope': 'x'
                }}), 'undeclared input')

    def test_rewriter_when_set_template_placeholder_must_be_declared(self):
        # A `when_set` template is rendered like any other, so what it reads
        # must be a declared input or a capture.
        def mutate(spec):
            rewriter = spec['ast.rewriter']['cxx.make_virtual']
            rewriter['inputs'].append('result_var')
            rewriter['replace']['replace'] = '{result_var}virtual '
            rewriter['when_set'] = {'result_var': '{undeclared} '}

        self._assert_invalid(mutate, 'undeclared input')

    def test_rewriter_when_set_template_may_read_a_capture(self):

        def mutate(spec):
            self._with_capture(spec)
            rewriter = spec['ast.rewriter']['cxx.make_virtual']
            rewriter['inputs'].append('result_var')
            rewriter['when_set'] = {
                'result_var': '{return_type} {result_var} = '
            }
            rewriter['replace']['replace'] = '{result_var}virtual '
            return spec

        spec = self._valid_spec()
        mutate(spec)
        rewriters = plaster.RewritersEval(repr(spec))
        self.assertIn('result_var',
                      rewriters.rewriter('cxx.make_virtual')['when_set'])


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
                         '  kind: field_declaration_list\n'
                         '  inside:\n'
                         '    kind: class_specifier\n'
                         '    has:\n'
                         '      field: name\n'
                         '      regex: ^{class_name}$\n')

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
            'template': _METHOD_DECL_RULE,
            'result': {
                'node': 'field_declaration'
            },
        },
        'cxx.find_class_private_section': {
            'template': _PRIVATE_SECTION_RULE,
            'result': {
                'node': 'access_specifier'
            },
        },
        'cxx.find_class_final': {
            'template': _FINAL_RULE,
            'result': {
                'node': 'virtual_specifier'
            },
        },
    },
    'ast.rewriter': {
        'cxx.make_virtual': {
            'matcher': 'cxx.find_class_method_decl',
            'inputs': ['class_name', 'method_name'],
            'replace': {
                're_pattern': r'^((?:\[\[.*?\]\]\s*)*)',
                'replace': r'\1virtual '
            },
            'result': {
                'node': 'field_declaration'
            },
        },
        'cxx.add_friend': {
            'matcher': 'cxx.find_class_private_section',
            'inputs': ['class_name', 'friend_type'],
            'first_match': True,
            'replace': {
                'consume_after': ':',
                're_pattern': '$',
                'replace': ':\\n  friend {friend_type};'
            },
            'result': {
                'node': 'access_specifier'
            },
        },
        'cxx.drop_final': {
            'matcher': 'cxx.find_class_final',
            'inputs': ['class_name'],
            'replace': {
                'consume_before': ' ',
                're_pattern': '^final$',
                'replace': ''
            },
            'result': {
                'node': 'virtual_specifier'
            },
        },
    },
}


class CxxMacrosEraserTest(unittest.TestCase):
    """Unit tests for CxxMacrosEraser."""

    # Both passes on by default: most tests below exercise one pass's
    # mechanics in isolation and don't care about the other's gating (that
    # independence gets its own tests further down).
    _BOTH_PASSES = plaster.BlankForParseOptions(macros=True,
                                                string_adjacent_macros=True)

    def _prepared(
            self,
            source: str,
            blank_for_parse: plaster.BlankForParseOptions = _BOTH_PASSES
    ) -> str:
        """Return the erased source, asserting length is preserved."""
        result = plaster.CxxMacrosEraser(blank_for_parse).erase(source)
        # The whole point is that offsets are preserved for byte-for-byte
        # remapping onto the untouched source.
        self.assertEqual(len(result.encode('utf-8')),
                         len(source.encode('utf-8')))
        return result

    def _assert_blanks(self, source: str, macro: str):
        """Assert `macro` is replaced by equal-length spaces, nothing else."""
        self.assertEqual(self._prepared(source),
                         source.replace(macro, ' ' * len(macro), 1))

    # -- export macros ----------------------------------------------------

    def test_blanks_simple_export_macro(self):
        self._assert_blanks('class MODULES_EXPORT Foo final {};',
                            'MODULES_EXPORT')

    def test_blanks_parenthesised_export_macro(self):
        self._assert_blanks('class COMPONENT_EXPORT(BASE) Foo {};',
                            'COMPONENT_EXPORT(BASE)')

    def test_blanks_struct_and_multiline_head(self):
        self._assert_blanks('struct NET_EXPORT\n    Foo {};', 'NET_EXPORT')

    def test_leaves_plain_class_untouched(self):
        for src in ('class Foo final {};', 'class Foo : public Base {};',
                    'class FooBar {};'):
            self.assertEqual(self._prepared(src), src)

    def test_leaves_all_caps_class_name_untouched(self):
        # An all-caps name without an `_EXPORT` suffix is not a macro.
        self.assertEqual(self._prepared('class URL final {};'),
                         'class URL final {};')

    def test_only_touches_class_head_macro(self):
        # An `_EXPORT`-suffixed token elsewhere (a member, a value) is left be.
        result = self._prepared(
            'class MODULES_EXPORT Foo {\n  int MY_EXPORT = 1;\n};')
        self.assertIn('int MY_EXPORT = 1;', result)
        self.assertNotIn('MODULES_EXPORT', result)

    # -- preprocessor conditionals ----------------------------------------

    def test_blanks_conditionals_anywhere(self):
        # Directives are blanked wherever they sit -- base list or class body --
        # while the code they guarded stays put.
        result = self._prepared('class C : public A\n'
                                '#if defined(USE_AURA)\n'
                                '    ,\n'
                                '         public D\n'
                                '#endif  // defined(USE_AURA)\n'
                                '{\n'
                                ' public:\n'
                                '#ifdef FOO\n'
                                '  void OnFoo();\n'
                                '#else\n'
                                '  void OnBar();\n'
                                '#endif\n'
                                '};\n')
        for directive in ('#if', '#ifdef', '#else', '#endif'):
            self.assertNotIn(directive, result)
        # Guarded code survives.
        for kept in ('public D', 'void OnFoo();', 'void OnBar();'):
            self.assertIn(kept, result)

    def test_blanks_every_directive_kind(self):
        for directive in ('#if X', '#ifdef X', '#ifndef X', '#elif X', '#else',
                          '#endif'):
            self.assertEqual(self._prepared(f'a\n{directive}\nb\n'),
                             f'a\n{" " * len(directive)}\nb\n')

    def test_leaves_non_conditional_directives_untouched(self):
        # `#include` / `#define` are not conditionals and must be preserved.
        for src in ('#include <memory>\n', '#define FOO 1\n',
                    '#pragma once\n'):
            self.assertEqual(self._prepared(src), src)

    # -- macros adjacent to string literals --------------------------------
    #
    # A bare identifier (optionally called) touching a string literal, with
    # only whitespace between them, has no raw C++ grammar: it is only valid
    # once a macro that expands to (or stringizes into) a string literal has
    # run. Left alone, tree-sitter drops the expression into an error node
    # that can swallow everything up to the next construct it can resync on.

    def test_blanks_macro_after_string_literal(self):
        self._assert_blanks('std::string("Skia/" STRINGIZE(SK_MILESTONE));',
                            'STRINGIZE(SK_MILESTONE)')

    def test_blanks_bare_macro_after_string_literal(self):
        self._assert_blanks('std::string("Skia/" SKIA_COMMIT_HASH);',
                            'SKIA_COMMIT_HASH')

    def test_blanks_macro_before_string_literal(self):
        self._assert_blanks('std::string(SKIA_COMMIT_HASH " built");',
                            'SKIA_COMMIT_HASH')

    def test_blanks_macro_chain_between_string_literals(self):
        # The real construct that triggered this: a `STRINGIZE(...)` call and
        # a bare macro, each adjacent to a string literal on at least one
        # side, chained together.
        result = self._prepared(
            'std::string("Skia/" STRINGIZE(SK_MILESTONE) " " '
            'SKIA_COMMIT_HASH);')
        self.assertNotIn('STRINGIZE', result)
        self.assertNotIn('SKIA_COMMIT_HASH', result)
        for kept in ('"Skia/"', '" "'):
            self.assertIn(kept, result)

    def test_leaves_plain_string_concatenation_untouched(self):
        # Two string literals with only whitespace between them is valid,
        # unrelated C++ (adjacent string literal concatenation).
        self.assertEqual(self._prepared('"foo" "bar"'), '"foo" "bar"')

    def test_leaves_string_with_operator_untouched(self):
        # An operator between the string and the identifier makes this
        # ordinary, already-parseable C++; nothing to blank.
        for src in ('"foo" + bar', '"foo" == bar', 'foo + "bar"'):
            self.assertEqual(self._prepared(src), src)

    def test_leaves_user_defined_literal_suffix_untouched(self):
        # No whitespace: a real user-defined literal suffix, not this
        # construct.
        self.assertEqual(self._prepared('"foo"s'), '"foo"s')

    def test_leaves_string_literal_prefix_untouched(self):
        # No whitespace: a real encoding prefix, not this construct.
        for src in ('u8"foo"', 'L"foo"', 'u"foo"', 'U"foo"'):
            self.assertEqual(self._prepared(src), src)

    def test_macro_after_string_handles_escaped_quote(self):
        self._assert_blanks(r'std::string("a\"b" FOO);', 'FOO')

    def test_leaves_encoded_prefix_adjacent_to_macro_untouched(self):
        # `u8"foo"`/`L"foo"`/etc. are excluded from `_CXX_STRING_LIT`
        # entirely (see its comment), so a macro next to one is left alone
        # rather than risk misreading the prefixed form.
        for src in ('std::string(u8"Skia/" FOO);', 'std::string(FOO L"x");'):
            self.assertEqual(self._prepared(src), src)

    def test_leaves_preprocessor_directive_with_string_untouched(self):
        # `#define FOO "bar"` and `#include "foo.h"` have the same *shape*
        # as the macro-adjacent-string construct (identifier/token then
        # whitespace then a string literal), but they are directive syntax,
        # not an expression -- the name/path must survive intact.
        for src in ('#define FOO "bar"\n', '#include "foo.h"\n',
                    '#define VERSION_STRING "v" MY_STRINGIZE(X)\n'):
            self.assertEqual(self._prepared(src), src)

    def test_directive_with_string_inside_conditional_still_protected(self):
        # The conditional lines around it are blanked as usual, but the
        # `#define` line's own content survives untouched.
        src = '#if X\n#define FOO "bar"\n#endif\n'
        result = self._prepared(src)
        self.assertIn('#define FOO "bar"', result)
        for directive in ('#if X', '#endif'):
            self.assertNotIn(directive, result)

    def test_leaves_raw_string_with_embedded_quote_untouched(self):
        # A raw string's content may contain unescaped `"` characters
        # (`some "quoted" text` below); naively pairing quotes would misread
        # `"quoted"` as a standalone string literal sandwiched between two
        # bare words, and blank them as if they were macros.
        src = 'Log(R"foo(some "quoted" text)foo" BAR);\n'
        self.assertEqual(self._prepared(src), src)

    def test_leaves_raw_string_adjacent_to_macro_untouched(self):
        # A raw string with no embedded quote is safe to reason about, but is
        # still excluded wholesale (rather than only when it has embedded
        # quotes) to keep the rule simple and uniformly safe.
        for src in ('Log(R"(hello)" FOO);\n', 'Log(FOO R"(hello)");\n'):
            self.assertEqual(self._prepared(src), src)

    def test_raw_string_delimiter_must_match_on_both_sides(self):
        # `)foo"` inside the content of a `R"bar(...)bar"` literal must not
        # be mistaken for that literal's own close.
        src = 'Log(R"bar(text with )foo" inside)bar" BAZ);\n'
        self.assertEqual(self._prepared(src), src)

    # -- Views METADATA_HEADER / BEGIN_METADATA / END_METADATA ------------
    #
    # Both are bare macro calls with no trailing `;`, sitting where only a
    # declaration is valid (a class body, or namespace scope right after the
    # class). Left alone, tree-sitter turns the call -- and, for
    # BEGIN_METADATA, everything up to end of file -- into one ERROR node.

    _METADATA_OPTS = plaster.BlankForParseOptions(metadata_header_macros=True)

    def test_blanks_metadata_header(self):
        result = self._prepared(
            'class Foo {\n  METADATA_HEADER(Foo, views::View)\n};\n',
            self._METADATA_OPTS)
        self.assertNotIn('METADATA_HEADER', result)
        for kept in ('Foo', 'views::View'):
            self.assertIn(kept, result)

    def test_blanks_metadata_header_single_arg(self):
        result = self._prepared('class Foo {\n  METADATA_HEADER(Foo)\n};\n',
                                self._METADATA_OPTS)
        self.assertNotIn('METADATA_HEADER', result)
        self.assertIn('Foo', result)

    def test_blanks_begin_metadata_block(self):
        result = self._prepared(
            'BEGIN_METADATA(Foo, views::View)\nEND_METADATA\n',
            self._METADATA_OPTS)
        self.assertNotIn('BEGIN_METADATA', result)
        self.assertNotIn('END_METADATA', result)
        for kept in ('Foo', 'views::View'):
            self.assertIn(kept, result)

    def test_blanks_begin_metadata_single_arg(self):
        result = self._prepared('BEGIN_METADATA(Foo)\nEND_METADATA\n',
                                self._METADATA_OPTS)
        self.assertNotIn('BEGIN_METADATA', result)
        self.assertNotIn('END_METADATA', result)
        self.assertIn('Foo', result)

    def test_blanks_property_macros_between_begin_and_end_metadata(self):
        # Property-registration calls in between are blanked wholesale --
        # nothing else in plaster ever needs to match them.
        result = self._prepared(
            'BEGIN_METADATA(Foo, views::View)\n'
            'ADD_PROPERTY_METADATA(int, SomeProp)\n'
            'END_METADATA\n', self._METADATA_OPTS)
        self.assertNotIn('ADD_PROPERTY_METADATA', result)
        self.assertNotIn('SomeProp', result)

    def test_metadata_header_name_and_base_keep_their_byte_offsets(self):
        # The whole point of this pass: a later op (e.g. rename_class) matches
        # against the blanked copy but edits the real source at the same byte
        # offsets, so `Foo`/`views::View` must land at the same position in
        # both, not merely leave the overall text the same length.
        src = 'class C {\n  METADATA_HEADER(Foo, views::View)\n};\n'
        result = self._prepared(src, self._METADATA_OPTS)
        self.assertEqual(result.index('Foo'), src.index('Foo'))
        self.assertEqual(result.index('views::View'), src.index('views::View'))

    def test_metadata_header_macros_off_by_default(self):
        src = 'class Foo {\n  METADATA_HEADER(Foo, views::View)\n};\n'
        self.assertEqual(self._prepared(src, plaster.BlankForParseOptions()),
                         src)

    def test_metadata_header_macros_not_enabled_by_the_other_two_flags(self):
        src = 'BEGIN_METADATA(Foo, views::View)\nEND_METADATA\n'
        result = self._prepared(
            src,
            plaster.BlankForParseOptions(macros=True,
                                         string_adjacent_macros=True))
        self.assertEqual(result, src)

    def test_metadata_header_macros_does_not_enable_the_other_two_passes(self):
        src = ('class MODULES_EXPORT Foo final {\n'
               '#if X\n'
               '  void Bar() { Log("v" STRINGIZE(V)); }\n'
               '#endif\n'
               '};\n')
        self.assertEqual(self._prepared(src, self._METADATA_OPTS), src)

    # -- the two passes are independently gated ----------------------------
    #
    # `blank_macros_for_ast_parsing` and
    # `blank_string_adjacent_macros_for_ast_parsing` are separate opt-ins:
    # each pass only runs when its own flag is set, regardless of the other.

    def test_string_adjacent_macros_alone_does_not_blank_export_macro(self):
        src = 'class MODULES_EXPORT Foo final {};'
        result = self._prepared(
            src,
            plaster.BlankForParseOptions(macros=False,
                                         string_adjacent_macros=True))
        self.assertEqual(result, src)

    def test_string_adjacent_macros_alone_does_not_blank_conditional(self):
        src = 'a\n#if X\nb\n#endif\n'
        result = self._prepared(
            src,
            plaster.BlankForParseOptions(macros=False,
                                         string_adjacent_macros=True))
        self.assertEqual(result, src)

    def test_macros_alone_does_not_blank_macro_after_string_literal(self):
        src = 'std::string("Skia/" STRINGIZE(SK_MILESTONE));'
        result = self._prepared(
            src,
            plaster.BlankForParseOptions(macros=True,
                                         string_adjacent_macros=False))
        self.assertEqual(result, src)

    def test_neither_flag_blanks_anything(self):
        src = ('class MODULES_EXPORT Foo final {\n'
               '#if X\n'
               '  void Bar() { Log("v" STRINGIZE(V)); }\n'
               '#endif\n'
               '};\n')
        self.assertEqual(self._prepared(src, plaster.BlankForParseOptions()),
                         src)

    def test_both_flags_blank_both_constructs(self):
        result = self._prepared(
            'class MODULES_EXPORT Foo final {\n'
            '  void Bar() { Log("v" STRINGIZE(V)); }\n'
            '};\n',
            plaster.BlankForParseOptions(macros=True,
                                         string_adjacent_macros=True))
        self.assertNotIn('MODULES_EXPORT', result)
        self.assertNotIn('STRINGIZE', result)

    # -- construction / identity -------------------------------------------

    def test_regexes_are_shared_across_instances(self):
        # Class-level: compiled once, not per `CxxMacrosEraser()` call.
        a = plaster.CxxMacrosEraser(plaster.BlankForParseOptions())
        b = plaster.CxxMacrosEraser(plaster.BlankForParseOptions())
        self.assertIs(a._EXPORT_MACRO_RE, b._EXPORT_MACRO_RE)
        self.assertIs(a._CXX_MACRO_AFTER_STRING_RE,
                      b._CXX_MACRO_AFTER_STRING_RE)

    def test_two_instances_do_not_share_options(self):
        macros_only = plaster.CxxMacrosEraser(
            plaster.BlankForParseOptions(macros=True))
        string_adjacent_only = plaster.CxxMacrosEraser(
            plaster.BlankForParseOptions(string_adjacent_macros=True))
        src = 'class MODULES_EXPORT C { void F() { Log("v" FOO); } };'
        self.assertNotIn('MODULES_EXPORT', macros_only.erase(src))
        self.assertIn('FOO', macros_only.erase(src))
        self.assertIn('MODULES_EXPORT', string_adjacent_only.erase(src))
        self.assertNotIn('FOO', string_adjacent_only.erase(src))

    def test_erase_is_repeatable_on_the_same_instance(self):
        eraser = plaster.CxxMacrosEraser(
            plaster.BlankForParseOptions(macros=True))
        src = 'class MODULES_EXPORT C {};'
        self.assertEqual(eraser.erase(src), eraser.erase(src))

    def test_opaque_span_is_frozen(self):
        span = plaster.CxxMacrosEraser._OpaqueSpan(0, 3, 'abc')
        self.assertEqual((span.start, span.end, span.text), (0, 3, 'abc'))
        with self.assertRaises(dataclasses.FrozenInstanceError):
            span.start = 1


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

    # -- metavariables ------------------------------------------------------

    # Binds `$TYPE` and `$NAME` always, and `$QUAL` only when the declaration
    # leads with a qualifier -- the shape a capture's candidate list relies on.
    _TYPED_METHOD_RULE = ('kind: field_declaration\n'
                          'all:\n'
                          '  - has:\n'
                          '      field: type\n'
                          '      pattern: $TYPE\n'
                          '  - has:\n'
                          '      kind: function_declarator\n'
                          '      stopBy: end\n'
                          '      has:\n'
                          '        field: declarator\n'
                          '        pattern: $NAME\n'
                          '  - any:\n'
                          '      - has:\n'
                          '          kind: type_qualifier\n'
                          '          pattern: $QUAL\n'
                          '      - not:\n'
                          '          has:\n'
                          '            kind: type_qualifier\n')

    _TYPED_SRC = 'class C {\n  const int* Foo();\n  void Bar();\n};\n'

    def _typed_matches(self) -> list[plaster.AstMatch]:
        return plaster.run_ast_grep(language='cpp',
                                    rule_body=self._TYPED_METHOD_RULE,
                                    source=self._TYPED_SRC)

    def test_exposes_metavariable_ranges(self):
        raw = self._TYPED_SRC.encode('utf-8')
        qualified = self._typed_matches()[0]
        text = {
            name: raw[start:end].decode()
            for name, (start, end) in qualified.metavars.items()
        }
        self.assertEqual(text, {'QUAL': 'const', 'TYPE': 'int', 'NAME': 'Foo'})
        # The span between two metavariables recovers what no single node
        # holds: the `*` sits on the declarator, the `const` before the type.
        start = qualified.metavars['QUAL'][0]
        self.assertEqual(raw[start:qualified.metavars['NAME'][0]].decode(),
                         'const int* ')

    def test_omits_unbound_metavariables(self):
        # `Bar` has no leading qualifier, so `$QUAL`'s `any:` branch never
        # matched and the metavariable is simply absent.
        plain = self._typed_matches()[1]
        self.assertEqual(sorted(plain.metavars), ['NAME', 'TYPE'])

    def test_match_without_metavariables_has_none(self):
        self.assertEqual(self._find('Foo', self._SRC)[0].metavars, {})


class AstRewriterTest(unittest.TestCase):
    """Integration tests for plaster.AstRewriter (real ast-grep binary).

    Driven with a synthetic RewritersEval built from `_SYNTHETIC_SPEC`, so the
    engine is exercised in isolation from whatever the shipped rewriters.pyl
    currently carries. Each op is invoked through a bound `Operation`; the
    consume tokens now live in the spec, not in the call.
    """

    _SRC = 'class C {\n  void Foo();\n  void Bar();\n};\n'

    def _rewriter(self, source: str = _SRC) -> plaster.AstRewriter:
        return plaster.AstRewriter(
            plaster.RewritersEval(repr(_SYNTHETIC_SPEC)), source)

    def test_make_virtual_single(self):
        rewriter = self._rewriter()
        count = rewriter.run(
            plaster.Operation('cxx.make_virtual', {
                'class_name': 'C',
                'method_name': 'Foo'
            }))
        self.assertEqual(count, 1)
        self.assertEqual(
            rewriter.content,
            'class C {\n  virtual void Foo();\n  void Bar();\n};\n')

    def test_make_virtual_destructor(self):
        # Destructors parse as `declaration` with a `destructor_name`, not the
        # `field_declaration`/`field_identifier` of a regular method.
        rewriter = self._rewriter('class C {\n public:\n  ~C();\n};\n')
        count = rewriter.run(
            plaster.Operation('cxx.make_virtual', {
                'class_name': 'C',
                'method_name': '~C'
            }))
        self.assertEqual(count, 1)
        self.assertEqual(rewriter.content,
                         'class C {\n public:\n  virtual ~C();\n};\n')

    def test_make_virtual_overloads_count_each(self):
        rewriter = self._rewriter(
            'class C {\n  void Foo();\n  void Foo(int x);\n};\n')
        count = rewriter.run(
            plaster.Operation('cxx.make_virtual', {
                'class_name': 'C',
                'method_name': 'Foo'
            }))
        self.assertEqual(count, 2)
        # Splicing from the end keeps the earlier overload's offset valid.
        self.assertEqual(
            rewriter.content, 'class C {\n  virtual void Foo();\n'
            '  virtual void Foo(int x);\n};\n')

    def test_no_match_leaves_content_unchanged(self):
        rewriter = self._rewriter()
        self.assertEqual(
            rewriter.run(
                plaster.Operation('cxx.make_virtual', {
                    'class_name': 'C',
                    'method_name': 'Nope'
                })), 0)
        self.assertEqual(rewriter.content, self._SRC)

    def test_content_accumulates_across_calls(self):
        rewriter = self._rewriter()
        rewriter.run(
            plaster.Operation('cxx.make_virtual', {
                'class_name': 'C',
                'method_name': 'Foo'
            }))
        rewriter.run(
            plaster.Operation('cxx.make_virtual', {
                'class_name': 'C',
                'method_name': 'Bar'
            }))
        self.assertEqual(
            rewriter.content,
            'class C {\n  virtual void Foo();\n  virtual void Bar();\n};\n')

    def test_add_friend_inserts_after_private_colon(self):
        rewriter = self._rewriter(
            'class C {\n public:\n  void Foo();\n private:\n  int x_;\n};\n')
        count = rewriter.run(
            plaster.Operation('cxx.add_friend', {
                'class_name': 'C',
                'friend_type': 'class BraveC'
            }))
        self.assertEqual(count, 1)
        # The friend lands as the first private line; the `:` is not duplicated.
        self.assertEqual(
            rewriter.content, 'class C {\n public:\n  void Foo();\n'
            ' private:\n  friend class BraveC;\n  int x_;\n};\n')

    def test_add_friend_no_private_section(self):
        rewriter = self._rewriter('class C {\n public:\n  void Foo();\n};\n')
        self.assertEqual(
            rewriter.run(
                plaster.Operation('cxx.add_friend', {
                    'class_name': 'C',
                    'friend_type': 'class BraveC'
                })), 0)
        self.assertEqual(rewriter.content,
                         'class C {\n public:\n  void Foo();\n};\n')

    def test_drop_final_with_base(self):
        # The class `final` is dropped (and the space before it); a method's
        # trailing `final` is left untouched.
        rewriter = self._rewriter(
            'class C final : public Base {\n  void f() final;\n};\n')
        self.assertEqual(
            rewriter.run(
                plaster.Operation('cxx.drop_final', {'class_name': 'C'})), 1)
        self.assertEqual(rewriter.content,
                         'class C : public Base {\n  void f() final;\n};\n')

    def test_drop_final_no_base(self):
        rewriter = self._rewriter('class C final {\n};\n')
        self.assertEqual(
            rewriter.run(
                plaster.Operation('cxx.drop_final', {'class_name': 'C'})), 1)
        self.assertEqual(rewriter.content, 'class C {\n};\n')

    def test_drop_final_absent(self):
        rewriter = self._rewriter('class C {\n};\n')
        self.assertEqual(
            rewriter.run(
                plaster.Operation('cxx.drop_final', {'class_name': 'C'})), 0)
        self.assertEqual(rewriter.content, 'class C {\n};\n')


class AstCaptureTest(unittest.TestCase):
    """Capture resolution and optional inputs (real ast-grep binary).

    Exercises the engine mechanics in isolation from the shipped ops: a matcher
    that binds three metavariables, a capture whose candidates fall back
    through them, and a rewriter with an optional input.
    """

    # `$NAME` always binds; `$TYPE` only when there is a return type (a
    # constructor has none) and `$QUAL` only on a qualified declaration.
    # Constructors parse as `declaration`, regular methods as
    # `field_declaration`.
    _RULE = ('any:\n'
             '  - kind: field_declaration\n'
             '  - kind: declaration\n'
             'all:\n'
             '  - has:\n'
             '      kind: function_declarator\n'
             '      stopBy: end\n'
             '      has:\n'
             '        field: declarator\n'
             '        pattern: $NAME\n'
             '        regex: ^{method_name}$\n'
             '  - any:\n'
             '      - has:\n'
             '          field: type\n'
             '          pattern: $TYPE\n'
             '      - not:\n'
             '          has:\n'
             '            field: type\n'
             '            pattern: $_\n'
             '  - any:\n'
             '      - has:\n'
             '          kind: type_qualifier\n'
             '          pattern: $QUAL\n'
             '      - not:\n'
             '          has:\n'
             '            kind: type_qualifier\n')

    _SPEC = {
        'ast.matcher': {
            'cxx.find_typed_method': {
                'template': _RULE,
                'result': {
                    'node': 'field_declaration',
                    'captures': {
                        'return_type': [
                            {
                                'span': ['QUAL', 'NAME']
                            },
                            {
                                'span': ['TYPE', 'NAME']
                            },
                        ],
                    },
                },
            },
        },
        'ast.rewriter': {
            'cxx.annotate': {
                'matcher': 'cxx.find_typed_method',
                'inputs': ['method_name', 'note'],
                'replace': {
                    're_pattern': '$',
                    'replace': '  // {note}: {return_type}',
                },
                'result': {
                    'node': 'field_declaration'
                },
            },
            'cxx.annotate_optional': {
                'matcher': 'cxx.find_typed_method',
                'inputs': ['method_name', 'prefix'],
                'when_set': {
                    'prefix': '{prefix} {return_type} -- ',
                },
                'replace': {
                    're_pattern': '$',
                    'replace': '  // {prefix}done',
                },
                'result': {
                    'node': 'field_declaration'
                },
            },
        },
    }

    def _run(self, source: str, op: plaster.Operation) -> str:
        rewriter = plaster.AstRewriter(plaster.RewritersEval(repr(self._SPEC)),
                                       source)
        rewriter.run(op)
        return rewriter.content

    def _annotate(self, source: str, method_name: str = 'Foo') -> str:
        return self._run(
            source,
            plaster.Operation('cxx.annotate', {
                'method_name': method_name,
                'note': 'type'
            }))

    def test_first_candidate_wins(self):
        # `$QUAL` binds, so the first span candidate resolves and carries the
        # qualifier and the `*` that flank the `type` field.
        result = self._annotate('class C {\n  const int* Foo();\n};\n')
        self.assertIn('// type: const int*', result)

    def test_falls_back_to_later_candidate(self):
        # No qualifier, so the first candidate cannot resolve and the second
        # supplies the value.
        result = self._annotate('class C {\n  int Foo();\n};\n')
        self.assertIn('// type: int', result)

    def test_collapses_whitespace_in_a_span(self):
        result = self._annotate('class C {\n  const std::map<int,\n'
                                '      std::string>&\n      Foo();\n};\n')
        self.assertIn('// type: const std::map<int, std::string>&', result)

    def test_reversed_span_raises(self):
        # Both metavariables bind, but the candidate names them in source order
        # `NAME`..`TYPE` -- backwards. Falling through to the next candidate
        # would hide a spec bug behind a value derived some other way, so this
        # is reported instead.
        spec = copy.deepcopy(self._SPEC)
        spec['ast.matcher']['cxx.find_typed_method']['result']['captures'] = {
            'return_type': [{
                'span': ['NAME', 'TYPE']
            }, {
                'literal': 'void'
            }],
        }
        rewriter = plaster.AstRewriter(plaster.RewritersEval(repr(spec)),
                                       'class C {\n  int Foo();\n};\n')
        with self.assertRaises(plaster.AstCaptureError) as ctx:
            rewriter.run(
                plaster.Operation('cxx.annotate', {
                    'method_name': 'Foo',
                    'note': 'type'
                }))
        self.assertIn('spans $NAME to $TYPE', str(ctx.exception))
        self.assertIn('wrong order', str(ctx.exception))

    def test_unresolvable_capture_raises(self):
        # A constructor binds neither `$QUAL` nor `$TYPE`, so no candidate
        # applies and the engine refuses rather than splicing an empty type.
        with self.assertRaises(plaster.AstCaptureError) as ctx:
            self._annotate('class C {\n  C();\n};\n', method_name='C')
        message = str(ctx.exception)
        self.assertIn('cxx.annotate', message)
        self.assertIn('cannot resolve `return_type`', message)
        # The diagnostic locates the match and reports what did bind.
        self.assertIn('line 2', message)
        self.assertIn('$NAME', message)

    def test_capture_is_only_resolved_when_a_template_asks(self):
        # `cxx.annotate_optional` names `return_type` only inside `when_set`,
        # so with the optional input unset the constructor rewrite succeeds.
        result = self._run(
            'class C {\n  C();\n};\n',
            plaster.Operation('cxx.annotate_optional', {
                'method_name': 'C',
                'prefix': ''
            }))
        self.assertEqual(result, 'class C {\n  C();  // done\n};\n')

    def test_optional_input_expands_when_set(self):
        result = self._run(
            'class C {\n  const int* Foo();\n};\n',
            plaster.Operation('cxx.annotate_optional', {
                'method_name': 'Foo',
                'prefix': 'returns'
            }))
        self.assertIn('// returns const int* -- done', result)

    def test_optional_input_renders_empty_when_unset(self):
        result = self._run(
            'class C {\n  const int* Foo();\n};\n',
            plaster.Operation('cxx.annotate_optional', {
                'method_name': 'Foo',
                'prefix': ''
            }))
        self.assertIn('// done', result)
        self.assertNotIn('--', result)


class _FlatAstGrepRewriter(plaster._AstGrepRewriter):
    """Minimal `_AstGrepRewriter` subclass: inherits the base parse/operations.

    Bound to `cxx.make_virtual` purely so the base's default 1:1 behaviour has a
    real op to resolve against; it adds nothing of its own.
    """

    NAME = 'flat_test_op'
    OP_ID = 'cxx.make_virtual'


class _ComposingAstGrepRewriter(plaster._AstGrepRewriter):
    """`_AstGrepRewriter` subclass that expands one body into several ops.

    Exists only to prove the base's `apply` drives and accumulates across an
    arbitrary `operations()` list -- the composition seam itself, with no
    concrete rewriter (CxxMakeVirtualRewriter/CxxAddFriendRewriter/CxxDropFinalRewriter) in the picture.
    """

    NAME = 'composing_test_op'
    OP_ID = 'cxx.make_virtual'

    def __init__(self, class_name: str, method_names: list[str]):
        super().__init__()
        self._class_name = class_name
        self._method_names = method_names

    def operations(self, count: int) -> list[plaster.Operation]:
        del count  # Each method is its own exactly-once operation.
        return [
            plaster.Operation('cxx.make_virtual', {
                'class_name': self._class_name,
                'method_name': method_name,
            }) for method_name in self._method_names
        ]


class _OptionalPairAstGrepRewriter(plaster._AstGrepRewriter):
    """Two optional ops plus a group rule: at least one must apply.

    Mirrors the shape of a hypothetical `make_class_overridable` (drop `final`
    or override the dtor -- either may be absent, but not both), to exercise
    per-op optionality and a cross-operation check via `validate_outcomes`.
    """

    NAME = 'optional_pair_test_op'
    OP_ID = 'cxx.make_virtual'

    def __init__(self, method_names: list[str]):
        super().__init__()
        self._method_names = method_names

    def operations(self, count: int) -> list[plaster.Operation]:
        del count
        return [
            plaster.Operation('cxx.make_virtual', {
                'class_name': 'C',
                'method_name': method_name,
            }, plaster.MatchExpectation.optional())
            for method_name in self._method_names
        ]

    def validate_outcomes(self, outcomes, description):
        errors = super().validate_outcomes(outcomes, description)
        if outcomes and all(matches == 0 for _, matches in outcomes):
            errors.append('at least one operation must apply')
        return errors


class AstGrepRewriterBaseTest(unittest.TestCase):
    """Unit tests for the `_AstGrepRewriter` base class on its own.

    The base is exercised through the two synthetic subclasses above, with a
    `RewritersEval` built from `_SYNTHETIC_SPEC` injected as the process
    singleton so `apply`/`declared_inputs` resolve against it instead of the
    shipped rewriters.pyl. Nothing here touches the concrete rewriters.
    """

    _SRC = 'class C {\n  void Foo();\n  void Bar();\n};\n'

    def setUp(self):
        plaster.RewritersEval._instance = plaster.RewritersEval(
            repr(_SYNTHETIC_SPEC))
        self.addCleanup(setattr, plaster.RewritersEval, '_instance', None)

    # -- declared_inputs (reads the spec, not a class constant) -------------

    def test_declared_inputs_read_from_injected_spec(self):
        self.assertEqual(_FlatAstGrepRewriter.declared_inputs(),
                         frozenset({'class_name', 'method_name'}))

    # -- default parse (flat body validation) -------------------------------

    def test_parse_builds_from_declared_inputs(self):
        rewriter = _FlatAstGrepRewriter.parse(
            {
                'class_name': 'C',
                'method_name': 'Foo'
            }, description='d')
        self.assertIsInstance(rewriter, _FlatAstGrepRewriter)

    def test_parse_rejects_non_mapping_body(self):
        with self.assertRaises(ValueError) as ctx:
            _FlatAstGrepRewriter.parse('nope', description='d')
        self.assertIn('must be a mapping', str(ctx.exception))

    def test_parse_rejects_unknown_arg(self):
        with self.assertRaises(ValueError) as ctx:
            _FlatAstGrepRewriter.parse(
                {
                    'class_name': 'C',
                    'method_name': 'Foo',
                    'bogus': 'x'
                },
                description='d')
        self.assertIn('Unrecognised flat_test_op arg', str(ctx.exception))

    def test_parse_rejects_missing_arg(self):
        with self.assertRaises(ValueError) as ctx:
            _FlatAstGrepRewriter.parse({'class_name': 'C'}, description='d')
        message = str(ctx.exception)
        self.assertIn('flat_test_op requires arg', message)
        self.assertIn('method_name', message)

    def test_parse_rejects_non_string_arg(self):
        with self.assertRaises(ValueError) as ctx:
            _FlatAstGrepRewriter.parse({
                'class_name': 'C',
                'method_name': 5
            },
                                       description='d')
        self.assertIn('`method_name` must be a string', str(ctx.exception))

    # -- default operations -------------------------------------------------

    def test_default_operations_is_a_single_operation(self):
        rewriter = _FlatAstGrepRewriter.parse(
            {
                'class_name': 'C',
                'method_name': 'Foo'
            }, description='d')
        self.assertEqual(rewriter.operations(1), [
            plaster.Operation('cxx.make_virtual', {
                'class_name': 'C',
                'method_name': 'Foo'
            })
        ])

    def test_default_operation_adopts_entry_count(self):
        # The flat single operation takes the entry's `count:` as its
        # expectation, preserving plaster's original count semantics.
        rewriter = _FlatAstGrepRewriter.parse(
            {
                'class_name': 'C',
                'method_name': 'Foo'
            }, description='d')
        self.assertEqual(
            rewriter.operations(2)[0].expectation,
            plaster.MatchExpectation.exactly(2))
        self.assertEqual(
            rewriter.operations(0)[0].expectation,
            plaster.MatchExpectation.at_least_one())

    # -- apply: drives the engine, then validates per operation -------------

    def test_apply_runs_a_single_operation(self):
        rewriter = _FlatAstGrepRewriter.parse(
            {
                'class_name': 'C',
                'method_name': 'Foo'
            }, description='d')
        content, errors = rewriter.apply(self._SRC, count=1, description='d')
        self.assertEqual(errors, [])
        self.assertEqual(
            content, 'class C {\n  virtual void Foo();\n  void Bar();\n};\n')

    def test_apply_runs_every_composed_operation(self):
        # The whole point of the base: `apply` runs every op `operations()`
        # yields against the same engine, so the edits from each land in the
        # final content.
        rewriter = _ComposingAstGrepRewriter('C', ['Foo', 'Bar'])
        content, errors = rewriter.apply(self._SRC, count=1, description='d')
        self.assertEqual(errors, [])
        self.assertEqual(
            content,
            'class C {\n  virtual void Foo();\n  virtual void Bar();\n};\n')

    def test_apply_validates_each_operation_independently(self):
        # A composed op that matches nothing fails its own expectation (exactly
        # one), while the matching one still applies to the content.
        rewriter = _ComposingAstGrepRewriter('C', ['Foo', 'Nope'])
        content, errors = rewriter.apply(self._SRC, count=1, description='d')
        self.assertEqual(errors, ['Unexpected number of matches (0 vs 1)'])
        self.assertEqual(
            content, 'class C {\n  virtual void Foo();\n  void Bar();\n};\n')

    def test_apply_with_no_operations_is_a_noop(self):
        rewriter = _ComposingAstGrepRewriter('C', [])
        content, errors = rewriter.apply(self._SRC, count=1, description='d')
        self.assertEqual(errors, [])
        self.assertEqual(content, self._SRC)

    # -- optional operations and cross-operation (group) rules -------------

    def test_optional_operation_never_fails_on_its_own(self):
        # 'Foo' matches; the optional 'Nope' matches nothing but, being
        # optional, contributes no error, and the group rule is satisfied.
        rewriter = _OptionalPairAstGrepRewriter(['Foo', 'Nope'])
        content, errors = rewriter.apply(self._SRC, count=1, description='d')
        self.assertEqual(errors, [])
        self.assertEqual(
            content, 'class C {\n  virtual void Foo();\n  void Bar();\n};\n')

    def test_group_rule_fails_when_no_optional_operation_applies(self):
        # Neither optional op matches, so the cross-operation rule fires even
        # though no individual op reported a count error.
        rewriter = _OptionalPairAstGrepRewriter(['Nope1', 'Nope2'])
        content, errors = rewriter.apply(self._SRC, count=1, description='d')
        self.assertEqual(errors, ['at least one operation must apply'])
        self.assertEqual(content, self._SRC)


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

    def test_rewriters_are_grouped_by_namespace(self):
        # One `Rewriters` index, with the namespaces as headings inside it --
        # the header and its hint are not repeated per group.
        code, out = self._parse('rewriters')
        self.assertEqual(code, 0)
        self.assertEqual(out.count('type "plaster --help <rewriter>"'), 1)
        for namespace in plaster._NAMESPACE_BY_NAME:
            self.assertIn(f'{namespace}:', out)
        # The namespace-agnostic rewriters get a heading of their own, last.
        self.assertIn('all:', out)
        self.assertLess(out.index('cxx:'), out.index('all:'))

    def test_namespaced_rewriter_is_listed_under_its_namespace(self):
        code, out = self._parse('rewriters')
        self.assertEqual(code, 0)
        cxx_section = out[out.index('cxx:'):out.index('all:')]
        agnostic_section = out[out.index('all:'):]
        self.assertIn('make_virtual', cxx_section)
        self.assertNotIn('make_virtual', agnostic_section)
        self.assertIn('regex', agnostic_section)

    def test_rewriter_topic_prints_its_docs(self):
        code, out = self._parse('regex')
        self.assertEqual(code, 0)
        self.assertIn('re.subn', out)
        self.assertIn('re_flags', out)

    def test_namespace_qualified_rewriter_topic_prints_its_docs(self):
        # `<namespace>.<name>` asks for one specific rewriter.
        code, out = self._parse('cxx.make_virtual')
        self.assertEqual(code, 0)
        self.assertIn('class_name', out)

    def test_qualified_topic_in_the_wrong_namespace_is_an_error(self):
        # The name is real, so the error names the namespaces it does serve
        # rather than falling through to the generic unknown-topic message.
        code, out = self._parse('js.make_virtual')
        self.assertEqual(code, 1)
        self.assertNotIn('Unknown help topic', out)
        self.assertIn('make_virtual', out)
        self.assertIn('js', out)
        self.assertIn('cxx', out)

    def test_qualified_topic_with_an_unknown_name_is_unknown(self):
        code, out = self._parse('cxx.not_a_rewriter')
        self.assertEqual(code, 1)
        self.assertIn('Unknown help topic', out)

    def test_shared_name_documents_every_namespace_and_hints_at_narrowing(
            self):
        # A bare name covers each namespace it is in, labels them, and says
        # how to ask for just one. Uses a purpose-built registry so the case
        # holds whether or not a real name happens to be shared today.
        shared = type(
            'AllSharedRewriter', (plaster.AllRegexRewriter, ), {
                'NAME': 'shared',
                'SUMMARY': 'The global one.',
                'HELP': 'Global docs.',
            })
        cxx = type(
            'CxxSharedRewriter', (plaster.AllRegexRewriter, ), {
                'NAME': 'shared',
                'SUMMARY': 'The C++ one.',
                'HELP': 'Cxx docs.',
                'namespace': classmethod(lambda cls: 'cxx'),
            })
        self.addCleanup(setattr, plaster, '_REWRITERS', plaster._REWRITERS)
        plaster._REWRITERS = plaster.RewriterRegistry(shared, cxx)

        code, out = self._parse('shared')
        self.assertEqual(code, 0)
        self.assertIn('Cxx docs.', out)
        self.assertIn('Global docs.', out)
        self.assertIn('<namespace>.shared', out)

        code, out = self._parse('cxx.shared')
        self.assertEqual(code, 0)
        self.assertIn('Cxx docs.', out)
        self.assertNotIn('Global docs.', out)

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


class RegexMacroSchemaTest(unittest.TestCase):
    """Schema and cross-reference validation for `regex_macro` ops."""

    def setUp(self):
        # load() memoises a process-wide instance; clear it so tests that
        # exercise the singleton start from a clean slate.
        plaster.RewritersEval._instance = None
        self.addCleanup(setattr, plaster.RewritersEval, '_instance', None)

    @staticmethod
    def _input(name: str, description: str = 'doc') -> dict:
        """A documented `inputs` entry: `{name, description}`."""
        return {'name': name, 'description': description}

    @classmethod
    def _valid_spec(cls) -> dict:
        """A minimal, schema-valid `regex_macro` spec as a Python dict."""
        return {
            'regex_macro': {
                'cxx.rename_constant': {
                    'description': 'Renames a constant.',
                    'inputs': [
                        cls._input('old_name'),
                        cls._input('new_name'),
                    ],
                    're_pattern': r'\b{old_name}\b',
                    'replace': '{new_name}',
                    're_flags': ['MULTILINE'],
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

    # -- access ---------------------------------------------------------

    def test_valid_spec_round_trips(self):
        rewriters = self._eval_valid()
        self.assertEqual(list(rewriters.regex_macros), ['cxx.rename_constant'])
        self.assertEqual(
            rewriters.regex_macro('cxx.rename_constant')['inputs'],
            [self._input('old_name'),
             self._input('new_name')])
        self.assertEqual(
            rewriters.regex_macro('cxx.rename_constant')['description'],
            'Renames a constant.')

    def test_unknown_op_access_raises(self):
        rewriters = self._eval_valid()
        with self.assertRaises(plaster.RewritersSchemaError):
            rewriters.regex_macro('cxx.nope')

    def test_exposed_mapping_is_read_only(self):
        rewriters = self._eval_valid()
        with self.assertRaises(TypeError):
            rewriters.regex_macros['x'] = {}

    def test_present_but_empty_is_valid(self):
        rewriters = plaster.RewritersEval("{'regex_macro': {}}")
        self.assertEqual(dict(rewriters.regex_macros), {})

    def test_absent_is_valid(self):
        # `regex_macro` is an optional top-level key, like `ast.matcher` and
        # `ast.rewriter`.
        rewriters = plaster.RewritersEval('{}')
        self.assertEqual(dict(rewriters.regex_macros), {})

    # -- op id ------------------------------------------------------------

    def test_op_id_unknown_prefix_rejected(self):

        def mutate(s):
            s['regex_macro']['py.rename_constant'] = s['regex_macro'].pop(
                'cxx.rename_constant')

        self._assert_invalid(mutate, 'Wrong keys')

    # -- field schema -------------------------------------------------------

    def test_missing_description_key_rejected(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].pop(
                'description'), 'Missing keys')

    def test_missing_inputs_key_rejected(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].pop('inputs'),
            'Missing keys')

    def test_missing_replace_key_rejected(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].pop('replace'),
            'Missing keys')

    def test_unknown_field_rejected(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].update(
                {'extra': 'x'}), 'Wrong keys')

    def test_inputs_must_be_a_list(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].__setitem__(
                'inputs', 'old_name'), "should be instance of 'list'")

    def test_input_entry_must_be_a_mapping(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].__setitem__(
                'inputs', ['old_name', self._input('new_name')]))

    def test_input_entry_missing_description_rejected(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].__setitem__(
                'inputs', [{
                    'name': 'old_name'
                }, self._input('new_name')]), 'Missing keys')

    def test_input_entry_unknown_field_rejected(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].__setitem__(
                'inputs', [{
                    **self._input('old_name'), 'extra': 'x'
                },
                           self._input('new_name')]), 'Wrong keys')

    def test_re_flags_must_be_list_of_strings(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].__setitem__(
                're_flags', 'MULTILINE'), "should be instance of 'list'")

    # -- pattern / re_pattern mutual exclusivity -----------------------------

    def test_both_pattern_and_re_pattern_rejected(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].update(
                {'pattern': '{old_name}'}), 'exactly one of')

    def test_neither_pattern_nor_re_pattern_rejected(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].pop(
                're_pattern'), 'exactly one of')

    def test_pattern_only_is_valid(self):
        spec = self._valid_spec()
        macro = spec['regex_macro']['cxx.rename_constant']
        del macro['re_pattern']
        macro['pattern'] = '{old_name}'
        rewriters = plaster.RewritersEval(repr(spec))
        self.assertEqual(
            rewriters.regex_macro('cxx.rename_constant')['pattern'],
            '{old_name}')

    # -- re_flags validity ----------------------------------------------

    def test_invalid_re_flags_entry_rejected(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].__setitem__(
                're_flags', ['NOT_A_FLAG']), 'invalid')

    # -- inputs <-> template cross-reference ---------------------------------

    def test_undeclared_input_rejected(self):
        # `replace` uses `{new_name}`, but it is dropped from `inputs`.
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant'].__setitem__(
                'inputs', [self._input('old_name')]), 'undeclared input')

    def test_unused_input_rejected(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant']['inputs'].append(
                self._input('unused')), 'never used')

    def test_duplicate_input_name_rejected(self):
        self._assert_invalid(
            lambda s: s['regex_macro']['cxx.rename_constant']['inputs'].append(
                self._input('old_name')), 'duplicate')

    # -- the real on-disk spec ------------------------------------------

    def test_real_rewriters_file_exposes_toggle_macro(self):
        rewriters = plaster.RewritersEval.load()
        self.assertIn('cxx.set_feature_flag_default_state',
                      rewriters.regex_macros)
        spec = rewriters.regex_macro('cxx.set_feature_flag_default_state')
        self.assertEqual([entry['name'] for entry in spec['inputs']],
                         ['feature_name', 'value'])


class RegexMacroEngineTest(unittest.TestCase):
    """Behavioural tests for `RegexMacroEngine.run`, against synthetic specs."""

    @staticmethod
    def _rewriters(macro: dict) -> plaster.RewritersEval:
        """Build a `RewritersEval` from a macro body given as `name: str`
        inputs; fills in the `description`/`{name, description}` schema
        boilerplate the individual test bodies below do not care about.
        """
        macro = dict(macro)
        macro.setdefault('description', 'A regex macro used for testing.')
        macro['inputs'] = [{
            'name': name,
            'description': 'doc'
        } for name in macro['inputs']]
        return plaster.RewritersEval(
            repr({'regex_macro': {
                'cxx.rename_constant': macro
            }}))

    def test_re_pattern_and_replace_are_rendered_with_inputs(self):
        rewriters = self._rewriters({
            'inputs': ['old_name', 'new_name'],
            're_pattern': r'\b{old_name}\b',
            'replace': '{new_name}',
        })
        engine = plaster.RegexMacroEngine(rewriters, 'int kOld = kOld + 1;')
        matches = engine.run('cxx.rename_constant', {
            'old_name': 'kOld',
            'new_name': 'kNew',
        })
        self.assertEqual(matches, 2)
        self.assertEqual(engine.content, 'int kNew = kNew + 1;')

    def test_pattern_is_escaped_and_rendered_with_inputs(self):
        # `pattern` is a literal: the rendered text is escaped for regex, so a
        # regex-meaningful input character like '.' matches only itself.
        rewriters = self._rewriters({
            'inputs': ['old_name', 'new_name'],
            'pattern': '{old_name}',
            'replace': '{new_name}',
        })
        engine = plaster.RegexMacroEngine(rewriters, 'a.b + axb')
        matches = engine.run('cxx.rename_constant', {
            'old_name': 'a.b',
            'new_name': 'X',
        })
        self.assertEqual(matches, 1)
        self.assertEqual(engine.content, 'X + axb')

    def test_re_flags_are_honoured(self):
        rewriters = self._rewriters({
            'inputs': ['name'],
            're_pattern': '^{name}$',
            're_flags': ['MULTILINE'],
            'replace': 'X',
        })
        engine = plaster.RegexMacroEngine(rewriters, 'foo\nfoo\n')
        matches = engine.run('cxx.rename_constant', {'name': 'foo'})
        self.assertEqual(matches, 2)
        self.assertEqual(engine.content, 'X\nX\n')

    def test_backreferences_in_replace_are_preserved(self):
        # `.format()` only touches `{}`; a `\1` backreference must reach
        # `re.subn` untouched.
        rewriters = self._rewriters({
            'inputs': ['name'],
            're_pattern': '({name})',
            'replace': r'[\1]',
        })
        engine = plaster.RegexMacroEngine(rewriters, 'foo bar')
        matches = engine.run('cxx.rename_constant', {'name': 'foo'})
        self.assertEqual(matches, 1)
        self.assertEqual(engine.content, '[foo] bar')

    def test_missing_input_raises(self):
        rewriters = self._rewriters({
            'inputs': ['old_name', 'new_name'],
            're_pattern': '{old_name}',
            'replace': '{new_name}',
        })
        engine = plaster.RegexMacroEngine(rewriters, 'kOld')
        with self.assertRaises(ValueError) as cm:
            engine.run('cxx.rename_constant', {'old_name': 'kOld'})
        self.assertIn('missing input(s): new_name', str(cm.exception))

    def test_unknown_input_raises(self):
        rewriters = self._rewriters({
            'inputs': ['name'],
            're_pattern': '{name}',
            'replace': 'x',
        })
        engine = plaster.RegexMacroEngine(rewriters, 'kOld')
        with self.assertRaises(ValueError) as cm:
            engine.run('cxx.rename_constant', {'name': 'kOld', 'extra': '1'})
        self.assertIn('unknown input(s): extra', str(cm.exception))

    def test_unknown_op_raises(self):
        rewriters = self._rewriters({
            'inputs': ['name'],
            're_pattern': '{name}',
            'replace': 'x',
        })
        engine = plaster.RegexMacroEngine(rewriters, 'kOld')
        with self.assertRaises(plaster.RewritersSchemaError):
            engine.run('cxx.nope', {'name': 'kOld'})

    def test_no_match_returns_zero_and_leaves_content_untouched(self):
        rewriters = self._rewriters({
            'inputs': ['name'],
            're_pattern': '{name}',
            'replace': 'x',
        })
        engine = plaster.RegexMacroEngine(rewriters, 'unrelated text')
        matches = engine.run('cxx.rename_constant', {'name': 'kOld'})
        self.assertEqual(matches, 0)
        self.assertEqual(engine.content, 'unrelated text')

    def test_successive_runs_accumulate_edits(self):
        rewriters = self._rewriters({
            'inputs': ['old_name', 'new_name'],
            're_pattern': r'\b{old_name}\b',
            'replace': '{new_name}',
        })
        engine = plaster.RegexMacroEngine(rewriters, 'kOne kTwo')
        engine.run('cxx.rename_constant', {
            'old_name': 'kOne',
            'new_name': 'kA'
        })
        engine.run('cxx.rename_constant', {
            'old_name': 'kTwo',
            'new_name': 'kB'
        })
        self.assertEqual(engine.content, 'kA kB')


class OverrideFeatureDefaultStateTest(unittest.TestCase):
    """Exercises the shipped `cxx.set_feature_flag_default_state` macro.

    The macro replaces a `BASE_FEATURE` call's whole last argument -- from its
    last top-level comma to the call's own closing `);` -- rather than trying
    to recognise a particular spelling of the state itself. These tests cover
    every argument shape the macro is meant to handle, plus the corner cases
    that shape implies: telling one call's `);` apart from a nested one's, and
    not running past this call into the next.
    """

    _OP_ID = 'cxx.set_feature_flag_default_state'

    def setUp(self):
        self.rewriters = plaster.RewritersEval.load()

    def _run(self, content: str, **inputs) -> tuple[int, str]:
        engine = plaster.RegexMacroEngine(self.rewriters, content)
        matches = engine.run(self._OP_ID, inputs)
        return matches, engine.content

    # -- legacy three-argument form: BASE_FEATURE(kFoo, "Foo", state) -------

    def test_three_argument_flips_disabled_to_enabled(self):
        source = ('BASE_FEATURE(kIPHDiscardRingFeature,\n'
                  '             "IPH_DiscardRing",\n'
                  '             base::FEATURE_DISABLED_BY_DEFAULT);\n')
        matches, content = self._run(source,
                                     feature_name='kIPHDiscardRingFeature',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(content, (
            '// kIPHDiscardRingFeature feature state is enforced via plaster rewrite.\n'
            'BASE_FEATURE(kIPHDiscardRingFeature,\n'
            '             "IPH_DiscardRing",\n'
            '             base::FEATURE_ENABLED_BY_DEFAULT);\n'))

    def test_three_argument_flips_enabled_to_disabled(self):
        source = ('BASE_FEATURE(kFoo,\n'
                  '             "Foo",\n'
                  '             base::FEATURE_ENABLED_BY_DEFAULT);\n')
        matches, content = self._run(source,
                                     feature_name='kFoo',
                                     value='base::FEATURE_DISABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            ('// kFoo feature state is enforced via plaster rewrite.\n'
             'BASE_FEATURE(kFoo,\n'
             '             "Foo",\n'
             '             base::FEATURE_DISABLED_BY_DEFAULT);\n'))

    def test_three_argument_single_line(self):
        source = 'BASE_FEATURE(kFoo, "Foo", base::FEATURE_DISABLED_BY_DEFAULT);'
        matches, content = self._run(source,
                                     feature_name='kFoo',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content, '// kFoo feature state is enforced via plaster rewrite.\n'
            'BASE_FEATURE(kFoo, "Foo", base::FEATURE_ENABLED_BY_DEFAULT);')

    # -- modern two-argument form: BASE_FEATURE(kFoo, state) -----------------
    # The display-name string was dropped entirely (https://crbug.com/1362858).

    def test_two_argument_form_multiline(self):
        source = ('BASE_FEATURE(kMyFeature,\n'
                  '             base::FEATURE_DISABLED_BY_DEFAULT);\n')
        matches, content = self._run(source,
                                     feature_name='kMyFeature',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            ('// kMyFeature feature state is enforced via plaster rewrite.\n'
             'BASE_FEATURE(kMyFeature,\n'
             '             base::FEATURE_ENABLED_BY_DEFAULT);\n'))

    def test_two_argument_form_single_line(self):
        source = 'BASE_FEATURE(kMyFeature, base::FEATURE_DISABLED_BY_DEFAULT);'
        matches, content = self._run(source,
                                     feature_name='kMyFeature',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            '// kMyFeature feature state is enforced via plaster rewrite.\n'
            'BASE_FEATURE(kMyFeature, base::FEATURE_ENABLED_BY_DEFAULT);')

    # -- preprocessor-conditional state: per-platform default states are
    # spelled out as an #if/#else/#endif rather than a single token. The whole
    # thing is the "last argument" here, and gets replaced wholesale, since
    # the macro overrides the state unconditionally.

    def test_preprocessor_conditional_state_is_replaced_wholesale(self):
        source = ('BASE_FEATURE(kStackScanMaxFramePointerToStackEndGap,\n'
                  '#if BUILDFLAG(IS_CHROMEOS)\n'
                  '             FEATURE_ENABLED_BY_DEFAULT\n'
                  '#else\n'
                  '             FEATURE_DISABLED_BY_DEFAULT\n'
                  '#endif\n'
                  ');\n')
        matches, content = self._run(
            source,
            feature_name='kStackScanMaxFramePointerToStackEndGap',
            value='base::FEATURE_DISABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        # The whole conditional is gone -- not merely one branch of it.
        self.assertNotIn('#if', content)
        self.assertNotIn('#else', content)
        self.assertNotIn('#endif', content)
        self.assertNotIn('BUILDFLAG', content)
        self.assertNotIn('FEATURE_ENABLED_BY_DEFAULT', content)
        self.assertIn('BASE_FEATURE(kStackScanMaxFramePointerToStackEndGap,',
                      content)
        self.assertIn(
            '// kStackScanMaxFramePointerToStackEndGap feature state is '
            'enforced via plaster rewrite.', content)
        self.assertTrue(
            content.rstrip().endswith('base::FEATURE_DISABLED_BY_DEFAULT);'))

    def test_preprocessor_conditional_does_not_confuse_nested_parens(self):
        # `BUILDFLAG(IS_CHROMEOS)` has its own closing `)`, immediately after
        # the feature name's comma; the match must not stop there instead of
        # at the call's real, statement-ending `);`.
        source = ('BASE_FEATURE(kFoo,\n'
                  '#if BUILDFLAG(IS_CHROMEOS)\n'
                  '             FEATURE_ENABLED_BY_DEFAULT\n'
                  '#else\n'
                  '             FEATURE_DISABLED_BY_DEFAULT\n'
                  '#endif\n'
                  ');\n')
        matches, content = self._run(source,
                                     feature_name='kFoo',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content, '// kFoo feature state is enforced via plaster rewrite.\n'
            'BASE_FEATURE(kFoo,\n'
            'base::FEATURE_ENABLED_BY_DEFAULT);\n')

    # -- namespace qualification: the state is matched wholesale, so any
    # spelling works without special-casing.

    def test_unqualified_state_inside_base_namespace(self):
        source = 'BASE_FEATURE(kMyFeature, FEATURE_DISABLED_BY_DEFAULT);'
        matches, content = self._run(source,
                                     feature_name='kMyFeature',
                                     value='FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            '// kMyFeature feature state is enforced via plaster rewrite.\n'
            'BASE_FEATURE(kMyFeature, FEATURE_ENABLED_BY_DEFAULT);')

    def test_fully_qualified_state(self):
        source = 'BASE_FEATURE(kMyFeature, ::base::FEATURE_DISABLED_BY_DEFAULT);'
        matches, content = self._run(
            source,
            feature_name='kMyFeature',
            value='::base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            '// kMyFeature feature state is enforced via plaster rewrite.\n'
            'BASE_FEATURE(kMyFeature, ::base::FEATURE_ENABLED_BY_DEFAULT);')

    def test_closing_parenthesis_is_preserved(self):
        # Regression check: the closing `);` sits in its own capture group,
        # so a careless replace template could swallow it.
        source = 'BASE_FEATURE(kMyFeature, base::FEATURE_DISABLED_BY_DEFAULT);'
        _, content = self._run(source,
                               feature_name='kMyFeature',
                               value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertTrue(content.rstrip().endswith(');'))

    # -- `value` introducing a brand-new conditional: `value` is only ever
    # spliced into `replace`, never into the compiled `re_pattern`, so a
    # `BUILDFLAG(IS_ANDROID)` inside it can no longer shift the pattern's own
    # capture-group numbering. The inserted comment names `feature_name`
    # rather than `value` for exactly this case: `feature_name` is always a
    # single identifier, so the comment stays a single, short line above the
    # `BASE_FEATURE` call regardless of how many lines a multi-line `value`
    # like this one spans below it.

    def test_new_conditional_value_with_parens_keeps_the_closing_paren(self):
        source = 'BASE_FEATURE(kFoo, base::FEATURE_ENABLED_BY_DEFAULT);\n'
        value = ('\n'
                 '#if BUILDFLAG(IS_ANDROID)\n'
                 '             base::FEATURE_ENABLED_BY_DEFAULT\n'
                 '#else\n'
                 '             base::FEATURE_DISABLED_BY_DEFAULT\n'
                 '#endif')
        matches, content = self._run(source, feature_name='kFoo', value=value)
        self.assertEqual(matches, 1)
        self.assertIn('BUILDFLAG(IS_ANDROID)', content)
        self.assertTrue(content.rstrip('\n').endswith('#endif);'))
        self.assertEqual(
            content, '// kFoo feature state is enforced via plaster rewrite.\n'
            'BASE_FEATURE(kFoo, \n'
            '#if BUILDFLAG(IS_ANDROID)\n'
            '             base::FEATURE_ENABLED_BY_DEFAULT\n'
            '#else\n'
            '             base::FEATURE_DISABLED_BY_DEFAULT\n'
            '#endif);\n')

    # -- multiple features in one file: every pairing of "fewer commas"
    # (two-argument/conditional) and "more commas" (three-argument) forms,
    # targeting either one, must stay within its own call. A two-argument
    # feature followed by a three-argument one is the case that actually
    # regressed: greedily matching "up to the last comma" without also
    # forbidding `;` let the match run straight past the two-argument call's
    # own `);` and land on the three-argument call's instead.

    def test_two_then_three_argument_targeting_the_first(self):
        source = (
            'BASE_FEATURE(kFeatureA, base::FEATURE_DISABLED_BY_DEFAULT);\n'
            '\n'
            'BASE_FEATURE(kFeatureB,\n'
            '             "FeatureB",\n'
            '             base::FEATURE_DISABLED_BY_DEFAULT);\n')
        matches, content = self._run(source,
                                     feature_name='kFeatureA',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            ('// kFeatureA feature state is enforced via plaster rewrite.\n'
             'BASE_FEATURE(kFeatureA, base::FEATURE_ENABLED_BY_DEFAULT);\n'
             '\n'
             'BASE_FEATURE(kFeatureB,\n'
             '             "FeatureB",\n'
             '             base::FEATURE_DISABLED_BY_DEFAULT);\n'))

    def test_two_then_three_argument_targeting_the_second(self):
        source = (
            'BASE_FEATURE(kFeatureA, base::FEATURE_DISABLED_BY_DEFAULT);\n'
            '\n'
            'BASE_FEATURE(kFeatureB,\n'
            '             "FeatureB",\n'
            '             base::FEATURE_DISABLED_BY_DEFAULT);\n')
        matches, content = self._run(source,
                                     feature_name='kFeatureB',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            ('BASE_FEATURE(kFeatureA, base::FEATURE_DISABLED_BY_DEFAULT);\n'
             '\n'
             '// kFeatureB feature state is enforced via plaster rewrite.\n'
             'BASE_FEATURE(kFeatureB,\n'
             '             "FeatureB",\n'
             '             base::FEATURE_ENABLED_BY_DEFAULT);\n'))

    def test_three_then_two_argument_targeting_the_first(self):
        source = (
            'BASE_FEATURE(kFeatureA,\n'
            '             "FeatureA",\n'
            '             base::FEATURE_DISABLED_BY_DEFAULT);\n'
            '\n'
            'BASE_FEATURE(kFeatureB, base::FEATURE_DISABLED_BY_DEFAULT);\n')
        matches, content = self._run(source,
                                     feature_name='kFeatureA',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            ('// kFeatureA feature state is enforced via plaster rewrite.\n'
             'BASE_FEATURE(kFeatureA,\n'
             '             "FeatureA",\n'
             '             base::FEATURE_ENABLED_BY_DEFAULT);\n'
             '\n'
             'BASE_FEATURE(kFeatureB, base::FEATURE_DISABLED_BY_DEFAULT);\n'))

    def test_three_then_two_argument_targeting_the_second(self):
        source = (
            'BASE_FEATURE(kFeatureA,\n'
            '             "FeatureA",\n'
            '             base::FEATURE_DISABLED_BY_DEFAULT);\n'
            '\n'
            'BASE_FEATURE(kFeatureB, base::FEATURE_DISABLED_BY_DEFAULT);\n')
        matches, content = self._run(source,
                                     feature_name='kFeatureB',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            ('BASE_FEATURE(kFeatureA,\n'
             '             "FeatureA",\n'
             '             base::FEATURE_DISABLED_BY_DEFAULT);\n'
             '\n'
             '// kFeatureB feature state is enforced via plaster rewrite.\n'
             'BASE_FEATURE(kFeatureB, base::FEATURE_ENABLED_BY_DEFAULT);\n'))

    def test_only_the_named_feature_is_overridden_when_both_are_three_argument(
            self):
        source = ('BASE_FEATURE(kFeatureA,\n'
                  '             "FeatureA",\n'
                  '             base::FEATURE_DISABLED_BY_DEFAULT);\n'
                  '\n'
                  'BASE_FEATURE(kFeatureB,\n'
                  '             "FeatureB",\n'
                  '             base::FEATURE_DISABLED_BY_DEFAULT);\n')
        matches, content = self._run(source,
                                     feature_name='kFeatureB',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertIn(
            'kFeatureA,\n'
            '             "FeatureA",\n'
            '             base::FEATURE_DISABLED_BY_DEFAULT', content)
        self.assertIn(
            '// kFeatureB feature state is enforced via plaster rewrite.\n'
            'BASE_FEATURE(kFeatureB,\n'
            '             "FeatureB",\n'
            '             base::FEATURE_ENABLED_BY_DEFAULT', content)

    # -- always-matches cases -------------------------------------------------
    #
    # Setting a feature to the value it already has still finds a match
    # (`count` of 1, never 0) and still rewrites the text, inserting the
    # `// <feature_name> feature state is enforced via plaster rewrite.`
    # comment: `count` answers "is this override in force", not "did the
    # text change shape", so the substitution can never silently stop
    # applying just because upstream's own default has converged on the
    # value Brave wants.

    def test_still_matches_when_two_argument_form_already_has_the_value(self):
        source = 'BASE_FEATURE(kFoo, base::FEATURE_DISABLED_BY_DEFAULT);'
        matches, content = self._run(source,
                                     feature_name='kFoo',
                                     value='base::FEATURE_DISABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content, '// kFoo feature state is enforced via plaster rewrite.\n'
            'BASE_FEATURE(kFoo, base::FEATURE_DISABLED_BY_DEFAULT);')

    def test_still_matches_when_three_argument_form_already_has_the_value(
            self):
        source = ('BASE_FEATURE(kFoo,\n'
                  '             "Foo",\n'
                  '             base::FEATURE_DISABLED_BY_DEFAULT);\n')
        matches, content = self._run(source,
                                     feature_name='kFoo',
                                     value='base::FEATURE_DISABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            ('// kFoo feature state is enforced via plaster rewrite.\n'
             'BASE_FEATURE(kFoo,\n'
             '             "Foo",\n'
             '             base::FEATURE_DISABLED_BY_DEFAULT);\n'))

    def test_still_matches_when_the_value_actually_differs(self):
        # Sanity check alongside the always-matches cases above: a genuinely
        # different value must still be found and applied.
        source = ('BASE_FEATURE(kFoo,\n'
                  '             "Foo",\n'
                  '             base::FEATURE_DISABLED_BY_DEFAULT);\n')
        matches, content = self._run(source,
                                     feature_name='kFoo',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            ('// kFoo feature state is enforced via plaster rewrite.\n'
             'BASE_FEATURE(kFoo,\n'
             '             "Foo",\n'
             '             base::FEATURE_ENABLED_BY_DEFAULT);\n'))

    def test_match_is_specific_to_the_named_feature(self):
        # The other feature in the file already holds the value being set on
        # kFeatureA; that's irrelevant to kFeatureA's own match, and kFeatureB
        # is untouched since it isn't the one named.
        source = (
            'BASE_FEATURE(kFeatureA, base::FEATURE_DISABLED_BY_DEFAULT);\n'
            '\n'
            'BASE_FEATURE(kFeatureB, base::FEATURE_ENABLED_BY_DEFAULT);\n')
        matches, content = self._run(source,
                                     feature_name='kFeatureA',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            ('// kFeatureA feature state is enforced via plaster rewrite.\n'
             'BASE_FEATURE(kFeatureA, base::FEATURE_ENABLED_BY_DEFAULT);\n'
             '\n'
             'BASE_FEATURE(kFeatureB, base::FEATURE_ENABLED_BY_DEFAULT);\n'))

    def test_already_set_match_does_not_leak_into_a_later_call(self):
        # kFeatureA already has the value being set -- and now matches
        # because of that, not despite it -- while kFeatureB, later in the
        # file, isn't targeted at all. kFeatureA's match must not cause the
        # engine to drift onto kFeatureB instead.
        source = (
            'BASE_FEATURE(kFeatureA, base::FEATURE_DISABLED_BY_DEFAULT);\n'
            '\n'
            'BASE_FEATURE(kFeatureB,\n'
            '             "FeatureB",\n'
            '             base::FEATURE_ENABLED_BY_DEFAULT);\n')
        matches, content = self._run(source,
                                     feature_name='kFeatureA',
                                     value='base::FEATURE_DISABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content,
            ('// kFeatureA feature state is enforced via plaster rewrite.\n'
             'BASE_FEATURE(kFeatureA, base::FEATURE_DISABLED_BY_DEFAULT);\n'
             '\n'
             'BASE_FEATURE(kFeatureB,\n'
             '             "FeatureB",\n'
             '             base::FEATURE_ENABLED_BY_DEFAULT);\n'))

    def test_preprocessor_conditional_state_is_replaced_by_a_matching_branch(
            self):
        # A conditional last argument is never a bare token, so it can never
        # equal `value` outright -- but every match rewrites regardless, so
        # setting either branch's own value still replaces the whole
        # conditional wholesale.
        source = ('BASE_FEATURE(kFoo,\n'
                  '#if BUILDFLAG(IS_CHROMEOS)\n'
                  '             FEATURE_ENABLED_BY_DEFAULT\n'
                  '#else\n'
                  '             FEATURE_DISABLED_BY_DEFAULT\n'
                  '#endif\n'
                  ');\n')
        matches, content = self._run(source,
                                     feature_name='kFoo',
                                     value='FEATURE_DISABLED_BY_DEFAULT')
        self.assertEqual(matches, 1)
        self.assertEqual(
            content, '// kFoo feature state is enforced via plaster rewrite.\n'
            'BASE_FEATURE(kFoo,\n'
            'FEATURE_DISABLED_BY_DEFAULT);\n')

    def test_no_match_for_a_different_feature_name(self):
        source = 'BASE_FEATURE(kFoo, base::FEATURE_DISABLED_BY_DEFAULT);'
        matches, content = self._run(source,
                                     feature_name='kOther',
                                     value='base::FEATURE_ENABLED_BY_DEFAULT')
        self.assertEqual(matches, 0)
        self.assertEqual(content, source)

    # -- input validation -----------------------------------------------

    def test_missing_inputs_raise(self):
        engine = plaster.RegexMacroEngine(self.rewriters, 'irrelevant')
        with self.assertRaises(ValueError):
            engine.run(self._OP_ID, {'feature_name': 'kFoo'})

    def test_unknown_input_raises(self):
        engine = plaster.RegexMacroEngine(self.rewriters, 'irrelevant')
        with self.assertRaises(ValueError):
            engine.run(
                self._OP_ID, {
                    'feature_name': 'kFoo',
                    'value': 'base::FEATURE_ENABLED_BY_DEFAULT',
                    'extra': 'x',
                })


if __name__ == '__main__':
    unittest.main()
