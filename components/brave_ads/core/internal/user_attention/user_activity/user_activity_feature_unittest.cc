/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsUserActivityFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kUserActivityFeature));
}

TEST(BraveAdsUserActivityFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUserActivityFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kUserActivityFeature));
}

TEST(BraveAdsUserActivityFeatureTest, MaximumUserActivityEvents) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserActivityFeature, {{"maximum_events", "7"}});

  // Act & Assert
  EXPECT_EQ(7, kMaximumUserActivityEvents.Get());
}

TEST(BraveAdsUserActivityFeatureTest, DefaultMaximumUserActivityEvents) {
  // Act & Assert
  EXPECT_EQ(3600, kMaximumUserActivityEvents.Get());
}

TEST(BraveAdsUserActivityFeatureTest,
     DefaultMaximumUserActivityEventsWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUserActivityFeature);

  // Act & Assert
  EXPECT_EQ(3600, kMaximumUserActivityEvents.Get());
}

TEST(BraveAdsUserActivityFeatureTest, UserActivityTriggers) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserActivityFeature, {{"triggers", "01=0.5;010203=1.0;0203=0.75"}});

  // Act & Assert
  EXPECT_EQ("01=0.5;010203=1.0;0203=0.75", kUserActivityTriggers.Get());
}

TEST(BraveAdsUserActivityFeatureTest, DefaultUserActivityTriggers) {
  // Act & Assert
  EXPECT_EQ(
      "0D0B14110D0B14110D0B14110D0B1411=-1.0;0D0B1411070707=-1.0;07070707=-1.0",
      kUserActivityTriggers.Get());
}

TEST(BraveAdsUserActivityFeatureTest, DefaultUserActivityTriggersWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUserActivityFeature);

  // Act & Assert
  EXPECT_EQ(
      "0D0B14110D0B14110D0B14110D0B1411=-1.0;0D0B1411070707=-1.0;07070707=-1.0",
      kUserActivityTriggers.Get());
}

TEST(BraveAdsUserActivityFeatureTest, UserActivityTimeWindow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserActivityFeature, {{"time_window", "2h"}});

  // Act & Assert
  EXPECT_EQ(base::Hours(2), kUserActivityTimeWindow.Get());
}

TEST(BraveAdsUserActivityFeatureTest, DefaultUserActivityTimeWindow) {
  // Act & Assert
  EXPECT_EQ(base::Minutes(15), kUserActivityTimeWindow.Get());
}

TEST(BraveAdsUserActivityFeatureTest,
     DefaultUserActivityTimeWindowWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUserActivityFeature);

  // Act & Assert
  EXPECT_EQ(base::Minutes(15), kUserActivityTimeWindow.Get());
}

TEST(BraveAdsUserActivityFeatureTest, UserActivityThreshold) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kUserActivityFeature, {{"threshold", "7.0"}});

  // Act & Assert
  EXPECT_EQ(7.0, kUserActivityThreshold.Get());
}

TEST(BraveAdsUserActivityFeatureTest, DefaultUserActivityThreshold) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kUserActivityThreshold.Get());
}

TEST(BraveAdsUserActivityFeatureTest,
     DefaultUserActivityThresholdWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kUserActivityFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kUserActivityThreshold.Get());
}

}  // namespace brave_ads
