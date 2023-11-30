/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsNewTabPageAdFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kNewTabPageAdFeature));
}

TEST(BraveAdsNewTabPageAdFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kNewTabPageAdFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kNewTabPageAdFeature));
}

TEST(BraveAdsNewTabPageAdFeatureTest, MaximumNewTabPageAdsPerHour) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNewTabPageAdFeature, {{"maximum_ads_per_hour", "42"}});

  // Act & Assert
  EXPECT_EQ(42, kMaximumNewTabPageAdsPerHour.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, DefaultMaximumNewTabPageAdsPerHour) {
  // Act & Assert
  EXPECT_EQ(4, kMaximumNewTabPageAdsPerHour.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest,
     DefaultMaximumNewTabPageAdsPerHourWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kNewTabPageAdFeature);

  // Act & Assert
  EXPECT_EQ(4, kMaximumNewTabPageAdsPerHour.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, MaximumNewTabPageAdsPerDay) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNewTabPageAdFeature, {{"maximum_ads_per_day", "24"}});

  // Act & Assert
  EXPECT_EQ(24, kMaximumNewTabPageAdsPerDay.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, DefaultMaximumNewTabPageAdsPerDay) {
  // Act & Assert
  EXPECT_EQ(20, kMaximumNewTabPageAdsPerDay.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest,
     DefaultMaximumNewTabPageAdsPerDayWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kNewTabPageAdFeature);

  // Act & Assert
  EXPECT_EQ(20, kMaximumNewTabPageAdsPerDay.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, NewTabPageAdMinimumWaitTime) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNewTabPageAdFeature, {{"minimum_wait_time", "10m"}});

  // Act & Assert
  EXPECT_EQ(base::Minutes(10), kNewTabPageAdMinimumWaitTime.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, DefaultNewTabPageAdMinimumWaitTime) {
  // Act & Assert
  EXPECT_EQ(base::Minutes(5), kNewTabPageAdMinimumWaitTime.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest,
     DefaultNewTabPageAdMinimumWaitTimeWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kNewTabPageAdFeature);

  // Act & Assert
  EXPECT_EQ(base::Minutes(5), kNewTabPageAdMinimumWaitTime.Get());
}

}  // namespace brave_ads
