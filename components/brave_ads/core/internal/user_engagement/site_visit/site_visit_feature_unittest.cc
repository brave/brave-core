/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSiteVisitFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kSiteVisitFeature));
}

TEST(BraveAdsSiteVisitFeatureTest, PageLandAfter) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(5), kPageLandAfter.Get());
}

TEST(BraveAdsSiteVisitFeatureTest, PageLandCap) {
  // Act & Assert
  EXPECT_EQ(0U, kPageLandCap.Get());
}

TEST(BraveAdsSiteVisitFeatureTest, ShouldSuspendAndResumePageLand) {
  // Act & Assert
  EXPECT_TRUE(kShouldSuspendAndResumePageLand.Get());
}

}  // namespace brave_ads
