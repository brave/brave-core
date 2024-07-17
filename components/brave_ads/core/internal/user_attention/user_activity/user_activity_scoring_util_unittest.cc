/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_scoring_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_feature.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsUserActivityScoringUtilTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kUserActivityFeature, {{"triggers", "0D=1.0;08=1.0"},
                               {"time_window", "1h"},
                               {"threshold", "2.0"}});
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(BraveAdsUserActivityScoringUtilTest, WasUserActive) {
  // Arrange
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClosedTab);

  // Act & Assert
  EXPECT_TRUE(WasUserActive());
}

TEST_F(BraveAdsUserActivityScoringUtilTest, WasUserInactive) {
  // Act & Assert
  EXPECT_FALSE(WasUserActive());
}

TEST_F(BraveAdsUserActivityScoringUtilTest, WasUserInactiveIfBelowThreshold) {
  // Arrange
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  // Act & Assert
  EXPECT_FALSE(WasUserActive());
}

TEST_F(BraveAdsUserActivityScoringUtilTest,
       WasUserInactiveAfterTimeWindowHasElapsed) {
  // Arrange
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClosedTab);

  AdvanceClockBy(kUserActivityTimeWindow.Get() + base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(WasUserActive());
}

}  // namespace brave_ads
