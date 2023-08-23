/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/notification_ads/notification_ad_event_handler.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/notification_ads/notification_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/public/ad_type.h"
#include "brave/components/brave_ads/core/public/ads/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

NotificationAdInfo BuildAndSaveAd() {
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  NotificationAdManager::GetInstance().Add(ad);
  return ad;
}

}  // namespace

class BraveAdsNotificationAdEventHandlerTest
    : public NotificationAdEventHandlerDelegate,
      public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    event_handler_.SetDelegate(this);
  }

  void OnDidFireNotificationAdServedEvent(
      const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_fire_served_ad_event_ = true;
  }

  void OnDidFireNotificationAdViewedEvent(
      const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_fire_viewed_ad_event_ = true;
  }

  void OnDidFireNotificationAdClickedEvent(
      const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_fire_clicked_ad_event_ = true;
  }

  void OnDidFireNotificationAdDismissedEvent(
      const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_fire_dismissed_ad_event_ = true;
  }

  void OnDidFireNotificationAdTimedOutEvent(
      const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_fire_timed_out_ad_event_ = true;
  }

  void OnFailedToFireNotificationAdEvent(
      const std::string& /*placement_id*/,
      const mojom::NotificationAdEventType /*event_type*/) override {
    did_fail_to_fire_ad_event_ = true;
  }

  void FireEvent(const std::string& placement_id,
                 const mojom::NotificationAdEventType& event_type,
                 const bool should_fire_event) {
    base::MockCallback<FireNotificationAdEventHandlerCallback> callback;
    EXPECT_CALL(callback,
                Run(/*success*/ should_fire_event, placement_id, event_type));

    event_handler_.FireEvent(placement_id, event_type, callback.Get());
  }

  NotificationAdEventHandler event_handler_;

  NotificationAdInfo ad_;
  bool did_fire_served_ad_event_ = false;
  bool did_fire_viewed_ad_event_ = false;
  bool did_fire_clicked_ad_event_ = false;
  bool did_fire_dismissed_ad_event_ = false;
  bool did_fire_timed_out_ad_event_ = false;
  bool did_fail_to_fire_ad_event_ = false;
};

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireServedEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, mojom::NotificationAdEventType::kServed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_TRUE(did_fire_served_ad_event_);
  EXPECT_FALSE(did_fire_viewed_ad_event_);
  EXPECT_FALSE(did_fire_clicked_ad_event_);
  EXPECT_FALSE(did_fire_dismissed_ad_event_);
  EXPECT_FALSE(did_fire_timed_out_ad_event_);
  EXPECT_FALSE(did_fail_to_fire_ad_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNotificationAd,
                                          ConfirmationType::kServed));
}

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, mojom::NotificationAdEventType::kViewed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_FALSE(did_fire_served_ad_event_);
  EXPECT_TRUE(did_fire_viewed_ad_event_);
  EXPECT_FALSE(did_fire_clicked_ad_event_);
  EXPECT_FALSE(did_fire_dismissed_ad_event_);
  EXPECT_FALSE(did_fire_timed_out_ad_event_);
  EXPECT_FALSE(did_fail_to_fire_ad_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNotificationAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, mojom::NotificationAdEventType::kClicked,
            /*should_fire_event*/ true);

  // Asser
  EXPECT_FALSE(did_fire_served_ad_event_);
  EXPECT_FALSE(did_fire_viewed_ad_event_);
  EXPECT_TRUE(did_fire_clicked_ad_event_);
  EXPECT_FALSE(did_fire_dismissed_ad_event_);
  EXPECT_FALSE(did_fire_timed_out_ad_event_);
  EXPECT_FALSE(did_fail_to_fire_ad_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNotificationAd,
                                          ConfirmationType::kClicked));
}

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireDismissedEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, mojom::NotificationAdEventType::kDismissed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_FALSE(did_fire_served_ad_event_);
  EXPECT_FALSE(did_fire_viewed_ad_event_);
  EXPECT_FALSE(did_fire_clicked_ad_event_);
  EXPECT_TRUE(did_fire_dismissed_ad_event_);
  EXPECT_FALSE(did_fire_timed_out_ad_event_);
  EXPECT_FALSE(did_fail_to_fire_ad_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kNotificationAd,
                                          ConfirmationType::kDismissed));
}

TEST_F(BraveAdsNotificationAdEventHandlerTest, FireTimedOutEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveAd();

  // Act
  FireEvent(ad.placement_id, mojom::NotificationAdEventType::kTimedOut,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_FALSE(did_fire_served_ad_event_);
  EXPECT_FALSE(did_fire_viewed_ad_event_);
  EXPECT_FALSE(did_fire_clicked_ad_event_);
  EXPECT_FALSE(did_fire_dismissed_ad_event_);
  EXPECT_TRUE(did_fire_timed_out_ad_event_);
  EXPECT_FALSE(did_fail_to_fire_ad_event_);
  EXPECT_EQ(ad, ad_);
}

TEST_F(BraveAdsNotificationAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange

  // Act
  FireEvent(kPlacementId, mojom::NotificationAdEventType::kViewed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_fire_served_ad_event_);
  EXPECT_FALSE(did_fire_viewed_ad_event_);
  EXPECT_FALSE(did_fire_clicked_ad_event_);
  EXPECT_FALSE(did_fire_dismissed_ad_event_);
  EXPECT_FALSE(did_fire_timed_out_ad_event_);
  EXPECT_TRUE(did_fail_to_fire_ad_event_);
  EXPECT_FALSE(ad_.IsValid());
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kNotificationAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kNotificationAd,
                                          ConfirmationType::kViewed));
}

}  // namespace brave_ads
