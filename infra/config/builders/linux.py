# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `brave.linux` builder group: the Linux official builders."""

from lib.config import builder_config, builders, gn_args, targets

builders.defaults.set(
    builder_group='brave.linux',
    recipe='brave_builder',
    execution_timeout_mins=300,
    siso_remote_jobs=1440,
    siso_reapi_priority=None,
    notifies=[],
)

builders.builder(
    name='linux-x64-official',
    builder_spec=builder_config.builder_spec(
        gclient_config=builder_config.gclient_config(
            custom_vars={
                'checkout_pgo_profiles': True,
            },
        ),
        chromium_config=builder_config.chromium_config(
            target_platform=builder_config.target_platform.LINUX,
            host_platform=builder_config.host_platform.LINUX,
            build_config=builder_config.build_config.RELEASE,
            target_arch=builder_config.target_arch.INTEL,
            target_bits=64,
        ),
        brave_config=builder_config.brave_config(
            is_brave_release_build=True,
            use_brave_hermetic_toolchain=True,
        ),
    ),
    gn_args=gn_args.config(
        configs=[
            'release',
            'official',
            # TODO(https://github.com/brave/brave-browser/issues/61946): The
            # published Linux builds are built locally; decide whether official
            # builds use remote execution.
            'remoteexec',
            'channel',
            'linux',
            'x64',
        ]
    ),
    targets=targets.bundle(
        targets=[
            'brave_all_unit_tests_linux',
            'brave_browser_tests',
            'brave_interactive_ui_tests',
            'chromium_unit_tests_linux',
            'browser_tests',
            'brave_network_tests_linux',
        ],
        additional_compile_targets=[
            'brave',
        ],
    ),
)
