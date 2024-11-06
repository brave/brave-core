/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/day_zero_browser_ui_expt/day_zero_browser_ui_expt_manager.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/p3a_service.h"
#include "brave/components/p3a/pref_names.h"
#include "chrome/browser/profiles/profile_manager_observer.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class DayZeroBrowserUIExptTest : public testing::Test,
                                 public testing::WithParamInterface<bool> {
 public:
  DayZeroBrowserUIExptTest()
      : testing_profile_manager_(TestingBrowserProcess::GetGlobal()) {}

  void SetUp() override {
    ASSERT_TRUE(testing_profile_manager_.SetUp());
    p3a::P3AService::RegisterPrefs(testing_local_state_.registry(), true);

    if (IsDayZeroEnabled()) {
      // base::WrapUnique for using private ctor.
      manager_ = base::WrapUnique(new DayZeroBrowserUIExptManager(
          g_browser_process->profile_manager(), &testing_local_state_));
    }
  }

  // Keeping below methods empty to add future features and test
  void CheckBrowserHasDayZeroUI(Profile* profile) {}
  void CheckBrowserHasOriginalUI(Profile* profile) {}

  bool IsDayZeroEnabled() { return GetParam(); }

  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple testing_local_state_;
  TestingProfileManager testing_profile_manager_;
  base::test::ScopedFeatureList feature_list_;
  std::unique_ptr<DayZeroBrowserUIExptManager> manager_;
};

TEST_P(DayZeroBrowserUIExptTest, PrefsTest) {
  // Create multiple profile and check UI's prefs values are updated based on
  // feature flag.
  auto* profile = testing_profile_manager_.CreateTestingProfile("TestProfile");
  auto* profile2 =
      testing_profile_manager_.CreateTestingProfile("TestProfile2");

  // Add check for DayZero prefs here
  if (IsDayZeroEnabled()) {
    CheckBrowserHasDayZeroUI(profile);
    CheckBrowserHasDayZeroUI(profile2);
  } else {
    CheckBrowserHasOriginalUI(profile);
    CheckBrowserHasOriginalUI(profile2);
  }

  // Disable p3a and check ui is back to original.
  testing_local_state_.SetBoolean(p3a::kP3AEnabled, false);
  CheckBrowserHasOriginalUI(profile);
  CheckBrowserHasOriginalUI(profile2);
}

INSTANTIATE_TEST_SUITE_P(DayZeroExpt,
                         DayZeroBrowserUIExptTest,
                         testing::Bool());
