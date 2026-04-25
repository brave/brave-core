#!/usr/bin/env vpython3
#
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
#
# prune_test_filters_test.py
#
# Tests prune_test_filters.py.
#

import unittest
import tempfile
import shutil
from pathlib import Path
from unittest import mock

# Import functions from your script
import prune_test_filters as ptf


class PruneTestFiltersTest(unittest.TestCase):

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.build_dir = Path(self.temp_dir) / "build"
        self.filter_dir = Path(self.temp_dir) / "filters"

        self.build_dir.mkdir()
        self.filter_dir.mkdir()

        # Patch global FILTER_DIR to point to temp
        self._orig_filter_dir = ptf.FILTER_DIR
        ptf.FILTER_DIR = self.filter_dir

    def tearDown(self):
        ptf.FILTER_DIR = self._orig_filter_dir
        shutil.rmtree(self.temp_dir)

    def write_filter(self, name, content):
        path = self.filter_dir / name
        path.write_text(content)
        return path

    def create_fake_binary(self, name, test_output):
        binary = self.build_dir / name
        binary.write_text("")

        def fake_check_output(cmd, **_kwargs):
            if str(binary) in cmd[0]:
                return test_output
            raise RuntimeError("unexpected binary")

        return fake_check_output

    def test_collect_tests_per_binary(self):
        self.write_filter("foo.filter", "")

        fake = self.create_fake_binary("foo", "SuiteA.\n  Test1\n  Test2\n")

        with mock.patch("subprocess.check_output", side_effect=fake):
            tests_by_binary = ptf.collect_tests_per_binary(
                self.build_dir, "linux")

        self.assertIn("foo", tests_by_binary)
        self.assertEqual(tests_by_binary["foo"],
                         {"SuiteA.Test1", "SuiteA.Test2"})

    def test_scan_filter_files_detects_obsolete(self):
        self.write_filter(
            "foo.filter", """# header

- SuiteA.Test1
- SuiteA.TestMissing
""")

        tests_by_binary = {"foo": {"SuiteA.Test1"}}

        result = ptf.scan_filter_files(tests_by_binary,
                                       current_platform="linux",
                                       apply=False)

        self.assertEqual(len(result), 1)
        file_path = next(iter(result))
        self.assertIn("- SuiteA.TestMissing", result[file_path])

    def test_scan_filter_files_apply_removes_obsolete(self):
        path = self.write_filter("foo.filter", """- SuiteA.Test1
- SuiteA.TestMissing
""")

        tests_by_binary = {"foo": {"SuiteA.Test1"}}

        ptf.scan_filter_files(tests_by_binary,
                              current_platform="linux",
                              apply=True)

        updated = path.read_text()
        self.assertIn("SuiteA.Test1", updated)
        self.assertNotIn("TestMissing", updated)

    def test_protected_filters_not_removed(self):
        protected = ptf.PROTECTED_FILTERS[0]

        self.write_filter("foo.filter", f"- {protected}\n")

        tests_by_binary = {"foo": set()}

        result = ptf.scan_filter_files(tests_by_binary,
                                       current_platform="linux",
                                       apply=False)

        # Should not be marked obsolete
        self.assertEqual(result, {})

    def test_platform_filtering(self):
        self.write_filter("foo-mac.filter", "- SuiteA.Test1\n")

        tests_by_binary = {"foo": {"SuiteA.Test1"}}

        result = ptf.scan_filter_files(tests_by_binary,
                                       current_platform="linux",
                                       apply=False)

        # File should be skipped entirely
        self.assertEqual(result, {})


if __name__ == "__main__":
    unittest.main()
