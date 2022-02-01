/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad.h"

#include <memory>

#include "base/guid.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad_observer.h"
#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad_permission_rules_unittest_util.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_unittest_util.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kUuid[] = "d2ef9bb0-a0dc-472c-bc49-62105bb6da68";
constexpr char kInvalidUuid[] = "";

constexpr char kCreativeInstanceId[] = "1547f94f-9086-4db9-a441-efb2f0365269";
constexpr char kInvalidCreativeInstanceId[] = "";

}  // namespace

class BatAdsNewTabPageAdTest : public NewTabPageAdObserver,
                               public UnitTestBase {
 protected:
  BatAdsNewTabPageAdTest()
      : new_tab_page_ad_(std::make_unique<NewTabPageAd>()) {
    new_tab_page_ad_->AddObserver(this);
  }

  ~BatAdsNewTabPageAdTest() override = default;

  void OnNewTabPageAdServed(const NewTabPageAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnNewTabPageAdViewed(const NewTabPageAdInfo& ad) override {
    ad_ = ad;
    did_view_ad_ = true;
  }

  void OnNewTabPageAdClicked(const NewTabPageAdInfo& ad) override {
    ad_ = ad;
    did_click_ad_ = true;
  }

  void OnNewTabPageAdEventFailed(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const mojom::NewTabPageAdEventType event_type) override {
    did_fail_to_fire_event_ = true;
  }

  CreativeNewTabPageAdInfo BuildAndSaveCreativeAd() {
    CreativeNewTabPageAdList creative_ads;
    const CreativeNewTabPageAdInfo& creative_ad = BuildCreativeNewTabPageAd();
    creative_ads.push_back(creative_ad);

    SaveCreativeAds(creative_ads);

    return creative_ad;
  }

  void ExpectAdEventCountEquals(const ConfirmationType& confirmation_type,
                                const int expected_count) {
    database::table::AdEvents database_table;
    database_table.GetAll([=](const bool success,
                              const AdEventList& ad_events) {
      ASSERT_TRUE(success);

      const int count =
          GetAdEventCount(AdType::kNewTabPageAd, confirmation_type, ad_events);
      EXPECT_EQ(expected_count, count);
    });
  }

  std::unique_ptr<NewTabPageAd> new_tab_page_ad_;

  NewTabPageAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BatAdsNewTabPageAdTest, FireViewedEvent) {
  // Arrange
  new_tab_page_ads::frequency_capping::ForcePermissionRules();

  const CreativeNewTabPageAdInfo& creative_ad = BuildAndSaveCreativeAd();

  // Act
  new_tab_page_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                              mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const NewTabPageAdInfo& expected_ad = BuildNewTabPageAd(creative_ad, kUuid);
  EXPECT_EQ(expected_ad, ad_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 1);
}

TEST_F(BatAdsNewTabPageAdTest, FireClickedEvent) {
  // Arrange
  new_tab_page_ads::frequency_capping::ForcePermissionRules();

  const CreativeNewTabPageAdInfo& creative_ad = BuildAndSaveCreativeAd();

  // Act
  new_tab_page_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                              mojom::NewTabPageAdEventType::kClicked);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const NewTabPageAdInfo& expected_ad = BuildNewTabPageAd(creative_ad, kUuid);
  EXPECT_EQ(expected_ad, ad_);

  ExpectAdEventCountEquals(ConfirmationType::kClicked, 1);
}

TEST_F(BatAdsNewTabPageAdTest, DoNotFireViewedEventIfAlreadyFired) {
  // Arrange
  new_tab_page_ads::frequency_capping::ForcePermissionRules();

  const CreativeNewTabPageAdInfo& creative_ad = BuildAndSaveCreativeAd();

  new_tab_page_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                              mojom::NewTabPageAdEventType::kViewed);

  // Act
  new_tab_page_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                              mojom::NewTabPageAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, 1);
}

