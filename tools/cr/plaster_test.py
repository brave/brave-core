#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from pathlib import Path
import hashlib
import io
import json
import time
from unittest.mock import patch
import sys

import plaster

from test.fake_chromium_src import FakeChromiumSrc


class PlasterTest(unittest.TestCase):

    def setUp(self):
        """Set up a fake Chromium repository for testing."""
        self.fake_chromium_src = FakeChromiumSrc()
        self.fake_chromium_src.setup()
        self.addCleanup(self.fake_chromium_src.cleanup)

        # Override PLASTER_FILES_PATH to use the rewrite path in the fake Brave
        # repo
        plaster.PLASTER_FILES_PATH = self.fake_chromium_src.brave / 'rewrite'

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
        patchinfo = plaster.PatchInfo(plaster_path)
        patchinfo_from_disk = json.loads(
            patchinfo.patchinfo.path.read_text(encoding='utf-8'))

        self.assertEqual(patchinfo_from_disk['schemaVersion'], 1)
        self.assertEqual(
            patchinfo_from_disk['patchChecksum'],
            hashlib.sha256(
                self.fake_chromium_src.get_patchfile_path_for_source(
                    self.fake_chromium_src.chromium,
                    test_file_chromium).read_text().encode()).hexdigest())
        self.assertEqual(patchinfo_from_disk['appliesTo'][0]['path'],
                         str(test_file_chromium))
        self.assertEqual(
            patchinfo_from_disk['appliesTo'][0]['checksum'],
            hashlib.sha256(
                (self.fake_chromium_src.chromium /
                 test_file_chromium).read_text().encode()).hexdigest())
        self.assertEqual(
            patchinfo_from_disk['plaster']['path'],
            str(plaster_path.relative_to(self.fake_chromium_src.brave)))
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
        # Now check should succeed (no sys.exit(1))
        class DummyArgs:

            def __init__(self):
                self.infra_mode = False
                self.verbose = False

        args = DummyArgs()
        # Should not call sys.exit at all
        with patch('sys.exit') as mock_exit:
            plaster.check(args)
            mock_exit.assert_not_called()

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
        with patch('sys.exit') as mock_exit:
            plaster.check(args)
            mock_exit.assert_not_called()
        # Now change one toml file to cause a failure
        changed_path = rewrite_paths[1]
        changed_path.write_text('''
          [[substitution]]
          description = 'Break the rule'
          re_pattern = 'foo2'
          replace = 'DIFFERENT'
        ''')
        # Now check should fail and print the file on stderr
        stderr = io.StringIO()
        with patch('sys.stderr', stderr), patch('sys.exit') as mock_exit:
            plaster.check(args)
            mock_exit.assert_called_once_with(1)
            output = stderr.getvalue()
            self.assertIn(str(changed_path), output)


if __name__ == '__main__':
    unittest.main()
