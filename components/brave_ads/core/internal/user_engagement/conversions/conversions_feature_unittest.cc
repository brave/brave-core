/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsConversionsFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kConversionsFeature));
}

TEST(BraveAdsConversionsFeatureTest, ConversionResourceVersion) {
  // Act & Assert
  EXPECT_EQ(1, kConversionResourceVersion.Get());
}

TEST(BraveAdsConversionsFeatureTest, HtmlMetaTagConversionIdPattern) {
  // Act & Assert
  EXPECT_EQ(R"~(<meta.*name="ad-conversion-id".*content="([-a-zA-Z0-9]*)".*>)~",
            kHtmlMetaTagConversionIdPattern.Get());
}

}  // namespace brave_ads
