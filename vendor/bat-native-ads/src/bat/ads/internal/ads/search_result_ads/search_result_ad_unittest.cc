/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/search_result_ads/search_result_ad.h"

#include <memory>

#include "base/guid.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/ads/permission_rules_unittest_util.h"
#include "bat/ads/internal/ads/search_result_ads/search_result_ad_builder.h"
#include "bat/ads/internal/ads/search_result_ads/search_result_ad_observer.h"
#include "bat/ads/internal/bundle/creative_search_result_ad_info.h"
#include "bat/ads/internal/bundle/creative_search_result_ad_unittest_util.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "bat/ads/search_result_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kUuid[] = "d2ef9bb0-a0dc-472c-bc49-62105bb6da68";
constexpr char kInvalidUuid[] = "";

constexpr char kCreativeInstanceId[] = "1547f94f-9086-4db9-a441-efb2f0365269";
constexpr char kInvalidCreativeInstanceId[] = "";

}  // namespace

class BatAdsSearchResultAdTest : public SearchResultAdObserver,
                                 public UnitTestBase {
 protected:
  BatAdsSearchResultAdTest()
      : search_result_ad_(std::make_unique<SearchResultAd>()) {
    search_result_ad_->AddObserver(this);
  }

  ~BatAdsSearchResultAdTest() override = default;

  void OnSearchResultAdServed(const SearchResultAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnSearchResultAdViewed(const SearchResultAdInfo& ad) override {
    ad_ = ad;
    did_view_ad_ = true;
  }

  void OnSearchResultAdClicked(const SearchResultAdInfo& ad) override {
    ad_ = ad;
    did_click_ad_ = true;
  }

  void OnSearchResultAdEventFailed(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const mojom::SearchResultAdEventType event_type) override {
    did_fail_to_fire_event_ = true;
  }

  CreativeSearchResultAdInfo BuildAndSaveCreativeAd() {
    CreativeSearchResultAdList creative_ads;
    const CreativeSearchResultAdInfo& creative_ad =
        BuildCreativeSearchResultAd();
    creative_ads.push_back(creative_ad);

    SaveCreativeAds(creative_ads);

    return creative_ad;
  }

  void ExpectAdEventCountEquals(const ConfirmationType& confirmation_type,
                                const int expected_count) {
    database::table::AdEvents database_table;
    database_table.GetAll(
        [=](const bool success, const AdEventList& ad_events) {
          ASSERT_TRUE(success);

          const int count = GetAdEventCount(AdType::kSearchResultAd,
                                            confirmation_type, ad_events);
          EXPECT_EQ(expected_count, count);
        });
  }

  std::unique_ptr<SearchResultAd> search_result_ad_;

  SearchResultAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BatAdsSearchResultAdTest, FireViewedEvent) {
  // Arrange
  ForcePermissionRules();

  const CreativeSearchResultAdInfo& creative_ad = BuildAndSaveCreativeAd();

  // Act
  search_result_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                               mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const SearchResultAdInfo& expected_ad =
      BuildSearchResultAd(creative_ad, kUuid);
  EXPECT_EQ(expected_ad, ad_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 1);
}

TEST_F(BatAdsSearchResultAdTest, FireClickedEvent) {
  // Arrange
  ForcePermissionRules();

  const CreativeSearchResultAdInfo& creative_ad = BuildAndSaveCreativeAd();

  // Act
  search_result_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                               mojom::SearchResultAdEventType::kClicked);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const SearchResultAdInfo& expected_ad =
      BuildSearchResultAd(creative_ad, kUuid);
  EXPECT_EQ(expected_ad, ad_);

  ExpectAdEventCountEquals(ConfirmationType::kClicked, 1);
}

