/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/search_result_ads/search_result_ad.h"

#include <memory>
#include <string>

#include "bat/ads/internal/account/deposits/deposits_database_table.h"
#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/conversions/conversion_info_aliases.h"
#include "bat/ads/internal/conversions/conversions_database_table.h"
#include "bat/ads/internal/creatives/permission_rules_unittest_util.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_builder.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "bat/ads/internal/serving/serving_features.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kPlacementId[] = "d2ef9bb0-a0dc-472c-bc49-62105bb6da68";
constexpr char kInvalidPlacementId[] = "";

constexpr char kCreativeInstanceId[] = "1547f94f-9086-4db9-a441-efb2f0365269";
constexpr char kInvalidCreativeInstanceId[] = "";

mojom::SearchResultAdPtr BuildAd(const std::string& placement_id,
                                 const std::string& creative_instance_id) {
  mojom::SearchResultAdPtr ad_mojom = mojom::SearchResultAd::New();

  ad_mojom->creative_instance_id = creative_instance_id;
  ad_mojom->placement_id = placement_id;
  ad_mojom->creative_set_id = "7a41297b-ff7f-4ca8-9787-b4c9c1105f01";
  ad_mojom->campaign_id = "be5d25ca-93e4-4a16-8f8b-4714abca31ed";
  ad_mojom->advertiser_id = "f82389c6-c6ca-4db5-99f9-724f038efddf";
  ad_mojom->target_url = GURL("https://brave.com");
  ad_mojom->headline_text = "headline";
  ad_mojom->description = "description";
  ad_mojom->value = 1.0;
  ad_mojom->conversion = mojom::Conversion::New();

  return ad_mojom;
}

mojom::SearchResultAdPtr BuildAdWithConversion(
    const std::string& placement_id,
    const std::string& creative_instance_id) {
  mojom::SearchResultAdPtr ad_mojom =
      BuildAd(placement_id, creative_instance_id);

  ad_mojom->conversion->type = "postview";
  ad_mojom->conversion->url_pattern = "https://brave.com/*";
  ad_mojom->conversion->advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  ad_mojom->conversion->observation_window = 3;
  ad_mojom->conversion->expire_at = DistantFutureAsTimestamp();

  return ad_mojom;
}

void ExpectAdEventCountEquals(const ConfirmationType& confirmation_type,
                              const int expected_count) {
  database::table::AdEvents database_table;
  database_table.GetAll([=](const bool success, const AdEventList& ad_events) {
    ASSERT_TRUE(success);

    const int count =
        GetAdEventCount(AdType::kSearchResultAd, confirmation_type, ad_events);
    EXPECT_EQ(expected_count, count);
  });
}

void ExpectDepositExistsForCreativeInstanceId(
    const std::string& creative_instance_id) {
  database::table::Deposits database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [=](const bool success,
          const absl::optional<DepositInfo>& deposit_optional) {
        ASSERT_TRUE(success);

        EXPECT_TRUE(deposit_optional);
      });
}

void ExpectConversionCountEquals(const size_t expected_count) {
  database::table::Conversions database_table;
  database_table.GetAll(
      [=](const bool success, const ConversionList& conversions) {
        ASSERT_TRUE(success);

        EXPECT_EQ(expected_count, conversions.size());
      });
}

}  // namespace