TEST_F(BatAdsNewTabPageAdTest, DoNotFireEventWithInvalidUuid) {
  // Arrange

  // Act
  new_tab_page_ad_->FireEvent(kInvalidUuid, kCreativeInstanceId,
                              mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

TEST_F(BatAdsNewTabPageAdTest, DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange

  // Act
  new_tab_page_ad_->FireEvent(kUuid, kInvalidCreativeInstanceId,
                              mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

TEST_F(BatAdsNewTabPageAdTest, DoNotFireEventWhenNotPermitted) {
  // Arrange
  const CreativeNewTabPageAdInfo& creative_ad = BuildAndSaveCreativeAd();

  // Act
  new_tab_page_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                              mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

TEST_F(BatAdsNewTabPageAdTest, DoNotFireEventIfCreativeInstanceIdWasNotFound) {
  // Arrange
  new_tab_page_ads::frequency_capping::ForcePermissionRules();

  // Act
  new_tab_page_ad_->FireEvent(kUuid, kCreativeInstanceId,
                              mojom::NewTabPageAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

TEST_F(BatAdsNewTabPageAdTest, FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  new_tab_page_ads::frequency_capping::ForcePermissionRules();

  const CreativeNewTabPageAdInfo& creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo& ad_event = BuildAdEvent(creative_ad, AdType::kNewTabPageAd,
                                             ConfirmationType::kViewed, Now());

  const int ads_per_hour = features::GetMaximumNewTabPageAdsPerHour();

  FireAdEvents(ad_event, ads_per_hour - 1);

  const std::string& uuid = base::GenerateGUID();

  // Act
  new_tab_page_ad_->FireEvent(uuid, creative_ad.creative_instance_id,
                              mojom::NewTabPageAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, ads_per_hour);
}

TEST_F(BatAdsNewTabPageAdTest, DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  new_tab_page_ads::frequency_capping::ForcePermissionRules();

  const CreativeNewTabPageAdInfo& creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo& ad_event = BuildAdEvent(creative_ad, AdType::kNewTabPageAd,
                                             ConfirmationType::kViewed, Now());

  const int ads_per_hour = features::GetMaximumNewTabPageAdsPerHour();

  FireAdEvents(ad_event, ads_per_hour);

  const std::string& uuid = base::GenerateGUID();

  // Act
  new_tab_page_ad_->FireEvent(uuid, creative_ad.creative_instance_id,
                              mojom::NewTabPageAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, ads_per_hour);
}

TEST_F(BatAdsNewTabPageAdTest, FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  new_tab_page_ads::frequency_capping::ForcePermissionRules();

  const CreativeNewTabPageAdInfo& creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo& ad_event = BuildAdEvent(creative_ad, AdType::kNewTabPageAd,
                                             ConfirmationType::kViewed, Now());

  const int ads_per_day = features::GetMaximumNewTabPageAdsPerDay();

  FireAdEvents(ad_event, ads_per_day - 1);

  AdvanceClock(base::Hours(1));

  const std::string& uuid = base::GenerateGUID();

  // Act
  new_tab_page_ad_->FireEvent(uuid, creative_ad.creative_instance_id,
                              mojom::NewTabPageAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, ads_per_day);
}

TEST_F(BatAdsNewTabPageAdTest, DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  new_tab_page_ads::frequency_capping::ForcePermissionRules();

  const CreativeNewTabPageAdInfo& creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo& ad_event = BuildAdEvent(creative_ad, AdType::kNewTabPageAd,
                                             ConfirmationType::kViewed, Now());

  const int ads_per_day = features::GetMaximumNewTabPageAdsPerDay();

  FireAdEvents(ad_event, ads_per_day);

  AdvanceClock(base::Hours(1));

  const std::string& uuid = base::GenerateGUID();

  // Act
  new_tab_page_ad_->FireEvent(uuid, creative_ad.creative_instance_id,
                              mojom::NewTabPageAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, ads_per_day);
}

}  // namespace ads
