#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from pathlib import Path

import plaster

from test.fake_chromium_src import FakeChromiumSrc


class PlasterTest(unittest.TestCase):

    def setUp(self):
        """Set up a fake Chromium repository for testing."""
        self.fake_chromium_src = FakeChromiumSrc()
        self.fake_chromium_src.setup()
        self.addCleanup(self.fake_chromium_src.cleanup)

        # Override OVERLAY_FILES_PATH to use the rewrite path in the fake Brave
        # repo
        plaster.OVERLAY_FILES_PATH = self.fake_chromium_src.brave / 'rewrite'

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
                rewrite_path = plaster.OVERLAY_FILES_PATH / toml_file.name
                rewrite_path.parent.mkdir(parents=True, exist_ok=True)
                rewrite_path.write_text(toml_file.read_text())

                # Use PlasterFile to apply the .toml file to the committed file.
                overlay_file = plaster.PlasterFile(rewrite_path)
                overlay_file.apply()

                # Check that the committed file matches the expected file.
                committed_file_path = (self.fake_chromium_src.chromium /
                                       original_file_to.name)
                self.assertEqual(
                    committed_file_path.read_text(), expected_file.read_text(),
                    f'File {committed_file_path} does not match '
                    f'expected {expected_file}')


if __name__ == '__main__':
    unittest.main()
