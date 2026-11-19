# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The test binaries tests run."""

from lib.config import targets

targets.binaries.windowed_test_launcher(
    name='brave_browser_tests',
    label='//brave/test:brave_browser_tests',
)
targets.binaries.console_test_launcher(
    name='brave_cert_pinning_tests',
    label='//brave/net:brave_cert_pinning_tests',
)
targets.binaries.windowed_test_launcher(
    name='brave_components_unittests',
    label='//brave/components:brave_components_unittests',
)
targets.binaries.console_test_launcher(
    name='brave_installer_unittests',
    label='//brave/test:brave_installer_unittests',
)
targets.binaries.windowed_test_launcher(
    name='brave_interactive_ui_tests',
    label='//brave/test:brave_interactive_ui_tests',
)
targets.binaries.windowed_test_launcher(
    name='brave_network_audit_tests',
    label='//brave/test:brave_network_audit_tests',
)
targets.binaries.windowed_test_launcher(
    name='brave_unit_tests',
    label='//brave/test:brave_unit_tests',
)
targets.binaries.console_test_launcher(
    name='brave_updater_unittests',
    label='//brave/updater:brave_updater_unittests',
)

# Chromium's suites, which Brave builds also run.
targets.binaries.console_test_launcher(
    name='base_unittests',
    label='//base:base_unittests',
)
targets.binaries.windowed_test_launcher(
    name='browser_tests',
    label='//chrome/test:browser_tests',
)
targets.binaries.windowed_test_launcher(
    name='components_unittests',
    label='//components:components_unittests',
)
targets.binaries.windowed_test_launcher(
    name='content_unittests',
    label='//content/test:content_unittests',
)
targets.binaries.console_test_launcher(
    name='net_unittests',
    label='//net:net_unittests',
)
targets.binaries.windowed_test_launcher(
    name='services_unittests',
    label='//services:services_unittests',
)
targets.binaries.windowed_test_launcher(
    name='unit_tests',
    label='//chrome/test:unit_tests',
)
