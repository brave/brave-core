/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <chrome/updater/test/integration_tests.cc>

namespace updater::test {

// These suites are parameterized over previously released updaters, which we
// don't have. See GetRealUpdaterLowerVersions() in our override of
// integration_tests_win.cc. Without the following, the test launcher refuses
// to run any test because the suites' parameter lists are empty.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(IntegrationLowerVersionTest);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(IntegrationSansInstallIdTest);

}  // namespace updater::test
