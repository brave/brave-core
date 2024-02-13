/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsNotificationAdFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kNotificationAdFeature));
}

TEST(BraveAdsNotificationAdFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kNotificationAdFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kNotificationAdFeature));
}

TEST(BraveAdsNotificationAdFeatureTest, DefaultNotificationAdsPerHour) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature, {{"default_ads_per_hour", "42"}});

  // Act & Assert
  EXPECT_EQ(42, kDefaultNotificationAdsPerHour.Get());
}

TEST(BraveAdsNotificationAdFeatureTest, DefaultDefaultNotificationAdsPerHour) {
  // Act & Assert
  EXPECT_EQ(10, kDefaultNotificationAdsPerHour.Get());
}

TEST(BraveAdsNotificationAdFeatureTest,
     DefaultDefaultNotificationAdsPerHourWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kNotificationAdFeature);

  // Act & Assert
  EXPECT_EQ(10, kDefaultNotificationAdsPerHour.Get());
}

TEST(BraveAdsNotificationAdFeatureTest, MaximumNotificationAdsPerDay) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature, {{"maximum_ads_per_day", "24"}});

  // Act & Assert
  EXPECT_EQ(24, kMaximumNotificationAdsPerDay.Get());
}

TEST(BraveAdsNotificationAdFeatureTest, DefaultMaximumNotificationAdsPerDay) {
  // Act & Assert
  EXPECT_EQ(100, kMaximumNotificationAdsPerDay.Get());
}

TEST(BraveAdsNotificationAdFeatureTest,
     DefaultMaximumNotificationAdsPerDayWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kNotificationAdFeature);

  // Act & Assert
  EXPECT_EQ(100, kMaximumNotificationAdsPerDay.Get());
}

TEST(BraveAdsNotificationAdFeatureTest,
     CanFallbackToCustomNotificationAdsDefault) {
  // Act & Assert
  EXPECT_FALSE(kCanFallbackToCustomNotificationAds.Get());
}

TEST(BraveAdsNotificationAdFeatureTest, CanFallbackToCustomNotificationAds) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature,
      {{"can_fallback_to_custom_notifications", "true"}});

  // Act & Assert
  EXPECT_TRUE(kCanFallbackToCustomNotificationAds.Get());
}

TEST(BraveAdsNotificationAdFeatureTest,
     DefaultCanFallbackToCustomNotificationAds) {
  // Act & Assert
  EXPECT_FALSE(kCanFallbackToCustomNotificationAds.Get());
}

TEST(BraveAdsNotificationAdFeatureTest,
     DefaultCanFallbackToCustomNotificationAdsWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kNotificationAdFeature);

  // Act & Assert
  EXPECT_FALSE(kCanFallbackToCustomNotificationAds.Get());
}

}  // namespace brave_ads
