/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/mock_callback.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/scoped_relaunch_chrome_browser_override.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"

// Upstream's "Relaunch to update" app menu command relaunches straight away on
// desktop, without the update dialog Brave used to go through. With no Sparkle
// update pending, macOS relaunches the same way.
class RelaunchToUpdateBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUpInProcessBrowserTestFixture() override {
    // The relaunch happens late in browser shutdown.
    EXPECT_CALL(mock_relaunch_callback_, Run(testing::_))
        .WillOnce(testing::Return(true));
  }

 private:
  base::MockCallback<upgrade_util::RelaunchChromeBrowserCallback>
      mock_relaunch_callback_;
  upgrade_util::ScopedRelaunchChromeBrowserOverride relaunch_chrome_override_{
      mock_relaunch_callback_.Get()};
};

IN_PROC_BROWSER_TEST_F(RelaunchToUpdateBrowserTest, RelaunchesWithoutDialog) {
  PrefService* local_state = g_browser_process->local_state();
  ASSERT_FALSE(local_state->GetBoolean(prefs::kWasRestarted));

  ui_test_utils::BrowserDestroyedObserver observer(browser());
  chrome::ExecuteCommand(browser(), IDC_UPGRADE_DIALOG);

  // A dialog would leave the browser running until it is accepted.
  EXPECT_TRUE(browser_shutdown::IsTryingToQuit());
  EXPECT_TRUE(local_state->GetBoolean(prefs::kWasRestarted));
  observer.Wait();
}
