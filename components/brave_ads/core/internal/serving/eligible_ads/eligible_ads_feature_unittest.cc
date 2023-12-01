/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/eligible_ads_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsEligibleAdFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kEligibleAdFeature));
}

TEST(BraveAdsEligibleAdFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kEligibleAdFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kEligibleAdFeature));
}

TEST(BraveAdsEligibleAdFeatureTest, BrowsingHistoryMaxCount) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kEligibleAdFeature, {{"browsing_history_max_count", "666"}});

  // Act & Assert
  EXPECT_EQ(666, kBrowsingHistoryMaxCount.Get());
}

TEST(BraveAdsEligibleAdFeatureTest, DefaultBrowsingHistoryMaxCount) {
  // Act & Assert
  EXPECT_EQ(5'000, kBrowsingHistoryMaxCount.Get());
}

TEST(BraveAdsEligibleAdFeatureTest,
     DefaultBrowsingHistoryMaxCountWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kEligibleAdFeature);

  // Act & Assert
  EXPECT_EQ(5'000, kBrowsingHistoryMaxCount.Get());
}

TEST(BraveAdsEligibleAdFeatureTest, BrowsingHistoryRecentDayRange) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kEligibleAdFeature, {{"browsing_history_recent_day_range", "7"}});

  // Act & Assert
  EXPECT_EQ(7, kBrowsingHistoryRecentDayRange.Get());
}

TEST(BraveAdsEligibleAdFeatureTest, DefaultBrowsingHistoryRecentDayRange) {
  // Act & Assert
  EXPECT_EQ(180, kBrowsingHistoryRecentDayRange.Get());
}

TEST(BraveAdsEligibleAdFeatureTest,
     DefaultBrowsingHistoryRecentDayRangeWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kEligibleAdFeature);

  // Act & Assert
  EXPECT_EQ(180, kBrowsingHistoryRecentDayRange.Get());
}

}  // namespace brave_ads
