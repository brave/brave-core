#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import os
import re
import sys
import unittest

from lib.config import *
from mock import call, MagicMock

dirname = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(dirname, '..'))


class TestConfig(unittest.TestCase):

    def test_get_chromedriver_version(self):
        pattern = "v[0-9]\.[0-9]+"
        m = re.match(pattern, get_chromedriver_version())
        self.assertTrue(m)


if __name__ == '__main__':
    print unittest.main()
