/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/idle_time.h"

#include <limits>
#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/features/user_activity/user_activity_features.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsIdleTimeTest : public UnitTestBase {
 protected:
  BatAdsIdleTimeTest() = default;

  ~BatAdsIdleTimeTest() override = default;
};

TEST_F(BatAdsIdleTimeTest, WasLocked) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kShouldDetectWasLockedParameter[] = "should_detect_was_locked";
  parameters[kShouldDetectWasLockedParameter] = "true";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool was_locked = WasLocked(true);

  // Assert
  EXPECT_TRUE(was_locked);
}

TEST_F(BatAdsIdleTimeTest, WasLockedIfShouldDetectWasLocked) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kShouldDetectWasLockedParameter[] = "should_detect_was_locked";
  parameters[kShouldDetectWasLockedParameter] = "true";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool was_locked = WasLocked(true);

  // Assert
  EXPECT_TRUE(was_locked);
}

TEST_F(BatAdsIdleTimeTest, WasNotLocked) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kShouldDetectWasLockedParameter[] = "should_detect_was_locked";
  parameters[kShouldDetectWasLockedParameter] = "true";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool was_locked = WasLocked(false);

  // Assert
  EXPECT_FALSE(was_locked);
}

TEST_F(BatAdsIdleTimeTest, WasNotLockedIfShouldNotDetectWasLocked) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kShouldDetectWasLockedParameter[] = "should_detect_was_locked";
  parameters[kShouldDetectWasLockedParameter] = "false";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool was_locked = WasLocked(true);

  // Assert
  EXPECT_FALSE(was_locked);
}

TEST_F(BatAdsIdleTimeTest, HasNotExceededMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kMaximumIdleTimeParameter[] = "maximum_idle_time";
  parameters[kMaximumIdleTimeParameter] = "10s";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool has_exceeded_maximum_idle_time = HasExceededMaximumIdleTime(10);

  // Assert
  EXPECT_FALSE(has_exceeded_maximum_idle_time);
}

TEST_F(BatAdsIdleTimeTest, HasNotExceededInfiniteMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kMaximumIdleTimeParameter[] = "maximum_idle_time";
  parameters[kMaximumIdleTimeParameter] = "0s";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool has_exceeded_maximum_idle_time =
      HasExceededMaximumIdleTime(std::numeric_limits<int>::max());

  // Assert
  EXPECT_FALSE(has_exceeded_maximum_idle_time);
}

TEST_F(BatAdsIdleTimeTest, HasExceededMaximumIdleTime) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kMaximumIdleTimeParameter[] = "maximum_idle_time";
  parameters[kMaximumIdleTimeParameter] = "10s";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool has_exceeded_maximum_idle_time = HasExceededMaximumIdleTime(11);

  // Assert
  EXPECT_TRUE(has_exceeded_maximum_idle_time);
}

TEST_F(BatAdsIdleTimeTest, UpdateIdleTimeThreshold) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kIdleTimeThresholdParameter[] = "idle_time_threshold";
  parameters[kIdleTimeThresholdParameter] = "5s";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  AdsClientHelper::Get()->SetIntegerPref(prefs::kIdleTimeThreshold, 10);

  ASSERT_TRUE(MaybeUpdateIdleTimeThreshold());

  // Act
  const int idle_time_threshold =
      AdsClientHelper::Get()->GetIntegerPref(prefs::kIdleTimeThreshold);

  // Assert
  const int expected_idle_time_threshold = 5;
  EXPECT_EQ(expected_idle_time_threshold, idle_time_threshold);
}

TEST_F(BatAdsIdleTimeTest, DoNotUpdateIdleTimeThreshold) {
  // Arrange
  base::FieldTrialParams parameters;
  const char kIdleTimeThresholdParameter[] = "idle_time_threshold";
  parameters[kIdleTimeThresholdParameter] = "10s";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back({features::user_activity::kFeature, parameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  AdsClientHelper::Get()->SetIntegerPref(prefs::kIdleTimeThreshold, 10);

  ASSERT_FALSE(MaybeUpdateIdleTimeThreshold());

  // Act
  const int idle_time_threshold =
      AdsClientHelper::Get()->GetIntegerPref(prefs::kIdleTimeThreshold);

  // Assert
  const int expected_idle_time_threshold = 10;
  EXPECT_EQ(expected_idle_time_threshold, idle_time_threshold);
}

}  // namespace ads
