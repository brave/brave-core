# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import chromium_presubmit_overrides

USE_PYTHON3 = True
PRESUBMIT_VERSION = '2.0.0'


# Adds support for chromium_presubmit_config.json5 and some helpers.
def CheckToModifyInputApi(input_api, _output_api):
    chromium_presubmit_overrides.modify_input_api(input_api)
    return []


# Ensure we add // IWYU pragma: export into overridden chromium headers to make
# IWYU aware that these headers actually export other headers.
def CheckOverriddenHeadersDeclareIWYUExport(input_api, output_api):
    files_to_check = (r'.+\.h$', )
    files_to_skip = ()

    file_filter = lambda f: input_api.FilterSourceFile(
        f, files_to_check=files_to_check, files_to_skip=files_to_skip)

    include_prefixes = ('#include "src/', '#include "../gen/')
    nolint = 'NOLINT'
    expected_suffix = '// IWYU pragma: export'

    items = []
    for f in input_api.AffectedSourceFiles(file_filter):
        for lineno, line in enumerate(f.NewContents(), 1):
            if not line.startswith(include_prefixes) or nolint in line:
                continue
            if line.endswith(expected_suffix):
                continue
            items.append(f'{f.LocalPath()}:{lineno}')

    if not items:
        return []

    return [
        output_api.PresubmitError(
            f'#include "src/**/*.h" should end with {expected_suffix}', items)
    ]
