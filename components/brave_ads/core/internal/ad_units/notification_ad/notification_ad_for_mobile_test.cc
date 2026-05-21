/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/test/permission_rules_test_util.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdForMobileIntegrationTest : public test::TestBase {
 public:
  BraveAdsNotificationAdForMobileIntegrationTest()
      : test::TestBase(/*is_integration_test=*/true) {}

 protected:
  void SetUpMocks() override {
    fake_operating_system_.SetType(OperatingSystemType::kAndroid);
    OperatingSystem::SetForTesting(&fake_operating_system_);

    const test::URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK,
           /*response_body=*/"/catalog_with_notification_ad.json"}}}};
    test::MockUrlResponses(ads_client_mock_, url_responses);
  }

  void ServeAd() {
    ASSERT_TRUE(ShouldServeAdsAtRegularIntervals());
    FastForwardClockTo(ServeAdAt());
  }
};

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest,
       ServeWhenUserBecomesActive) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      {kNotificationAdServingFeature});

  test::ForcePermissionRules();

  // Act & Assert
  base::RunLoop run_loop;
  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce([&](const NotificationAdInfo& ad) {
        EXPECT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
        run_loop.Quit();
      });

  ServeAd();
  run_loop.Run();
}

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest,
       DoNotServeWhenUserBecomesActiveIfPermissionRulesAreDenied) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      {kNotificationAdServingFeature});

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, ShowNotificationAd).Times(0);

  ServeAd();
}

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest,
       ServeAtRegularIntervals) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      {kNotificationAdServingFeature});

  // Act & Assert
  EXPECT_TRUE(ShouldServeAdsAtRegularIntervals());
}

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest, TriggerViewedEvent) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      {kNotificationAdServingFeature});

  test::ForcePermissionRules();

  NotificationAdInfo ad;
  {
    base::RunLoop run_loop;
    EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
        .WillOnce([&](const NotificationAdInfo& served_ad) {
          ad = served_ad;
          run_loop.Quit();
        });
    ServeAd();
    run_loop.Run();
  }
  ASSERT_TRUE(NotificationAdManager::GetInstance().Exists(ad.placement_id));

  // Act & Assert
  base::test::TestFuture<bool> test_future;
  GetAds().TriggerNotificationAdEvent(
      ad.placement_id, mojom::NotificationAdEventType::kViewedImpression,
      test_future.GetCallback());
  EXPECT_TRUE(test_future.Get());

  EXPECT_TRUE(NotificationAdManager::GetInstance().Exists(ad.placement_id));
}

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest, TriggerClickedEvent) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      {kNotificationAdServingFeature});

  test::ForcePermissionRules();

  NotificationAdInfo ad;
  {
    base::RunLoop run_loop;
    EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
        .WillOnce([&](const NotificationAdInfo& served_ad) {
          ad = served_ad;
          run_loop.Quit();
        });
    ServeAd();
    run_loop.Run();
  }
  ASSERT_TRUE(NotificationAdManager::GetInstance().Exists(ad.placement_id));

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, CloseNotificationAd(ad.placement_id));

  base::test::TestFuture<bool> test_future;
  GetAds().TriggerNotificationAdEvent(ad.placement_id,
                                      mojom::NotificationAdEventType::kClicked,
                                      test_future.GetCallback());
  EXPECT_TRUE(test_future.Get());

  EXPECT_FALSE(NotificationAdManager::GetInstance().Exists(ad.placement_id));
}

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest, TriggerDismissedEvent) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      {kNotificationAdServingFeature});

  test::ForcePermissionRules();

  NotificationAdInfo ad;
  {
    base::RunLoop run_loop;
    EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
        .WillOnce([&](const NotificationAdInfo& served_ad) {
          ad = served_ad;
          run_loop.Quit();
        });
    ServeAd();
    run_loop.Run();
  }
  ASSERT_TRUE(NotificationAdManager::GetInstance().Exists(ad.placement_id));

  // Act & Assert
  base::test::TestFuture<bool> test_future;
  GetAds().TriggerNotificationAdEvent(
      ad.placement_id, mojom::NotificationAdEventType::kDismissed,
      test_future.GetCallback());
  EXPECT_TRUE(test_future.Get());

  EXPECT_FALSE(NotificationAdManager::GetInstance().Exists(ad.placement_id));
}

TEST_F(BraveAdsNotificationAdForMobileIntegrationTest, TriggerTimedOutEvent) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      {kNotificationAdServingFeature});

  test::ForcePermissionRules();

  base::RunLoop run_loop;
  EXPECT_CALL(ads_client_mock_, ShowNotificationAd)
      .WillOnce([&](const NotificationAdInfo& ad) {
        ASSERT_TRUE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));

        // Act & Assert
        base::MockCallback<ResultCallback> callback;
        base::RunLoop ad_event_run_loop(
            base::RunLoop::Type::kNestableTasksAllowed);
        EXPECT_CALL(callback, Run(/*success=*/true))
            .WillOnce(
                base::test::RunOnceClosure(ad_event_run_loop.QuitClosure()));
        GetAds().TriggerNotificationAdEvent(
            ad.placement_id, mojom::NotificationAdEventType::kTimedOut,
            callback.Get());
        ad_event_run_loop.Run();

        EXPECT_FALSE(
            NotificationAdManager::GetInstance().Exists(ad.placement_id));
        run_loop.Quit();
      });

  ServeAd();
  run_loop.Run();
}

}  // namespace brave_ads
