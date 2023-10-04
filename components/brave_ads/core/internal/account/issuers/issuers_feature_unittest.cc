/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsIssuersFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kIssuersFeature));
}

TEST(BraveAdsIssuersFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kIssuersFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kIssuersFeature));
}

TEST(BraveAdsIssuersFeatureTest, MaximumIssuerPublicKeys) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kIssuersFeature, {{"maximum_public_keys", "1"}});

  // Act & Assert
  EXPECT_EQ(1, kMaximumIssuerPublicKeys.Get());
}

TEST(BraveAdsIssuersFeatureTest, DefaultMaximumIssuerPublicKeys) {
  // Act & Assert
  EXPECT_EQ(6, kMaximumIssuerPublicKeys.Get());
}

TEST(BraveAdsIssuersFeatureTest, DefaultMaximumIssuerPublicKeysWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kIssuersFeature);

  // Act & Assert
  EXPECT_EQ(6, kMaximumIssuerPublicKeys.Get());
}

}  // namespace brave_ads
