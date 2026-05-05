# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import chromium_presubmit_overrides
import os
import re

# Full 40-char SHA hashes as produced by `git diff --full-index`.
_FULL_INDEX_RE = re.compile(r'^index ([0-9a-f]{40})\.\.([0-9a-f]{40})( \d+)?$')

PRESUBMIT_VERSION = '2.0.0'
TEST_FILE_PATTERN = [r'.+_test\.py$']


# Adds support for chromium_presubmit_config.json5 and some helpers.
def CheckToModifyInputApi(input_api, _output_api):
    chromium_presubmit_overrides.modify_input_api(input_api)
    return []


def CheckTests(input_api, output_api):
    return input_api.canned_checks.RunUnitTestsInDirectory(
        input_api, output_api, '.', files_to_check=TEST_FILE_PATTERN)


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


def CheckPatchFileIndexHeader(input_api, output_api):
    """Checks that patch files use full SHA hashes in the index line.

    Patches must be generated with `git diff --full-index` so that index lines
    contain full 40-character SHA hashes rather than abbreviated ones.

    Expected header format:
        diff --git a/path/to/file.cc b/path/to/file.cc
        index a1b2c3d4e5f6...(40 hex chars)..f6e5d4c3b2a1 100644
        --- a/path/to/file.cc
        +++ b/path/to/file.cc
    """
    files_to_check = (r'.+\.patch$', )
    files_to_skip = ()

    file_filter = lambda f: input_api.FilterSourceFile(
        f, files_to_check=files_to_check, files_to_skip=files_to_skip)

    items = []
    for f in input_api.AffectedSourceFiles(file_filter):
        contents = list(f.NewContents())
        if not contents:
            continue

        # The index line should appear within the first few header lines.
        index_line = None
        for i, line in enumerate(contents[:5]):
            if line.startswith('index '):
                index_line = (i + 1, line)
                break

        if index_line is None:
            items.append(
                f'{f.LocalPath()}: missing index line in patch header')
            continue

        lineno, line = index_line
        if not _FULL_INDEX_RE.match(line):
            items.append(f'{f.LocalPath()}:{lineno}: index line must use full'
                         ' 40-character SHA hashes (generate patches with'
                         ' `git diff --full-index`).\n'
                         f'  Found: {line}')

    if not items:
        return []

    return [
        output_api.PresubmitError(
            'Patch index line does not use full SHA hashes', items)
    ]


def CheckPatchFileSingleSource(input_api, output_api):
    """Checks that each patch file patches exactly one source file.

    Each .patch file must contain a single `diff --git` section. Patching
    multiple files in one .patch file is not allowed; use one patch file per
    source file instead.
    """
    files_to_check = (r'.+\.patch$', )
    files_to_skip = ()

    file_filter = lambda f: input_api.FilterSourceFile(
        f, files_to_check=files_to_check, files_to_skip=files_to_skip)

    items = []
    for f in input_api.AffectedSourceFiles(file_filter):
        contents = list(f.NewContents())

        diff_headers = [(i + 1, line) for i, line in enumerate(contents)
                        if line.startswith('diff --git ')]

        if len(diff_headers) != 1:
            count = len(diff_headers)
            detail = (f'  Found {count} diff --git headers'
                      if count > 1 else '  No diff --git header found')
            items.append(f'{f.LocalPath()}: patch must contain exactly one'
                         f' source file.\n{detail}')

    if not items:
        return []

    return [
        output_api.PresubmitError(
            'Patch file must patch exactly one source file', items)
    ]
