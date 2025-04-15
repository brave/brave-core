/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsUserActivityFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kUserActivityFeature));
}

TEST(BraveAdsUserActivityFeatureTest, MaximumUserActivityEvents) {
  // Act & Assert
  EXPECT_EQ(3'600U, kMaximumUserActivityEvents.Get());
}

TEST(BraveAdsUserActivityFeatureTest, UserActivityTriggers) {
  // Act & Assert
  EXPECT_EQ(
      "0D0B14110D0B14110D0B14110D0B1411=-1.0;0D0B1411070707=-1.0;07070707=-1.0",
      kUserActivityTriggers.Get());
}

TEST(BraveAdsUserActivityFeatureTest, UserActivityTimeWindow) {
  // Act & Assert
  EXPECT_EQ(base::Minutes(15), kUserActivityTimeWindow.Get());
}

TEST(BraveAdsUserActivityFeatureTest, UserActivityThreshold) {
  // Act & Assert
  EXPECT_DOUBLE_EQ(0.0, kUserActivityThreshold.Get());
}

}  // namespace brave_ads
