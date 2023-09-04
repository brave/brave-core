/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAccountFeatureTest, NextPaymentDay) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["next_payment_day"] = "5";
  enabled_features.emplace_back(kAccountStatementFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(5, kNextPaymentDay.Get());
}

TEST(BraveAdsAccountFeatureTest, DefaultNextPaymentDay) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(7, kNextPaymentDay.Get());
}

TEST(BraveAdsAccountFeatureTest, DefaultNextPaymentDayWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kAccountStatementFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(7, kNextPaymentDay.Get());
}

TEST(BraveAdsAccountFeatureTest, MinEstimatedEarningsMultiplier) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["min_estimated_earnings_multiplier"] = "0.5";
  enabled_features.emplace_back(kAccountStatementFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(0.5, kMinEstimatedEarningsMultiplier.Get());
}

TEST(BraveAdsAccountFeatureTest, DefaultMinEstimatedEarningsMultiplier) {
  // Arrange

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(0.8, kMinEstimatedEarningsMultiplier.Get());
}

TEST(BraveAdsAccountFeatureTest,
     DefaultMinEstimatedEarningsMultiplierWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kAccountStatementFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_DOUBLE_EQ(0.8, kMinEstimatedEarningsMultiplier.Get());
}

}  // namespace brave_ads
