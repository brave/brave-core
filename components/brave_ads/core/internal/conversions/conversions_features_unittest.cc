/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversions_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::features {

TEST(BatAdsConversionsFeaturesTest, IsConversionsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsConversionsEnabled());
}

TEST(BatAdsConversionsFeaturesTest, IsConversionsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kConversions);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsConversionsEnabled());
}

TEST(BatAdsConversionsFeaturesTest, GetConversionsResourceVersion) {
  // Arrange
  base::FieldTrialParams params;
  params["resource_version"] = "0";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kConversions, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(0, GetConversionsResourceVersion());
}

TEST(BatAdsConversionsFeaturesTest, DefaultConversionsResourceVersion) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(1, GetConversionsResourceVersion());
}

TEST(BatAdsConversionsFeaturesTest,
     DefaultConversionsResourceVersionWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kConversions);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(1, GetConversionsResourceVersion());
}

TEST(BatAdsConversionsFeaturesTest, GetConversionIdPattern) {
  // Arrange
  base::FieldTrialParams params;
  params["conversion_id_pattern"] = "*";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kConversions, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ("*", GetConversionIdPattern());
}

TEST(BatAdsConversionsFeaturesTest, DefaultConversionIdPattern) {
  // Arrange

  // Act

  // Assert
  const std::string expected_pattern =
      R"~(<meta.*name="ad-conversion-id".*content="([-a-zA-Z0-9]*)".*>)~";
  EXPECT_EQ(expected_pattern, GetConversionIdPattern());
}

TEST(BatAdsConversionsFeaturesTest, DefaultConversionIdPatternWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kConversions);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  const std::string expected_pattern =
      R"~(<meta.*name="ad-conversion-id".*content="([-a-zA-Z0-9]*)".*>)~";
  EXPECT_EQ(expected_pattern, GetConversionIdPattern());
}

}  // namespace brave_ads::features
