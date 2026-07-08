/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/shutdown_handlers.h"

#include "base/check.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/test/gtest_util.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif

namespace brave_vpn::v2 {

// These tests deliberately never perform a *successful* Install() in the
// parent test process: installation mutates process-global state (POSIX
// signal dispositions, the Windows console control handler chain) that would
// leak into every other test in the binary. Successful installs happen only
// inside death-test children, which are separate processes.

TEST(ShutdownHandlersDeathTest, NullCallbackChecks) {
  EXPECT_CHECK_DEATH({
    [[maybe_unused]] ShutdownHandlers handlers{base::RepeatingClosure()};
  });
}

TEST(ShutdownHandlersDeathTest, InstallTwiceAfterSuccessChecks) {
#if BUILDFLAG(IS_WIN)
  if (!::GetConsoleWindow()) {
    GTEST_SKIP() << "Install() is a no-op without an attached console, so the "
                    "second call cannot trip the CHECK.";
  }
#endif
  // Both Install() calls run inside the death-test child process; the CHECK
  // inside the statement guards test validity (the death must come from the
  // *second* install, not from a failed first one).
  EXPECT_CHECK_DEATH({
    ShutdownHandlers handlers(base::DoNothing());
    CHECK(handlers.Install());
    handlers.Install();
  });
}

TEST(ShutdownHandlersTest, SignalShutdownCompleteWithoutInstallIsSafe) {
  ShutdownHandlers handlers(base::DoNothing());
  handlers.SignalShutdownComplete();
}

#if BUILDFLAG(IS_WIN)
TEST(ShutdownHandlersTest, InstallWithoutConsoleReturnsFalseAndIsRetriable) {
  if (::GetConsoleWindow()) {
    GTEST_SKIP() << "Test runner has a console attached; the consoleless path "
                    "cannot be exercised in this process.";
  }
  ShutdownHandlers handlers(base::DoNothing());
  EXPECT_FALSE(handlers.Install());
  // A false return leaves no state behind, so calling Install() again is
  // allowed by contract and must not CHECK.
  EXPECT_FALSE(handlers.Install());
  // And the rest of the API stays safe to call.
  handlers.SignalShutdownComplete();
}
#endif  // BUILDFLAG(IS_WIN)

}  // namespace brave_vpn::v2
