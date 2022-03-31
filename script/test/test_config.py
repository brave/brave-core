#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import unittest
from lib import config


class TestConfig(unittest.TestCase):

    def test_output_dir_android_arm(self):
        self.assertEquals(config.output_dir(
            'android', 'arm').endswith('android_Release_arm'), True)

    def test_output_dir_android_arm64(self):
        self.assertEquals(config.output_dir(
            'android', 'arm64').endswith('android_Release_arm64'), True)

    def test_output_dir_android_x86(self):
        self.assertEquals(config.output_dir(
            'android', 'x86').endswith('android_Release_x86'), True)

    def test_output_dir_android_x64(self):
        self.assertEquals(config.output_dir(
            'android', 'x64').endswith('android_Release'), True)

    def test_output_dir_darwin_x64(self):
        self.assertEquals(config.output_dir(
            'darwin', 'x64').endswith('Release'), True)

    def test_output_dir_darwin_x64(self):
        self.assertEquals(config.output_dir(
            'darwin', 'arm64').endswith('Release_arm64'), True)

    def test_output_dir_linux_x64(self):
        self.assertEquals(config.output_dir(
            'linux', 'x64').endswith('Release'), True)

    def test_output_dir_win32_x86(self):
        self.assertEquals(config.output_dir(
            'win32', 'x86').endswith('Release_x86'), True)

    def test_output_dir_win32_x64(self):
        self.assertEquals(config.output_dir(
            'win32', 'x64').endswith('Release'), True)
