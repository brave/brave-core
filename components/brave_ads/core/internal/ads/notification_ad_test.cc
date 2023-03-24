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
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/history/history_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/p2a/impressions/p2a_impression.h"
#include "brave/components/brave_ads/core/internal/privacy/p2a/opportunities/p2a_opportunity.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

using ::testing::_;
using ::testing::Invoke;

class BatAdsNotificationAdIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);

    ForcePermissionRulesForTesting();
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {"/v9/catalog",
         {{net::HTTP_OK, "/catalog_with_notification_ad.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }

  void ServeAd() {
    const std::string name =
        privacy::p2a::GetAdOpportunityNameForAdType(AdType::kNotificationAd);
    EXPECT_CALL(*ads_client_mock_, RecordP2AEvent(name, _));

    GetAds()->OnUserDidBecomeActive(base::TimeDelta::Min(),
                                    /*screen_was_locked*/ false);
  }
};

TEST_F(BatAdsNotificationAdIntegrationTest, Serve) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
      }));

  // Act
  ServeAd();

  // Assert
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kNotificationAd, ConfirmationType::kServed));
  EXPECT_EQ(0, GetHistoryItemCount());
  EXPECT_EQ(0, GetTransactionCount());
}

TEST_F(BatAdsNotificationAdIntegrationTest, DoNotServeAtRegularIntervals) {
  // Arrange

  // Act

  // Assert
  ASSERT_FALSE(notification_ads::ShouldServeAdsAtRegularIntervals());
}

TEST_F(BatAdsNotificationAdIntegrationTest, TriggerServedEvent) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));

        // Act
        GetAds()->TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kServed);

        // Assert
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
        EXPECT_EQ(1, GetAdEventCount(AdType::kNotificationAd,
                                     ConfirmationType::kServed));
        EXPECT_EQ(0, GetHistoryItemCount());
        EXPECT_EQ(0, GetTransactionCount());
      }));

  ServeAd();
}

TEST_F(BatAdsNotificationAdIntegrationTest, TriggerViewedEvent) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));

        // Act
        GetAds()->TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kViewed);

        // Assert
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
        EXPECT_EQ(1, GetAdEventCount(AdType::kNotificationAd,
                                     ConfirmationType::kViewed));
        EXPECT_EQ(1, GetHistoryItemCount());
        EXPECT_EQ(1, GetTransactionCount());
      }));

  const std::string name =
      privacy::p2a::GetAdImpressionNameForAdType(AdType::kNotificationAd);
  EXPECT_CALL(*ads_client_mock_, RecordP2AEvent(name, _));

  ServeAd();
}

TEST_F(BatAdsNotificationAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
        EXPECT_CALL(*ads_client_mock_, CloseNotificationAd(ad.placement_id));

        // Act
        GetAds()->TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kClicked);

        // Assert
        EXPECT_EQ(1, GetAdEventCount(AdType::kNotificationAd,
                                     ConfirmationType::kClicked));
        EXPECT_EQ(1, GetHistoryItemCount());
        EXPECT_EQ(1, GetTransactionCount());
      }));

  EXPECT_CALL(*ads_client_mock_, AddTrainingSample(_));

  ServeAd();
}

TEST_F(BatAdsNotificationAdIntegrationTest, TriggerDismissedEvent) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));

        // Act
        GetAds()->TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kDismissed);

        // Assert
        EXPECT_FALSE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
        EXPECT_EQ(1, GetAdEventCount(AdType::kNotificationAd,
                                     ConfirmationType::kDismissed));
        EXPECT_EQ(1, GetHistoryItemCount());
        EXPECT_EQ(1, GetTransactionCount());
      }));

  EXPECT_CALL(*ads_client_mock_, AddTrainingSample(_));

  ServeAd();
}

TEST_F(BatAdsNotificationAdIntegrationTest, TriggerTimedOutEvent) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));

        // Act
        GetAds()->TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kTimedOut);

        // Assert
        EXPECT_FALSE(
            NotificationAdManager::GetInstance()->Exists(ad.placement_id));
        EXPECT_EQ(0, GetHistoryItemCount());
        EXPECT_EQ(0, GetTransactionCount());
      }));

  EXPECT_CALL(*ads_client_mock_, AddTrainingSample(_));

  ServeAd();
}

}  // namespace brave_ads
