/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/notification_ad_util.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"
#include "bat/ads/notification_ad_info.h"
#include "brave/components/brave_ads/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::Invoke;

namespace ads {

namespace {

void BuildAndShowNotificationAd() {
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  ShowNotificationAd(ad);
}

}  // namespace

class BatAdsNotificationAdUtilTest : public UnitTestBase {};

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

TEST_F(BatAdsNotificationAdUtilTest, ShowNotificationAd) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([](const NotificationAdInfo& ad) {
        // Act

        // Assert
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
      }));

  BuildAndShowNotificationAd();
}

TEST_F(BatAdsNotificationAdUtilTest, DismissNotificationAd) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));

        // Act
        DismissNotificationAd(ad.placement_id);

        // Assert
        ASSERT_FALSE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
      }));

  BuildAndShowNotificationAd();
}

TEST_F(BatAdsNotificationAdUtilTest, CloseNotificationAd) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, CloseNotificationAd)
      .WillOnce(Invoke([](const std::string& placement_id) {
        // Act

        // Assert
        ASSERT_FALSE(
            NotificationAdManager::GetInstance()->Exists(placement_id));
      }));

  EXPECT_CALL(*ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));

        // Act
        CloseNotificationAd(ad.placement_id);

        // Assert
        ASSERT_FALSE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
      }));

  BuildAndShowNotificationAd();
}

TEST_F(BatAdsNotificationAdUtilTest, NotificationAdTimedOut) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));

        // Act
        NotificationAdTimedOut(ad.placement_id);

        // Assert
        ASSERT_FALSE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
      }));

  BuildAndShowNotificationAd();
}

}  // namespace ads
