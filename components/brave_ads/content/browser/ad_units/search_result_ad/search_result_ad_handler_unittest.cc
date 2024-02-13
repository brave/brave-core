/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/browser/ads_service_mock.h"
#include "brave/components/brave_ads/content/browser/ad_units/search_result_ad/search_result_ad_converting_util.h"
#include "brave/components/brave_ads/content/browser/ad_units/search_result_ad/search_result_ad_handler.h"
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
    const mojom::SearchResultAdInfoPtr& search_result_ad_info1,
    const mojom::SearchResultAdInfoPtr& search_result_ad_info2) {
  EXPECT_EQ(search_result_ad_info1->placement_id,
            search_result_ad_info2->placement_id);
  EXPECT_EQ(search_result_ad_info1->advertiser_id,
            search_result_ad_info2->advertiser_id);
  EXPECT_EQ(search_result_ad_info1->campaign_id,
            search_result_ad_info2->campaign_id);
  EXPECT_EQ(search_result_ad_info1->creative_instance_id,
            search_result_ad_info2->creative_instance_id);
  EXPECT_EQ(search_result_ad_info1->creative_set_id,
            search_result_ad_info2->creative_set_id);
  EXPECT_EQ(search_result_ad_info1->description,
            search_result_ad_info2->description);
  EXPECT_EQ(search_result_ad_info1->headline_text,
            search_result_ad_info2->headline_text);
  EXPECT_EQ(search_result_ad_info1->target_url,
            search_result_ad_info2->target_url);
  EXPECT_EQ(search_result_ad_info1->type, search_result_ad_info2->type);
  EXPECT_EQ(search_result_ad_info1->value, search_result_ad_info2->value);
  ASSERT_TRUE(search_result_ad_info1->conversion);
  ASSERT_TRUE(search_result_ad_info2->conversion);
  EXPECT_EQ(search_result_ad_info1->conversion->observation_window,
            search_result_ad_info2->conversion->observation_window);
  EXPECT_EQ(search_result_ad_info1->conversion->url_pattern,
            search_result_ad_info2->conversion->url_pattern);
  EXPECT_EQ(search_result_ad_info1->conversion
                ->verifiable_advertiser_public_key_base64,
            search_result_ad_info2->conversion
                ->verifiable_advertiser_public_key_base64);
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
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kViewed, testing::_))
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
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kViewed, testing::_))
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
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kViewed, testing::_))
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
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kViewed, testing::_))
      .WillOnce([](mojom::SearchResultAdInfoPtr ad_mojom,
                   mojom::SearchResultAdEventType /*event_type*/,
                   TriggerAdEventCallback /*callback*/) {
        EXPECT_FALSE(ad_mojom->conversion);
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

  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kViewed, testing::_))
      .WillOnce([&web_page](mojom::SearchResultAdInfoPtr ad_mojom,
                            mojom::SearchResultAdEventType /*event_type*/,
                            TriggerAdEventCallback /*callback*/) {
        const auto search_result_ads =
            ConvertWebPageEntitiesToSearchResultAds(web_page->entities);
        ASSERT_TRUE(search_result_ads.contains(kTestWebPagePlacementId));
        CompareSearchResultAdInfosWithNonEmptyConversion(
            search_result_ads.at(kTestWebPagePlacementId), ad_mojom);
      });

  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kClicked, testing::_))
      .Times(2)
      .WillRepeatedly([&web_page](mojom::SearchResultAdInfoPtr ad_mojom,
                                  mojom::SearchResultAdEventType /*event_type*/,
                                  TriggerAdEventCallback /*callback*/) {
        const auto search_result_ads =
            ConvertWebPageEntitiesToSearchResultAds(web_page->entities);
        ASSERT_TRUE(search_result_ads.contains(kTestWebPagePlacementId));
        CompareSearchResultAdInfosWithNonEmptyConversion(
            search_result_ads.at(kTestWebPagePlacementId), ad_mojom);
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
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kViewed, testing::_))
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
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(
          testing::_, mojom::SearchResultAdEventType::kViewed, testing::_));
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
