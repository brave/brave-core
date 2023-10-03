/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/units/new_tab_page_ad/new_tab_page_ad_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsNewTabPageAdFeatureTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kNewTabPageAdFeature));
}

TEST(BraveAdsNewTabPageAdFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kNewTabPageAdFeature);

  // Act

  // Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kNewTabPageAdFeature));
}

TEST(BraveAdsNewTabPageAdFeatureTest, MaximumAdsPerHour) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNewTabPageAdFeature, {{"maximum_ads_per_hour", "42"}});

  // Act

  // Assert
  EXPECT_EQ(42, kMaximumNewTabPageAdsPerHour.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, DefaultMaximumAdsPerHour) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(4, kMaximumNewTabPageAdsPerHour.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, DefaultMaximumAdsPerHourWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kNewTabPageAdFeature);

  // Act

  // Assert
  EXPECT_EQ(4, kMaximumNewTabPageAdsPerHour.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, MaximumAdsPerDay) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNewTabPageAdFeature, {{"maximum_ads_per_day", "24"}});

  // Act

  // Assert
  EXPECT_EQ(24, kMaximumNewTabPageAdsPerDay.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, DefaultMaximumAdsPerDay) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(20, kMaximumNewTabPageAdsPerDay.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, DefaultMaximumAdsPerDayWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kNewTabPageAdFeature);

  // Act

  // Assert
  EXPECT_EQ(20, kMaximumNewTabPageAdsPerDay.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, MinimumWaitTime) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNewTabPageAdFeature, {{"minimum_wait_time", "10m"}});

  // Act

  // Assert
  EXPECT_EQ(base::Minutes(10), kNewTabPageAdMinimumWaitTime.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, DefaultMinimumWaitTime) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::Minutes(5), kNewTabPageAdMinimumWaitTime.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, DefaultMinimumWaitTimeWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kNewTabPageAdFeature);

  // Act

  // Assert
  EXPECT_EQ(base::Minutes(5), kNewTabPageAdMinimumWaitTime.Get());
}

}  // namespace brave_ads
