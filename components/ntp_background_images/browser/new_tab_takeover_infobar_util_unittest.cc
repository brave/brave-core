/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/new_tab_takeover_infobar_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_rewards/core/pref_registry.h"
#include "brave/components/ntp_background_images/common/pref_constants.h"
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

  void EnableSupportNewTabPageAdConfirmationsForNonRewards() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{brave_ads::kNewTabPageAdFeature,
          {{"should_support_confirmations_for_non_rewards", "true"}}}},
        {});
  }

  void DisableSupportNewTabPageAdConfirmationsForNonRewards() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{brave_ads::kNewTabPageAdFeature,
          {{"should_support_confirmations_for_non_rewards", "false"}}}},
        {});
  }

  void EnableRewards() {
    pref_service()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  }

  void DisableRewards() {
    pref_service()->SetBoolean(brave_rewards::prefs::kEnabled, false);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(
    NewTabTakeoverInfobarUtilTest,
    DoNotShowInfobarWhenRewardsDisabledAndSupportNewTabPageAdConfirmationsForNonRewardsDisabled) {
  DisableRewards();
  DisableSupportNewTabPageAdConfirmationsForNonRewards();

  EXPECT_FALSE(ShouldShowNewTabTakeoverInfobar(pref_service()));
}

TEST_F(
    NewTabTakeoverInfobarUtilTest,
    DoNotShowInfobarWhenRewardsEnabledAndSupportNewTabPageAdConfirmationsForNonRewardsDisabled) {
  EnableRewards();
  DisableSupportNewTabPageAdConfirmationsForNonRewards();

  EXPECT_FALSE(ShouldShowNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest, ShowNewTabTakeoverInfobarUntilThreshold) {
  DisableRewards();
  EnableSupportNewTabPageAdConfirmationsForNonRewards();

  for (int i = 0; i < kNewTabTakeoverInfobarShowCountThreshold; ++i) {
    EXPECT_TRUE(ShouldShowNewTabTakeoverInfobar(pref_service()));
    RecordNewTabTakeoverInfobarWasShown(pref_service());
  }

  EXPECT_FALSE(ShouldShowNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest,
       ShouldNotShowNewTabTakeoverInfobarWhenRewardsEnabled) {
  EnableRewards();
  EnableSupportNewTabPageAdConfirmationsForNonRewards();

  EXPECT_FALSE(ShouldShowNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest,
       ShouldNotShowNewTabTakeoverInfobarWhenThresholdIsReached) {
  DisableRewards();
  EnableSupportNewTabPageAdConfirmationsForNonRewards();

  pref_service()->SetInteger(prefs::kNewTabTakeoverInfobarShowCount, 0);

  EXPECT_FALSE(ShouldShowNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest, SuppressNewTabTakeoverInfobar) {
  DisableRewards();
  EnableSupportNewTabPageAdConfirmationsForNonRewards();

  RecordNewTabTakeoverInfobarWasShown(pref_service());
  EXPECT_TRUE(ShouldShowNewTabTakeoverInfobar(pref_service()));

  SuppressNewTabTakeoverInfobar(pref_service());
  EXPECT_FALSE(ShouldShowNewTabTakeoverInfobar(pref_service()));
}

}  // namespace ntp_background_images
