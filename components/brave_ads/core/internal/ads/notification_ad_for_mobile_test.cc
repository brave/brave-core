/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/time/time.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/history/history_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/p2a/impressions/p2a_impression.h"
#include "brave/components/brave_ads/core/internal/privacy/p2a/opportunities/p2a_opportunity.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::_;
using ::testing::Invoke;

class BraveAdsNotificationAdForMobileIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);

    ForcePermissionRulesForTesting();
  }

  void SetUpMocks() override {
    MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

    const URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK,
           /*response_body*/ "/catalog_with_notification_ad.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }

  void ServeNextAd() {
    ASSERT_TRUE(ShouldServeAdsAtRegularIntervals());

    const std::string name =
        privacy::p2a::GetAdOpportunityNameForAdType(AdType::kNotificationAd);
    EXPECT_CALL(ads_client_mock_, RecordP2AEvent(name, _));

    FastForwardClockTo(ServeAdAt());
  }

  void ServeAd() {
    NotifyUserDidBecomeActive(base::TimeDelta::Min(),
                              /*screen_was_locked*/ false);
  }
};

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest,
       ServeAtRegularIntervals) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
      }));

  // Act
  ServeNextAd();

  // Assert
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kNotificationAd, ConfirmationType::kServed));
  EXPECT_EQ(0U, GetHistoryItemCount());
  EXPECT_EQ(0U, GetTransactionCount());
}

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest,
       DoNotServeWhenUserBecomesActive) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, ShowNotificationAd(_)).Times(0);

  // Act
  ServeAd();

  // Assert
  EXPECT_EQ(
      0U, GetAdEventCount(AdType::kNotificationAd, ConfirmationType::kServed));
}

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest, TriggerServedEvent) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));

        // Act
        GetAds().TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kServed);

        // Assert
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
        EXPECT_EQ(1U, GetAdEventCount(AdType::kNotificationAd,
                                      ConfirmationType::kServed));
        EXPECT_EQ(0U, GetHistoryItemCount());
        EXPECT_EQ(0U, GetTransactionCount());
      }));

  ServeNextAd();
}

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest, TriggerViewedEvent) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));

        // Act
        GetAds().TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kViewed);

        // Assert
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
        EXPECT_EQ(1U, GetAdEventCount(AdType::kNotificationAd,
                                      ConfirmationType::kViewed));
        EXPECT_EQ(1U, GetHistoryItemCount());
        EXPECT_EQ(1U, GetTransactionCount());
      }));

  const std::string name =
      privacy::p2a::GetAdImpressionNameForAdType(AdType::kNotificationAd);
  EXPECT_CALL(ads_client_mock_, RecordP2AEvent(name, _));

  ServeNextAd();
}

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest, TriggerClickedEvent) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
        EXPECT_CALL(ads_client_mock_, CloseNotificationAd(ad.placement_id));

        // Act
        GetAds().TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kClicked);

        // Assert
        EXPECT_EQ(1U, GetAdEventCount(AdType::kNotificationAd,
                                      ConfirmationType::kClicked));
        EXPECT_EQ(1U, GetHistoryItemCount());
        EXPECT_EQ(1U, GetTransactionCount());
      }));

  EXPECT_CALL(ads_client_mock_, AddTrainingSample(_));

  ServeNextAd();
}

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest, TriggerDismissedEvent) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));

        // Act
        GetAds().TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kDismissed);

        // Assert
        EXPECT_FALSE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
        EXPECT_EQ(1U, GetAdEventCount(AdType::kNotificationAd,
                                      ConfirmationType::kDismissed));
        EXPECT_EQ(1U, GetHistoryItemCount());
        EXPECT_EQ(1U, GetTransactionCount());
      }));

  EXPECT_CALL(ads_client_mock_, AddTrainingSample(_));

  ServeNextAd();
}

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest, TriggerTimedOutEvent) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));

        // Act
        GetAds().TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kTimedOut);

        // Assert
        EXPECT_FALSE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
        EXPECT_EQ(0U, GetHistoryItemCount());
        EXPECT_EQ(0U, GetTransactionCount());
      }));

  EXPECT_CALL(ads_client_mock_, AddTrainingSample(_));

  ServeNextAd();
}

}  // namespace brave_ads
