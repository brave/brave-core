# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Presubmit script for changes affecting tools/cr/

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

import os

PRESUBMIT_VERSION = '2.0.0'


def CheckTests(input_api, output_api):
    """Run every *_test.py file found under this directory.
    """
    script_dir = input_api.PresubmitLocalPath()
    tests = []
    for root, dirs, files in os.walk(script_dir):
        dirs[:] = [d for d in dirs if d != '__pycache__']
        for f in files:
            if not f.endswith('_test.py'):
                continue
            test_path = input_api.os_path.join(root, f)
            tests.append(
                input_api.Command(
                    name=test_path,
                    cmd=[test_path],
                    kwargs={'cwd': script_dir},
                    message=output_api.PresubmitError,
                ))
    return input_api.RunTests(tests)
