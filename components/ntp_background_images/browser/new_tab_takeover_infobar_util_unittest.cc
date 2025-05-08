/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/new_tab_takeover_infobar_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_rewards/core/pref_registry.h"
#include "brave/components/ntp_background_images/common/infobar_constants.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

class NewTabTakeoverInfobarUtilTest : public testing::Test {
 public:
  NewTabTakeoverInfobarUtilTest() {
    RegisterProfilePrefs(pref_service_.registry());
    brave_rewards::RegisterProfilePrefs(pref_service_.registry());
  }

  ~NewTabTakeoverInfobarUtilTest() override = default;

  PrefService* pref_service() { return &pref_service_; }

  void SetShouldSupportConfirmationsForNonRewardsFeatureEnabled(bool enabled) {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{brave_ads::kNewTabPageAdFeature,
          {{"should_support_confirmations_for_non_rewards",
            enabled ? "true" : "false"}}}},
        {});
  }

  void SetRewardsEnabled(bool enabled) {
    pref_service()->SetBoolean(brave_rewards::prefs::kEnabled, enabled);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(NewTabTakeoverInfobarUtilTest, ShouldDisplayInfobar) {
  SetRewardsEnabled(/*enabled=*/false);
  SetShouldSupportConfirmationsForNonRewardsFeatureEnabled(/*enabled=*/true);

  for (int i = 0; i < kNewTabTakeoverInfobarRemainingDisplayCountThreshold;
       ++i) {
    EXPECT_TRUE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
    RecordNewTabTakeoverInfobarWasDisplayed(pref_service());
  }

  EXPECT_FALSE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

TEST_F(
    NewTabTakeoverInfobarUtilTest,
    ShouldNotDisplayInfobarIfShouldSupportConfirmationsForNonRewardsFeatureIsDisabled) {
  SetRewardsEnabled(/*enabled=*/false);
  SetShouldSupportConfirmationsForNonRewardsFeatureEnabled(/*enabled=*/false);

  EXPECT_FALSE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

TEST_F(
    NewTabTakeoverInfobarUtilTest,
    DoNotShowInfobarWhenRewardsEnabledAndSupportNewTabPageAdConfirmationsForNonRewardsDisabled) {
  SetRewardsEnabled(/*enabled=*/true);
  SetShouldSupportConfirmationsForNonRewardsFeatureEnabled(/*enabled=*/false);

  EXPECT_FALSE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest,
       ShouldNotDisplayInfobarIfRewardsIsEnabled) {
  SetRewardsEnabled(/*enabled=*/true);
  SetShouldSupportConfirmationsForNonRewardsFeatureEnabled(/*enabled=*/true);

  EXPECT_FALSE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest,
       ShouldNotDisplayInfobarWhenThresholdIsMet) {
  SetRewardsEnabled(/*enabled=*/false);
  SetShouldSupportConfirmationsForNonRewardsFeatureEnabled(/*enabled=*/true);

  pref_service()->SetInteger(prefs::kNewTabTakeoverInfobarRemainingDisplayCount,
                             0);

  EXPECT_FALSE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest,
       ShouldNotDisplayInfobarWhenThresholdIsExceeded) {
  SetRewardsEnabled(/*enabled=*/false);
  SetShouldSupportConfirmationsForNonRewardsFeatureEnabled(/*enabled=*/true);

  pref_service()->SetInteger(prefs::kNewTabTakeoverInfobarRemainingDisplayCount,
                             -1);

  EXPECT_FALSE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest, RecordInfobarWasDisplayed) {
  SetRewardsEnabled(/*enabled=*/false);
  SetShouldSupportConfirmationsForNonRewardsFeatureEnabled(/*enabled=*/true);

  RecordNewTabTakeoverInfobarWasDisplayed(pref_service());
  EXPECT_TRUE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest, SuppressNewTabTakeoverInfobar) {
  SetRewardsEnabled(/*enabled=*/false);
  SetShouldSupportConfirmationsForNonRewardsFeatureEnabled(/*enabled=*/true);

  SuppressNewTabTakeoverInfobar(pref_service());
  EXPECT_FALSE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

}  // namespace ntp_background_images
