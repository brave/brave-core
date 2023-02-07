/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/search_result_ads/search_result_ad_event_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "absl/types/optional.h"
#include "base/functional/bind.h"
#include "bat/ads/internal/account/deposits/deposit_info.h"
#include "bat/ads/internal/account/deposits/deposits_database_table.h"
#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/conversions/conversion_info.h"
#include "bat/ads/internal/conversions/conversions_database_table.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_builder.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::search_result_ads {

namespace {

constexpr char kPlacementId[] = "d2ef9bb0-a0dc-472c-bc49-62105bb6da68";
constexpr char kInvalidPlacementId[] = "";

constexpr char kCreativeInstanceId[] = "1547f94f-9086-4db9-a441-efb2f0365269";
constexpr char kInvalidCreativeInstanceId[] = "";

mojom::SearchResultAdInfoPtr BuildAd(const std::string& placement_id,
                                     const std::string& creative_instance_id) {
  mojom::SearchResultAdInfoPtr ad_mojom = mojom::SearchResultAdInfo::New();

  ad_mojom->creative_instance_id = creative_instance_id;
  ad_mojom->placement_id = placement_id;
  ad_mojom->creative_set_id = "7a41297b-ff7f-4ca8-9787-b4c9c1105f01";
  ad_mojom->campaign_id = "be5d25ca-93e4-4a16-8f8b-4714abca31ed";
  ad_mojom->advertiser_id = "f82389c6-c6ca-4db5-99f9-724f038efddf";
  ad_mojom->target_url = GURL("https://brave.com");
  ad_mojom->headline_text = "headline";
  ad_mojom->description = "description";
  ad_mojom->value = 1.0;
  ad_mojom->conversion = mojom::ConversionInfo::New();

  return ad_mojom;
}

mojom::SearchResultAdInfoPtr BuildAdWithConversion(
    const std::string& placement_id,
    const std::string& creative_instance_id) {
  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAd(placement_id, creative_instance_id);

  ad_mojom->conversion->type = "postview";
  ad_mojom->conversion->url_pattern = "https://brave.com/*";
  ad_mojom->conversion->advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  ad_mojom->conversion->observation_window = 3;
  ad_mojom->conversion->expire_at = DistantFuture();

  return ad_mojom;
}

void ExpectDepositExistsForCreativeInstanceId(
    const std::string& creative_instance_id) {
  const database::table::Deposits database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      base::BindOnce(
          [](const bool success, const absl::optional<DepositInfo>& deposit) {
            ASSERT_TRUE(success);

            EXPECT_TRUE(deposit);
          }));
}

void ExpectConversionCountEquals(const size_t expected_count) {
  const database::table::Conversions database_table;
  database_table.GetAll(base::BindOnce(
      [](const size_t expected_count, const bool success,
         const ConversionList& conversions) {
        ASSERT_TRUE(success);

        EXPECT_EQ(expected_count, conversions.size());
      },
      expected_count));
}

}  // namespace

class BatAdsSearchResultAdEventHandlerTest : public EventHandlerObserver,
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

  void FireEvent(mojom::SearchResultAdInfoPtr ad_mojom,
                 const mojom::SearchResultAdEventType event_type) {
    event_handler_->FireEvent(
        std::move(ad_mojom), event_type,
        base::BindOnce([](const bool success, const std::string& placement_id,
                          const mojom::SearchResultAdEventType event_type) {}));
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
      const SearchResultAdInfo& /*ad*/,
      const mojom::SearchResultAdEventType /*event_type*/) override {
    did_fail_to_fire_event_ = true;
  }

  std::unique_ptr<EventHandler> event_handler_;

  SearchResultAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BatAdsSearchResultAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  ForcePermissionRulesForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed);
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const SearchResultAdInfo expected_ad = BuildSearchResultAd(ad_mojom);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  ExpectDepositExistsForCreativeInstanceId(kCreativeInstanceId);
  ExpectConversionCountEquals(0);
}

TEST_F(BatAdsSearchResultAdEventHandlerTest, FireViewedEventWithConversion) {
  // Arrange
  ForcePermissionRulesForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAdWithConversion(kPlacementId, kCreativeInstanceId);

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const SearchResultAdInfo expected_ad = BuildSearchResultAd(ad_mojom);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  ExpectDepositExistsForCreativeInstanceId(kCreativeInstanceId);
  ExpectConversionCountEquals(1);
}

