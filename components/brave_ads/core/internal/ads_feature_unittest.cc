/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsBraveAdsFeatureTest, ShouldAlwaysRunService) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysRunBraveAdsServiceFeature);

  // Act & Assert
  EXPECT_TRUE(ShouldAlwaysRunService());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldNotAlwaysRunService) {
  // Act & Assert
  EXPECT_FALSE(ShouldAlwaysRunService());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldAlwaysTriggerNewTabPageAdEvents) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveNewTabPageAdEventsFeature);

  // Act & Assert
  EXPECT_TRUE(ShouldAlwaysTriggerNewTabPageAdEvents());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldNotAlwaysTriggerNewTabPageAdEvents) {
  // Act & Assert
  EXPECT_FALSE(ShouldAlwaysTriggerNewTabPageAdEvents());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldSupportSearchResultAds) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportSearchResultAds());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldNotSupportSearchResultAds) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      kShouldSupportSearchResultAdsFeature);

  // Act & Assert
  EXPECT_FALSE(ShouldSupportSearchResultAds());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldAlwaysTriggerSearchResultAdEvents) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  // Act & Assert
  EXPECT_TRUE(ShouldAlwaysTriggerSearchResultAdEvents());
}

TEST(BraveAdsBraveAdsFeatureTest, ShouldNotAlwaysTriggerSearchResultAdEvents) {
  // Act & Assert
  EXPECT_FALSE(ShouldAlwaysTriggerSearchResultAdEvents());
}

}  // namespace brave_ads
