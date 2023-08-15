# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import pathlib
import platform

USE_PYTHON3 = True
PRESUBMIT_VERSION = '2.0.0'


def CheckChange(input_api, output_api):
  tests = []
  results = []
  testing_env = dict(input_api.environ)
  testing_path = pathlib.Path(input_api.PresubmitLocalPath())
  vpython3_spec_path = input_api.os_path.join(testing_path, '..', '..', '..',
                                              'third_party', 'crossbench',
                                              '.vpython3')
  testing_env['PYTHONPATH'] = input_api.os_path.pathsep.join(
      map(str, [testing_path]))

  # Pytype (not supported on windows)
  if platform.system() in ('Linux', 'Darwin'):
    tests.append(
        input_api.Command(
            name='pytype',
            cmd=[
                input_api.python3_executable,
                '-vpython-spec',
                vpython3_spec_path,
                '-m',
                'pytype',
                '--keep-going',
                '--jobs=auto',
                '--overriding-parameter-count-checks',
                str(testing_path),
            ],
            message=output_api.PresubmitError,
            kwargs={},
            python3=True,
        ))
  # ---------------------------------------------------------------------------
  # Run all test
  results += input_api.RunTests(tests)
  return results
