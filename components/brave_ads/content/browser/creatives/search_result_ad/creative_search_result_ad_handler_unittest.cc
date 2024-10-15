/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_handler.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/strings/strcat.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/browser/ads_service_mock.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_constants.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_web_page_entities_extractor.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_web_page_entities_test_util.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_test_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/document_metadata/document_metadata.mojom.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kAllowedDomain[] = "https://search.brave.com";
constexpr char kDisallowedDomain[] = "https://brave.com";

}  // namespace

class BraveAdsCreativeSearchResultAdHandlerTest : public ::testing::Test {
 public:
  BraveAdsCreativeSearchResultAdHandlerTest() {
    scoped_feature_list_.InitAndEnableFeature(
        kShouldSupportSearchResultAdsFeature);
  }

  static void SimulateMaybeExtractCreativeAdPlacementIdsFromWebPageCallback(
      CreativeSearchResultAdHandler* creative_search_result_ad_handler,
      blink::mojom::WebPagePtr mojom_web_page) {
    CHECK(creative_search_result_ad_handler);

    base::MockCallback<ExtractCreativeAdPlacementIdsFromWebPageCallback>
        callback;
    EXPECT_CALL(callback, Run)
        .WillOnce([creative_search_result_ad_handler](
                      std::vector<mojom::CreativeSearchResultAdInfoPtr>
                          creative_search_result_ads) {
          for (auto& creative_search_result_ad : creative_search_result_ads) {
            creative_search_result_ad_handler
                ->MaybeTriggerCreativeAdViewedEvent(
                    std::move(creative_search_result_ad));
          }
        });

    creative_search_result_ad_handler
        ->MaybeExtractCreativeAdPlacementIdsFromWebPageCallback(
            mojo::Remote<blink::mojom::DocumentMetadata>(), callback.Get(),
            std::move(mojom_web_page));
  }

 protected:
  AdsServiceMock ads_service_mock_{nullptr};

  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(BraveAdsCreativeSearchResultAdHandlerTest, Create) {
  // Act
  const auto creative_search_result_ad_handler =
      CreativeSearchResultAdHandler::MaybeCreate(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_creative_ad_viewed_events=*/true);

  // Assert
  EXPECT_TRUE(creative_search_result_ad_handler);
}

TEST_F(BraveAdsCreativeSearchResultAdHandlerTest,
       DoNotCreateWhenAdsServiceIsNull) {
  // Act
  const auto creative_search_result_ad_handler =
      CreativeSearchResultAdHandler::MaybeCreate(
          /*ads_service=*/nullptr, GURL(kAllowedDomain),
          /*should_trigger_creative_ad_viewed_events=*/true);

  // Assert
  EXPECT_FALSE(creative_search_result_ad_handler);
}

TEST_F(BraveAdsCreativeSearchResultAdHandlerTest,
       DoNotCreateForDisallowedDomain) {
  // Act
  const auto creative_search_result_ad_handler =
      CreativeSearchResultAdHandler::MaybeCreate(
          &ads_service_mock_, GURL(kDisallowedDomain),
          /*should_trigger_creative_ad_viewed_events=*/true);

  // Assert
  EXPECT_FALSE(creative_search_result_ad_handler);
}

TEST_F(BraveAdsCreativeSearchResultAdHandlerTest,
       ShouldNotTriggerAdViewedEvents) {
  // Arrange
  const auto creative_search_result_ad_handler =
      CreativeSearchResultAdHandler::MaybeCreate(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_creative_ad_viewed_events=*/false);
  ASSERT_TRUE(creative_search_result_ad_handler);

  blink::mojom::WebPagePtr mojom_web_page =
      test::CreativeSearchResultAdMojomWebPage(/*excluded_property_names=*/{});
  ASSERT_TRUE(mojom_web_page);

  // Act & Assert
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          ::testing::_, mojom::SearchResultAdEventType::kViewedImpression,
          ::testing::_))
      .Times(0);

  SimulateMaybeExtractCreativeAdPlacementIdsFromWebPageCallback(
      creative_search_result_ad_handler.get(), std::move(mojom_web_page));
}

TEST_F(BraveAdsCreativeSearchResultAdHandlerTest,
       DoNotTriggerViewedOrClickedAdEventsForInvalidWebPage) {
  // Arrange
  const auto creative_search_result_ad_handler =
      CreativeSearchResultAdHandler::MaybeCreate(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_creative_ad_viewed_events=*/true);
  ASSERT_TRUE(creative_search_result_ad_handler);

  // Act & Assert
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          ::testing::_, mojom::SearchResultAdEventType::kViewedImpression,
          ::testing::_))
      .Times(0);

  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          ::testing::_, mojom::SearchResultAdEventType::kClicked, ::testing::_))
      .Times(0);

  SimulateMaybeExtractCreativeAdPlacementIdsFromWebPageCallback(
      creative_search_result_ad_handler.get(), blink::mojom::WebPagePtr());
}

TEST_F(BraveAdsCreativeSearchResultAdHandlerTest,
       DoNotTriggerViewedOrClickedAdEventsForEmptyWebPage) {
  // Arrange
  const auto creative_search_result_ad_handler =
      CreativeSearchResultAdHandler::MaybeCreate(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_creative_ad_viewed_events=*/true);
  ASSERT_TRUE(creative_search_result_ad_handler);

  // Act & Assert
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          ::testing::_, mojom::SearchResultAdEventType::kViewedImpression,
          ::testing::_))
      .Times(0);

  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          ::testing::_, mojom::SearchResultAdEventType::kClicked, ::testing::_))
      .Times(0);

  SimulateMaybeExtractCreativeAdPlacementIdsFromWebPageCallback(
      creative_search_result_ad_handler.get(), blink::mojom::WebPage::New());
}

