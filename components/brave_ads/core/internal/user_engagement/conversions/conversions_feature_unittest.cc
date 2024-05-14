/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsConversionsFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kConversionsFeature));
}

TEST(BraveAdsConversionsFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kConversionsFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kConversionsFeature));
}

TEST(BraveAdsConversionsFeatureTest, ConversionResourceVersion) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kConversionsFeature, {{"resource_version", "0"}});

  // Act & Assert
  EXPECT_EQ(0, kConversionResourceVersion.Get());
}

TEST(BraveAdsConversionsFeatureTest, DefaultConversionResourceVersion) {
  // Act & Assert
  EXPECT_EQ(1, kConversionResourceVersion.Get());
}

TEST(BraveAdsConversionsFeatureTest,
     DefaultConversionResourceVersionWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kConversionsFeature);

  // Act & Assert
  EXPECT_EQ(1, kConversionResourceVersion.Get());
}

TEST(BraveAdsConversionsFeatureTest, HtmlMetaTagConversionIdPattern) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kConversionsFeature, {{"html_meta_tag_id_pattern", "*"}});

  // Act & Assert
  EXPECT_EQ("*", kHtmlMetaTagConversionIdPattern.Get());
}

TEST(BraveAdsConversionsFeatureTest, DefaultHtmlMetaTagConversionIdPattern) {
  // Act & Assert
  EXPECT_EQ(R"~(<meta.*name="ad-conversion-id".*content="([-a-zA-Z0-9]*)".*>)~",
            kHtmlMetaTagConversionIdPattern.Get());
}

TEST(BraveAdsConversionsFeatureTest,
     DefaultHtmlMetaTagConversionIdPatternWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kConversionsFeature);

  // Act & Assert
  EXPECT_EQ(R"~(<meta.*name="ad-conversion-id".*content="([-a-zA-Z0-9]*)".*>)~",
            kHtmlMetaTagConversionIdPattern.Get());
}

}  // namespace brave_ads
