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

TEST(BraveAdsAdEventFeatureTest, DebounceViewedAdEventFor) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdEventFeature, {{"debounce_viewed_ad_event_for", "5s"}});

  // Act & Assert
  EXPECT_EQ(base::Seconds(5), kDebounceViewedAdEventFor.Get());
}

TEST(BraveAdsAdEventFeatureTest, DefaultDebounceViewedAdEventFor) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(0), kDebounceViewedAdEventFor.Get());
}

TEST(BraveAdsAdEventFeatureTest, DefaultDebounceViewedAdEventForWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAdEventFeature);

  // Act & Assert
  EXPECT_EQ(base::Seconds(0), kDebounceViewedAdEventFor.Get());
}

TEST(BraveAdsAdEventFeatureTest, DebounceClickedAdEventFor) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdEventFeature, {{"debounce_clicked_ad_event_for", "5s"}});

  // Act & Assert
  EXPECT_EQ(base::Seconds(5), kDebounceClickedAdEventFor.Get());
}

TEST(BraveAdsAdEventFeatureTest, DefaultDebounceClickedAdEventFor) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(0), kDebounceClickedAdEventFor.Get());
}

TEST(BraveAdsAdEventFeatureTest, DefaultDebounceClickedAdEventForWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAdEventFeature);

  // Act & Assert
  EXPECT_EQ(base::Seconds(0), kDebounceClickedAdEventFor.Get());
}

}  // namespace brave_ads
