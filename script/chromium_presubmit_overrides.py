# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

# This file is executed (not imported) in the context of src/PRESUBMIT.py, it
# uses existing functions from a global scope to change them. This allows us to
# alter root Chromium presubmit checks without introducing too much conflict.

import copy

import lib.chromium_presubmit_utils as chromium_presubmit_utils
import lib.override_utils as override_utils

_BRAVE_DEFAULT_FILES_TO_SKIP = (r'win_build_output[\\/].*', )


# Modify depot_tools-bundled checks (Chromium calls them canned checks).
# These modification will stay active across all PRESUBMIT.py files, i.e.
# src/PRESUBMIT.py, src/brave/PRESUBMIT.py.
def _modify_canned_checks(canned_checks):
    # pylint: disable=unused-variable

    # Disable upstream-specific license check.
    @chromium_presubmit_utils.override_check(canned_checks)
    def CheckLicense(*_, **__):
        return []

    # We don't use OWNERS files.
    @chromium_presubmit_utils.override_check(canned_checks)
    def CheckOwnersFormat(*_, **__):
        return []

    # We don't use OWNERS files.
    @chromium_presubmit_utils.override_check(canned_checks)
    def CheckOwners(*_, **__):
        return []

    # We don't use AUTHORS file.
    @chromium_presubmit_utils.override_check(canned_checks)
    def CheckAuthorizedAuthor(*_, **__):
        return []

    # We don't upload change to Chromium git.
    @chromium_presubmit_utils.override_check(canned_checks)
    def CheckChangeWasUploaded(*_, **__):
        return []

    # We don't upload change to Chromium git.
    @chromium_presubmit_utils.override_check(canned_checks)
    def CheckChangeHasBugField(*_, **__):
        return []

    # We don't upload change to Chromium git.
    @chromium_presubmit_utils.override_check(canned_checks)
    def CheckTreeIsOpen(*_, **__):
        return []

    # Changes from upstream:
    # 1. Generate PresubmitError instead of Warning on format issue.
    # 2. Replace suggested command from upstream-specific to 'npm run format'.
    @chromium_presubmit_utils.override_check(canned_checks)
    def CheckPatchFormatted(original_check, input_api, output_api, *args,
                            **kwargs):
        kwargs = {
            **kwargs,
            'bypass_warnings': False,
            'check_python': True,
            'result_factory': output_api.PresubmitError,
        }
        result = original_check(input_api, output_api, *args, **kwargs)
        # If presubmit generates "Please run git cl format --js" message, we
        # should replace the command with "npm run format -- --js". The order of
        # these replacements ensure we do this properly.
        replacements = [
            (' format --', ' format -- --'),
            ('git cl format', 'npm run format'),
            ('gn format', 'npm run format'),
            ('rust-fmt', 'rust'),
            ('swift-format', 'swift'),
        ]
        for item in result:
            for replacement in replacements:
                # pylint: disable=protected-access
                item._message = item._message.replace(replacement[0],
                                                      replacement[1])
        return result

    # Changes from upstream:
    # 1. Run lint only on *changed* files instead of getting *all* files from
    #    the directory. Upstream does it to catch breakages in unmodified files,
    #    but it's very resource intensive, moreover for our setup it covers all
    #    files from vendor and other directories which we should ignore.
    # 2. Set is_committing=True to force PresubmitErrors instead of Warnings.
    @chromium_presubmit_utils.override_check(canned_checks)
    def GetPylint(original_check, input_api, *args, **kwargs):
        def _FetchAllFiles(_, input_api, files_to_check, files_to_skip):
            src_filter = lambda f: input_api.FilterSourceFile(
                f, files_to_check=files_to_check, files_to_skip=files_to_skip)
            return [
                f.LocalPath()
                for f in input_api.AffectedSourceFiles(src_filter)
            ]

        with override_utils.override_scope_function(input_api.canned_checks,
                                                    _FetchAllFiles):
            with override_utils.override_scope_variable(
                    input_api, 'is_committing', True):
                return original_check(input_api, *args, **kwargs)


# Override the first ever check defined in PRESUBMIT.py to make changes to
# input_api before any real check is run.
@chromium_presubmit_utils.override_check(
    globals(), chromium_presubmit_utils.get_first_check_name(globals()))
