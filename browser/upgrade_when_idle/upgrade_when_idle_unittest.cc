// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/upgrade_when_idle/upgrade_when_idle.h"

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "chrome/browser/first_run/scoped_relaunch_chrome_browser_override.h"
#include "chrome/browser/first_run/upgrade_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/idle/scoped_set_idle_state.h"

namespace brave {

class UpgradeWhenIdleTest : public testing::Test {
 public:
  UpgradeWhenIdleTest()
      : mock_relaunch_callback_(),
        relaunch_chrome_override_(mock_relaunch_callback_.Get()),
        profile_manager_(TestingBrowserProcess::GetGlobal()) {}

  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());
    profile_ = profile_manager_.CreateTestingProfile("TestProfile");
    upgrade_when_idle_ = std::make_unique<UpgradeWhenIdle>();
  }

  void TearDown() override {
    upgrade_when_idle_.reset();
    profile_ = nullptr;
  }

 protected:
  void RunImplementation(ui::IdleState state, bool expect_upgrade) {
    base::RunLoop run_loop;
    upgrade_when_idle_->SetCheckIdleCallbackForTesting(run_loop.QuitClosure());
    if (expect_upgrade) {
      EXPECT_CALL(mock_relaunch_callback_, Run);
    }
    ui::ScopedSetIdleState scoped_set_idle_state(state);
    upgrade_when_idle_->OnUpgradeRecommended();
    task_environment_.FastForwardBy(base::Minutes(3));
    run_loop.Run();
  }

  std::unique_ptr<Browser> CreateTestBrowser() {
    Browser::CreateParams params(profile_.get(), true);
    return CreateBrowserWithTestWindowForParams(params);
  }

 private:
  std::unique_ptr<UpgradeWhenIdle> upgrade_when_idle_;
  content::BrowserTaskEnvironment task_environment_{
      content::BrowserTaskEnvironment::TimeSource::MOCK_TIME};
  ::testing::StrictMock<
      base::MockCallback<upgrade_util::RelaunchChromeBrowserCallback>>
      mock_relaunch_callback_;
  upgrade_util::ScopedRelaunchChromeBrowserOverride relaunch_chrome_override_;
  TestingProfileManager profile_manager_;
  raw_ptr<TestingProfile> profile_;
};

TEST_F(UpgradeWhenIdleTest, UpgradeWhenIdle) {
  RunImplementation(ui::IDLE_STATE_IDLE, true);
}

TEST_F(UpgradeWhenIdleTest, UpgradeWhenLocked) {
  RunImplementation(ui::IDLE_STATE_LOCKED, true);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenActive) {
  RunImplementation(ui::IDLE_STATE_ACTIVE, false);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenStateUnknown) {
  RunImplementation(ui::IDLE_STATE_UNKNOWN, false);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenOpenWindows) {
  std::unique_ptr<Browser> test_browser = CreateTestBrowser();
  RunImplementation(ui::IDLE_STATE_IDLE, false);
}

}  // namespace brave
