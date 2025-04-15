/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsNewTabPageAdFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kNewTabPageAdFeature));
}

TEST(BraveAdsNewTabPageAdFeatureTest, MaximumNewTabPageAdsPerHour) {
  // Act & Assert
  EXPECT_EQ(4U, kMaximumNewTabPageAdsPerHour.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, MaximumNewTabPageAdsPerDay) {
  // Act & Assert
  EXPECT_EQ(20U, kMaximumNewTabPageAdsPerDay.Get());
}

TEST(BraveAdsNewTabPageAdFeatureTest, NewTabPageAdMinimumWaitTime) {
  // Act & Assert
  EXPECT_EQ(base::Minutes(1), kNewTabPageAdMinimumWaitTime.Get());
}

}  // namespace brave_ads
