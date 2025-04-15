/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsNotificationAdServingFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kNotificationAdServingFeature));
}

TEST(BraveAdsNotificationAdServingFeatureTest, NotificationAdServingVersion) {
  // Act & Assert
  EXPECT_EQ(2, kNotificationAdServingVersion.Get());
}

TEST(BraveAdsNotificationAdServingFeatureTest, ServeFirstNotificationAdAfter) {
  // Act & Assert
  EXPECT_EQ(base::Minutes(2), kServeFirstNotificationAdAfter.Get());
}

TEST(BraveAdsNotificationAdServingFeatureTest,
     MinimumDelayBeforeServingNotificationAd) {
  // Act & Assert
  EXPECT_EQ(base::Minutes(1), kMinimumDelayBeforeServingNotificationAd.Get());
}

TEST(BraveAdsNotificationAdServingFeatureTest,
     RetryServingNotificationAdAfter) {
  // Act & Assert
  EXPECT_EQ(base::Minutes(2), kRetryServingNotificationAdAfter.Get());
}

}  // namespace brave_ads
