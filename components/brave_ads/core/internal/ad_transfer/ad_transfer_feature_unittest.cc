/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_transfer/ad_transfer_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAdTransferFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kAdTransferFeature));
}

TEST(BraveAdsAdTransferFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAdTransferFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAdTransferFeature));
}

TEST(BraveAdsAdTransferFeatureTest, TransferAfter) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdTransferFeature, {{"ad_transfer_after", "7s"}});

  // Act & Assert
  EXPECT_EQ(base::Seconds(7), kTransferAdAfter.Get());
}

TEST(BraveAdsAdTransferFeatureTest, DefaultTransferAfter) {
  // Act & Assert
  EXPECT_EQ(base::Seconds(10), kTransferAdAfter.Get());
}

TEST(BraveAdsAdTransferFeatureTest, DefaultTransferAfterWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAdTransferFeature);

  // Act & Assert
  EXPECT_EQ(base::Seconds(10), kTransferAdAfter.Get());
}

TEST(BraveAdsAdTransferFeatureTest, TransferCap) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAdTransferFeature, {{"ad_transfer_cap", "7"}});

  // Act & Assert
  EXPECT_EQ(7, kTransferAdCap.Get());
}

TEST(BraveAdsAdTransferFeatureTest, DefaultTransferCap) {
  // Act & Assert
  EXPECT_EQ(1, kTransferAdCap.Get());
}

TEST(BraveAdsAdTransferFeatureTest, DefaultTransferCapWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAdTransferFeature);

  // Act & Assert
  EXPECT_EQ(1, kTransferAdCap.Get());
}

}  // namespace brave_ads
