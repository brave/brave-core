/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/feature/user_attention_feature.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsUserAttentionFeatureTest, GetIdleThreshold) {
  // Arrange
  base::FieldTrialParams params;
  params["idle_threshold"] = "7s";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kUserAttentionFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(7), kIdleThreshold.Get());
}

TEST(BraveAdsUserAttentionFeatureTest, DefaultIdleThreshold) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(5), kIdleThreshold.Get());
}

TEST(BraveAdsUserAttentionFeatureTest, DefaultIdleThresholdWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kUserAttentionFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Seconds(5), kIdleThreshold.Get());
}

}  // namespace brave_ads
