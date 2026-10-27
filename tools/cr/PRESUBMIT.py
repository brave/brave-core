# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Presubmit script for changes affecting tools/cr/

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

import chromium_presubmit_overrides
import os
import re

PRESUBMIT_VERSION = '2.0.0'

# A test file runs as one process, so a file with many (git heavy) tests is the
# last to finish. Large ones are split into shards that run in parallel.
TESTS_PER_SHARD = 12
MAX_SHARDS = 8
# Below this many CPUs the biggest file is not the bottleneck, so shards would
# only add startup cost.
MIN_CPUS_TO_SHARD = 4


# Adds support for chromium_presubmit_config.json5 and some helpers.
def CheckToModifyInputApi(input_api, _output_api):
    chromium_presubmit_overrides.modify_input_api(input_api)
    return []


def CheckTests(input_api, output_api):
    if all(f.LocalPath().endswith('.md') for f in input_api.AffectedFiles()):
        return []
    script_dir = input_api.PresubmitLocalPath()
    tests = []
    groups = {}
    for root, dirs, files in os.walk(script_dir):
        dirs[:] = [d for d in dirs if d != '__pycache__']
        for f in files:
            if not f.endswith('_test.py'):
                continue
            test_path = input_api.os_path.join(root, f)
            with open(test_path, encoding='utf-8') as test_file:
                test_count = len(
                    re.findall(r'^\s+def test_', test_file.read(), re.M)
                )
            cpus = os.cpu_count() or 1
            shards = 1
            if cpus >= MIN_CPUS_TO_SHARD:
                shards = min(
                    MAX_SHARDS, cpus, -(-test_count // TESTS_PER_SHARD)
                )
            if shards <= 1:
                invocations = [(test_path, [test_path])]
            else:
                invocations = [
                    (
                        f'{test_path} [{shard + 1}/{shards}]',
                        [
                            input_api.os_path.join(
                                script_dir, 'test', 'run_shard.py'
                            ),
                            test_path,
                            str(shard),
                            str(shards),
                        ],
                    )
                    for shard in range(shards)
                ]
            for name, cmd in invocations:
                groups[name] = os.path.relpath(test_path, script_dir).replace(
                    os.sep, '/'
                )
                tests.append(
                    input_api.Command(
                        name=name,
                        cmd=cmd,
                        kwargs={'cwd': script_dir},
                        message=output_api.PresubmitError,
                    )
                )
    return input_api.RunTestsWithProgress('CheckTests', tests, groups=groups)
