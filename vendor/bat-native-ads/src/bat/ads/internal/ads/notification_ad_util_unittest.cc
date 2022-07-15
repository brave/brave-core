/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/notification_ad_util.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::Invoke;

namespace ads {

class BatAdsNotificationAdUtilTest : public UnitTestBase {
 protected:
  BatAdsNotificationAdUtilTest() = default;

  ~BatAdsNotificationAdUtilTest() override = default;

  void ShowAdNotification() {
    const CreativeNotificationAdInfo creative_ad =
        BuildCreativeNotificationAd();
    const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
    ShowNotification(ad);
  }
};

TEST_F(BatAdsNotificationAdUtilTest, CanServeIfUserIsActive) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act

  // Assert
  EXPECT_TRUE(CanServeIfUserIsActive());
}

TEST_F(BatAdsNotificationAdUtilTest, DoNotServeIfUserIsActive) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  // Act

  // Assert
  EXPECT_FALSE(CanServeIfUserIsActive());
}

TEST_F(BatAdsNotificationAdUtilTest, ShouldServe) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  // Act

  // Assert
  EXPECT_TRUE(ShouldServe());
}

TEST_F(BatAdsNotificationAdUtilTest, ShouldNotServe) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  // Act

  // Assert
  EXPECT_FALSE(ShouldServe());
}

TEST_F(BatAdsNotificationAdUtilTest, CanServeAtRegularIntervals) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  // Act

  // Assert
  EXPECT_TRUE(CanServeAtRegularIntervals());
}

TEST_F(BatAdsNotificationAdUtilTest, DoNotServeAtRegularIntervals) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act

  // Assert
  EXPECT_FALSE(CanServeAtRegularIntervals());
}

TEST_F(BatAdsNotificationAdUtilTest, ShowNotification) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowNotification)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        // Act

        // Assert
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
      }));

  ShowAdNotification();
}

TEST_F(BatAdsNotificationAdUtilTest, DismissNotification) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowNotification)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));

        // Act
        DismissNotification(ad.placement_id);

        // Assert
        ASSERT_FALSE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
      }));

  ShowAdNotification();
}

TEST_F(BatAdsNotificationAdUtilTest, CloseNotification) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, CloseNotification)
      .WillOnce(Invoke([=](const std::string& placement_id) {
        // Act

        // Assert
        ASSERT_FALSE(
            NotificationAdManager::GetInstance()->Exists(placement_id));
      }));

  EXPECT_CALL(*ads_client_mock_, ShowNotification)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));

        // Act
        CloseNotification(ad.placement_id);

        // Assert
        ASSERT_FALSE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
      }));

  ShowAdNotification();
}

TEST_F(BatAdsNotificationAdUtilTest, NotificationTimedOut) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowNotification)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));

        // Act
        NotificationTimedOut(ad.placement_id);

        // Assert
        ASSERT_FALSE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
      }));

  ShowAdNotification();
}

}  // namespace ads
