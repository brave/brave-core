/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsInlineContentAdFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kInlineContentAdFeature));
}

TEST(BraveAdsInlineContentAdFeatureTest, MaximumInlineContentAdsPerHour) {
  // Act & Assert
  EXPECT_EQ(6U, kMaximumInlineContentAdsPerHour.Get());
}

TEST(BraveAdsInlineContentAdFeatureTest, MaximumInlineContentAdsPerDay) {
  // Act & Assert
  EXPECT_EQ(20U, kMaximumInlineContentAdsPerDay.Get());
}

}  // namespace brave_ads
