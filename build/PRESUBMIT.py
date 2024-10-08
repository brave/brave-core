# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import re

import chromium_presubmit_overrides

PRESUBMIT_VERSION = '2.0.0'


# Adds support for chromium_presubmit_config.json5 and some helpers.
def CheckToModifyInputApi(input_api, _output_api):
    chromium_presubmit_overrides.modify_input_api(input_api)
    return []


def CheckCIFeatures(input_api, output_api):
    files_to_check = (r'build/\.ci_features$', )

    file_filter = lambda f: input_api.FilterSourceFile(
        f, files_to_check=files_to_check)

    expected_re = r'''
        # Allow comments starting with #.
        (^\#.*$)
        |
        # Allow lower_snake_case features without spaces before and after.
        (^[a-z][a-z0-9_]*$)
    '''

    items = []
    for f in input_api.AffectedSourceFiles(file_filter):
        for lineno, line in enumerate(f.NewContents(), 1):
            if not line:
                continue
            if re.match(expected_re, line, re.VERBOSE):
                continue
            items.append(f'{f.LocalPath()}:{lineno} {line}')

    if not items:
        return []

    return [
        output_api.PresubmitError(
            f'build/.ci_features lines don\'t match regex {expected_re}',
            items)
    ]


def CheckUnitTests(input_api, output_api):
    return input_api.canned_checks.RunUnitTestsInDirectory(
        input_api, output_api, '.', files_to_check=[r'.+_test.py$'])
