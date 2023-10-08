/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/anti_targeting_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAntiTargetingFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kAntiTargetingFeature));
}

TEST(BraveAdsAntiTargetingFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAntiTargetingFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAntiTargetingFeature));
}

TEST(BraveAdsAntiTargetingFeatureTest, AntiTargetingResourceVersion) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAntiTargetingFeature, {{"resource_version", "0"}});

  // Act & Assert
  EXPECT_EQ(0, kAntiTargetingResourceVersion.Get());
}

TEST(BraveAdsAntiTargetingFeatureTest, DefaultAntiTargetingResourceVersion) {
  // Act & Assert
  EXPECT_EQ(1, kAntiTargetingResourceVersion.Get());
}

TEST(BraveAdsAntiTargetingFeatureTest,
     DefaultAntiTargetingResourceVersionWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAntiTargetingFeature);

  // Act & Assert
  EXPECT_EQ(1, kAntiTargetingResourceVersion.Get());
}

}  // namespace brave_ads
