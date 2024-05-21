/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSearchResultAdFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kSearchResultAdFeature));
}

TEST(BraveAdsSearchResultAdFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kSearchResultAdFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kSearchResultAdFeature));
}

TEST(BraveAdsSearchResultAdFeatureTest, MaximumSearchResultAdsPerHour) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kSearchResultAdFeature, {{"maximum_ads_per_hour", "42"}});

  // Act & Assert
  EXPECT_EQ(42, kMaximumSearchResultAdsPerHour.Get());
}

TEST(BraveAdsSearchResultAdFeatureTest, DefaultMaximumSearchResultAdsPerHour) {
  // Act & Assert
  EXPECT_EQ(0, kMaximumSearchResultAdsPerHour.Get());
}

TEST(BraveAdsSearchResultAdFeatureTest,
     DefaultMaximumSearchResultAdsPerHourWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kSearchResultAdFeature);

  // Act & Assert
  EXPECT_EQ(0, kMaximumSearchResultAdsPerHour.Get());
}

TEST(BraveAdsSearchResultAdFeatureTest, MaximumSearchResultAdsPerDay) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kSearchResultAdFeature, {{"maximum_ads_per_day", "24"}});

  // Act & Assert
  EXPECT_EQ(24, kMaximumSearchResultAdsPerDay.Get());
}

TEST(BraveAdsSearchResultAdFeatureTest, DefaultMaximumSearchResultAdsPerDay) {
  // Act & Assert
  EXPECT_EQ(0, kMaximumSearchResultAdsPerDay.Get());
}

TEST(BraveAdsSearchResultAdFeatureTest,
     DefaultMaximumSearchResultAdsPerDayWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kSearchResultAdFeature);

  // Act & Assert
  EXPECT_EQ(0, kMaximumSearchResultAdsPerDay.Get());
}

}  // namespace brave_ads
