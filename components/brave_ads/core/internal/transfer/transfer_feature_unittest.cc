/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/transfer/transfer_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsTransferFeatureTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kTransferFeature));
}

TEST(BraveAdsTransferFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kTransferFeature);

  // Act

  // Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kTransferFeature));
}

TEST(BraveAdsTransferFeatureTest, TransferredAfter) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kTransferFeature, {{"transferred_after", "7s"}});

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(7), kTransferredAfter.Get());
}

TEST(BraveAdsTransferFeatureTest, DefaultTransferredAfter) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(10), kTransferredAfter.Get());
}

TEST(BraveAdsTransferFeatureTest, DefaultTransferredAfterWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kTransferFeature);

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(10), kTransferredAfter.Get());
}

}  // namespace brave_ads
