/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSiteVisitFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kSiteVisitFeature));
}

TEST(BraveAdsSiteVisitFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kSiteVisitFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kSiteVisitFeature));
}

TEST(BraveAdsSiteVisitFeatureTest, PageLandAfter) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kSiteVisitFeature, {{"page_land_after", "7s"}});

  // Act & Assert
  EXPECT_EQ(base::Seconds(7), kPageLandAfter.Get());
}

TEST(BraveAdsSiteVisitFeatureTest, DefaultPageLandAfter) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(5), kPageLandAfter.Get());
}

TEST(BraveAdsSiteVisitFeatureTest, DefaultPageLandAfterWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kSiteVisitFeature);

  // Act & Assert
  EXPECT_EQ(base::Seconds(5), kPageLandAfter.Get());
}

TEST(BraveAdsSiteVisitFeatureTest, PageLandCap) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kSiteVisitFeature, {{"page_land_cap", "7"}});

  // Act & Assert
  EXPECT_EQ(7, kPageLandCap.Get());
}

TEST(BraveAdsSiteVisitFeatureTest, DefaultPageLandCap) {
  // Act & Assert
  EXPECT_EQ(0, kPageLandCap.Get());
}

TEST(BraveAdsSiteVisitFeatureTest, DefaultPageLandCapWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kSiteVisitFeature);

  // Act & Assert
  EXPECT_EQ(0, kPageLandCap.Get());
}

}  // namespace brave_ads
