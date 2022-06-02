/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/notification_ads/notification_ad.h"

#include <memory>

#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ad_events/notification_ads/notification_ad_observer.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"
#include "bat/ads/internal/deprecated/creatives/notification_ads/notification_ads.h"
#include "bat/ads/internal/serving/serving_features.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kPlacementId[] = "d2ef9bb0-a0dc-472c-bc49-62105bb6da68";
}  // namespace

class BatAdsNotificationAdTest : public NotificationAdObserver,
                                 public UnitTestBase {
 protected:
  BatAdsNotificationAdTest()
      : notification_ad_(std::make_unique<NotificationAd>()) {
    notification_ad_->AddObserver(this);
  }

  ~BatAdsNotificationAdTest() override = default;

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
      const std::string& placement_id,
      const mojom::NotificationAdEventType event_type) override {
    did_fail_to_fire_event_ = true;
  }

  NotificationAdInfo BuildAndSaveNotificationAd() {
    const CreativeNotificationAdInfo& creative_ad =
        BuildCreativeNotificationAd();

    const NotificationAdInfo& ad = BuildNotificationAd(creative_ad);
    NotificationAds::Get()->PushBack(ad);
    return ad;
  }

  void ExpectAdEventCountEquals(const ConfirmationType& confirmation_type,
                                const int expected_count) {
    database::table::AdEvents database_table;
    database_table.GetAll(
        [=](const bool success, const AdEventList& ad_events) {
          ASSERT_TRUE(success);

          const int count = GetAdEventCount(AdType::kNotificationAd,
                                            confirmation_type, ad_events);
          EXPECT_EQ(expected_count, count);
        });
  }

  std::unique_ptr<NotificationAd> notification_ad_;

  NotificationAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_dismiss_ad_ = false;
  bool did_time_out_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BatAdsNotificationAdTest, FireServedEvent) {
  // Arrange
  const NotificationAdInfo& ad = BuildAndSaveNotificationAd();

  // Act
  notification_ad_->FireEvent(ad.placement_id,
                              mojom::NotificationAdEventType::kServed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_TRUE(NotificationAds::Get()->Exists(ad.placement_id));

  ExpectAdEventCountEquals(ConfirmationType::kServed, 1);
}

TEST_F(BatAdsNotificationAdTest, FireViewedEvent) {
  // Arrange
  const NotificationAdInfo& ad = BuildAndSaveNotificationAd();

  // Act
  notification_ad_->FireEvent(ad.placement_id,
                              mojom::NotificationAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_TRUE(NotificationAds::Get()->Exists(ad.placement_id));

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 1);
}

TEST_F(BatAdsNotificationAdTest, FireClickedEvent) {
  // Arrange
  const NotificationAdInfo& ad = BuildAndSaveNotificationAd();

  // Act
  notification_ad_->FireEvent(ad.placement_id,
                              mojom::NotificationAdEventType::kClicked);

  // Asser
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_FALSE(NotificationAds::Get()->Exists(ad.placement_id));

  ExpectAdEventCountEquals(ConfirmationType::kClicked, 1);
}

TEST_F(BatAdsNotificationAdTest, FireDismissedEvent) {
  // Arrange
  const NotificationAdInfo& ad = BuildAndSaveNotificationAd();

  // Act
  notification_ad_->FireEvent(ad.placement_id,
                              mojom::NotificationAdEventType::kDismissed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_dismiss_ad_);
  EXPECT_FALSE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_FALSE(NotificationAds::Get()->Exists(ad.placement_id));

  ExpectAdEventCountEquals(ConfirmationType::kDismissed, 1);
}

TEST_F(BatAdsNotificationAdTest, FireTimedOutEvent) {
  // Arrange
  const NotificationAdInfo& ad = BuildAndSaveNotificationAd();

  // Act
  notification_ad_->FireEvent(ad.placement_id,
                              mojom::NotificationAdEventType::kTimedOut);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_dismiss_ad_);
  EXPECT_TRUE(did_time_out_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(ad, ad_);
  EXPECT_FALSE(NotificationAds::Get()->Exists(ad.placement_id));
}

TEST_F(BatAdsNotificationAdTest, DoNotFireEventIfUuidWasNotFound) {
  // Arrange

  // Act
  notification_ad_->FireEvent(kPlacementId,
                              mojom::NotificationAdEventType::kViewed);

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
