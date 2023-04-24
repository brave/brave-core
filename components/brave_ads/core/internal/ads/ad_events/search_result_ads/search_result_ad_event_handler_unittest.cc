/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/search_result_ads/search_result_ad_event_handler.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposit_info.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/search_result_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_info.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_unittest_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

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

class BraveAdsSearchResultAdEventHandlerTest
    : public SearchResultAdEventHandlerDelegate,
      public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    event_handler_.SetDelegate(this);
  }

  void FireEvent(mojom::SearchResultAdInfoPtr ad_mojom,
                 const mojom::SearchResultAdEventType event_type) {
    event_handler_.FireEvent(
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

  SearchResultAdEventHandler event_handler_;

  SearchResultAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BraveAdsSearchResultAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  ForcePermissionRulesForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_guids*/ false);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed);
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(BuildSearchResultAd(ad_mojom), ad_);
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  ExpectDepositExistsForCreativeInstanceId(ad_mojom->creative_instance_id);
  ExpectConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest, FireViewedEventWithConversion) {
  // Arrange
  ForcePermissionRulesForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdWithConversion(/*should_use_random_guids*/ false);

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(BuildSearchResultAd(ad_mojom), ad_);
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  ExpectDepositExistsForCreativeInstanceId(ad_mojom->creative_instance_id);
  ExpectConversionCountEquals(1);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireViewedEventIfAlreadyFired) {
  // Arrange
  ForcePermissionRulesForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_guids*/ false);

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed);
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  ExpectDepositExistsForCreativeInstanceId(ad_mojom->creative_instance_id);
  ExpectConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  ForcePermissionRulesForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_guids*/ false);

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed);
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kClicked);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(BuildSearchResultAd(ad_mojom), ad_);
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  EXPECT_EQ(
      1, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kClicked));
  ExpectConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireClickedEventIfAlreadyFired) {
  // Arrange
  ForcePermissionRulesForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_guids*/ false);

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
  ExpectConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_guids*/ false);
  ad_mojom->placement_id = kInvalidPlacementId;

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kServed));
  ExpectConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_guids*/ false);
  ad_mojom->creative_instance_id = kInvalidCreativeInstanceId;

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  ExpectConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest, DoNotFireEventWhenNotPermitted) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_guids*/ false);

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kViewed);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(
      0, GetAdEventCount(AdType::kSearchResultAd, ConfirmationType::kViewed));
  ExpectConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_guids*/ false);

  const int ads_per_hour = kMaximumSearchResultAdsPerHour.Get();

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
  ExpectDepositExistsForCreativeInstanceId(ad.creative_instance_id);
  ExpectConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_guids*/ false);

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo ad_event = BuildAdEvent(ad, AdType::kSearchResultAd,
                                            ConfirmationType::kServed, Now());

  const int ads_per_hour = kMaximumSearchResultAdsPerHour.Get();

  FireAdEvents(ad_event, ads_per_hour);

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed);

  // Assert
  EXPECT_EQ(ads_per_hour, GetAdEventCount(AdType::kSearchResultAd,
                                          ConfirmationType::kServed));
  ExpectConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_guids*/ false);

  const int ads_per_day = kMaximumSearchResultAdsPerDay.Get();

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
  ExpectDepositExistsForCreativeInstanceId(ad.creative_instance_id);
  ExpectConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  ForcePermissionRulesForTesting();

  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_guids*/ false);

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo ad_event = BuildAdEvent(ad, AdType::kSearchResultAd,
                                            ConfirmationType::kServed, Now());

  const int ads_per_day = kMaximumSearchResultAdsPerDay.Get();

  FireAdEvents(ad_event, ads_per_day);

  AdvanceClockBy(base::Hours(1));

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed);

  // Assert
  EXPECT_EQ(ads_per_day, GetAdEventCount(AdType::kSearchResultAd,
                                         ConfirmationType::kServed));
  ExpectConversionCountEquals(0);
}

}  // namespace brave_ads
