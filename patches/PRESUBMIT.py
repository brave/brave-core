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
            # Prevent removing empty lines, it's a guaranteed git conflict.
            if line == '-':
                items.append(f'{f.LocalPath()}:{lineno} (removed empty line)')
                continue

            # Prevent adding empty lines at diff block boundaries. This doesn't
            # affect git conflict resolution, but adds unnecessary noise. It's
            # okay to have empty lines in the middle of a diff block.
            if line != '+':
                continue

            # Look at previous and next lines to determine if this empty line is
            # at diff block boundary.
            prev_line = contents[lineno - 2] if lineno > 1 else ''
            next_line = contents[lineno] if lineno < len(contents) else ''

            # Empty line at block boundary or single-line block.
            if not (prev_line.startswith('+') and next_line.startswith('+')):
                items.append(f'{f.LocalPath()}:{lineno} (added empty line)')

    if not items:
        return []

    return [
        output_api.PresubmitError(
            'Patch file should not add or remove empty lines', items)
    ]
