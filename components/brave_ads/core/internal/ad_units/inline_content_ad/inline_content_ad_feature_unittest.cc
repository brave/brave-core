/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveInlineContentAdFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kInlineContentAdFeature));
}

TEST(BraveInlineContentAdFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kInlineContentAdFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kInlineContentAdFeature));
}

TEST(BraveInlineContentAdFeatureTest, MaximumInlineContentAdsPerHour) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kInlineContentAdFeature, {{"maximum_ads_per_hour", "42"}});

  // Act & Assert
  EXPECT_EQ(42, kMaximumInlineContentAdsPerHour.Get());
}

TEST(BraveInlineContentAdFeatureTest, DefaultMaximumInlineContentAdsPerHour) {
  // Act & Assert
  EXPECT_EQ(6, kMaximumInlineContentAdsPerHour.Get());
}

TEST(BraveInlineContentAdFeatureTest,
     DefaultMaximumInlineContentAdsPerHourWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kInlineContentAdFeature);

  // Act & Assert
  EXPECT_EQ(6, kMaximumInlineContentAdsPerHour.Get());
}

TEST(BraveInlineContentAdFeatureTest, MaximumInlineContentAdsPerDay) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kInlineContentAdFeature, {{"maximum_ads_per_day", "24"}});

  // Act & Assert
  EXPECT_EQ(24, kMaximumInlineContentAdsPerDay.Get());
}

TEST(BraveInlineContentAdFeatureTest, DefaultMaximumInlineContentAdsPerDay) {
  // Act & Assert
  EXPECT_EQ(20, kMaximumInlineContentAdsPerDay.Get());
}

TEST(BraveInlineContentAdFeatureTest,
     DefaultMaximumInlineContentAdsPerDayWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kInlineContentAdFeature);

  // Act & Assert
  EXPECT_EQ(20, kMaximumInlineContentAdsPerDay.Get());
}

}  // namespace brave_ads
