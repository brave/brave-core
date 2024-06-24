/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/ad_units/search_result_ad/search_result_ad_handler.h"

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/browser/ads_service_mock.h"
#include "brave/components/brave_ads/content/browser/ad_units/search_result_ad/search_result_ad_converting_util.h"
#include "brave/components/brave_ads/content/browser/ad_units/search_result_ad/test_web_page_util.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

using OnRetrieveSearchResultAdCallback =
    base::OnceCallback<void(std::vector<std::string> placement_ids)>;

constexpr char kAllowedDomain[] = "https://search.brave.com";
constexpr char kNotAllowedDomain[] = "https://brave.com";
constexpr char kSearchResultAdClickUrl[] =
    "https://search.brave.com/a/redirect?";
constexpr char kPlacementId[] = "placement_id";

blink::mojom::WebPagePtr CreateTestWebPage(
    std::vector<std::string_view> attributes_to_skip = {}) {
  blink::mojom::WebPagePtr web_page = blink::mojom::WebPage::New();
  web_page->entities = CreateTestWebPageEntities(std::move(attributes_to_skip));
  return web_page;
}

GURL GetSearchResultAdClickedUrl() {
  return GURL(base::StrCat(
      {kSearchResultAdClickUrl, kPlacementId, "=", kTestWebPagePlacementId}));
}

void CompareSearchResultAdInfosWithNonEmptyConversion(
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad_1,
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad_2) {
  EXPECT_EQ(mojom_creative_ad_1->placement_id,
            mojom_creative_ad_2->placement_id);
  EXPECT_EQ(mojom_creative_ad_1->advertiser_id,
            mojom_creative_ad_2->advertiser_id);
  EXPECT_EQ(mojom_creative_ad_1->campaign_id, mojom_creative_ad_2->campaign_id);
  EXPECT_EQ(mojom_creative_ad_1->creative_instance_id,
            mojom_creative_ad_2->creative_instance_id);
  EXPECT_EQ(mojom_creative_ad_1->creative_set_id,
            mojom_creative_ad_2->creative_set_id);
  EXPECT_EQ(mojom_creative_ad_1->description, mojom_creative_ad_2->description);
  EXPECT_EQ(mojom_creative_ad_1->headline_text,
            mojom_creative_ad_2->headline_text);
  EXPECT_EQ(mojom_creative_ad_1->target_url, mojom_creative_ad_2->target_url);
  EXPECT_EQ(mojom_creative_ad_1->type, mojom_creative_ad_2->type);
  EXPECT_EQ(mojom_creative_ad_1->value, mojom_creative_ad_2->value);
  ASSERT_TRUE(mojom_creative_ad_1->conversion);
  ASSERT_TRUE(mojom_creative_ad_2->conversion);
  EXPECT_EQ(mojom_creative_ad_1->conversion->observation_window,
            mojom_creative_ad_2->conversion->observation_window);
  EXPECT_EQ(mojom_creative_ad_1->conversion->url_pattern,
            mojom_creative_ad_2->conversion->url_pattern);
  EXPECT_EQ(
      mojom_creative_ad_1->conversion->verifiable_advertiser_public_key_base64,
      mojom_creative_ad_2->conversion->verifiable_advertiser_public_key_base64);
}

}  // namespace

class SearchResultAdHandlerTest : public ::testing::Test {
 public:
  SearchResultAdHandlerTest() {
    scoped_feature_list_.InitAndEnableFeature(
        kShouldSupportSearchResultAdsFeature);
  }

  static void SimulateOnRetrieveSearchResultAdEntities(
      SearchResultAdHandler* search_result_ad_handler,
      OnRetrieveSearchResultAdCallback callback,
      blink::mojom::WebPagePtr web_page) {
    search_result_ad_handler->OnRetrieveSearchResultAdEntities(
        mojo::Remote<blink::mojom::DocumentMetadata>(), std::move(callback),
        std::move(web_page));
  }

 protected:
  AdsServiceMock ads_service_mock_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(SearchResultAdHandlerTest,
       IncognitoModeMaybeCreateSearchResultAdHandler) {
  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          nullptr, GURL(kAllowedDomain), /*should_trigger_viewed_event*/ true);

  EXPECT_FALSE(search_result_ad_handler.get());
}

TEST_F(SearchResultAdHandlerTest,
       NotAllowedDomainMaybeCreateSearchResultAdHandler) {
  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kNotAllowedDomain),
          /*should_trigger_viewed_event*/ true);

  EXPECT_FALSE(search_result_ad_handler.get());
}

