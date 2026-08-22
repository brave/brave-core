# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Presubmit script for changes affecting infra/
"""

import os

PRESUBMIT_VERSION = '2.0.0'


def CheckTests(input_api, output_api):
    """Run every *_test.py file found under this directory.
    """
    script_dir = input_api.PresubmitLocalPath()
    tests = []
    for root, dirs, files in os.walk(script_dir):
        dirs[:] = [d for d in dirs if d != '__pycache__']
        for f in files:
            if not f.endswith('_test.py'):
                continue
            test_path = input_api.os_path.join(root, f)
            tests.append(
                input_api.Command(
                    name=test_path,
                    cmd=[test_path],
                    kwargs={'cwd': script_dir},
                    message=output_api.PresubmitError,
                ))
    return input_api.RunTests(tests)


def CheckBotsSnapshotOutput(input_api, output_api):
    """Verifies infra/config/generated/builders/ is up to date.

    Mirrors upstream's canned `CheckLucicfgGenOutput`: runs the generator in
    check-only mode, unconditionally (regardless of which files changed), and
    turns a non-zero exit - drift, or a file under generated/builders/ that
    is no longer produced by anything - into a presubmit error.
    """
    bots_py = os.path.join(input_api.PresubmitLocalPath(), 'bots', 'bots.py')
    return input_api.RunTests([
        input_api.Command(
            name='bots.py snapshot --check',
            cmd=[input_api.python3_executable, bots_py, 'snapshot', '--check'],
            kwargs={},
            message=output_api.PresubmitError,
        ),
    ])


def CheckBotsValidateOutput(input_api, output_api):
    """Sanity-checks infra/config/generated/builders/.

    Mirrors upstream's `tools/mb/PRESUBMIT.py`'s `CheckMbValidate`: runs
    `bots.py validate` and turns a non-zero exit into a presubmit error.
    """
    bots_py = os.path.join(input_api.PresubmitLocalPath(), 'bots', 'bots.py')
    return input_api.RunTests([
        input_api.Command(
            name='bots.py validate',
            cmd=[input_api.python3_executable, bots_py, 'validate'],
            kwargs={},
            message=output_api.PresubmitError,
        ),
    ])