TEST_F(BatAdsSearchResultAdTest, DoNotFireViewedEventIfAlreadyFired) {
  // Arrange
  ForcePermissionRules();

  const CreativeSearchResultAdInfo& creative_ad = BuildAndSaveCreativeAd();

  search_result_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                               mojom::SearchResultAdEventType::kViewed);

  // Act
  search_result_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                               mojom::SearchResultAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, 1);
}

TEST_F(BatAdsSearchResultAdTest, DoNotFireEventWithInvalidUuid) {
  // Arrange

  // Act
  search_result_ad_->FireEvent(kInvalidUuid, kCreativeInstanceId,
                               mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

TEST_F(BatAdsSearchResultAdTest, DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange

  // Act
  search_result_ad_->FireEvent(kUuid, kInvalidCreativeInstanceId,
                               mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

TEST_F(BatAdsSearchResultAdTest, DoNotFireEventWhenNotPermitted) {
  // Arrange
  const CreativeSearchResultAdInfo& creative_ad = BuildAndSaveCreativeAd();

  // Act
  search_result_ad_->FireEvent(kUuid, creative_ad.creative_instance_id,
                               mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

TEST_F(BatAdsSearchResultAdTest,
       DoNotFireEventIfCreativeInstanceIdWasNotFound) {
  // Arrange
  ForcePermissionRules();

  // Act
  search_result_ad_->FireEvent(kUuid, kCreativeInstanceId,
                               mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

TEST_F(BatAdsSearchResultAdTest, FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRules();

  const CreativeSearchResultAdInfo& creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo& ad_event = BuildAdEvent(
      creative_ad, AdType::kSearchResultAd, ConfirmationType::kViewed, Now());

  const int ads_per_hour = features::GetMaximumSearchResultAdsPerHour();

  FireAdEvents(ad_event, ads_per_hour - 1);

  const std::string& uuid = base::GenerateGUID();

  // Act
  search_result_ad_->FireEvent(uuid, creative_ad.creative_instance_id,
                               mojom::SearchResultAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, ads_per_hour);
}

TEST_F(BatAdsSearchResultAdTest, DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRules();

  const CreativeSearchResultAdInfo& creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo& ad_event = BuildAdEvent(
      creative_ad, AdType::kSearchResultAd, ConfirmationType::kViewed, Now());

  const int ads_per_hour = features::GetMaximumSearchResultAdsPerHour();
  FireAdEvents(ad_event, ads_per_hour);

  const std::string& uuid = base::GenerateGUID();

  // Act
  search_result_ad_->FireEvent(uuid, creative_ad.creative_instance_id,
                               mojom::SearchResultAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, ads_per_hour);
}

TEST_F(BatAdsSearchResultAdTest, FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRules();

  const CreativeSearchResultAdInfo& creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo& ad_event = BuildAdEvent(
      creative_ad, AdType::kSearchResultAd, ConfirmationType::kViewed, Now());

  const int ads_per_day = features::GetMaximumSearchResultAdsPerDay();

  FireAdEvents(ad_event, ads_per_day - 1);

  AdvanceClock(base::Hours(1));

  const std::string& uuid = base::GenerateGUID();

  // Act
  search_result_ad_->FireEvent(uuid, creative_ad.creative_instance_id,
                               mojom::SearchResultAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, ads_per_day);
}

TEST_F(BatAdsSearchResultAdTest, DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRules();

  const CreativeSearchResultAdInfo& creative_ad = BuildAndSaveCreativeAd();
  const AdEventInfo& ad_event = BuildAdEvent(
      creative_ad, AdType::kSearchResultAd, ConfirmationType::kViewed, Now());

  const int ads_per_day = features::GetMaximumSearchResultAdsPerDay();

  FireAdEvents(ad_event, ads_per_day);

  AdvanceClock(base::Hours(1));

  const std::string& uuid = base::GenerateGUID();

  // Act
  search_result_ad_->FireEvent(uuid, creative_ad.creative_instance_id,
                               mojom::SearchResultAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, ads_per_day);
}

}  // namespace ads
