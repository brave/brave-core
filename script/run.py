#!/usr/bin/env python3

# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import sys

from shutil import which
from subprocess import run

# On Windows, run(...) does not search PATH for the executable. So do it here:
executable_path = which(sys.argv[1])

# pylint: disable=subprocess-run-check
sys.exit(run([executable_path] + sys.argv[2:]).returncode)
