# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

PRESUBMIT_VERSION = '2.0.0'


def CheckTests(input_api, output_api):
    return input_api.canned_checks.RunUnitTestsInDirectory(
        input_api, output_api, '.', files_to_check=[r'.+_test\.py$']
    )
