/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsPromotedContentAdFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kPromotedContentAdFeature));
}

TEST(BraveAdsPromotedContentAdFeatureTest, MaximumPromotedContentAdsPerHour) {
  // Act & Assert
  EXPECT_EQ(4U, kMaximumPromotedContentAdsPerHour.Get());
}

TEST(BraveAdsPromotedContentAdFeatureTest, MaximumPromotedContentAdsPerDay) {
  // Act & Assert
  EXPECT_EQ(20U, kMaximumPromotedContentAdsPerDay.Get());
}

}  // namespace brave_ads
