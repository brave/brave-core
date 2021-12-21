/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_notifications/ad_notification.h"

#include <memory>

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notification_builder.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notification_observer.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_unittest_util.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kUuid[] = "d2ef9bb0-a0dc-472c-bc49-62105bb6da68";

}  // namespace

class BatAdsAdNotificationTest : public AdNotificationObserver,
                                 public UnitTestBase {
 protected:
  BatAdsAdNotificationTest()
      : ad_notification_(std::make_unique<AdNotification>()) {
    ad_notification_->AddObserver(this);
  }

  ~BatAdsAdNotificationTest() override = default;

  void OnAdNotificationServed(const AdNotificationInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnAdNotificationViewed(const AdNotificationInfo& ad) override {
    ad_ = ad;
    did_view_ad_ = true;
  }

  void OnAdNotificationClicked(const AdNotificationInfo& ad) override {
    ad_ = ad;
    did_click_ad_ = true;
  }

  void OnAdNotificationDismissed(const AdNotificationInfo& ad) override {
    ad_ = ad;
    did_dismiss_ad_ = true;
  }

  void OnAdNotificationTimedOut(const AdNotificationInfo& ad) override {
    ad_ = ad;
    did_time_out_ad_ = true;
  }

  void OnAdNotificationEventFailed(
      const std::string& uuid,
      const mojom::AdNotificationEventType event_type) override {
    did_fail_to_fire_event_ = true;
  }

  AdNotificationInfo BuildAndSaveAdNotification() {
    const CreativeAdNotificationInfo& creative_ad =
        BuildCreativeAdNotification();

    const AdNotificationInfo& ad = BuildAdNotification(creative_ad);
    AdNotifications::Get()->PushBack(ad);
    return ad;
  }

  void ExpectAdEventCountEquals(const ConfirmationType& confirmation_type,
                                const int expected_count) {
    database::table::AdEvents database_table;
    database_table.GetAll(
        [=](const bool success, const AdEventList& ad_events) {
          ASSERT_TRUE(success);

          const int count = GetAdEventCount(AdType::kAdNotification,
                                            confirmation_type, ad_events);
          EXPECT_EQ(expected_count, count);
        });
  }

  std::unique_ptr<AdNotification> ad_notification_;

  AdNotificationInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_dismiss_ad_ = false;
  bool did_time_out_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BatAdsAdNotificationTest, FireServedEvent) {
  // Arrange
  const AdNotificationInfo& ad = BuildAndSaveAdNotification();

  // Act
  ad_notification_->FireEvent(ad.uuid, mojom::AdNotificationEventType::kServed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_TRUE(AdNotifications::Get()->Exists(ad.uuid));

  ExpectAdEventCountEquals(ConfirmationType::kServed, 1);
}

TEST_F(BatAdsAdNotificationTest, FireViewedEvent) {
  // Arrange
  const AdNotificationInfo& ad = BuildAndSaveAdNotification();

  // Act
  ad_notification_->FireEvent(ad.uuid, mojom::AdNotificationEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_TRUE(AdNotifications::Get()->Exists(ad.uuid));

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 1);
}

TEST_F(BatAdsAdNotificationTest, FireClickedEvent) {
  // Arrange
  const AdNotificationInfo& ad = BuildAndSaveAdNotification();

  // Act
  ad_notification_->FireEvent(ad.uuid,
                              mojom::AdNotificationEventType::kClicked);

  // Asser
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_FALSE(AdNotifications::Get()->Exists(ad.uuid));

  ExpectAdEventCountEquals(ConfirmationType::kClicked, 1);
}

TEST_F(BatAdsAdNotificationTest, FireDismissedEvent) {
  // Arrange
  const AdNotificationInfo& ad = BuildAndSaveAdNotification();

  // Act
  ad_notification_->FireEvent(ad.uuid,
                              mojom::AdNotificationEventType::kDismissed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_FALSE(AdNotifications::Get()->Exists(ad.uuid));

  ExpectAdEventCountEquals(ConfirmationType::kDismissed, 1);
}

TEST_F(BatAdsAdNotificationTest, FireTimedOutEvent) {
  // Arrange
  const AdNotificationInfo& ad = BuildAndSaveAdNotification();

  // Act
  ad_notification_->FireEvent(ad.uuid,
                              mojom::AdNotificationEventType::kTimedOut);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_TRUE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_FALSE(AdNotifications::Get()->Exists(ad.uuid));
}

TEST_F(BatAdsAdNotificationTest, DoNotFireEventIfUuidWasNotFound) {
  // Arrange

  // Act
  ad_notification_->FireEvent(kUuid, mojom::AdNotificationEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

}  // namespace ads
