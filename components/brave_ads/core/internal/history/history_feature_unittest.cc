/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/history_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsHistoryFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kHistoryFeature));
}

TEST(BraveAdsHistoryFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kHistoryFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kHistoryFeature));
}

TEST(BraveAdsHistoryFeatureTest, HistoryTimeWindow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kHistoryFeature, {{"time_window", "1d"}});

  // Act & Assert
  EXPECT_EQ(base::Days(1), kHistoryTimeWindow.Get());
}

TEST(BraveAdsHistoryFeatureTest, DefaultHistoryTimeWindow) {
  // Act & Assert
  EXPECT_EQ(base::Days(30), kHistoryTimeWindow.Get());
}

TEST(BraveAdsHistoryFeatureTest, DefaultHistoryTimeWindowWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kHistoryFeature);

  // Act & Assert
  EXPECT_EQ(base::Days(30), kHistoryTimeWindow.Get());
}

}  // namespace brave_ads
