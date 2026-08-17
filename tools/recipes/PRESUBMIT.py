# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os

PRESUBMIT_VERSION = '2.0.0'

# Recipe simulation tests that should be run by the recipes engine
_RECIPE_SIM_TEST_RE = r'recipes_test\.py$'


def _MakeTestCommand(input_api, output_api, script_dir, test_path):
    return input_api.Command(
        name=test_path,
        cmd=[test_path],
        kwargs={'cwd': script_dir},
        message=output_api.PresubmitError,
    )


def CheckUnitTests(input_api, output_api):
    """Run the machinery/unit tests (every *_test.py except the sim suite)."""
    script_dir = input_api.PresubmitLocalPath()
    tests = []
    for root, dirs, files in os.walk(script_dir):
        dirs[:] = [d for d in dirs if d != '__pycache__']
        for f in files:
            if not f.endswith('_test.py') or input_api.re.match(
                    _RECIPE_SIM_TEST_RE, f):
                continue
            tests.append(
                _MakeTestCommand(input_api, output_api, script_dir,
                                 input_api.os_path.join(root, f)))
    return input_api.RunTests(tests)


def CheckRecipeSimulationTests(input_api, output_api):
    """Run the recipe simulation suite on its own, sequentially."""
    script_dir = input_api.PresubmitLocalPath()
    unittests_dir = input_api.os_path.join(script_dir, 'unittests')
    tests = []
    for f in input_api.os_listdir(unittests_dir):
        test_path = input_api.os_path.join(unittests_dir, f)
        if input_api.re.match(_RECIPE_SIM_TEST_RE,
                              f) and input_api.os_path.isfile(test_path):
            tests.append(
                _MakeTestCommand(input_api, output_api, script_dir, test_path))
    return input_api.RunTests(tests, parallel=False)
