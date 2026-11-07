/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/mock_callback.h"
#import "brave/browser/mac/fake_sparkle.h"
#import "brave/browser/mac/sparkle_glue.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/first_run/scoped_relaunch_chrome_browser_override.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/lifetime/application_lifetime_desktop.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"

// With a Sparkle update waiting to be installed on quit, every restart entry
// point must let Sparkle install it and relaunch; relaunching the old bundle
// left the updated browser not starting or crashing.
class RelaunchToUpdateSparkleBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    glue_ = [[SparkleGlue alloc] init];
    updater_ = [[FakeSUUpdater alloc] init];
    updater_.driver = [[FakeSUUpdateDriver alloc] init];
    [SparkleGlue setSharedSparkleGlueForTesting:glue_];
  }

  void TearDownOnMainThread() override {
    [SparkleGlue setSharedSparkleGlueForTesting:nil];
    InProcessBrowserTest::TearDownOnMainThread();
  }

  // Sparkle took over, so upstream neither quits nor relaunches.
  void ExpectSparkleInstalled() {
    EXPECT_EQ(1, updater_.driver.installCount);
    EXPECT_TRUE(updater_.driver.lastRelaunch);
    EXPECT_FALSE(updater_.driver.lastShowUI);
    EXPECT_FALSE(browser_shutdown::IsTryingToQuit());
  }

  base::MockCallback<upgrade_util::RelaunchChromeBrowserCallback>
      mock_relaunch_callback_;
  SparkleGlue* __strong glue_;
  FakeSUUpdater* __strong updater_;

 private:
  upgrade_util::ScopedRelaunchChromeBrowserOverride relaunch_chrome_override_{
      mock_relaunch_callback_.Get()};
};

IN_PROC_BROWSER_TEST_F(RelaunchToUpdateSparkleBrowserTest,
                       UpgradeCommandInstallsPendingUpdate) {
  EXPECT_CALL(mock_relaunch_callback_, Run(testing::_)).Times(0);
  SimulateSparkleUpdateWillBeInstalledOnQuit(glue_, updater_);

  chrome::ExecuteCommand(browser(), IDC_UPGRADE_DIALOG);

  ExpectSparkleInstalled();
}

IN_PROC_BROWSER_TEST_F(RelaunchToUpdateSparkleBrowserTest,
                       AttemptRestartInstallsPendingUpdate) {
  EXPECT_CALL(mock_relaunch_callback_, Run(testing::_)).Times(0);
  SimulateSparkleUpdateWillBeInstalledOnQuit(glue_, updater_);

  chrome::AttemptRestart();

  ExpectSparkleInstalled();
}

IN_PROC_BROWSER_TEST_F(RelaunchToUpdateSparkleBrowserTest,
                       RelaunchIgnoreUnloadHandlersInstallsPendingUpdate) {
  EXPECT_CALL(mock_relaunch_callback_, Run(testing::_)).Times(0);
  SimulateSparkleUpdateWillBeInstalledOnQuit(glue_, updater_);

  chrome::RelaunchIgnoreUnloadHandlers();

  ExpectSparkleInstalled();
}

// Without a pending update, Sparkle has nothing to install, so upstream
// relaunches as on other platforms.
IN_PROC_BROWSER_TEST_F(RelaunchToUpdateSparkleBrowserTest,
                       UpgradeCommandWithoutPendingUpdateRelaunches) {
  EXPECT_CALL(mock_relaunch_callback_, Run(testing::_))
      .WillOnce(testing::Return(true));
  [glue_ setUpdaterForTesting:updater_];

  ui_test_utils::BrowserDestroyedObserver observer(browser());
  chrome::ExecuteCommand(browser(), IDC_UPGRADE_DIALOG);

  EXPECT_TRUE(browser_shutdown::IsTryingToQuit());
  EXPECT_EQ(0, updater_.driver.installCount);
  observer.Wait();
}
