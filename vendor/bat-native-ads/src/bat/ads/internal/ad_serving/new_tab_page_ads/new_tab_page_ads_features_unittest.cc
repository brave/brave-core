/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/new_tab_page_ads/new_tab_page_ads_features.h"

#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace new_tab_page_ads {

TEST(BatAdsNewTabPageAdsFeaturesTest, NewTabPageAdsEnabled) {
  // Arrange

  // Act
  const bool is_enabled = features::IsEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsNewTabPageAdsFeaturesTest, NewTabPageAdsDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kFeature);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool is_enabled = features::IsEnabled();

  // Assert
  EXPECT_FALSE(is_enabled);
}

}  // namespace new_tab_page_ads
}  // namespace ads
