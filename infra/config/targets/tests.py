# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The tests bundles include."""

from lib.config import targets

targets.tests.gtest_test(name='brave_browser_tests')
targets.tests.gtest_test(name='brave_cert_pinning_tests')
targets.tests.gtest_test(name='brave_components_unittests')
targets.tests.gtest_test(name='brave_installer_unittests')
targets.tests.gtest_test(name='brave_interactive_ui_tests')
targets.tests.gtest_test(
    name='brave_network_audit_tests',
    args=[
        '--ui-test-action-timeout=320000',
        '--test-launcher-timeout=2200000',
    ],
)
targets.tests.gtest_test(name='brave_unit_tests')
targets.tests.gtest_test(name='brave_updater_unittests')

# Chromium's suites, which Brave builds also run.
targets.tests.gtest_test(name='base_unittests')
targets.tests.gtest_test(name='browser_tests')
targets.tests.gtest_test(name='components_unittests')
targets.tests.gtest_test(name='content_unittests')
targets.tests.gtest_test(name='net_unittests')
targets.tests.gtest_test(name='services_unittests')
targets.tests.gtest_test(name='unit_tests')
