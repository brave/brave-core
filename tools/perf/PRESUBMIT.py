# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import pathlib
import platform
import os

PRESUBMIT_VERSION = '2.0.0'


def CheckChange(input_api, output_api):
  tests = []
  results = []
  testing_path = pathlib.Path(input_api.PresubmitLocalPath())

  # Pytype (not supported on windows)
  if platform.system() in ('Linux', 'Darwin'):
    run_pytype_path = os.path.join(str(testing_path), 'tests', 'run_pytype.py')
    tests.append(
        input_api.Command(
            name='pytype',
            cmd=[
                input_api.python3_executable,
                run_pytype_path,
            ],
            message=output_api.PresubmitError,
            kwargs={},
            python3=True,
        ))
  # ---------------------------------------------------------------------------
  # Run all test
  results += input_api.RunTests(tests)
  return results
