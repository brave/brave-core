/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_builder.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/creative_search_result_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_handler_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_test_util.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSearchResultAdEventHandlerUtilForRewardsTest
    : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    scoped_feature_list_.InitAndEnableFeature(
        kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  database::table::CreativeSetConversions
      creative_set_conversions_database_table_;
};

TEST_F(BraveAdsSearchResultAdEventHandlerUtilForRewardsTest,
       SaveCreativeSetConversionForViewedEvent) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act
  MaybeBuildAndSaveCreativeSetConversion(
      mojom_creative_ad, mojom::SearchResultAdEventType::kViewedImpression);

  // Assert
  const std::optional<CreativeSetConversionInfo> creative_set_conversion =
      FromMojomMaybeBuildCreativeSetConversion(mojom_creative_ad);
  ASSERT_TRUE(creative_set_conversion);

  base::MockCallback<database::table::GetCreativeSetConversionsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true, CreativeSetConversionList{
                                                  *creative_set_conversion}));
  creative_set_conversions_database_table_.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilForRewardsTest,
       DoNotSaveCreativeSetConversionForNonViewedEvent) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act
  for (int i = 0;
       i < static_cast<int>(mojom::SearchResultAdEventType::kMaxValue); ++i) {
    const auto event_type = static_cast<mojom::SearchResultAdEventType>(i);
    if (event_type != mojom::SearchResultAdEventType::kViewedImpression) {
      MaybeBuildAndSaveCreativeSetConversion(mojom_creative_ad, event_type);
    }
  }

  // Assert
  base::MockCallback<database::table::GetCreativeSetConversionsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true, ::testing::IsEmpty()));
  creative_set_conversions_database_table_.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilForRewardsTest,
       DoNotSaveCreativeSetConversionForViewedEventWithoutConversion) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAd(/*should_generate_random_uuids=*/true);

  // Act
  MaybeBuildAndSaveCreativeSetConversion(
      mojom_creative_ad, mojom::SearchResultAdEventType::kViewedImpression);

  // Assert
  base::MockCallback<database::table::GetCreativeSetConversionsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true, ::testing::IsEmpty()));
  creative_set_conversions_database_table_.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilForRewardsTest,
       AllowedToFireEventWithConversion) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act & Assert
  for (int i = 0;
       i < static_cast<int>(mojom::SearchResultAdEventType::kMaxValue); ++i) {
    const auto event_type = static_cast<mojom::SearchResultAdEventType>(i);
    EXPECT_TRUE(IsAllowedToFireAdEvent(mojom_creative_ad, event_type));
  }
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilForRewardsTest,
       AllowedtoFireEventWithoutConversion) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAd(
          /*should_generate_random_uuids=*/true);

  // Act & Assert
  for (int i = 0;
       i < static_cast<int>(mojom::SearchResultAdEventType::kMaxValue); ++i) {
    const auto event_type = static_cast<mojom::SearchResultAdEventType>(i);
    EXPECT_TRUE(IsAllowedToFireAdEvent(mojom_creative_ad, event_type));
  }
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilForRewardsTest,
       ShouldFireEventIfAdPlacementWasServed) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  AdEventList ad_events;
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_TRUE(ShouldFireAdEvent(
      ad, ad_events, mojom::SearchResultAdEventType::kViewedImpression));
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilForRewardsTest,
       ShouldNotFireEventIfAdPlacementWasNeverServed) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  // Act & Assert
  EXPECT_FALSE(ShouldFireAdEvent(
      ad, /*ad_events=*/{}, mojom::SearchResultAdEventType::kViewedImpression));
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilForRewardsTest,
       ShouldFireNonDuplicateViewedEvent) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  AdEventList ad_events;
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_TRUE(ShouldFireAdEvent(
      ad, ad_events, mojom::SearchResultAdEventType::kViewedImpression));
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilForRewardsTest,
       ShouldNotFireDuplicateViewedEvent) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  AdEventList ad_events;
  const AdEventInfo ad_event_1 = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);
  const AdEventInfo ad_event_2 = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_2);

  // Act & Assert
  EXPECT_FALSE(ShouldFireAdEvent(
      ad, ad_events, mojom::SearchResultAdEventType::kViewedImpression));
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilForRewardsTest,
       ShouldFireNonDuplicateClickedEvent) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  AdEventList ad_events;
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event);

  // Act & Assert
  EXPECT_TRUE(ShouldFireAdEvent(ad, ad_events,
                                mojom::SearchResultAdEventType::kClicked));
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilForRewardsTest,
       ShouldNotFireDuplicateClickedEvent) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);
  const SearchResultAdInfo ad = FromMojomBuildSearchResultAd(mojom_creative_ad);

  AdEventList ad_events;
  const AdEventInfo ad_event_1 = BuildAdEvent(
      ad, ConfirmationType::kServedImpression, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);
  const AdEventInfo ad_event_2 =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/test::Now());
  ad_events.push_back(ad_event_2);

  // Act & Assert
  EXPECT_FALSE(ShouldFireAdEvent(ad, ad_events,
                                 mojom::SearchResultAdEventType::kClicked));
}

}  // namespace brave_ads
