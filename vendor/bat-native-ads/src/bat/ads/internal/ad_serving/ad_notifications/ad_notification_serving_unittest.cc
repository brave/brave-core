/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving_features.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsAdServingFeaturesTest, AdNotificationServingEnabled) {
  // Arrange

  // Act
  const bool is_enabled = features::IsAdNotificationServingEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsAdServingFeaturesTest, DefaultBrowsingHistoryMaxCount) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(5000, features::GetBrowsingHistoryMaxCount());
}

TEST(BatAdsAdServingFeaturesTest, DefaultBrowsingHistoryDaysAgo) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(180, features::GetBrowsingHistoryDaysAgo());
}

}  // namespace ads
