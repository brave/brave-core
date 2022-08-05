/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/notification_ad_serving_util.h"

#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/internal/base/unittest/unittest_time_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace notification_ads {

class BatAdsNotificationAdServingUtilTest : public UnitTestBase {
 protected:
  BatAdsNotificationAdServingUtilTest() = default;

  ~BatAdsNotificationAdServingUtilTest() override = default;
};

TEST_F(BatAdsNotificationAdServingUtilTest, ShouldServeAdsAtRegularIntervals) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  // Act

  // Assert
  EXPECT_TRUE(ShouldServeAdsAtRegularIntervals());
}

TEST_F(BatAdsNotificationAdServingUtilTest,
       ShouldNotServeAdsAtRegularIntervals) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act

  // Assert
  EXPECT_FALSE(ShouldServeAdsAtRegularIntervals());
}

TEST_F(BatAdsNotificationAdServingUtilTest, SetServeAdAt) {
  // Arrange

  // Act
  SetServeAdAt(DistantFuture());

  // Assert
  EXPECT_EQ(DistantFuture(), ServeAdAt());
}

TEST_F(BatAdsNotificationAdServingUtilTest,
       CalculateDelayBeforeServingTheFirstAd) {
  // Arrange
  AdsClientHelper::GetInstance()->ClearPref(prefs::kServeAdAt);

  // Act

  // Assert
  EXPECT_EQ(base::Minutes(2), CalculateDelayBeforeServingAnAd());
}

TEST_F(BatAdsNotificationAdServingUtilTest,
       CalculateDelayBeforeServingAPastDueAd) {
  // Arrange
  SetServeAdAt(DistantPast());

  // Act

  // Assert
  EXPECT_EQ(base::Minutes(1), CalculateDelayBeforeServingAnAd());
}

TEST_F(BatAdsNotificationAdServingUtilTest, CalculateDelayBeforeServingAnAd) {
  // Arrange
  SetServeAdAt(DistantFuture());

  // Act

  // Assert
  EXPECT_EQ(DistantFuture() - Now(), CalculateDelayBeforeServingAnAd());
}

}  // namespace notification_ads
}  // namespace ads
