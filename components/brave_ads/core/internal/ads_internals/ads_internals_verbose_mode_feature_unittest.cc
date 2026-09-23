/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads_internals/ads_internals_verbose_mode_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAdsInternalsVerboseModeFeatureTest, IsDisabledByDefault) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(/*enabled_features=*/{},
                                       /*disabled_features=*/{});

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAdsInternalsVerboseModeFeature));
}

}  // namespace brave_ads
