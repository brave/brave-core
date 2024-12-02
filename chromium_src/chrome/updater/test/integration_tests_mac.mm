/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// We disable Omaha 4's background task to prevent it from running at the same
// time as Sparkle. This breaks upstream's integration tests. The code in this
// file fixes them and tests our desired behavior.

#include "chrome/updater/test/integration_tests_mac.h"

#include <optional>

#include "base/auto_reset.h"
#include "base/files/file_path.h"
#include "chrome/updater/updater_scope.h"
#include "chrome/updater/util/mac_util.h"

namespace {

thread_local bool g_in_expect_installed = false;

}  // namespace

namespace updater {

std::optional<base::FilePath> GetWakeTaskPlistPath_BraveImpl(
    UpdaterScope scope) {
  if (g_in_expect_installed) {
    // Pretend that the wake job is registered by returning an existing path:
    return base::FilePath("/");
  }
  return GetWakeTaskPlistPath(scope);
}

}  // namespace updater

#define GetWakeTaskPlistPath GetWakeTaskPlistPath_BraveImpl
#define ExpectInstalled ExpectInstalled_ChromiumImpl
#include "src/chrome/updater/test/integration_tests_mac.mm"
#undef GetWakeTaskPlistPath
#undef ExpectInstalled

namespace updater::test {

void ExpectInstalled(UpdaterScope scope) {
  // Test for our desired behavior. We don't want the wake job to be registered:
  EXPECT_FALSE(base::PathExists(*GetWakeTaskPlistPath(scope)));

  // Call the original implementation in such a way that it believes the wake
  // job is registered:
  base::AutoReset<bool> auto_reset(&g_in_expect_installed, true);
  ExpectInstalled_ChromiumImpl(scope);
}

}  // namespace updater::test
