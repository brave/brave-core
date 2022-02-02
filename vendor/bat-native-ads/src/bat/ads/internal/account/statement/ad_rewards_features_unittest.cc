/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/ad_rewards_features.h"

#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsAdRewardsFeaturesTest, AdRewardsEnabled) {
  // Arrange

  // Act
  const bool is_enabled = features::IsAdRewardsEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsAdRewardsFeaturesTest, AdRewardsDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdRewards);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool is_enabled = features::IsAdRewardsEnabled();

  // Assert
  EXPECT_FALSE(is_enabled);
}

TEST(BatAdsAdRewardsFeaturesTest, AdRewardsNextPaymentDay) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kAdRewardsParameters;
  const char kNextPaymentDayParameter[] = "next_payment_day";
  kAdRewardsParameters[kNextPaymentDayParameter] = "7";
  enabled_features.push_back({features::kAdRewards, kAdRewardsParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int next_payment_day = features::GetAdRewardsNextPaymentDay();

  // Assert
  const int expected_next_payment_day = 7;
  EXPECT_EQ(expected_next_payment_day, next_payment_day);
}

TEST(BatAdsAdRewardsFeaturesTest, AdRewardsDefaultNextPaymentDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int next_payment_day = features::GetAdRewardsNextPaymentDay();

  // Assert
  const int expected_next_payment_day = 5;
  EXPECT_EQ(expected_next_payment_day, next_payment_day);
}

TEST(BatAdsAdRewardsFeaturesTest, DisabledAdRewardsDefaultNextPaymentDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdRewards);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int next_payment_day = features::GetAdRewardsNextPaymentDay();

  // Assert
  const int expected_next_payment_day = 5;
  EXPECT_EQ(expected_next_payment_day, next_payment_day);
}

}  // namespace ads
