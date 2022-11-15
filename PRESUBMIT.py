# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import os
import sys

USE_PYTHON3 = True
PRESUBMIT_VERSION = '2.0.0'


def CheckChangeLintsClean(input_api, output_api):
    return input_api.canned_checks.CheckChangeLintsClean(input_api,
                                                         output_api,
                                                         lint_filters=[])


def CheckPylint(input_api, output_api):
    extra_paths_list = os.environ['PYTHONPATH'].split(';' if sys.platform ==
                                                      'win32' else ':')
    return input_api.canned_checks.RunPylint(
        input_api,
        output_api,
        pylintrc=input_api.os_path.join(input_api.PresubmitLocalPath(),
                                        '.pylintrc'),
        extra_paths_list=extra_paths_list,
        version='2.7')
