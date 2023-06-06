/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/strings/string_piece.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/browser/ads_service_mock.h"
#include "brave/components/brave_ads/common/search_result_ad_feature.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_handler.h"
#include "brave/components/brave_ads/core/search_result_ad/test_web_page_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

using testing::_;
using testing::Mock;
using testing::Return;

using OnRetrieveSearchResultAdCallback =
    base::OnceCallback<void(std::vector<std::string> placement_ids)>;

constexpr char kAllowedDomain[] = "https://search.brave.com";
constexpr char kNotAllowedDomain[] = "https://brave.com";
constexpr char kSearchResultAdClickUrl[] =
    "https://search.brave.com/a/redirect?";
constexpr char kPlacementId[] = "placement_id";

blink::mojom::WebPagePtr CreateTestWebPage(
    std::vector<base::StringPiece> attributes_to_skip = {}) {
  blink::mojom::WebPagePtr web_page = blink::mojom::WebPage::New();
  web_page->entities = CreateTestWebPageEntities(std::move(attributes_to_skip));
  return web_page;
}

GURL GetSearchResultAdClickedUrl() {
  return GURL(base::StrCat(
      {kSearchResultAdClickUrl, kPlacementId, "=", kTestWebPagePlacementId}));
}

}  // namespace

class SearchResultAdHandlerTest : public ::testing::Test {
 public:
  SearchResultAdHandlerTest() {
    feature_list_.InitAndEnableFeature(kSearchResultAdFeature);
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
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(SearchResultAdHandlerTest,
       BraveAdsDisabledMaybeCreateSearchResultAdHandler) {
  EXPECT_CALL(ads_service_mock_, IsEnabled()).WillOnce(Return(false));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);

  EXPECT_FALSE(search_result_ad_handler.get());
}

TEST_F(SearchResultAdHandlerTest,
       IncognitoModeMaybeCreateSearchResultAdHandler) {
  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          nullptr, GURL(kAllowedDomain), /*should_trigger_viewed_event*/ true);

  EXPECT_FALSE(search_result_ad_handler.get());
}

TEST_F(SearchResultAdHandlerTest,
       NotAllowedDomainMaybeCreateSearchResultAdHandler) {
  EXPECT_CALL(ads_service_mock_, IsEnabled()).WillOnce(Return(true));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kNotAllowedDomain),
          /*should_trigger_viewed_event*/ true);

  EXPECT_FALSE(search_result_ad_handler.get());
}

TEST_F(SearchResultAdHandlerTest, NullWebPage) {
  EXPECT_CALL(ads_service_mock_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(_, mojom::SearchResultAdEventType::kViewed, _))
      .Times(0);
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  _, mojom::SearchResultAdEventType::kClicked, _))
      .Times(0);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run(_))
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
  EXPECT_CALL(ads_service_mock_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(_, mojom::SearchResultAdEventType::kViewed, _))
      .Times(0);
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  _, mojom::SearchResultAdEventType::kClicked, _))
      .Times(0);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run(_))
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
  EXPECT_CALL(ads_service_mock_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(_, mojom::SearchResultAdEventType::kViewed, _))
      .Times(0);
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  _, mojom::SearchResultAdEventType::kClicked, _))
      .Times(0);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  // "data-rewards-value" is missed.
  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run(_))
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
  EXPECT_CALL(ads_service_mock_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  _, mojom::SearchResultAdEventType::kViewed, _));
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  _, mojom::SearchResultAdEventType::kClicked, _));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  // "data-conversion-type-value" is missed.
  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run(_))
      .WillOnce([&search_result_ad_handler](
                    const std::vector<std::string>& placement_ids) {
        for (const std::string& placement_id : placement_ids) {
          search_result_ad_handler->MaybeTriggerSearchResultAdViewedEvent(
              placement_id);
        }
      });
  SimulateOnRetrieveSearchResultAdEntities(
      search_result_ad_handler.get(), callback.Get(),
      CreateTestWebPage({"data-conversion-type-value"}));

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, BraveAdsBecomeDisabled) {
  EXPECT_CALL(ads_service_mock_, IsEnabled()).WillOnce(Return(true));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());
  Mock::VerifyAndClearExpectations(&ads_service_mock_);

  EXPECT_CALL(ads_service_mock_, IsEnabled()).WillRepeatedly(Return(false));
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(_, mojom::SearchResultAdEventType::kViewed, _))
      .Times(0);
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  _, mojom::SearchResultAdEventType::kClicked, _))
      .Times(0);

  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run(_))
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

TEST_F(SearchResultAdHandlerTest, BraveAdsViewedClicked) {
  EXPECT_CALL(ads_service_mock_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  _, mojom::SearchResultAdEventType::kViewed, _));
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  _, mojom::SearchResultAdEventType::kClicked, _))
      .Times(2);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run(_))
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

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, BraveAdsTabRestored) {
  EXPECT_CALL(ads_service_mock_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(
      ads_service_mock_,
      TriggerSearchResultAdEvent(_, mojom::SearchResultAdEventType::kViewed, _))
      .Times(0);
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  _, mojom::SearchResultAdEventType::kClicked, _));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ false);
  ASSERT_TRUE(search_result_ad_handler.get());

  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run(_))
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
  EXPECT_CALL(ads_service_mock_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  _, mojom::SearchResultAdEventType::kViewed, _));
  EXPECT_CALL(ads_service_mock_,
              TriggerSearchResultAdEvent(
                  _, mojom::SearchResultAdEventType::kClicked, _))
      .Times(0);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_mock_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  base::MockCallback<OnRetrieveSearchResultAdCallback> callback;
  EXPECT_CALL(callback, Run(_))
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
