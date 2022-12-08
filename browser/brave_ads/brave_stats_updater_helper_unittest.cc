/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/brave_stats_updater_helper.h"

#include <memory>

#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_ads {

class BraveStatsUpdaterHelperTest : public testing::Test {
 public:
  BraveStatsUpdaterHelperTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());

    profile_one_ = profile_manager_.CreateTestingProfile("TestProfile1");
    AdsService::RegisterProfilePrefs(
        profile_one_->GetTestingPrefService()->registry());
    profile_one_->GetPrefs()->SetBoolean(ads::prefs::kEnabled, true);

    profile_two_ = profile_manager_.CreateTestingProfile("TestProfile2");
    AdsService::RegisterProfilePrefs(
        profile_two_->GetTestingPrefService()->registry());

    brave_stats_updater_helper_ = std::make_unique<BraveStatsUpdaterHelper>();
  }

  void TearDown() override { brave_stats_updater_helper_.release(); }

  TestingPrefServiceSimple* GetLocalState() {
    return profile_manager_.local_state()->Get();
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<BraveStatsUpdaterHelper> brave_stats_updater_helper_;
  TestingProfileManager profile_manager_;
  TestingProfile* profile_one_;
  TestingProfile* profile_two_;
};

#if !BUILDFLAG(IS_ANDROID)
TEST_F(BraveStatsUpdaterHelperTest, ProfileSwitch) {
  profile_manager_.UpdateLastUser(profile_one_);
  EXPECT_EQ(GetLocalState()->GetBoolean(ads::prefs::kEnabledForLastProfile),
            true);

  profile_manager_.UpdateLastUser(profile_two_);
  EXPECT_EQ(GetLocalState()->GetBoolean(ads::prefs::kEnabledForLastProfile),
            false);

  profile_manager_.UpdateLastUser(profile_one_);
  EXPECT_EQ(GetLocalState()->GetBoolean(ads::prefs::kEnabledForLastProfile),
            true);
}

TEST_F(BraveStatsUpdaterHelperTest, EnabledUpdate) {
  profile_manager_.UpdateLastUser(profile_one_);
  EXPECT_EQ(GetLocalState()->GetBoolean(ads::prefs::kEnabledForLastProfile),
            true);

  profile_two_->GetPrefs()->SetBoolean(ads::prefs::kEnabled, true);
  EXPECT_EQ(GetLocalState()->GetBoolean(ads::prefs::kEnabledForLastProfile),
            true);

  profile_one_->GetPrefs()->SetBoolean(ads::prefs::kEnabled, false);
  EXPECT_EQ(GetLocalState()->GetBoolean(ads::prefs::kEnabledForLastProfile),
            false);

  profile_manager_.UpdateLastUser(profile_two_);
  EXPECT_EQ(GetLocalState()->GetBoolean(ads::prefs::kEnabledForLastProfile),
            true);
}
#endif

}  // namespace brave_ads
