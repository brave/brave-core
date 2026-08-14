# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `brave.sanitisers` builder group: the Linux ASan builders."""

from lib.config import builders, gn_args

builders.defaults.set(
    builder_group='brave.sanitisers',
    execution_timeout_mins=270,
    channel='nightly',
    notifies=[
        'browser-sanitizers-bot',
    ],
)

builders.builder(
    name='linux-x64-asan-brave',
    sync_config=builders.sync_config(target_os='linux', target_cpu='x64'),
    gn_args=gn_args.config(configs=[
        'release',
        'asan',
        'remoteexec',
        'nightly',
        'linux',
        'x64',
    ]),
    targets=builders.targets(
        compile=[
            'brave:all',
        ],
        tests=[
            'brave_all_unit_tests',
            'brave_browser_tests',
            'brave_interactive_ui_tests',
            'brave_network_audit_tests',
        ],
    ),
)

builders.builder(
    name='linux-x64-asan-chromium',
    sync_config=builders.sync_config(target_os='linux', target_cpu='x64'),
    gn_args=gn_args.config(configs=[
        'release',
        'asan',
        'remoteexec',
        'nightly',
        'linux',
        'x64',
    ]),
    targets=builders.targets(
        compile=[
            'brave:all',
        ],
        tests=[
            'chromium_unit_tests',
            'browser_tests',
        ],
    ),
)
