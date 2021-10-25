/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity_scoring_util.h"

#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/features/user_activity/user_activity_features.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/internal/user_activity/user_activity.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUserActivityScoringUtilTest : public UnitTestBase {
 protected:
  BatAdsUserActivityScoringUtilTest() = default;

  ~BatAdsUserActivityScoringUtilTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    base::FieldTrialParams parameters;
    const char kTriggersParameter[] = "triggers";
    parameters[kTriggersParameter] = "0D=1.0;08=1.0";
    const char kTimeWindowParameter[] = "time_window";
    parameters[kTimeWindowParameter] = "1h";
    const char kThresholdParameter[] = "threshold";
    parameters[kThresholdParameter] = "2.0";
    std::vector<base::test::ScopedFeatureList::FeatureAndParams>
        enabled_features;
    enabled_features.push_back({features::user_activity::kFeature, parameters});

    const std::vector<base::Feature> disabled_features;

    scoped_feature_list_.InitWithFeaturesAndParameters(enabled_features,
                                                       disabled_features);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(BatAdsUserActivityScoringUtilTest, WasUserActive) {
  // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);

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
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);

  // Act
  const bool was_user_active = WasUserActive();

  // Assert
  EXPECT_FALSE(was_user_active);
}

TEST_F(BatAdsUserActivityScoringUtilTest,
       WasUserInactiveAfterTimeWindowHasElapsed) {
  // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);

  const base::TimeDelta elapsed_time_window =
      features::user_activity::GetTimeWindow() + base::Seconds(1);
  AdvanceClock(elapsed_time_window);

  // Act
  const bool was_user_active = WasUserActive();

  // Assert
  EXPECT_FALSE(was_user_active);
}

}  // namespace ads
