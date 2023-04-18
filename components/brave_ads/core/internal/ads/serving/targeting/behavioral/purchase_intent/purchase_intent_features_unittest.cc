/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/purchase_intent/purchase_intent_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::targeting {

TEST(BraveAdsPurchaseIntentFeaturesTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsPurchaseIntentEnabled());
}

TEST(BraveAdsPurchaseIntentFeaturesTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kPurchaseIntentFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsPurchaseIntentEnabled());
}

TEST(BraveAdsPurchaseIntentFeaturesTest, GetPurchaseIntentResourceVersion) {
  // Arrange
  base::FieldTrialParams params;
  params["resource_version"] = "0";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kPurchaseIntentFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(0, kPurchaseIntentResourceVersion.Get());
}

TEST(BraveAdsPurchaseIntentFeaturesTest, DefaultPurchaseIntentResourceVersion) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(1, kPurchaseIntentResourceVersion.Get());
}

TEST(BraveAdsPurchaseIntentFeaturesTest,
     DefaultPurchaseIntentResourceVersionWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kPurchaseIntentFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(1, kPurchaseIntentResourceVersion.Get());
}

TEST(BraveAdsPurchaseIntentFeaturesTest, GetPurchaseIntentThreshold) {
  // Arrange
  base::FieldTrialParams params;
  params["threshold"] = "5";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kPurchaseIntentFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(5, kPurchaseIntentThreshold.Get());
}

TEST(BraveAdsPurchaseIntentFeaturesTest, DefaultPurchaseIntentThreshold) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(3, kPurchaseIntentThreshold.Get());
}

TEST(BraveAdsPurchaseIntentFeaturesTest,
     DefaultPurchaseIntentThresholdWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kPurchaseIntentFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(3, kPurchaseIntentThreshold.Get());
}

TEST(BraveAdsPurchaseIntentFeaturesTest, GetPurchaseIntentTimeWindow) {
  // Arrange
  base::FieldTrialParams params;
  params["time_window"] = "1d";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kPurchaseIntentFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Days(1), kPurchaseIntentTimeWindow.Get());
}

TEST(BraveAdsPurchaseIntentFeaturesTest, DefaultPurchaseIntentTimeWindow) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::Days(7), kPurchaseIntentTimeWindow.Get());
}

TEST(BraveAdsPurchaseIntentFeaturesTest,
     DefaultPurchaseIntentTimeWindowWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kPurchaseIntentFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(base::Days(7), kPurchaseIntentTimeWindow.Get());
}

}  // namespace brave_ads::targeting
