/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAdEventFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kAdEventFeature));
}

TEST(BraveAdsAdEventFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAdEventFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAdEventFeature));
}

TEST(BraveAdsAdEventFeatureTest, DeduplicateViewedAdEventFor) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdEventFeature, {{"deduplicate_viewed_ad_event_for", "5s"}});

  // Act & Assert
  EXPECT_EQ(base::Seconds(5), kDeduplicateViewedAdEventFor.Get());
}

TEST(BraveAdsAdEventFeatureTest, DefaultDeduplicateViewedAdEventFor) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(0), kDeduplicateViewedAdEventFor.Get());
}

TEST(BraveAdsAdEventFeatureTest,
     DefaultDeduplicateViewedAdEventForWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAdEventFeature);

  // Act & Assert
  EXPECT_EQ(base::Seconds(0), kDeduplicateViewedAdEventFor.Get());
}

TEST(BraveAdsAdEventFeatureTest, DeduplicateClickedAdEventFor) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdEventFeature, {{"deduplicate_clicked_ad_event_for", "5s"}});

  // Act & Assert
  EXPECT_EQ(base::Seconds(5), kDeduplicateClickedAdEventFor.Get());
}

TEST(BraveAdsAdEventFeatureTest, DefaultDeduplicateClickedAdEventFor) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(0), kDeduplicateClickedAdEventFor.Get());
}

TEST(BraveAdsAdEventFeatureTest,
     DefaultDeduplicateClickedAdEventForWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAdEventFeature);

  // Act & Assert
  EXPECT_EQ(base::Seconds(0), kDeduplicateClickedAdEventFor.Get());
}

}  // namespace brave_ads
