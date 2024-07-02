/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/search_result_ads/search_result_ad_event_handler_util.h"

#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/creative_search_result_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSearchResultAdEventHandlerUtilTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    scoped_feature_list_.InitAndEnableFeature(
        kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  database::table::CreativeSetConversions
      creative_set_conversions_database_table_;
};

TEST_F(BraveAdsSearchResultAdEventHandlerUtilTest,
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

TEST_F(BraveAdsSearchResultAdEventHandlerUtilTest,
       DoNotSaveCreativeSetConversionForNonViewedEvent) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act
  for (auto i = 0;
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

TEST_F(BraveAdsSearchResultAdEventHandlerUtilTest,
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

TEST_F(BraveAdsSearchResultAdEventHandlerUtilTest,
       SaveCreativeSetConversionForClickedEventAndNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act
  MaybeBuildAndSaveCreativeSetConversion(
      mojom_creative_ad, mojom::SearchResultAdEventType::kClicked);

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

TEST_F(BraveAdsSearchResultAdEventHandlerUtilTest,
       DoNotSaveCreativeSetConversionForNonClickedEventAndNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act
  for (auto i = 0;
       i < static_cast<int>(mojom::SearchResultAdEventType::kMaxValue); ++i) {
    const auto event_type = static_cast<mojom::SearchResultAdEventType>(i);
    if (event_type != mojom::SearchResultAdEventType::kClicked) {
      MaybeBuildAndSaveCreativeSetConversion(mojom_creative_ad, event_type);
    }
  }

  // Assert
  base::MockCallback<database::table::GetCreativeSetConversionsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true, ::testing::IsEmpty()));
  creative_set_conversions_database_table_.GetUnexpired(callback.Get());
}

TEST_F(
    BraveAdsSearchResultAdEventHandlerUtilTest,
    DoNotSaveCreativeSetConversionForClickedEventAndNonRewardsUserWithoutConversion) {
  // Arrange
  test::DisableBraveRewards();

  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAd(/*should_generate_random_uuids=*/true);

  // Act
  MaybeBuildAndSaveCreativeSetConversion(
      mojom_creative_ad, mojom::SearchResultAdEventType::kClicked);

  // Assert
  base::MockCallback<database::table::GetCreativeSetConversionsCallback>
      callback;
  EXPECT_CALL(callback, Run(/*success=*/true, ::testing::IsEmpty()));
  creative_set_conversions_database_table_.GetUnexpired(callback.Get());
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilTest,
       CannotFireViewedEventForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(CanFireEvent(mojom_creative_ad,
                            mojom::SearchResultAdEventType::kViewedImpression));
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilTest,
       CannotFireViewedEventWithoutConversionForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAd(/*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(CanFireEvent(mojom_creative_ad,
                            mojom::SearchResultAdEventType::kViewedImpression));
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilTest,
       CanFireClickedEventForRewardsUser) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_TRUE(CanFireEvent(mojom_creative_ad,
                           mojom::SearchResultAdEventType::kClicked));
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilTest,
       CannotFireNonClickedEventForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act & Assert
  for (auto i = 0;
       i < static_cast<int>(mojom::SearchResultAdEventType::kMaxValue); ++i) {
    const auto event_type = static_cast<mojom::SearchResultAdEventType>(i);
    if (event_type != mojom::SearchResultAdEventType::kClicked) {
      EXPECT_FALSE(CanFireEvent(mojom_creative_ad, event_type));
    }
  }
}

TEST_F(
    BraveAdsSearchResultAdEventHandlerUtilTest,
    CannotFireClickedEventWhenShouldNotAlwaysTriggerSearchResultAdEventsForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  scoped_feature_list_.Reset();

  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(CanFireEvent(mojom_creative_ad,
                            mojom::SearchResultAdEventType::kClicked));
}

TEST_F(BraveAdsSearchResultAdEventHandlerUtilTest,
       CannotFireClickedEventWithoutConversionForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAd(/*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(CanFireEvent(mojom_creative_ad,
                            mojom::SearchResultAdEventType::kClicked));
}

}  // namespace brave_ads
