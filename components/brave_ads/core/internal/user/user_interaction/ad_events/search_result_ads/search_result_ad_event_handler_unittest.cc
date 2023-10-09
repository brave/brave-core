/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/search_result_ads/search_result_ad_event_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
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
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/search_result_ads/search_result_ad_event_handler_delegate_mock.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/units/search_result_ad/search_result_ad_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

void ExpectDepositExistsForCreativeInstanceId(
    const std::string& creative_instance_id) {
  base::MockCallback<database::table::GetDepositsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, /*deposit=*/::testing::Ne(absl::nullopt)));
  const database::table::Deposits database_table;
  database_table.GetForCreativeInstanceId(creative_instance_id, callback.Get());
}

void ExpectCreativeSetConversionCountEquals(const size_t expected_count) {
  base::MockCallback<database::table::GetConversionsCallback> callback;
  EXPECT_CALL(
      callback,
      Run(/*success=*/true,
          /*creative_set_conversions=*/::testing::SizeIs(expected_count)));
  const database::table::CreativeSetConversions database_table;
  database_table.GetAll(callback.Get());
}

}  // namespace

class BraveAdsSearchResultAdEventHandlerTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    event_handler_.SetDelegate(&delegate_mock_);

    test::ForcePermissionRules();
  }

  void FireEvent(mojom::SearchResultAdInfoPtr ad_mojom,
                 const mojom::SearchResultAdEventType& event_type,
                 const bool should_fire_event) {
    base::MockCallback<FireSearchResultAdEventHandlerCallback> callback;
    EXPECT_CALL(callback, Run(/*success=*/should_fire_event,
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
  ::testing::StrictMock<SearchResultAdEventHandlerDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsSearchResultAdEventHandlerTest, FireServedEvent) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdServedEvent(ad));

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest, FireViewedEvent) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdServedEvent(ad));

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdViewedEvent(ad));

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed,
            /*should_fire_event=*/true);

  ExpectDepositExistsForCreativeInstanceId(ad_mojom->creative_instance_id);
  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest, FireViewedEventWithConversion) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAdWithConversion(
          /*should_use_random_uuids=*/true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdServedEvent(ad));

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdViewedEvent(ad));

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed,
            /*should_fire_event=*/true);

  ExpectDepositExistsForCreativeInstanceId(ad_mojom->creative_instance_id);
  ExpectCreativeSetConversionCountEquals(1);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasAlreadyViewed) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdServedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdViewedEvent(ad));

  FireEvents(ad_mojom->Clone(),
             {mojom::SearchResultAdEventType::kServed,
              mojom::SearchResultAdEventType::kViewed},
             /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireSearchResultAdEvent(
                                  ad, mojom::SearchResultAdEventType::kViewed));

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed,
            /*should_fire_event=*/false);

  ExpectDepositExistsForCreativeInstanceId(ad_mojom->creative_instance_id);
  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireViewedEventIfAdPlacementWasNotServed) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireSearchResultAdEvent(
                                  ad, mojom::SearchResultAdEventType::kViewed));

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kViewed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest, FireClickedEvent) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdServedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdViewedEvent(ad));

  FireEvents(ad_mojom->Clone(),
             {mojom::SearchResultAdEventType::kServed,
              mojom::SearchResultAdEventType::kViewed},
             /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdClickedEvent(ad));

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kClicked,
            /*should_fire_event=*/true);

  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireClickedEventIfAdPlacementWasAlreadyClicked) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdServedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdViewedEvent(ad));
  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdClickedEvent(ad));

  FireEvents(ad_mojom->Clone(),
             {mojom::SearchResultAdEventType::kServed,
              mojom::SearchResultAdEventType::kViewed,
              mojom::SearchResultAdEventType::kClicked},
             /*should_fire_event=*/true);

  // Act & Assert
  EXPECT_CALL(delegate_mock_,
              OnFailedToFireSearchResultAdEvent(
                  ad, mojom::SearchResultAdEventType::kClicked));

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kClicked,
            /*should_fire_event=*/false);

  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventIfMissingAdPlacement) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  ad_mojom->placement_id = kMissingPlacementId;
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireSearchResultAdEvent(
                                  ad, mojom::SearchResultAdEventType::kViewed));

  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kViewed,
            /*should_fire_event=*/false);

  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventWithInvalidPlacementId) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  ad_mojom->placement_id = kInvalidPlacementId;
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireSearchResultAdEvent(
                                  ad, mojom::SearchResultAdEventType::kServed));

  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event=*/false);

  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventWithInvalidCreativeInstanceId) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  ad_mojom->creative_instance_id = kInvalidCreativeInstanceId;
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireSearchResultAdEvent(
                                  ad, mojom::SearchResultAdEventType::kServed));

  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event=*/false);

  ExpectCreativeSetConversionCountEquals(0);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       FireEventIfNotExceededAdsPerHourCap) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at=*/Now());
  test::RecordAdEvents(ad_event, kMaximumSearchResultAdsPerHour.Get() - 1);

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdServedEvent(ad));

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerHourCap) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at=*/Now());
  test::RecordAdEvents(ad_event, kMaximumSearchResultAdsPerHour.Get());

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireSearchResultAdEvent(
                                  ad, mojom::SearchResultAdEventType::kServed));

  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event=*/false);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       FireEventIfNotExceededAdsPerDayCap) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at=*/Now());
  test::RecordAdEvents(ad_event, kMaximumSearchResultAdsPerDay.Get() - 1);

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnDidFireSearchResultAdServedEvent(ad));

  FireEvent(ad_mojom->Clone(), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event=*/true);
}

TEST_F(BraveAdsSearchResultAdEventHandlerTest,
       DoNotFireEventIfExceededAdsPerDayCap) {
  // Arrange
  mojom::SearchResultAdInfoPtr ad_mojom =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kServed, /*created_at=*/Now());
  test::RecordAdEvents(ad_event, kMaximumSearchResultAdsPerDay.Get());

  AdvanceClockBy(base::Days(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFireSearchResultAdEvent(
                                  ad, mojom::SearchResultAdEventType::kServed));

  FireEvent(std::move(ad_mojom), mojom::SearchResultAdEventType::kServed,
            /*should_fire_event=*/false);
}

}  // namespace brave_ads