TEST_F(BraveAdsCreativeSearchResultAdHandlerTest,
       DoNotTriggerViewedOrClickedAdEventsForInvalidCreativeAd) {
  // Arrange
  const auto creative_search_result_ad_handler =
      CreativeSearchResultAdHandler::MaybeCreate(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_creative_ad_viewed_events=*/true);
  ASSERT_TRUE(creative_search_result_ad_handler);

  // Act & Assert
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          ::testing::_, mojom::SearchResultAdEventType::kViewedImpression,
          ::testing::_))
      .Times(0);

  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          ::testing::_, mojom::SearchResultAdEventType::kClicked, ::testing::_))
      .Times(0);

  SimulateMaybeExtractCreativeAdPlacementIdsFromWebPageCallback(
      creative_search_result_ad_handler.get(),
      test::CreativeSearchResultAdMojomWebPage(
          /*excluded_property_names=*/{kCreativeAdRewardsValuePropertyName}));
}

TEST_F(BraveAdsCreativeSearchResultAdHandlerTest,
       DoNotTriggerViewedOrClickedAdEventsForInvalidCreativeSetConversion) {
  // Arrange
  const auto creative_search_result_ad_handler =
      CreativeSearchResultAdHandler::MaybeCreate(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_creative_ad_viewed_events=*/true);
  ASSERT_TRUE(creative_search_result_ad_handler);

  // Act & Assert
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          ::testing::_, mojom::SearchResultAdEventType::kViewedImpression,
          ::testing::_))
      .WillOnce([](mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                   mojom::SearchResultAdEventType /*mojom_ad_event_type*/,
                   TriggerAdEventCallback /*callback*/) {
        EXPECT_FALSE(mojom_creative_ad->creative_set_conversion);
      });

  SimulateMaybeExtractCreativeAdPlacementIdsFromWebPageCallback(
      creative_search_result_ad_handler.get(),
      test::CreativeSearchResultAdMojomWebPage(
          /*excluded_property_names=*/{
              kCreativeSetConversionUrlPatternPropertyName}));
}

TEST_F(BraveAdsCreativeSearchResultAdHandlerTest,
       TriggerViewedAndClickedAdEvents) {
  // Arrange
  const auto creative_search_result_ad_handler =
      CreativeSearchResultAdHandler::MaybeCreate(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_creative_ad_viewed_events=*/true);
  ASSERT_TRUE(creative_search_result_ad_handler);

  const blink::mojom::WebPagePtr mojom_web_page =
      test::CreativeSearchResultAdMojomWebPage(/*excluded_property_names=*/{});
  ASSERT_TRUE(mojom_web_page);

  // Act & Assert
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          ::testing::_, mojom::SearchResultAdEventType::kViewedImpression,
          ::testing::_))
      .WillOnce([&mojom_web_page](
                    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                    mojom::SearchResultAdEventType /*mojom_ad_event_type*/,
                    TriggerAdEventCallback /*callback*/) {
        const std::vector<mojom::CreativeSearchResultAdInfoPtr>
            creative_search_result_ads =
                ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
                    mojom_web_page->entities);
        ASSERT_EQ(creative_search_result_ads.size(), 1u);
        ASSERT_EQ(creative_search_result_ads[0]->placement_id,
                  test::kCreativeAdPlacementId);

        EXPECT_EQ(creative_search_result_ads[0], mojom_creative_ad);
      });

  SimulateMaybeExtractCreativeAdPlacementIdsFromWebPageCallback(
      creative_search_result_ad_handler.get(), mojom_web_page->Clone());
}

TEST_F(BraveAdsCreativeSearchResultAdHandlerTest,
       TriggerViewedAndClickedAdEventsWithUnreservedCharactersInPlacementId) {
  // Arrange
  const auto creative_search_result_ad_handler =
      CreativeSearchResultAdHandler::MaybeCreate(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_creative_ad_viewed_events=*/true);
  ASSERT_TRUE(creative_search_result_ad_handler);

  const blink::mojom::WebPagePtr mojom_web_page =
      test::CreativeSearchResultAdMojomWebPageWithProperty(
          /*name=*/"data-placement-id",
          /*value=*/test::kCreativeAdPlacementIdWithUnreservedCharacters);
  ASSERT_TRUE(mojom_web_page);

  // Act & Assert
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          ::testing::_, mojom::SearchResultAdEventType::kViewedImpression,
          ::testing::_))
      .WillOnce([&mojom_web_page](
                    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                    mojom::SearchResultAdEventType /*mojom_ad_event_type*/,
                    TriggerAdEventCallback /*callback*/) {
        const std::vector<mojom::CreativeSearchResultAdInfoPtr>
            creative_search_result_ads =
                ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
                    mojom_web_page->entities);
        ASSERT_EQ(creative_search_result_ads.size(), 1u);
        ASSERT_EQ(creative_search_result_ads[0]->placement_id,
                  test::kEscapedCreativeAdPlacementIdWithUnreservedCharacters);

        EXPECT_EQ(creative_search_result_ads[0], mojom_creative_ad);
      });

  SimulateMaybeExtractCreativeAdPlacementIdsFromWebPageCallback(
      creative_search_result_ad_handler.get(), mojom_web_page->Clone());
}

}  // namespace brave_ads
