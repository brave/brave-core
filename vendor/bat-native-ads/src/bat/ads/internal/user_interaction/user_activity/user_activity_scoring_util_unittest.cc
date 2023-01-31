/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/user_activity/user_activity_scoring_util.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_features.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_manager.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUserActivityScoringUtilTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    base::FieldTrialParams params;
    params["triggers"] = "0D=1.0;08=1.0";
    params["time_window"] = "1h";
    params["threshold"] = "2.0";
    std::vector<base::test::FeatureRefAndParams> enabled_features;
    enabled_features.emplace_back(user_activity::features::kFeature, params);

    const std::vector<base::test::FeatureRef> disabled_features;

    scoped_feature_list_.InitWithFeaturesAndParameters(enabled_features,
                                                       disabled_features);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(BatAdsUserActivityScoringUtilTest, WasUserActive) {
  // Arrange
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClosedTab);

  // Act
  const bool was_user_active = WasUserActive();

  // Assert
  EXPECT_TRUE(was_user_active);
}

TEST_F(BatAdsUserActivityScoringUtilTest, WasUserInactive) {
  // Arrange

  // Act
  const bool was_user_active = WasUserActive();

  // Assert
  EXPECT_FALSE(was_user_active);
}

TEST_F(BatAdsUserActivityScoringUtilTest, WasUserInactiveIfBelowThreshold) {
  // Arrange
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  // Act
  const bool was_user_active = WasUserActive();

  // Assert
  EXPECT_FALSE(was_user_active);
}

TEST_F(BatAdsUserActivityScoringUtilTest,
       WasUserInactiveAfterTimeWindowHasElapsed) {
  // Arrange
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClosedTab);

  const base::TimeDelta elapsed_time_window =
      user_activity::features::GetTimeWindow() + base::Seconds(1);
  AdvanceClockBy(elapsed_time_window);

  // Act
  const bool was_user_active = WasUserActive();

  // Assert
  EXPECT_FALSE(was_user_active);
}

}  // namespace ads
