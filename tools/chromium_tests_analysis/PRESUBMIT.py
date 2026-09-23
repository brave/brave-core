# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Presubmit script for changes affecting tools/chromium_tests_analysis/
"""

import os

PRESUBMIT_VERSION = '2.0.0'


def CheckTests(input_api, output_api):
    """Runs the unit tests in this directory.
    """
    script_dir = input_api.PresubmitLocalPath()
    tests = [
        input_api.Command(
            name=name,
            cmd=[os.path.join(script_dir, name)],
            kwargs={'cwd': script_dir},
            message=output_api.PresubmitError,
        ) for name in sorted(os.listdir(script_dir))
        if name.endswith('_test.py')
    ]
    return input_api.RunTests(tests)
