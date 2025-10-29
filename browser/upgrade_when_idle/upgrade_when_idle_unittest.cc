// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/upgrade_when_idle/upgrade_when_idle.h"

#include <initializer_list>
#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/browser/first_run/scoped_relaunch_chrome_browser_override.h"
#include "chrome/browser/first_run/upgrade_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/idle/scoped_set_idle_state.h"

namespace brave {

namespace {

class UpgradeWhenIdleForTest : public UpgradeWhenIdle {
 public:
  explicit UpgradeWhenIdleForTest(ProfileManager* profile_manager)
      : UpgradeWhenIdle(profile_manager) {}
  size_t GetBrowserWindowCount() override { return browser_window_count_; }
  void SetBrowserWindowCount(size_t browser_window_count) {
    browser_window_count_ = browser_window_count;
  }

 private:
  size_t browser_window_count_ = 0;
};

}  // namespace

class UpgradeWhenIdleTest : public testing::Test {
 public:
  UpgradeWhenIdleTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()),
        mock_relaunch_callback_(),
        relaunch_chrome_override_(mock_relaunch_callback_.Get()) {}

  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());
    upgrade_when_idle_ = std::make_unique<UpgradeWhenIdleForTest>(
        profile_manager_.profile_manager());
    profile_ = profile_manager_.CreateTestingProfile("TestProfile");
  }

  void TearDown() override { profile_ = nullptr; }

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

  void SimulateOpenBrowserWindow() {
    upgrade_when_idle_->SetBrowserWindowCount(1);
  }

  void SetPref(const std::string& pref_name) {
    profile_->GetPrefs()->SetBoolean(pref_name, true);
  }

  void SetClearBrowsingDataOnExitList(
      std::initializer_list<const char*> data_types) {
    base::Value::List list;
    for (const char* data_type : data_types) {
      list.Append(data_type);
    }
    profile_->GetPrefs()->SetList(
        browsing_data::prefs::kClearBrowsingDataOnExitList, std::move(list));
  }

 private:
  content::BrowserTaskEnvironment task_environment_{
      content::BrowserTaskEnvironment::TimeSource::MOCK_TIME};
  TestingProfileManager profile_manager_;
  std::unique_ptr<UpgradeWhenIdleForTest> upgrade_when_idle_;
  ::testing::StrictMock<
      base::MockCallback<upgrade_util::RelaunchChromeBrowserCallback>>
      mock_relaunch_callback_;
  upgrade_util::ScopedRelaunchChromeBrowserOverride relaunch_chrome_override_;
  raw_ptr<TestingProfile> profile_;
};

TEST_F(UpgradeWhenIdleTest, UpgradeWhenIdle) {
  RunImplementation(ui::IDLE_STATE_IDLE, /*expect_upgrade=*/true);
}

TEST_F(UpgradeWhenIdleTest, UpgradeWhenLocked) {
  RunImplementation(ui::IDLE_STATE_LOCKED, /*expect_upgrade=*/true);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenActive) {
  RunImplementation(ui::IDLE_STATE_ACTIVE, /*expect_upgrade=*/false);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenStateUnknown) {
  RunImplementation(ui::IDLE_STATE_UNKNOWN, /*expect_upgrade=*/false);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenOpenWindows) {
  SimulateOpenBrowserWindow();
  RunImplementation(ui::IDLE_STATE_IDLE, /*expect_upgrade=*/false);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenDeleteBrowsingHistoryOnExit) {
  SetPref(browsing_data::prefs::kDeleteBrowsingHistoryOnExit);
  RunImplementation(ui::IDLE_STATE_IDLE, /*expect_upgrade=*/false);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenDeleteDownloadHistoryOnExit) {
  SetPref(browsing_data::prefs::kDeleteDownloadHistoryOnExit);
  RunImplementation(ui::IDLE_STATE_IDLE, /*expect_upgrade=*/false);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenDeleteCacheOnExit) {
  SetPref(browsing_data::prefs::kDeleteCacheOnExit);
  RunImplementation(ui::IDLE_STATE_IDLE, /*expect_upgrade=*/false);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenDeleteCookiesOnExit) {
  SetPref(browsing_data::prefs::kDeleteCookiesOnExit);
  RunImplementation(ui::IDLE_STATE_IDLE, /*expect_upgrade=*/false);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenDeletePasswordsOnExit) {
  SetPref(browsing_data::prefs::kDeletePasswordsOnExit);
  RunImplementation(ui::IDLE_STATE_IDLE, /*expect_upgrade=*/false);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenDeleteFormDataOnExit) {
  SetPref(browsing_data::prefs::kDeleteFormDataOnExit);
  RunImplementation(ui::IDLE_STATE_IDLE, /*expect_upgrade=*/false);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenDeleteHostedAppsDataOnExit) {
  SetPref(browsing_data::prefs::kDeleteHostedAppsDataOnExit);
  RunImplementation(ui::IDLE_STATE_IDLE, /*expect_upgrade=*/false);
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenDeleteSiteSettingsOnExit) {
  SetPref(browsing_data::prefs::kDeleteSiteSettingsOnExit);
  RunImplementation(ui::IDLE_STATE_IDLE, /*expect_upgrade=*/false);
}

#if BUILDFLAG(ENABLE_AI_CHAT)
TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenDeleteBraveLeoHistoryOnExit) {
  SetPref(browsing_data::prefs::kDeleteBraveLeoHistoryOnExit);
  RunImplementation(ui::IDLE_STATE_IDLE, /*expect_upgrade=*/false);
}
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenClearBrowsingDataOnExitList) {
  SetClearBrowsingDataOnExitList(
      {"browsing_history", "cached_images_and_files"});
  RunImplementation(ui::IDLE_STATE_IDLE, /*expect_upgrade=*/false);
}

}  // namespace brave
