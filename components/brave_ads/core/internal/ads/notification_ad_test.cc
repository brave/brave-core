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

class BraveAdsNotificationAdIntegrationTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUpForTesting(/*is_integration_test*/ true);

    ForcePermissionRulesForTesting();
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK,
           /*response_body*/ "/catalog_with_notification_ad.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }

  void ServeAd() {
    const std::string name =
        privacy::p2a::GetAdOpportunityNameForAdType(AdType::kNotificationAd);
    EXPECT_CALL(ads_client_mock_, RecordP2AEvent(name, _));

    NotifyUserDidBecomeActive(base::TimeDelta::Min(),
                              /*screen_was_locked*/ false);
  }
};

TEST_F(BraveAdsNotificationAdIntegrationTest, Serve) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce(Invoke([](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
      }));

  // Act
  ServeAd();

  // Assert
  EXPECT_EQ(
      1U, GetAdEventCount(AdType::kNotificationAd, ConfirmationType::kServed));
  EXPECT_EQ(0U, GetHistoryItemCount());
  EXPECT_EQ(0U, GetTransactionCount());
}

TEST_F(BraveAdsNotificationAdIntegrationTest, DoNotServeAtRegularIntervals) {
  // Arrange

  // Act

  // Assert
  ASSERT_FALSE(ShouldServeAdsAtRegularIntervals());
}

TEST_F(BraveAdsNotificationAdIntegrationTest, TriggerServedEvent) {
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

  ServeAd();
}

TEST_F(BraveAdsNotificationAdIntegrationTest, TriggerViewedEvent) {
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

  ServeAd();
}

TEST_F(BraveAdsNotificationAdIntegrationTest, TriggerClickedEvent) {
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

  ServeAd();
}

TEST_F(BraveAdsNotificationAdIntegrationTest, TriggerDismissedEvent) {
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

  ServeAd();
}

TEST_F(BraveAdsNotificationAdIntegrationTest, TriggerTimedOutEvent) {
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

  ServeAd();
}

}  // namespace brave_ads
