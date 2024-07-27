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

TEST(BraveAdsEligibleAdFeatureTest, SiteHistoryMaxCount) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kEligibleAdFeature, {{"site_history_max_count", "666"}});

  // Act & Assert
  EXPECT_EQ(666, kSiteHistoryMaxCount.Get());
}

TEST(BraveAdsEligibleAdFeatureTest, DefaultSiteHistoryMaxCount) {
  // Act & Assert
  EXPECT_EQ(5'000, kSiteHistoryMaxCount.Get());
}

TEST(BraveAdsEligibleAdFeatureTest, DefaultSiteHistoryMaxCountWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kEligibleAdFeature);

  // Act & Assert
  EXPECT_EQ(5'000, kSiteHistoryMaxCount.Get());
}

TEST(BraveAdsEligibleAdFeatureTest, SiteHistoryRecentDayRange) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kEligibleAdFeature, {{"site_history_recent_day_range", "7"}});

  // Act & Assert
  EXPECT_EQ(7, kSiteHistoryRecentDayRange.Get());
}

TEST(BraveAdsEligibleAdFeatureTest, DefaultSiteHistoryRecentDayRange) {
  // Act & Assert
  EXPECT_EQ(180, kSiteHistoryRecentDayRange.Get());
}

TEST(BraveAdsEligibleAdFeatureTest,
     DefaultSiteHistoryRecentDayRangeWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kEligibleAdFeature);

  // Act & Assert
  EXPECT_EQ(180, kSiteHistoryRecentDayRange.Get());
}

}  // namespace brave_ads
