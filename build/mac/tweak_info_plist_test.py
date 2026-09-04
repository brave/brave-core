#!/usr/bin/env python3

# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import plistlib
import subprocess
import sys
import tempfile
import unittest

from pathlib import Path

_SCRIPT = Path(__file__).with_name('tweak_info_plist.py')


class TweakInfoPlistTest(unittest.TestCase):

    def _run_tweak(self, extra_args):
        with tempfile.TemporaryDirectory() as temp_dir:
            input_path = Path(temp_dir) / 'input.plist'
            output_path = Path(temp_dir) / 'output.plist'
            with input_path.open('wb') as plist_file:
                plistlib.dump(
                    {
                        'CFBundleIdentifier': 'com.brave.Browser.nightly',
                        'CFBundleShortVersionString': '151.1.93.132',
                        'CFBundleVersion': '93.132',
                        'KSChannelID': 'stale-channel',
                    }, plist_file)

            result = subprocess.run([
                sys.executable,
                str(_SCRIPT),
                f'--plist={input_path}',
                f'--output={output_path}',
                '--brave_channel=nightly',
                '--brave_version=1.93.132',
                *extra_args,
            ],
                                    check=False,
                                    capture_output=True,
                                    text=True)
            self.assertEqual(0, result.returncode, result.stderr)
            with output_path.open('rb') as plist_file:
                return plistlib.load(plist_file)

    def test_signing_removes_channel_for_packaging(self):
        plist = self._run_tweak([])

        self.assertNotIn('KSChannelID', plist)

    def test_skip_signing_embeds_channel_for_unsigned_build(self):
        plist = self._run_tweak(['--skip_signing'])

        self.assertEqual('nightly', plist['KSChannelID'])


if __name__ == '__main__':
    unittest.main()
