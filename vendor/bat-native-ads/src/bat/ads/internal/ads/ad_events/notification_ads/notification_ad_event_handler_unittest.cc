/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_handler.h"

#include <memory>

#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_handler_observer.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"
#include "bat/ads/notification_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::notification_ads {

namespace {

constexpr char kPlacementId[] = "d2ef9bb0-a0dc-472c-bc49-62105bb6da68";

NotificationAdInfo BuildAndSaveNotificationAd() {
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  NotificationAdManager::GetInstance()->Add(ad);
  return ad;
}

}  // namespace

class BatAdsNotificationAdEventHandlerTest : public EventHandlerObserver,
                                             public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    event_handler_ = std::make_unique<EventHandler>();
    event_handler_->AddObserver(this);
  }

  void TearDown() override {
    event_handler_->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnNotificationAdServed(const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnNotificationAdViewed(const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_view_ad_ = true;
  }

  void OnNotificationAdClicked(const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_click_ad_ = true;
  }

  void OnNotificationAdDismissed(const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_dismiss_ad_ = true;
  }

  void OnNotificationAdTimedOut(const NotificationAdInfo& ad) override {
    ad_ = ad;
    did_time_out_ad_ = true;
  }

  void OnNotificationAdEventFailed(
      const std::string& /*placement_id*/,
      const mojom::NotificationAdEventType /*event_type*/) override {
    did_fail_to_fire_event_ = true;
  }

  std::unique_ptr<EventHandler> event_handler_;

  NotificationAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_dismiss_ad_ = false;
  bool did_time_out_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BatAdsNotificationAdEventHandlerTest, FireServedEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveNotificationAd();

  // Act
  event_handler_->FireEvent(ad.placement_id,
                            mojom::NotificationAdEventType::kServed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kNotificationAd, ConfirmationType::kServed));
}

TEST_F(BatAdsNotificationAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveNotificationAd();

  // Act
  event_handler_->FireEvent(ad.placement_id,
                            mojom::NotificationAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kNotificationAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsNotificationAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveNotificationAd();

  // Act
  event_handler_->FireEvent(ad.placement_id,
                            mojom::NotificationAdEventType::kClicked);

  // Asser
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kNotificationAd, ConfirmationType::kClicked));
}

TEST_F(BatAdsNotificationAdEventHandlerTest, FireDismissedEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveNotificationAd();

  // Act
  event_handler_->FireEvent(ad.placement_id,
                            mojom::NotificationAdEventType::kDismissed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_EQ(1, GetAdEventCount(AdType::kNotificationAd,
                               ConfirmationType::kDismissed));
}

TEST_F(BatAdsNotificationAdEventHandlerTest, FireTimedOutEvent) {
  // Arrange
  const NotificationAdInfo ad = BuildAndSaveNotificationAd();

  // Act
  event_handler_->FireEvent(ad.placement_id,
                            mojom::NotificationAdEventType::kTimedOut);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_TRUE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
}

TEST_F(BatAdsNotificationAdEventHandlerTest, DoNotFireEventIfUuidWasNotFound) {
  // Arrange

  // Act
  event_handler_->FireEvent(kPlacementId,
                            mojom::NotificationAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0, GetAdEventCount(AdType::kNotificationAd, ConfirmationType::kViewed));
}

}  // namespace ads::notification_ads
