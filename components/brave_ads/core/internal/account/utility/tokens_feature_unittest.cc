/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/tokens_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsTokensFeatureTest, MinConfirmationTokens) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAccountTokensFeature, {{"min_confirmation_tokens", "7"}});

  // Act

  // Assert
  EXPECT_EQ(7, kMinConfirmationTokens.Get());
}

TEST(BraveAdsTokensFeatureTest, DefaultMinConfirmationTokens) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(20, kMinConfirmationTokens.Get());
}

TEST(BraveAdsTokensFeatureTest, DefaultMinConfirmationTokensWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAccountTokensFeature);

  // Act

  // Assert
  EXPECT_EQ(20, kMinConfirmationTokens.Get());
}

TEST(BraveAdsTokensFeatureTest, MaxConfirmationTokens) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAccountTokensFeature, {{"max_confirmation_tokens", "21"}});

  // Act

  // Assert
  EXPECT_EQ(21, kMaxConfirmationTokens.Get());
}

TEST(BraveAdsTokensFeatureTest, DefaultMaxConfirmationTokens) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(50, kMaxConfirmationTokens.Get());
}

TEST(BraveAdsTokensFeatureTest, DefaultMaxConfirmationTokensWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAccountTokensFeature);

  // Act

  // Assert
  EXPECT_EQ(50, kMaxConfirmationTokens.Get());
}

}  // namespace brave_ads
