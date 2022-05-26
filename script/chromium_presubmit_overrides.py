# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

# This file is executed (not imported) in the context of src/PRESUBMIT.py, it
# uses existing functions from a global scope to change them. This allows us to
# alter root Chromium presubmit checks without introducing too much conflict.

# pylint: disable=protected-access

import copy

import lib.chromium_presubmit_utils as chromium_presubmit_utils

_BRAVE_DEFAULT_FILES_TO_SKIP = (r'win_build_output[\\/].*', )


# Modify depot_tools-bundled checks (Chromium calls them canned checks).
def _modify_canned_checks(input_api):
    input_api.DEFAULT_FILES_TO_SKIP += _BRAVE_DEFAULT_FILES_TO_SKIP

    # pylint: disable=unused-variable

    # Disable upstream-specific license check.
    @chromium_presubmit_utils.override_check(input_api.canned_checks)
    def CheckLicense(*_, **__):
        return []

    # We don't use OWNERS files.
    @chromium_presubmit_utils.override_check(input_api.canned_checks)
    def CheckOwnersFormat(*_, **__):
        return []

    # We don't use OWNERS files.
    @chromium_presubmit_utils.override_check(input_api.canned_checks)
    def CheckOwners(*_, **__):
        return []

    # We don't upload change to Chromium git.
    @chromium_presubmit_utils.override_check(input_api.canned_checks)
    def CheckChangeWasUploaded(*_, **__):
        return []

    # We don't upload change to Chromium git.
    @chromium_presubmit_utils.override_check(input_api.canned_checks)
    def CheckChangeHasBugField(*_, **__):
        return []

    # We don't upload change to Chromium git.
    @chromium_presubmit_utils.override_check(input_api.canned_checks)
    def CheckTreeIsOpen(*_, **__):
        return []

    # Enforce format check, fix output text to show our command instead.
    @chromium_presubmit_utils.override_check(input_api.canned_checks)
    def CheckPatchFormatted(original_check, input_api, output_api, *args,
                            **kwargs):
        if 'result_factory' in kwargs:
            del kwargs['result_factory']
        result = original_check(input_api,
                                output_api,
                                *args,
                                result_factory=output_api.PresubmitError,
                                **kwargs)
        for item in result:
            item._message = item._message.replace('git cl format',
                                                  'npm run format --')
            item._message = item._message.replace('gn format',
                                                  'npm run format --')
        return result

    # Override Pylint checker to do per-file lint.
    @chromium_presubmit_utils.override_check(input_api.canned_checks)
    def GetPylint(original_check, input_api, *args, **kwargs):
        # Change upstream logic to lint each Python file individually instead of
        # getting *all* files from the root directory. Upstream does it to catch
        # breakages in unmodified files.
        def _FetchAllFiles(_, input_api, files_to_check, files_to_skip):
            src_filter = lambda f: input_api.FilterSourceFile(
                f, files_to_check=files_to_check, files_to_skip=files_to_skip)
            return [
                f.LocalPath()
                for f in input_api.AffectedSourceFiles(src_filter)
            ]

        # Make Pylint work on high core Windows machines. crbug.com/1190269
        cpu_count = input_api.cpu_count
        if input_api.is_windows:
            cpu_count = min(cpu_count, 32)

        with chromium_presubmit_utils.override_scope_function(
                input_api.canned_checks, _FetchAllFiles):
            with chromium_presubmit_utils.override_scope_variable(
                    input_api, 'cpu_count', cpu_count):
                # is_committing = True forces pylint to be errors.
                with chromium_presubmit_utils.override_scope_variable(
                        input_api, 'is_committing', True):
                    return original_check(input_api, *args, **kwargs)


# Override the first ever check defined in PRESUBMIT.py to make changes to
# input_api/output_api before any real check is run.
@chromium_presubmit_utils.override_check(
    globals(), chromium_presubmit_utils.get_first_check_name(globals()))
def OverriddenFirstCheck(original_check, input_api, output_api, *args,
                         **kwargs):
    _modify_canned_checks(input_api)
    return original_check(input_api, output_api, *args, **kwargs)


# We don't use OWNERS files.
@chromium_presubmit_utils.override_check(globals())
def CheckSecurityOwners(*_, **__):
    return []


# This validates added strings with screenshot tests which we don't use.
@chromium_presubmit_utils.override_check(globals())
def CheckStrings(*_, **__):
    return []


# Don't check upstream pydeps.
@chromium_presubmit_utils.override_check(globals())
def CheckPydepsNeedsUpdating(*_, **__):
    return []


# Override to add brave/ prefix for the header guard checks.
@chromium_presubmit_utils.override_check(globals())
def CheckForIncludeGuards(original_check, input_api, output_api, *args,
                          **kwargs):
    def AffectedSourceFiles(self, original_method, source_file):
        def PrependBrave(affected_file):
            affected_file = copy.copy(affected_file)
            affected_file._path = f'brave/{affected_file._path}'
            return affected_file

        return [
            PrependBrave(f) for f in filter(self.FilterSourceFile,
                                            original_method(source_file))
        ]

    with chromium_presubmit_utils.override_scope_function(
            input_api, AffectedSourceFiles):
        return original_check(input_api, output_api, *args, **kwargs)


# Ignore some files to check for JSON parse errors.
@chromium_presubmit_utils.override_check(globals())
def CheckParseErrors(original_check, input_api, output_api, *args, **kwargs):
    # pylint: disable=undefined-variable
    _KNOWN_TEST_DATA_AND_INVALID_JSON_FILE_PATTERNS.append(r'tsconfig\.json$')
    return original_check(input_api, output_api, *args, **kwargs)


# Modify check to include all files to check, otherwise it checks only
# ^chrome, ^components, etc. (ignoring ^brave).
@chromium_presubmit_utils.override_check(globals())
def CheckMPArchApiUsage(original_check, input_api, output_api, *args,
                        **kwargs):
    def AffectedFiles(self, original_method, *_, **__):
        return original_method(False, self.FilterSourceFile)

    with chromium_presubmit_utils.override_scope_function(
            input_api, AffectedFiles):
        return original_check(input_api, output_api, *args, **kwargs)
