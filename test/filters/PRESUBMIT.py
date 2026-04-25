# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import chromium_presubmit_overrides

PRESUBMIT_VERSION = '2.0.0'


# Adds support for chromium_presubmit_config.json5 and some helpers.
def CheckToModifyInputApi(input_api, _output_api):
    chromium_presubmit_overrides.modify_input_api(input_api)
    return []


# Ensure that all entries in the filter files start with the minus.
def CheckFilterEntriesStartWithMinus(input_api, output_api):
    files_to_check = (r'.+\.filter$', )
    files_to_skip = ()

    file_filter = lambda f: input_api.FilterSourceFile(
        f, files_to_check=files_to_check, files_to_skip=files_to_skip)

    items = []
    for f in input_api.AffectedSourceFiles(file_filter):
        for lineno, line in enumerate(f.NewContents(), 1):
            # Skip empty lines and comments.
            if not line or line.startswith('#'):
                continue
            if not line.startswith('-'):
                items.append(f'{f.LocalPath()}:{lineno} {line}')

    if not items:
        return []

    return [
        output_api.PresubmitError(
            "Entries in upstream test filters should start with '-'", items)
    ]
