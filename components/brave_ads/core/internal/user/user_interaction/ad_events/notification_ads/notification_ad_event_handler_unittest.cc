/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/notification_ads/notification_ad_event_handler.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/notification_ads/notification_ad_event_handler_delegate_mock.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

NotificationAdInfo BuildAndSaveAd() {
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*should_use_random_uuids=*/false);
  NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  NotificationAdManager::GetInstance().Add(ad);
  return ad;
}

}  // namespace

class BraveAdsNotificationAdEventHandlerTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    event_handler_.SetDelegate(&delegate_mock_);
  }

  void FireEvent(const std::string& placement_id,
                 const mojom::NotificationAdEventType& event_type,
                 const bool should_fire_event) {
    base::MockCallback<FireNotificationAdEventHandlerCallback> callback;
    EXPECT_CALL(callback,
                Run(/*success=*/should_fire_event, placement_id, event_type));
    event_handler_.FireEvent(placement_id, event_type, callback.Get());
  }

  NotificationAdEventHandler event_handler_;
  ::testing::StrictMock<NotificationAdEventHandlerDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireServedEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireNotificationAdServedEvent(ad));

  FireEvent(ad.placement_id, mojom::NotificationAdEventType::kServed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireNotificationAdViewedEvent(ad));

  FireEvent(ad.placement_id, mojom::NotificationAdEventType::kViewed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireNotificationAdClickedEvent(ad));

  FireEvent(ad.placement_id, mojom::NotificationAdEventType::kClicked,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireDismissedEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireNotificationAdDismissedEvent(ad));

  FireEvent(ad.placement_id, mojom::NotificationAdEventType::kDismissed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireTimedOutEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireNotificationAdTimedOutEvent(ad));

  FireEvent(ad.placement_id, mojom::NotificationAdEventType::kTimedOut,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsNotificationAdEventHandlerTest,
       DoNotFireEventIfMissingPlacementId) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveAd();

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireNotificationAdEvent(
                                  kMissingPlacementId,
                                  mojom::NotificationAdEventType::kViewed));

  FireEvent(kMissingPlacementId, mojom::NotificationAdEventType::kViewed,
            /*should_fire_event=*/false);
}

}  // namespace brave_ads
