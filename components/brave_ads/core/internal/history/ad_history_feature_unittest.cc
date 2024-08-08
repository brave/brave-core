/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_history_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAdHistoryFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kAdHistoryFeature));
}

TEST(BraveAdsAdHistoryFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAdHistoryFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAdHistoryFeature));
}

TEST(BraveAdsAdHistoryFeatureTest, AdHistoryTimeWindow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdHistoryFeature, {{"retention_period", "1d"}});

  // Act & Assert
  EXPECT_EQ(base::Days(1), kAdHistoryRetentionPeriod.Get());
}

TEST(BraveAdsAdHistoryFeatureTest, DefaultAdHistoryTimeWindow) {
  // Act & Assert
  EXPECT_EQ(base::Days(30), kAdHistoryRetentionPeriod.Get());
}

TEST(BraveAdsAdHistoryFeatureTest, DefaultHistoryTimeWindowWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAdHistoryFeature);

  // Act & Assert
  EXPECT_EQ(base::Days(30), kAdHistoryRetentionPeriod.Get());
}

}  // namespace brave_ads
