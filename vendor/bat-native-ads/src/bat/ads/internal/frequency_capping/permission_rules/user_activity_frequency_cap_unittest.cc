/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/user_activity_frequency_cap.h"

#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/internal/user_activity/user_activity_features.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUserActivityFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsUserActivityFrequencyCapTest() = default;

  ~BatAdsUserActivityFrequencyCapTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    base::FieldTrialParams parameters;
    const char kTriggersParameter[] = "triggers";
    parameters[kTriggersParameter] = "0D=1.0;0E=1.0;08=1.0";
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

TEST_F(BatAdsUserActivityFrequencyCapTest,
       AllowAdIfUserActivityScoreIsEqualToTheThreshold) {
  // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);

  // Act
  UserActivityFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsUserActivityFrequencyCapTest,
       AllowAdIfUserActivityScoreIsGreaterThanTheThreshold) {
  // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kPlayedMedia);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);

  // Act
  UserActivityFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsUserActivityFrequencyCapTest,
       DoNotAllowAdIfUserActivityScoreIsLessThanTheThreshold) {
  // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);

  // Act
  UserActivityFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