TEST_F(SearchResultAdHandlerTest, NullWebPage) {
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  testing::_, mojom::SearchResultAdEventType::kViewedImpression,
                  testing::_))
      .Times(0);
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kClicked, testing::_))
      .Times(0);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&search_result_ad_handler](
                    const std::vector<std::string>& placement_ids) {
        for (const std::string& placement_id : placement_ids) {
          search_result_ad_handler->MaybeTriggerSearchResultAdViewedEvent(
              placement_id);
        }
      });
  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           callback.Get(),
                                           blink::mojom::WebPagePtr());

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, EmptyWebPage) {
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  testing::_, mojom::SearchResultAdEventType::kViewedImpression,
                  testing::_))
      .Times(0);
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kClicked, testing::_))
      .Times(0);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&search_result_ad_handler](
                    const std::vector<std::string>& placement_ids) {
        for (const std::string& placement_id : placement_ids) {
          search_result_ad_handler->MaybeTriggerSearchResultAdViewedEvent(
              placement_id);
        }
      });
  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           callback.Get(),
                                           blink::mojom::WebPage::New());

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, NotValidSearchResultAd) {
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  testing::_, mojom::SearchResultAdEventType::kViewedImpression,
                  testing::_))
      .Times(0);
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kClicked, testing::_))
      .Times(0);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  // "data-rewards-value" is missed.
  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&search_result_ad_handler](
                    const std::vector<std::string>& placement_ids) {
        for (const std::string& placement_id : placement_ids) {
          search_result_ad_handler->MaybeTriggerSearchResultAdViewedEvent(
              placement_id);
        }
      });
  SimulateOnRetrieveSearchResultAdEntities(
      search_result_ad_handler.get(), callback.Get(),
      CreateTestWebPage({"data-rewards-value"}));

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, EmptyConversions) {
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  testing::_, mojom::SearchResultAdEventType::kViewedImpression,
                  testing::_))
      .WillOnce([](mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                   mojom::SearchResultAdEventType /*event_type*/,
                   TriggerAdEventCallback /*callback*/) {
        EXPECT_FALSE(mojom_creative_ad->conversion);
      });
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kClicked, testing::_));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&search_result_ad_handler](
                    const std::vector<std::string>& placement_ids) {
        for (const std::string& placement_id : placement_ids) {
          search_result_ad_handler->MaybeTriggerSearchResultAdViewedEvent(
              placement_id);
        }
      });
  // "data-conversion-url-pattern-value" is missed so conversions won't be
  // parsed.
  SimulateOnRetrieveSearchResultAdEntities(
      search_result_ad_handler.get(), callback.Get(),
      CreateTestWebPage({"data-conversion-url-pattern-value"}));

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, BraveAdsViewedClicked) {
  blink::mojom::WebPagePtr web_page = CreateTestWebPage();
  ASSERT_TRUE(web_page);

  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  testing::_, mojom::SearchResultAdEventType::kViewedImpression,
                  testing::_))
      .WillOnce([&web_page](
                    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                    mojom::SearchResultAdEventType /*event_type*/,
                    TriggerAdEventCallback /*callback*/) {
        const auto search_result_ads =
            ConvertWebPageEntitiesToCreativeSearchResultAds(web_page->entities);
        ASSERT_TRUE(search_result_ads.contains(kTestWebPagePlacementId));
        CompareSearchResultAdInfosWithNonEmptyConversion(
            search_result_ads.at(kTestWebPagePlacementId), mojom_creative_ad);
      });

  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kClicked, testing::_))
      .Times(2)
      .WillRepeatedly(
          [&web_page](mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
                      mojom::SearchResultAdEventType /*event_type*/,
                      TriggerAdEventCallback /*callback*/) {
            const auto search_result_ads =
                ConvertWebPageEntitiesToCreativeSearchResultAds(
                    web_page->entities);
            ASSERT_TRUE(search_result_ads.contains(kTestWebPagePlacementId));
            CompareSearchResultAdInfosWithNonEmptyConversion(
                search_result_ads.at(kTestWebPagePlacementId),
                mojom_creative_ad);
          });

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&search_result_ad_handler](
                    const std::vector<std::string>& placement_ids) {
        for (const std::string& placement_id : placement_ids) {
          search_result_ad_handler->MaybeTriggerSearchResultAdViewedEvent(
              placement_id);
        }
      });
  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           callback.Get(), web_page->Clone());

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, BraveAdsTabRestored) {
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  testing::_, mojom::SearchResultAdEventType::kViewedImpression,
                  testing::_))
      .Times(0);
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kClicked, testing::_));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ false);
  ASSERT_TRUE(search_result_ad_handler.get());

  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&search_result_ad_handler](
                    const std::vector<std::string>& placement_ids) {
        for (const std::string& placement_id : placement_ids) {
          search_result_ad_handler->MaybeTriggerSearchResultAdViewedEvent(
              placement_id);
        }
      });
  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           callback.Get(), CreateTestWebPage());

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, WrongClickedUrl) {
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  testing::_, mojom::SearchResultAdEventType::kViewedImpression,
                  testing::_));
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kClicked, testing::_))
      .Times(0);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&search_result_ad_handler](
                    const std::vector<std::string>& placement_ids) {
        for (const std::string& placement_id : placement_ids) {
          search_result_ad_handler->MaybeTriggerSearchResultAdViewedEvent(
              placement_id);
        }
      });
  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           callback.Get(), CreateTestWebPage());

  GURL url(base::StrCat({kSearchResultAdClickUrl, kPlacementId}));
  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(url);

  url = GURL(base::StrCat({kSearchResultAdClickUrl, "not_placement_id=id"}));
  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(url);

  url = GURL(base::StrCat({kSearchResultAdClickUrl}));
  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(url);
}

}  // namespace brave_ads
