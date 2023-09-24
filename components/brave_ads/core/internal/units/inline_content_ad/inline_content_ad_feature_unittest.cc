/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/units/inline_content_ad/inline_content_ad_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveInlineContentAdFeatureTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsInlineContentAdFeatureEnabled());
}

TEST(BraveInlineContentAdFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kInlineContentAdFeature);

  // Act

  // Assert
  EXPECT_FALSE(IsInlineContentAdFeatureEnabled());
}

TEST(BraveInlineContentAdFeatureTest, MaximumAdsPerHour) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kInlineContentAdFeature, {{"maximum_ads_per_hour", "42"}});

  // Act

  // Assert
  EXPECT_EQ(42, kMaximumInlineContentAdsPerHour.Get());
}

TEST(BraveInlineContentAdFeatureTest, DefaultMaximumAdsPerHour) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(6, kMaximumInlineContentAdsPerHour.Get());
}

TEST(BraveInlineContentAdFeatureTest, DefaultMaximumAdsPerHourWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kInlineContentAdFeature);

  // Act

  // Assert
  EXPECT_EQ(6, kMaximumInlineContentAdsPerHour.Get());
}

TEST(BraveInlineContentAdFeatureTest, MaximumAdsPerDay) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kInlineContentAdFeature, {{"maximum_ads_per_day", "24"}});

  // Act

  // Assert
  EXPECT_EQ(24, kMaximumInlineContentAdsPerDay.Get());
}

TEST(BraveInlineContentAdFeatureTest, DefaultMaximumAdsPerDay) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(20, kMaximumInlineContentAdsPerDay.Get());
}

TEST(BraveInlineContentAdFeatureTest, DefaultMaximumAdsPerDayWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kInlineContentAdFeature);

  // Act

  // Assert
  EXPECT_EQ(20, kMaximumInlineContentAdsPerDay.Get());
}

}  // namespace brave_ads
