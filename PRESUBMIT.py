# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

USE_PYTHON3 = True
PRESUBMIT_VERSION = '2.0.0'


def CheckChangeLintsClean(input_api, output_api):
    return input_api.canned_checks.CheckChangeLintsClean(input_api,
                                                         output_api,
                                                         lint_filters=[])


def CheckPylint(input_api, output_api):
    return input_api.canned_checks.RunPylint(input_api,
                                             output_api,
                                             version='2.7')
