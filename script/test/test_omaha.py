#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import os
import sys
import unittest
from mock import patch

from lib.helpers import release_channel
from lib.util import get_host_arch, omaha_channel, get_platform
from lib.config import PLATFORM

dirname = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(dirname, '..'))


class TestOmahaLib(unittest.TestCase):

    @patch('lib.util.get_host_arch')
    @patch('lib.util.get_platform')
    def test_win_omaha_channel(self, mock_get_platform, mock_get_host_arch):
        PLATFORM = 'win32'
        os.environ['PLATFORM'] = 'win32'
        mock_get_platform.return_value = PLATFORM
        # print("DEBUG: PLATFORM: {}".format(PLATFORM))
        for CHANNEL in ['nightly', 'dev', 'beta', 'release']:
            os.environ['CHANNEL'] = CHANNEL
            # print("DEBUG: CHANNEL: {}".format(CHANNEL))
            for TARGET_ARCH in ['x64', 'ia32']:
                os.environ['TARGET_ARCH'] = TARGET_ARCH
                if TARGET_ARCH == 'ia32':
                    mock_get_host_arch.return_value = 'x86'
                else:
                    mock_get_host_arch.return_value = TARGET_ARCH
                # print("DEBUG: TARGET_ARCH: {}".format(TARGET_ARCH))
                # print("DEBUG: omaha_channel(): {}".format(omaha_channel()))
                if release_channel() in ['nightly', 'beta']:
                    chan = release_channel()[0:2]
                elif release_channel() in ['dev', 'release']:
                    chan = release_channel()[0:3]
                self.assertEquals('{}-{}'.format(mock_get_host_arch.return_value, chan),
                                  omaha_channel(PLATFORM, TARGET_ARCH, False))

    @patch('lib.util.get_host_arch')
    @patch('lib.util.get_platform')
    def test_darwin_omaha_channel(self, mock_get_platform, mock_get_host_arch):
        PLATFORM = 'darwin'
        os.environ['PLATFORM'] = PLATFORM
        mock_get_platform.return_value = PLATFORM
        # print("DEBUG: PLATFORM: {}".format(PLATFORM))
        for CHANNEL in ['beta', 'dev', 'release']:
            os.environ['CHANNEL'] = CHANNEL
            # print("DEBUG: CHANNEL: {}".format(CHANNEL))
            for TARGET_ARCH in ['x64']:
                os.environ['TARGET_ARCH'] = TARGET_ARCH
                mock_get_host_arch.return_value = TARGET_ARCH
                # print("DEBUG: TARGET_ARCH: {}".format(TARGET_ARCH))
                # print("DEBUG: omaha_channel(): {}".format(omaha_channel()))
                if PLATFORM is 'darwin':
                    self.assertEquals(CHANNEL if CHANNEL not in 'release' else 'stable',
                                      omaha_channel(PLATFORM, TARGET_ARCH, False))


if __name__ == '__main__':
    print unittest.main()
