# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os

PRESUBMIT_VERSION = '2.0.0'

# Recipe simulation tests that should be run by the recipes engine
_RECIPE_SIM_TEST_RE = r'recipes_test\.py$'


def CheckUnitTests(input_api, output_api):
    """Run the machinery/unit tests (every *_test.py except the sim suite)."""
    script_dir = input_api.PresubmitLocalPath()
    tests = []
    for root, dirs, files in os.walk(script_dir):
        dirs[:] = [d for d in dirs if d != '__pycache__']
        if any(f.endswith('_test.py') for f in files):
            tests.extend(
                input_api.canned_checks.GetUnitTestsInDirectory(
                    input_api,
                    output_api,
                    root,
                    files_to_check=[r'.+_test\.py$'],
                    files_to_skip=[_RECIPE_SIM_TEST_RE]))
    return input_api.RunTests(tests)


def CheckRecipeSimulationTests(input_api, output_api):
    """Run the recipe simulation suite on its own, sequentially."""
    tests = input_api.canned_checks.GetUnitTestsInDirectory(
        input_api,
        output_api,
        input_api.os_path.join(input_api.PresubmitLocalPath(), 'unittests'),
        files_to_check=[_RECIPE_SIM_TEST_RE])
    return input_api.RunTests(tests, parallel=False)
