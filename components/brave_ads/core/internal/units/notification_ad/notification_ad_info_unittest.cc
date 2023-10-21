/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdInfoTest : public UnitTestBase {};

TEST_F(BraveAdsNotificationAdInfoTest, IsValid) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act & Assert
  EXPECT_TRUE(ad.IsValid());
}

TEST_F(BraveAdsNotificationAdInfoTest, IsInvalid) {
  // Arrange
  const NotificationAdInfo ad;

  // Act & Assert
  EXPECT_FALSE(ad.IsValid());
}

}  // namespace brave_ads
