#!/usr/bin/env vpython3

# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
A temporary fix for FFMpeg perf bug:
https://github.com/brave/brave-browser/issues/34639

The script re-checkouts the good ffmpeg revision used in CR119.
Consider removing after FFMpeg is updated.
"""

import os
import sys
import subprocess

from lib.config import SOURCE_ROOT

FFMPEG_DIR = os.path.join(SOURCE_ROOT, '..', 'third_party', 'ffmpeg')
STABLE_REVISION = 'acb78dc0f416f6ef009192d94dc07c05effabfda'
BAD_REVISION = 'e1ca3f06adec15150a171bc38f550058b4bbb23b'


def main():
    current_rev = subprocess.check_output(['git', 'rev-parse', 'HEAD'],
                                          cwd=FFMPEG_DIR,
                                          universal_newlines=True).rstrip()

    if current_rev == STABLE_REVISION:
        return 0

    # FFMpeg is updated. The perf bug is probably fixed.
    # Consider removing this file.
    assert current_rev == BAD_REVISION

    subprocess.check_call(['git', 'checkout', '--force', STABLE_REVISION],
                          cwd=FFMPEG_DIR)
    return 0


if __name__ == '__main__':
    sys.exit(main())
