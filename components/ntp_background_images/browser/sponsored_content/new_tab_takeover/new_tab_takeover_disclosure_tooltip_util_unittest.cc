/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/sponsored_content/new_tab_takeover/new_tab_takeover_disclosure_tooltip_util.h"

#include <cstddef>

#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_rewards/core/pref_registry.h"
#include "brave/components/ntp_background_images/common/new_tab_takeover_disclosure_constants.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

class NewTabTakeoverDisclosureTooltipUtilTest : public testing::Test {
 public:
  NewTabTakeoverDisclosureTooltipUtilTest() {
    RegisterProfilePrefs(pref_service_.registry());
    brave_rewards::RegisterProfilePrefs(pref_service_.registry());
  }

  ~NewTabTakeoverDisclosureTooltipUtilTest() override = default;

  PrefService* pref_service() { return &pref_service_; }

  void SetRewardsEnabled(bool enabled) {
    pref_service()->SetBoolean(brave_rewards::prefs::kEnabled, enabled);
  }

 private:
  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(NewTabTakeoverDisclosureTooltipUtilTest,
       AutoDisplaysTheTooltipUntilTheDisplayCountIsExhausted) {
  SetRewardsEnabled(/*enabled=*/false);

  for (size_t i = 0;
       i < kNewTabTakeoverDisclosureTooltipRemainingDisplayCountThreshold;
       ++i) {
    EXPECT_TRUE(
        ShouldAutoDisplayNewTabTakeoverDisclosureTooltip(pref_service()));
    RecordNewTabTakeoverDisclosureTooltipWasDisplayed(pref_service());
  }

  EXPECT_FALSE(
      ShouldAutoDisplayNewTabTakeoverDisclosureTooltip(pref_service()));
}

TEST_F(NewTabTakeoverDisclosureTooltipUtilTest,
       DoesNotAutoDisplayTheTooltipWhenRewardsIsEnabled) {
  SetRewardsEnabled(/*enabled=*/true);

  EXPECT_FALSE(
      ShouldAutoDisplayNewTabTakeoverDisclosureTooltip(pref_service()));
}

TEST_F(NewTabTakeoverDisclosureTooltipUtilTest,
       DoesNotAutoDisplayTheTooltipWhenTheRemainingCountIsNegative) {
  SetRewardsEnabled(/*enabled=*/false);

  pref_service()->SetInteger(
      prefs::kNewTabTakeoverDisclosureTooltipRemainingDisplayCount, -1);

  EXPECT_FALSE(
      ShouldAutoDisplayNewTabTakeoverDisclosureTooltip(pref_service()));
}

TEST_F(NewTabTakeoverDisclosureTooltipUtilTest,
       RecordingADisplayDecrementsTheRemainingCount) {
  SetRewardsEnabled(/*enabled=*/false);

  RecordNewTabTakeoverDisclosureTooltipWasDisplayed(pref_service());

  EXPECT_TRUE(ShouldAutoDisplayNewTabTakeoverDisclosureTooltip(pref_service()));
}

TEST_F(NewTabTakeoverDisclosureTooltipUtilTest,
       SuppressingTheTooltipStopsFurtherAutoDisplays) {
  SetRewardsEnabled(/*enabled=*/false);

  SuppressNewTabTakeoverDisclosureTooltip(pref_service());

  EXPECT_FALSE(
      ShouldAutoDisplayNewTabTakeoverDisclosureTooltip(pref_service()));
}

TEST_F(NewTabTakeoverDisclosureTooltipUtilTest,
       EnablingRewardsAfterPartialDisplaysStopsAutoDisplayAndKeepsTheCount) {
  SetRewardsEnabled(/*enabled=*/false);

  RecordNewTabTakeoverDisclosureTooltipWasDisplayed(pref_service());
  RecordNewTabTakeoverDisclosureTooltipWasDisplayed(pref_service());

  SetRewardsEnabled(/*enabled=*/true);

  EXPECT_FALSE(
      ShouldAutoDisplayNewTabTakeoverDisclosureTooltip(pref_service()));
  EXPECT_EQ(kNewTabTakeoverDisclosureTooltipRemainingDisplayCountThreshold - 2,
            pref_service()->GetInteger(
                prefs::kNewTabTakeoverDisclosureTooltipRemainingDisplayCount));
}

TEST_F(NewTabTakeoverDisclosureTooltipUtilTest,
       RecordingTheLastDisplayDecrementsTheCountToZero) {
  SetRewardsEnabled(/*enabled=*/false);

  pref_service()->SetInteger(
      prefs::kNewTabTakeoverDisclosureTooltipRemainingDisplayCount, 1);

  RecordNewTabTakeoverDisclosureTooltipWasDisplayed(pref_service());

  EXPECT_FALSE(
      ShouldAutoDisplayNewTabTakeoverDisclosureTooltip(pref_service()));
  EXPECT_EQ(0,
            pref_service()->GetInteger(
                prefs::kNewTabTakeoverDisclosureTooltipRemainingDisplayCount));
}

TEST_F(NewTabTakeoverDisclosureTooltipUtilTest,
       RecordingADisplayWhenCountIsZeroGoesNegativeButStillDoesNotAutoDisplay) {
  SetRewardsEnabled(/*enabled=*/false);

  pref_service()->SetInteger(
      prefs::kNewTabTakeoverDisclosureTooltipRemainingDisplayCount, 0);

  RecordNewTabTakeoverDisclosureTooltipWasDisplayed(pref_service());

  EXPECT_EQ(-1,
            pref_service()->GetInteger(
                prefs::kNewTabTakeoverDisclosureTooltipRemainingDisplayCount));
  EXPECT_FALSE(
      ShouldAutoDisplayNewTabTakeoverDisclosureTooltip(pref_service()));
}

}  // namespace ntp_background_images