def OverriddenFirstCheck(original_check, input_api, output_api):
    _modify_canned_checks(input_api.canned_checks)
    input_api.DEFAULT_FILES_TO_SKIP += _BRAVE_DEFAULT_FILES_TO_SKIP
    return original_check(input_api, output_api)


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


# Changes from upstream:
# 1. Add 'brave/' prefix for header guard checks to properly validate guards.
@chromium_presubmit_utils.override_check(globals())
def CheckForIncludeGuards(original_check, input_api, output_api):
    def AffectedSourceFiles(self, original_method, source_file):
        def PrependBrave(affected_file):
            # pylint: disable=protected-access
            affected_file = copy.copy(affected_file)
            affected_file._path = f'brave/{affected_file._path}'
            return affected_file

        return [
            PrependBrave(f) for f in filter(self.FilterSourceFile,
                                            original_method(source_file))
        ]

    with override_utils.override_scope_function(input_api,
                                                AffectedSourceFiles):
        return original_check(input_api, output_api)


# Changes from upstream:
# 1. Extend _KNOWN_TEST_DATA_AND_INVALID_JSON_FILE_PATTERNS with files to
#    ignore.
@chromium_presubmit_utils.override_check(globals())
def CheckParseErrors(original_check, input_api, output_api):
    # pylint: disable=undefined-variable
    _KNOWN_TEST_DATA_AND_INVALID_JSON_FILE_PATTERNS.append(r'tsconfig\.json$')
    return original_check(input_api, output_api)


# Changes from upstream:
# 1. Remove ^(chrome|components|content|extensions) filter to cover all files
#    in the repository, because brave/ structure is slightly different.
@chromium_presubmit_utils.override_check(globals())
def CheckMPArchApiUsage(original_check, input_api, output_api):
    def AffectedFiles(self, original_method, *args, **kwargs):
        kwargs['file_filter'] = self.FilterSourceFile
        return original_method(*args, **kwargs)

    with override_utils.override_scope_function(input_api, AffectedFiles):
        return original_check(input_api, output_api)


# Changes from upstream:
# 1. Skip chromium_src/ files.
@chromium_presubmit_utils.override_check(globals())
def CheckForRelativeIncludes(original_check, input_api, output_api):
    def AffectedFiles(self, original_method, *args, **kwargs):
        files_to_skip = input_api.DEFAULT_FILES_TO_SKIP + (
            r'^chromium_src[\\/]', )
        file_filter = lambda x: self.FilterSourceFile(
            x, files_to_skip=files_to_skip)
        kwargs['file_filter'] = file_filter
        return original_method(*args, **kwargs)

    with override_utils.override_scope_function(input_api, AffectedFiles):
        return original_check(input_api, output_api)


# Changes from upstream:
# 1. Skip chromium_src/ files.
@chromium_presubmit_utils.override_check(globals())
def CheckForCcIncludes(original_check, input_api, output_api):
    def AffectedFiles(self, original_method, *args, **kwargs):
        files_to_skip = input_api.DEFAULT_FILES_TO_SKIP + (
            r'^chromium_src[\\/]', )
        file_filter = lambda x: self.FilterSourceFile(
            x, files_to_skip=files_to_skip)
        kwargs['file_filter'] = file_filter
        return original_method(*args, **kwargs)

    with override_utils.override_scope_function(input_api, AffectedFiles):
        return original_check(input_api, output_api)


# Changes from upstream:
# 1. Ignore java files, currently a lot of DEPS issues.
@chromium_presubmit_utils.override_check(globals())
def CheckUnwantedDependencies(original_check, input_api, output_api):
    def AffectedFiles(self, original_method, *args, **kwargs):
        files_to_skip = input_api.DEFAULT_FILES_TO_SKIP + (r'.*\.java$', )
        file_filter = lambda x: self.FilterSourceFile(
            x, files_to_skip=files_to_skip)
        kwargs['file_filter'] = file_filter
        return original_method(*args, **kwargs)

    with override_utils.override_scope_function(input_api, AffectedFiles):
        return original_check(input_api, output_api)
