# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import chromium_presubmit_overrides
import os

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


def CheckPatchFileSource(input_api, output_api):
    """Checks that the patchfile is patching the expected source."""
    files_to_check = (r'.+\.patch$', )
    files_to_skip = ()

    file_filter = lambda f: input_api.FilterSourceFile(
        f, files_to_check=files_to_check, files_to_skip=files_to_skip)

    items = []
    for f in input_api.AffectedSourceFiles(file_filter):
        patch_filename = os.path.basename(f.LocalPath())
        if not patch_filename.endswith('.patch'):
            continue

        contents = list(f.NewContents())
        if not contents:
            items.append(f'{f.LocalPath()}: empty patch file')
            continue
        first_line = contents[0].strip()
        # Expecting: diff --git a/<path> b/<path>
        if not first_line.startswith(
                'diff --git a/') or ' b/' not in first_line:
            items.append(
                f'{f.LocalPath()}: first line does not contain a valid diff'
                ' header.\n'
                f'  Found:    {first_line}')
            continue

        try:
            a_path = first_line.split('diff --git a/', 1)[1].split(' b/', 1)[0]
        except Exception:
            items.append(f'{f.LocalPath()}: failed to parse diff header.\n'
                         f'  Found:    {first_line}')
            continue

        expected_patch_filename = a_path.replace('/', '-') + '.patch'
        if patch_filename != expected_patch_filename:
            items.append(
                f'{f.LocalPath()}: patch filename does not match source file in'
                ' diff header.\n'
                f'  Expected: {expected_patch_filename}\n'
                f'  Found:    {patch_filename}')

    if not items:
        return []
    return [
        output_api.PresubmitError(
            'Patch filename does not match source file in diff header', items)
    ]
