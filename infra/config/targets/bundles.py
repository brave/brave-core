# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The bundles builders compile and test."""

from lib.config import targets

# The Linux members of the `brave_all_unit_tests` group in //brave/BUILD.gn.
targets.bundle(
    name='brave_all_unit_tests_linux',
    targets=[
        'brave_components_unittests',
        'brave_unit_tests',
        'brave_installer_unittests',
        'brave_updater_unittests',
    ],
)

# The Linux members of the `chromium_unit_tests` group in //brave/BUILD.gn.
targets.bundle(
    name='chromium_unit_tests_linux',
    targets=[
        'base_unittests',
        'net_unittests',
        'services_unittests',
        'unit_tests',
        'content_unittests',
        'components_unittests',
    ],
)
