#!/usr/bin/env python3

# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from subprocess import run

import sys

libdmg_hfsplus_dir = sys.argv[1]

_run = lambda *args: run(args, cwd=libdmg_hfsplus_dir, check=True)

_run('cmake', '.', '-B', 'build')
_run('make', '-C', 'build/dmg')
