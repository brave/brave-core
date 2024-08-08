/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/mock_callback.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_test_util.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdIntegrationTest : public test::TestBase {
 protected:
  void SetUp() override { test::TestBase::SetUp(/*is_integration_test=*/true); }

  void SetUpMocks() override {
    const test::URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK,
           /*response_body=*/"/catalog_with_notification_ad.json"}}}};
    test::MockUrlResponses(ads_client_mock_, url_responses);
  }

  void ServeAd() {
    NotifyUserDidBecomeActive(/*idle_time=*/base::TimeDelta::Min(),
                              /*screen_was_locked=*/false);
  }
};

TEST_F(BraveAdsNotificationAdIntegrationTest, ServeAd) {
  // Arrange
  test::ForcePermissionRules();

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, RecordP2AEvents);

  EXPECT_CALL(ads_client_mock_, ShowNotificationAd);

  ServeAd();
}

TEST_F(BraveAdsNotificationAdIntegrationTest,
       DoNotServeIfPermissionRulesAreDenied) {
  // Act & Assert
  EXPECT_CALL(ads_client_mock_, RecordP2AEvents).Times(0);

  EXPECT_CALL(ads_client_mock_, ShowNotificationAd).Times(0);

  ServeAd();
}

TEST_F(BraveAdsNotificationAdIntegrationTest,
       ShouldNotServeAtRegularIntervals) {
  // Act & Assert
  EXPECT_FALSE(ShouldServeAdsAtRegularIntervals());
}

TEST_F(BraveAdsNotificationAdIntegrationTest, TriggerViewedEvent) {
  // Arrange
  test::ForcePermissionRules();

  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce(::testing::Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));

        // Act & Assert
        base::MockCallback<TriggerAdEventCallback> callback;
        EXPECT_CALL(callback, Run(/*success=*/true));
        GetAds().TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kViewedImpression,
            callback.Get());

        EXPECT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
      }));

  ServeAd();
}

TEST_F(BraveAdsNotificationAdIntegrationTest, TriggerClickedEvent) {
  // Arrange
  test::ForcePermissionRules();

  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce(::testing::Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));

        // Act & Assert
        EXPECT_CALL(ads_client_mock_, CloseNotificationAd(ad.placement_id));

        base::MockCallback<TriggerAdEventCallback> callback;
        EXPECT_CALL(callback, Run(/*success=*/true));
        GetAds().TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kClicked,
            callback.Get());

        EXPECT_FALSE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
      }));

  ServeAd();
}

TEST_F(BraveAdsNotificationAdIntegrationTest, TriggerDismissedEvent) {
  // Arrange
  test::ForcePermissionRules();

  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce(::testing::Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));

        // Act & Assert
        base::MockCallback<TriggerAdEventCallback> callback;
        EXPECT_CALL(callback, Run(/*success=*/true));
        GetAds().TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kDismissed,
            callback.Get());

        EXPECT_FALSE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
      }));

  ServeAd();
}

TEST_F(BraveAdsNotificationAdIntegrationTest, TriggerTimedOutEvent) {
  // Arrange
  test::ForcePermissionRules();

  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce(::testing::Invoke([=](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));

        // Act & Assert
        base::MockCallback<TriggerAdEventCallback> callback;
        EXPECT_CALL(callback, Run(/*success=*/true));
        GetAds().TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kTimedOut,
            callback.Get());

        EXPECT_FALSE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
      }));

  ServeAd();
}

}  // namespace brave_ads
