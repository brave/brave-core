/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/new_tab_takeover_infobar_util.h"

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

  void SetRewardsEnabled(bool enabled) {
    pref_service()->SetBoolean(brave_rewards::prefs::kEnabled, enabled);
  }

 private:
  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(NewTabTakeoverInfobarUtilTest, ShouldDisplayInfobar) {
  SetRewardsEnabled(/*enabled=*/false);

  for (int i = 0; i < kNewTabTakeoverInfobarRemainingDisplayCountThreshold;
       ++i) {
    EXPECT_TRUE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
    RecordNewTabTakeoverInfobarWasDisplayed(pref_service());
  }

  EXPECT_FALSE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest,
       ShouldNotDisplayInfobarIfRewardsIsEnabled) {
  SetRewardsEnabled(/*enabled=*/true);

  EXPECT_FALSE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest,
       ShouldNotDisplayInfobarWhenThresholdIsMet) {
  SetRewardsEnabled(/*enabled=*/false);

  pref_service()->SetInteger(prefs::kNewTabTakeoverInfobarRemainingDisplayCount,
                             0);

  EXPECT_FALSE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest,
       ShouldNotDisplayInfobarWhenThresholdIsExceeded) {
  SetRewardsEnabled(/*enabled=*/false);

  pref_service()->SetInteger(prefs::kNewTabTakeoverInfobarRemainingDisplayCount,
                             -1);

  EXPECT_FALSE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest, RecordInfobarWasDisplayed) {
  SetRewardsEnabled(/*enabled=*/false);

  RecordNewTabTakeoverInfobarWasDisplayed(pref_service());
  EXPECT_TRUE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

TEST_F(NewTabTakeoverInfobarUtilTest, SuppressNewTabTakeoverInfobar) {
  SetRewardsEnabled(/*enabled=*/false);

  SuppressNewTabTakeoverInfobar(pref_service());
  EXPECT_FALSE(ShouldDisplayNewTabTakeoverInfobar(pref_service()));
}

}  // namespace ntp_background_images
