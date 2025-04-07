# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Presubmit script for changes affecting tools/cr/

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

PRESUBMIT_VERSION = '2.0.0'
TEST_FILE_PATTERN = [r'.+_test.py$']


def CheckTests(input_api, output_api):
    # input_api.logging.debug('testing this output')
    return input_api.canned_checks.RunUnitTestsInDirectory(
        input_api, output_api, '.', files_to_check=TEST_FILE_PATTERN)
