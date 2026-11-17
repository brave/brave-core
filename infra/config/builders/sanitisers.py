# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `brave.sanitisers` builder group: the Linux ASan builders."""

from lib.config import builder_config, builders, gn_args, targets

builders.defaults.set(
    builder_group='brave.sanitisers',
    recipe='brave_builder',
    execution_timeout_mins=270,
    channel='nightly',
    siso_remote_jobs=1440,
    notifies=[
        'browser-sanitizers-bot',
    ],
)

builders.builder(
    name='linux-x64-asan-brave',
    builder_spec=builder_config.builder_spec(
        gclient_config=builder_config.gclient_config(),
        chromium_config=builder_config.chromium_config(
            target_platform=builder_config.target_platform.LINUX,
            host_platform=builder_config.host_platform.LINUX,
            build_config=builder_config.build_config.RELEASE,
            target_arch=builder_config.target_arch.INTEL,
            target_bits=64,
        ),
    ),
    gn_args=gn_args.config(
        configs=[
            'release',
            'asan',
            'remoteexec',
            'nightly',
            'linux',
            'x64',
        ]
    ),
    targets=targets.bundle(
        targets=[
            'brave_all_unit_tests_linux',
            'brave_browser_tests',
            'brave_interactive_ui_tests',
            'brave_network_audit_tests',
        ],
        additional_compile_targets=[
            'brave',
        ],
        mixins=[
            'asan_no_odr_violation_detection',
        ],
    ),
)

builders.builder(
    name='linux-x64-asan-chromium',
    builder_spec=builder_config.builder_spec(
        gclient_config=builder_config.gclient_config(),
        chromium_config=builder_config.chromium_config(
            target_platform=builder_config.target_platform.LINUX,
            host_platform=builder_config.host_platform.LINUX,
            build_config=builder_config.build_config.RELEASE,
            target_arch=builder_config.target_arch.INTEL,
            target_bits=64,
        ),
    ),
    gn_args=gn_args.config(
        configs=[
            'release',
            'asan',
            'remoteexec',
            'nightly',
            'linux',
            'x64',
        ]
    ),
    targets=targets.bundle(
        targets=[
            'chromium_unit_tests_linux',
            'browser_tests',
        ],
        additional_compile_targets=[
            'brave',
        ],
        mixins=[
            'asan_no_odr_violation_detection',
        ],
    ),
)