class BatAdsSearchResultAdTest : public SearchResultAdObserver,
                                 public UnitTestBase {
 protected:
  BatAdsSearchResultAdTest()
      : search_result_ad_(std::make_unique<SearchResultAd>()) {
    search_result_ad_->AddObserver(this);
  }

  ~BatAdsSearchResultAdTest() override = default;

  void FireEvent(const mojom::SearchResultAdPtr& ad_mojom,
                 const mojom::SearchResultAdEventType event_type) {
    search_result_ad_->FireEvent(
        ad_mojom, event_type,
        [](const bool success, const std::string& placement_id,
           const mojom::SearchResultAdEventType event_type) {});
  }

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
      const SearchResultAdInfo& ad,
      const mojom::SearchResultAdEventType event_type) override {
    did_fail_to_fire_event_ = true;
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

  const mojom::SearchResultAdPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  // Act
  FireEvent(ad_mojom, mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const SearchResultAdInfo expected_ad = BuildSearchResultAd(ad_mojom);
  EXPECT_EQ(expected_ad, ad_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 1);
  ExpectDepositExistsForCreativeInstanceId(kCreativeInstanceId);
  ExpectConversionCountEquals(0);
}

TEST_F(BatAdsSearchResultAdTest, FireViewedEventWithConversion) {
  // Arrange
  ForcePermissionRules();

  const mojom::SearchResultAdPtr ad_mojom =
      BuildAdWithConversion(kPlacementId, kCreativeInstanceId);

  // Act
  FireEvent(ad_mojom, mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const SearchResultAdInfo expected_ad = BuildSearchResultAd(ad_mojom);
  EXPECT_EQ(expected_ad, ad_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 1);
  ExpectDepositExistsForCreativeInstanceId(kCreativeInstanceId);
  ExpectConversionCountEquals(1);
}

TEST_F(BatAdsSearchResultAdTest, FireClickedEvent) {
  // Arrange
  ForcePermissionRules();

  const mojom::SearchResultAdPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  // Act
  FireEvent(ad_mojom, mojom::SearchResultAdEventType::kClicked);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const SearchResultAdInfo expected_ad = BuildSearchResultAd(ad_mojom);
  EXPECT_EQ(expected_ad, ad_);

  ExpectAdEventCountEquals(ConfirmationType::kClicked, 1);
}

TEST_F(BatAdsSearchResultAdTest, DoNotFireViewedEventIfAlreadyFired) {
  // Arrange
  ForcePermissionRules();

  const mojom::SearchResultAdPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  FireEvent(ad_mojom, mojom::SearchResultAdEventType::kViewed);

  // Act
  FireEvent(ad_mojom, mojom::SearchResultAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, 1);
  ExpectDepositExistsForCreativeInstanceId(kCreativeInstanceId);
  ExpectConversionCountEquals(0);
}

TEST_F(BatAdsSearchResultAdTest, DoNotFireEventWithInvalidPlacementId) {
  // Arrange
  const mojom::SearchResultAdPtr ad_mojom =
      BuildAd(kInvalidPlacementId, kCreativeInstanceId);

  // Act
  FireEvent(ad_mojom, mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

TEST_F(BatAdsSearchResultAdTest, DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange
  const mojom::SearchResultAdPtr ad_mojom =
      BuildAd(kPlacementId, kInvalidCreativeInstanceId);

  // Act
  FireEvent(ad_mojom, mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);

  ExpectAdEventCountEquals(ConfirmationType::kViewed, 0);
}

TEST_F(BatAdsSearchResultAdTest, DoNotFireEventWhenNotPermitted) {
  // Arrange
  const mojom::SearchResultAdPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  // Act
  FireEvent(ad_mojom, mojom::SearchResultAdEventType::kViewed);

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

  const mojom::SearchResultAdPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  const SearchResultAdInfo& ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo& ad_event = BuildAdEvent(ad, AdType::kSearchResultAd,
                                             ConfirmationType::kViewed, Now());

  const int ads_per_hour = features::GetMaximumSearchResultAdsPerHour();
  FireAdEvents(ad_event, ads_per_hour - 1);

  // Act
  FireEvent(ad_mojom, mojom::SearchResultAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, ads_per_hour);
  ExpectDepositExistsForCreativeInstanceId(kCreativeInstanceId);
  ExpectConversionCountEquals(0);
}

TEST_F(BatAdsSearchResultAdTest, DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRules();

  const mojom::SearchResultAdPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  const SearchResultAdInfo& ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo& ad_event = BuildAdEvent(ad, AdType::kSearchResultAd,
                                             ConfirmationType::kViewed, Now());

  const int ads_per_hour = features::GetMaximumSearchResultAdsPerHour();
  FireAdEvents(ad_event, ads_per_hour);

  // Act
  FireEvent(ad_mojom, mojom::SearchResultAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, ads_per_hour);
}

TEST_F(BatAdsSearchResultAdTest, FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRules();

  const mojom::SearchResultAdPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  const SearchResultAdInfo& ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo& ad_event = BuildAdEvent(ad, AdType::kSearchResultAd,
                                             ConfirmationType::kViewed, Now());

  const int ads_per_day = features::GetMaximumSearchResultAdsPerDay();

  FireAdEvents(ad_event, ads_per_day - 1);

  AdvanceClock(base::Hours(1));

  // Act
  FireEvent(ad_mojom, mojom::SearchResultAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, ads_per_day);
  ExpectDepositExistsForCreativeInstanceId(kCreativeInstanceId);
  ExpectConversionCountEquals(0);
}

TEST_F(BatAdsSearchResultAdTest, DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRules();

  const mojom::SearchResultAdPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  const SearchResultAdInfo& ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo& ad_event = BuildAdEvent(ad, AdType::kSearchResultAd,
                                             ConfirmationType::kViewed, Now());

  const int ads_per_day = features::GetMaximumSearchResultAdsPerDay();

  FireAdEvents(ad_event, ads_per_day);

  AdvanceClock(base::Hours(1));

  // Act
  FireEvent(ad_mojom, mojom::SearchResultAdEventType::kViewed);

  // Assert
  ExpectAdEventCountEquals(ConfirmationType::kViewed, ads_per_day);
}

}  // namespace ads
