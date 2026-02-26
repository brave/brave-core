# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import re

PRESUBMIT_VERSION = '2.0.0'


def CheckImportSpecifierMatchesFile(input_api, output_api):
    """Checks that relative import specifier matches the actual file on disk.

    When running TypeScript natively via Node.js (no transpilation), imports
    must use the real file extension. For example, if a file is named config.ts,
    importing it as './config.js' is an error.
    """

    IMPORT_RE = re.compile(
        r'''
            (?:
                \bfrom \s+      # static: import/export ... from
              | \bimport \s* \( # dynamic: import(
              | \bimport \s+    # side-effect: import './foo.js'
            )
            \s* ['"]
            ( \.\.?/            # ./ or ../
              [^'"]*            # path
              \. [cm]?[jt]s     # file extension
            )
            ['"]
        ''', re.VERBOSE)

    files_to_check = (r'.+\.[cm]?[jt]s$', )
    file_filter = lambda f: input_api.FilterSourceFile(
        f, files_to_check=files_to_check)

    items = []
    for f in input_api.AffectedSourceFiles(file_filter):
        file_dir = os.path.dirname(f.AbsoluteLocalPath())
        for lineno, line in enumerate(f.NewContents(), 1):
            for match in IMPORT_RE.finditer(line):
                specifier = match.group(1)
                resolved = os.path.normpath(os.path.join(file_dir, specifier))

                if not os.path.isfile(resolved):
                    items.append(f'{f.LocalPath()}:{lineno}: '
                                 f'file not found: {specifier}')

    if not items:
        return []

    return [
        output_api.PresubmitError(
            'Import path references a file that doesn\'t exist. '
            'Check the file extension.', items)
    ]
