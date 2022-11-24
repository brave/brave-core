# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import sys

USE_PYTHON3 = True
PRESUBMIT_VERSION = '2.0.0'

# pylint: disable=line-too-long


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


def CheckLicense(input_api, output_api):
    """Verifies the Brave license header."""

    files_to_check = input_api.DEFAULT_FILES_TO_CHECK + (r'.+\.gni?$', )
    files_to_skip = input_api.DEFAULT_FILES_TO_SKIP + (
        r"\.storybook/",
        r"ios/browser/api/ledger/legacy_database/core_data_models/",
        r'win_build_output/',
    )

    current_year = int(input_api.time.strftime('%Y'))
    allowed_years = (str(s) for s in reversed(range(2015, current_year + 1)))
    years_re = '(' + '|'.join(allowed_years) + ')'

    # License regexp to match in NEW and MOVED files, it doesn't allow variance.
    # Note: presubmit machinery cannot distinguish between NEW and MOVED files,
    # that's why we cannot force this regexp to have a precise year, also
    # uplifts may fail during year change period, so the year check is relaxed.
    new_file_license_re = input_api.re.compile((
        r'.*? Copyright \(c\) %(year)s The Brave Authors\. All rights reserved\.\n'
        r'.*? This Source Code Form is subject to the terms of the Mozilla Public\n'
        r'.*? License, v\. 2\.0\. If a copy of the MPL was not distributed with this file,\n'
        r'.*? You can obtain one at https://mozilla.org/MPL/2\.0/\.(?: \*/)?\n'
    ) % {'year': years_re}, input_api.re.MULTILINE)

    # License regexp to match in EXISTING files, it allows some variance.
    existing_file_license_re = input_api.re.compile((
        r'.*? Copyright \(c\) %(year)s The Brave Authors\. All rights reserved\.\n'
        r'.*? This Source Code Form is subject to the terms of the Mozilla Public\n'
        r'.*? License, v\. 2\.0\. If a copy of the MPL was not distributed with this(\n.*?)? file,\n?'
        r'.*? (y|Y)ou can obtain one at https?://mozilla.org/MPL/2\.0/\.(?: \*/)?\n'
    ) % {'year': years_re}, input_api.re.MULTILINE)

    # License template for new files. Includes current year.
    expected_license_template = (
        '%(comment)s Copyright (c) %(year)s The Brave Authors. All rights reserved.\n'
        '%(comment)s This Source Code Form is subject to the terms of the Mozilla Public\n'
        '%(comment)s License, v. 2.0. If a copy of the MPL was not distributed with this file,\n'
        '%(comment)s You can obtain one at https://mozilla.org/MPL/2.0/.\n'
    ) % {
        'comment': '#',
        'year': current_year,
    }

    assert new_file_license_re.search(expected_license_template)
    assert existing_file_license_re.search(expected_license_template)

    bad_new_files = []
    bad_files = []
    sources = lambda affected_file: input_api.FilterSourceFile(
        affected_file,
        files_to_check=files_to_check,
        files_to_skip=files_to_skip)
    for f in input_api.AffectedSourceFiles(sources):
        contents = input_api.ReadFile(f, 'r')[:1000].replace('\r\n', '\n')
        if not contents:
            continue
        if f.Action() == 'A':  # 'A' means "Added", also includes moved files.
            if not new_file_license_re.search(contents):
                bad_new_files.append(f.LocalPath())
        else:
            if not existing_file_license_re.search(contents):
                bad_files.append(f.LocalPath())

    # Show this to simplify copy-paste when an invalid license is found.
    expected_license_message = (
        f'Expected one of license headers:\n'
        f'{expected_license_template.replace("#", "//")}\n'
        f'{expected_license_template}')

    result = []
    if bad_new_files:
        result.append(
            output_api.PresubmitError(expected_license_message,
                                      items=bad_new_files))
    if bad_files:
        result.append(
            output_api.PresubmitPromptWarning(expected_license_message,
                                              items=bad_files))
    return result
