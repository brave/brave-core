# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import chromium_presubmit_overrides

PRESUBMIT_VERSION = '2.0.0'


# Adds support for chromium_presubmit_config.json5 and some helpers.
def CheckToModifyInputApi(input_api, _output_api):
    chromium_presubmit_overrides.modify_input_api(input_api)
    return []


def CheckPatchFile(input_api, output_api):
    files_to_check = (r'.+\.patch$', )
    files_to_skip = ()

    file_filter = lambda f: input_api.FilterSourceFile(
        f, files_to_check=files_to_check, files_to_skip=files_to_skip)

    items = []
    for f in input_api.AffectedSourceFiles(file_filter):
        contents = list(f.NewContents())
        for lineno, line in enumerate(contents, 1):
            # Only check empty added/removed lines.
            if line not in ('+', '-'):
                continue

            # Check if this empty line is part of a diff hunk.
            is_in_hunk = False
            if 1 < lineno < len(contents):
                is_in_hunk = (contents[lineno - 2].startswith(line)
                              and contents[lineno].startswith(line))

            if not is_in_hunk:
                items.append(f'{f.LocalPath()}:{lineno} ({line}empty line)')

    if not items:
        return []

    return [
        output_api.PresubmitPromptWarning(
            'Patch should not add or remove empty lines at hunk boundaries',
            items)
    ]
