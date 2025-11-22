/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/ads_service_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsServiceFeatureTest, IsDisabled) {
  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAdsServiceFeature));
}

TEST(BraveAdsServiceFeatureTest, ShouldNotSupportOhttp) {
  // Act & Assert
  EXPECT_FALSE(kShouldSupportOhttp.Get());
}

}  // namespace brave_ads
