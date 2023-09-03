/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/search_result_ads/search_result_ad_event_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposit_info.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_unittest_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/units/search_result_ad/search_result_ad_feature.h"
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

void ExpectCreativeSetConversionCountEquals(const size_t expected_count) {
  const database::table::CreativeSetConversions database_table;
  database_table.GetAll(base::BindOnce(
      [](const size_t expected_count, const bool success,
         const CreativeSetConversionList& creative_set_conversions) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_count, creative_set_conversions.size());
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

    ForcePermissionRulesForTesting();
  }

  void OnDidFireSearchResultAdServedEvent(
      const SearchResultAdInfo& ad) override {
    ad_ = ad;
    did_serve_ad_ = true;
  }

  void OnDidFireSearchResultAdViewedEvent(
      const SearchResultAdInfo& ad) override {
    ad_ = ad;
    did_view_ad_ = true;
  }

  void OnDidFireSearchResultAdClickedEvent(
      const SearchResultAdInfo& ad) override {
    ad_ = ad;
    did_click_ad_ = true;
  }

  void OnFailedToFireSearchResultAdEvent(
      const SearchResultAdInfo& /*ad*/,
      const mojom::SearchResultAdEventType /*event_type*/) override {
    did_fail_to_fire_event_ = true;
  }

  void FireEvent(mojom::SearchResultAdInfoPtr ad_mojom,
                 const mojom::SearchResultAdEventType& event_type,
                 const bool should_fire_event) {
    base::MockCallback<FireSearchResultAdEventHandlerCallback> callback;
    EXPECT_CALL(callback, Run(/*success*/ should_fire_event,
                              ad_mojom->placement_id, event_type));

    event_handler_.FireEvent(std::move(ad_mojom), event_type, callback.Get());
  }

  void FireEvents(
      mojom::SearchResultAdInfoPtr ad_mojom,
      const std::vector<mojom::SearchResultAdEventType>& event_types,
      const bool should_fire_event) {
    for (const auto& event_type : event_types) {
      FireEvent(ad_mojom->Clone(), event_type, should_fire_event);
    }
  }

  SearchResultAdEventHandler event_handler_;

  SearchResultAdInfo ad_;
  bool did_serve_ad_ = false;
  bool did_view_ad_ = false;
  bool did_click_ad_ = false;
  bool did_fail_to_fire_event_ = false;
};

TEST_F(BraveAdsSearchResultAdEventHandlerTest, FireServedEvent) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(BuildSearchResultAd(ad_mojom), ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kServed));
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event*/ true);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(BuildSearchResultAd(ad_mojom), ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kViewed));
  ExpectDepositExistsForCreativeInstanceId(ad_mojom->creative_instance_id);
  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest, FireViewedEventWithConversion) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdWithConversionForTesting(
          /*should_use_random_uuids*/ true);

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event*/ true);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(BuildSearchResultAd(ad_mojom), ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kViewed));
  ExpectDepositExistsForCreativeInstanceId(ad_mojom->creative_instance_id);
  ExpectCreativeSetConversionCountEquals(1);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasAlreadyViewed) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);

  FireEvents(ad_mojom->Clone(),
             {mojom::SearchResultAdEventType::kServed,
              mojom::SearchResultAdEventType::kViewed},
             /*should_fire_event*/ true);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kViewed));
  ExpectDepositExistsForCreativeInstanceId(ad_mojom->creative_instance_id);
  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kInlineContentAd,
                                          ConfirmationType::kViewed));
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);

  FireEvents(ad_mojom->Clone(),
             {mojom::SearchResultAdEventType::kServed,
              mojom::SearchResultAdEventType::kViewed},
             /*should_fire_event*/ true);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kClicked,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_TRUE(did_serve_ad_);
  EXPECT_TRUE(did_view_ad_);
  EXPECT_TRUE(did_click_ad_);
  EXPECT_FALSE(did_fail_to_fire_event_);
  EXPECT_EQ(BuildSearchResultAd(ad_mojom), ad_);
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kClicked));
  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);

  FireEvents(ad_mojom->Clone(),
             {mojom::SearchResultAdEventType::kServed,
              mojom::SearchResultAdEventType::kViewed,
              mojom::SearchResultAdEventType::kClicked},
             /*should_fire_event*/ true);

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kClicked,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kServed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kViewed));
  EXPECT_EQ(1U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kClicked));
  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventIfMissingAdPlacement) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);
  ad_mojom->placement_id = kMissingPlacementId;

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kViewed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kViewed));
  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);
  ad_mojom->placement_id = kInvalidPlacementId;

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kServed));
  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);
  ad_mojom->creative_instance_id = kInvalidCreativeInstanceId;

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_FALSE(did_serve_ad_);
  EXPECT_FALSE(did_view_ad_);
  EXPECT_FALSE(did_click_ad_);
  EXPECT_TRUE(did_fail_to_fire_event_);
  EXPECT_EQ(0U, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                          ConfirmationType::kServed));
  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  const size_t ads_per_hour = kMaximumSearchResultAdsPerHour.Get();

  FireAdEventsForTesting(ad_event, ads_per_hour - 1);

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_EQ(ads_per_hour, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                                    ConfirmationType::kServed));
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  const size_t ads_per_hour = kMaximumSearchResultAdsPerHour.Get();

  FireAdEventsForTesting(ad_event, ads_per_hour);

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(ads_per_hour, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                                    ConfirmationType::kServed));
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  const size_t ads_per_day = kMaximumSearchResultAdsPerDay.Get();

  FireAdEventsForTesting(ad_event, ads_per_day - 1);

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act
  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event*/ true);

  // Assert
  EXPECT_EQ(ads_per_day, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                                   ConfirmationType::kServed));
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);

  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at*/ Now());

  const size_t ads_per_day = kMaximumSearchResultAdsPerDay.Get();

  FireAdEventsForTesting(ad_event, ads_per_day);

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act
  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event*/ false);

  // Assert
  EXPECT_EQ(ads_per_day, GetAdEventCountForTesting(AdType::kSearchResultAd,
                                                   ConfirmationType::kServed));
}

}  // namespace brave_ads
