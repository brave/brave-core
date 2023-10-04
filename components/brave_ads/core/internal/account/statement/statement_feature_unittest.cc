/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAccountFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kAccountStatementFeature));
}

TEST(BraveAdsAccountFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAccountStatementFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAccountStatementFeature));
}

TEST(BraveAdsAccountFeatureTest, NextPaymentDay) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAccountStatementFeature, {{"next_payment_day", "5"}});

  // Act & Assert
  EXPECT_EQ(5, kNextPaymentDay.Get());
}

TEST(BraveAdsAccountFeatureTest, DefaultNextPaymentDay) {
  // Act & Assert
  EXPECT_EQ(7, kNextPaymentDay.Get());
}

TEST(BraveAdsAccountFeatureTest, DefaultNextPaymentDayWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAccountStatementFeature);

  // Act & Assert
  EXPECT_EQ(7, kNextPaymentDay.Get());
}

TEST(BraveAdsAccountFeatureTest, MinEstimatedEarningsMultiplier) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAccountStatementFeature,
      {{"minimum_estimated_earnings_multiplier", "0.5"}});

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.5, kMinEstimatedEarningsMultiplier.Get());
}

TEST(BraveAdsAccountFeatureTest, DefaultMinEstimatedEarningsMultiplier) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.8, kMinEstimatedEarningsMultiplier.Get());
}

TEST(BraveAdsAccountFeatureTest,
     DefaultMinEstimatedEarningsMultiplierWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kAccountStatementFeature);

  // Act & Assert
  EXPECT_DOUBLE_EQ(0.8, kMinEstimatedEarningsMultiplier.Get());
}

}  // namespace brave_ads
