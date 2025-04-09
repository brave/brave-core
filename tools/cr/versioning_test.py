#!/usr/bin/env vpython3
# # Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest
from unittest.mock import patch

from versioning import (load_package_file, read_chromium_version_file,
                        get_uplift_branch_name_from_package)
from versioning import Version

from test.fake_chromium_src import FakeChromiumSrc
import json


class VersioningTest(unittest.TestCase):

    def setUp(self):
        """Set up a fake Chromium repository for testing."""
        self.fake_chromium_src = FakeChromiumSrc()
        self.fake_chromium_src.setup()
        self.addCleanup(self.fake_chromium_src.cleanup)

    def test_load_package_file(self):
        """Test that load_package_file loads the correct package.json."""
        # Update the package.json file with a specific version
        test_version_1 = "1.2.3.4"
        v1 = self.fake_chromium_src.update_brave_version(test_version_1)

        # Load the package.json file using load_package_file
        loaded_package_1 = load_package_file("HEAD")

        # Assert that the loaded version matches the first updated version
        self.assertEqual(
            loaded_package_1.get("config").get("projects").get("chrome").get(
                "tag"),
            test_version_1,
        )

        # Update the package.json file with a second version
        test_version_2 = "2.3.4.5"
        self.fake_chromium_src.update_brave_version(test_version_2)

        loaded_package_2 = load_package_file("HEAD")
        self.assertEqual(
            loaded_package_2.get("config").get("projects").get("chrome").get(
                "tag"),
            test_version_2,
        )

        # Assert that the hash passed in is being used to load the file.
        self.assertEqual(
            loaded_package_1,
            load_package_file(v1),
        )

    def test_version_parts(self):
        """Test that Version.parts correctly splits the version string."""
        version = Version("1.2.3.4")
        self.assertEqual(version.parts, (1, 2, 3, 4))

        version = Version("10.20.30.40")
        self.assertEqual(version.parts, (10, 20, 30, 40))

        with self.assertRaises(ValueError):
            Version("1.2.3")  # Invalid version format

        with self.assertRaises(ValueError):
            Version("1.2.3.4.5")  # Invalid version format

        with self.assertRaises(ValueError):
            Version("asdfasdf")  # Invalid version format

    def test_version_major(self):
        """Test that Version.major correctly retrieves the major version."""
        version = Version("1.2.3.4")
        self.assertEqual(version.major, 1)

        version = Version("10.20.30.40")
        self.assertEqual(version.major, 10)

        version = Version("0.1.2.3")
        self.assertEqual(version.major, 0)

    def test_version_comparisons(self):
        """Test that Version comparisons work correctly."""
        # Test equality
        self.assertEqual(Version("1.2.3.4"), Version("1.2.3.4"))
        self.assertNotEqual(Version("1.2.3.4"), Version("1.2.3.5"))

        # Test less than
        self.assertTrue(Version("1.2.3.4") < Version("1.2.3.5"))
        self.assertTrue(Version("1.2.3.5") < Version("2.0.0.0"))
        self.assertFalse(Version("2.0.0.0") < Version("1.2.3.4"))
        self.assertFalse(Version("10.20.3.4") < Version("1.2.3.5"))
        self.assertFalse(Version("1.20.3.4") < Version("1.2.3.5"))
        self.assertFalse(Version("1.2.30.4") < Version("1.2.3.4"))
        self.assertFalse(Version("1.2.3.40") < Version("1.2.3.4"))

        # Test greater than (via __lt__)
        self.assertTrue(Version("2.0.0.0") > Version("1.2.3.5"))
        self.assertFalse(Version("1.2.3.4") > Version("1.2.3.4"))
        self.assertTrue(Version("10.20.3.4") > Version("1.2.3.5"))
        self.assertTrue(Version("1.20.3.4") > Version("1.2.3.5"))
        self.assertTrue(Version("1.2.30.4") > Version("1.2.3.4"))
        self.assertTrue(Version("1.2.3.40") > Version("1.2.3.4"))

    def test_version_from_git(self):
        """Test that Version.from_git retrieves the correct version."""
        # Update the package.json file with a specific version
        test_version_1 = "3.4.5.6"
        commit_hash_1 = self.fake_chromium_src.update_brave_version(
            test_version_1)

        # Retrieve the version using Version.from_git with HEAD
        version_1 = Version.from_git("HEAD")
        self.assertEqual(str(version_1), test_version_1)
        self.assertEqual(version_1.parts, (3, 4, 5, 6))

        # Update the package.json file with another version
        test_version_2 = "4.5.6.7"
        commit_hash_2 = self.fake_chromium_src.update_brave_version(
            test_version_2)

        # Retrieve the version using Version.from_git with HEAD
        version_2 = Version.from_git("HEAD")
        self.assertEqual(str(version_2), test_version_2)
        self.assertEqual(version_2.parts, (4, 5, 6, 7))

        # Retrieve the version using Version.from_git with the first commit hash
        version_1_again = Version.from_git(commit_hash_1)
        self.assertEqual(str(version_1_again), test_version_1)
        self.assertEqual(version_1_again.parts, (3, 4, 5, 6))

        # Retrieve the second version using Version.from_git
        version_2_again = Version.from_git(commit_hash_2)
        self.assertEqual(str(version_2_again), test_version_2)
        self.assertEqual(version_2_again.parts, (4, 5, 6, 7))

    def test_get_googlesource_diff_link(self):
        """Test the link generated by get_googlesource_diff_link."""
        version_1 = Version("1.2.3.4")
        version_2 = Version("2.3.4.5")

        expected_link = ("https://chromium.googlesource.com/chromium/src"
                         "/+log/1.2.3.4..2.3.4.5?pretty=fuller&n=10000")

        self.assertEqual(version_2.get_googlesource_diff_link(version_1),
                         expected_link)

    def test_read_chromium_version_file(self):
        """Test the result of read_chromium_version_file."""
        # Add a tag with a specific version
        test_version_1 = "1.2.3.4"
        self.fake_chromium_src.add_tag(test_version_1)

        # Read the version from the VERSION file
        version_1 = read_chromium_version_file()
        self.assertEqual(str(version_1), test_version_1)
        self.assertEqual(version_1.parts, (1, 2, 3, 4))

        # Add another tag with a different version
        test_version_2 = "5.6.7.8"
        self.fake_chromium_src.add_tag(test_version_2)

        # Read the updated version from the VERSION file
        version_2 = read_chromium_version_file()
        self.assertEqual(str(version_2), test_version_2)
        self.assertEqual(version_2.parts, (5, 6, 7, 8))

    def test_get_uplift_branch_name_from_package(self):
        """Test the result of get_uplift_branch_name_from_package."""
        # Update the package.json file with a specific version
        test_version = '1.2.3'
        self.fake_chromium_src.write_and_stage_file(
            'package.json', json.dumps({'version': test_version}),
            self.fake_chromium_src.brave)
        self.fake_chromium_src.commit('Update package.json',
                                      self.fake_chromium_src.brave)

        # Assert the uplift branch name is generated correctly
        uplift_branch_name = get_uplift_branch_name_from_package()
        self.assertEqual(uplift_branch_name, '1.2.x')


if __name__ == "__main__":
    unittest.main()
