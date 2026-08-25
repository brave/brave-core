/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "chrome/updater/test/integration_tests_impl.h"

#define GetRealUpdaterLowerVersions GetRealUpdaterLowerVersions_ChromiumImpl
#include <chrome/updater/test/integration_tests_win.cc>
#undef GetRealUpdaterLowerVersions

namespace updater::test {

// Upstream looks for previously released updaters under
// `old_updater/<BROWSER_NAME>_win_*`. Those come from Google's CIPD and only
// exist for Chromium and Chrome. For Brave, upstream's implementation
// dereferences a null FileVersionInfo and crashes while gtest registers the
// parameterized tests. We have no previously released Omaha 4 builds to test
// against yet, so the parameterized tests only run against the current build.
std::vector<TestUpdaterVersion> GetRealUpdaterLowerVersions(
    const std::string& arch_suffix) {
  return {};
}

}  // namespace updater::test