TEST_F(BatAdsSearchResultAdEventHandlerTest,
       DoNotFireViewedEventIfAlreadyFired) {
  // Arrange
  ForcePermissionRulesForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed);
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  ExpectDepositExistsForCreativeInstanceId(kCreativeInstanceId);
  ExpectConversionCountEquals(0);
}

TEST_F(BatAdsSearchResultAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  ForcePermissionRulesForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed);
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kClicked);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  const SearchResultAdInfo expected_ad = BuildSearchResultAd(ad_mojom);
  EXPECT_EQ(expected_ad, ad_);
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kClicked));
}

TEST_F(BatAdsSearchResultAdEventHandlerTest,
       DoNotFireClickedEventIfAlreadyFired) {
  // Arrange
  ForcePermissionRulesForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed);
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kClicked);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kClicked);

  // Assert
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kClicked));
}

TEST_F(BatAdsSearchResultAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAd(kInvalidPlacementId, kCreativeInstanceId);

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
}

TEST_F(BatAdsSearchResultAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAd(kPlacementId, kInvalidCreativeInstanceId);

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsSearchResultAdEventHandlerTest, DoNotFireEventWhenNotPermitted) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
}

TEST_F(BatAdsSearchResultAdEventHandlerTest,
       FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  const int ads_per_hour = features::GetMaximumSearchResultAdsPerHour();

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo served_ad_event = BuildAdEvent(
      ad, AdType::kSearchResultAd, ConfirmationType::kServed, Now());
  FireAdEvents(served_ad_event, ads_per_hour - 1);
  const AdEventInfo viewed_ad_event = BuildAdEvent(
      ad, AdType::kSearchResultAd, ConfirmationType::kViewed, Now());
  FireAdEvents(viewed_ad_event, ads_per_hour - 1);

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_EQ(ads_per_hour, GetAdEventCount(AdType::kSearchResultAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(ads_per_hour, GetAdEventCount(AdType::kSearchResultAd,
                                          ConfirmationType::kViewed));
  ExpectDepositExistsForCreativeInstanceId(kCreativeInstanceId);
  ExpectConversionCountEquals(0);
}

TEST_F(BatAdsSearchResultAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo ad_event = BuildAdEvent(ad, AdType::kSearchResultAd,
                                            ConfirmationType::kServed, Now());

  const int ads_per_hour = features::GetMaximumSearchResultAdsPerHour();
  FireAdEvents(ad_event, ads_per_hour);

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed);

  // Assert
  EXPECT_EQ(ads_per_hour, GetAdEventCount(AdType::kSearchResultAd,
                                          ConfirmationType::kServed));
}

TEST_F(BatAdsSearchResultAdEventHandlerTest,
       FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  const int ads_per_day = features::GetMaximumSearchResultAdsPerDay();

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo served_ad_event = BuildAdEvent(
      ad, AdType::kSearchResultAd, ConfirmationType::kServed, Now());
  FireAdEvents(served_ad_event, ads_per_day - 1);
  const AdEventInfo viewed_ad_event = BuildAdEvent(
      ad, AdType::kSearchResultAd, ConfirmationType::kViewed, Now());
  FireAdEvents(viewed_ad_event, ads_per_day - 1);

  AdvanceClockBy(base::Hours(1));

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_EQ(ads_per_day, GetAdEventCount(AdType::kSearchResultAd,
                                         ConfirmationType::kServed));
  EXPECT_EQ(ads_per_day, GetAdEventCount(AdType::kSearchResultAd,
                                         ConfirmationType::kViewed));
  ExpectDepositExistsForCreativeInstanceId(kCreativeInstanceId);
  ExpectConversionCountEquals(0);
}

TEST_F(BatAdsSearchResultAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildAd(kPlacementId, kCreativeInstanceId);

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo ad_event = BuildAdEvent(ad, AdType::kSearchResultAd,
                                            ConfirmationType::kServed, Now());

  const int ads_per_day = features::GetMaximumSearchResultAdsPerDay();

  FireAdEvents(ad_event, ads_per_day);

  AdvanceClockBy(base::Hours(1));

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed);

  // Assert
  EXPECT_EQ(ads_per_day, GetAdEventCount(AdType::kSearchResultAd,
                                         ConfirmationType::kServed));
}

}  // namespace ads::search_result_ads
