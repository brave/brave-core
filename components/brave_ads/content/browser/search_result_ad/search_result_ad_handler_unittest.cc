/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/browser/mock_ads_service.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_handler.h"
#include "brave/components/brave_ads/core/browser/search_result_ad/test_web_page_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::_;
using testing::Mock;
using testing::Return;

namespace brave_ads {
namespace {

constexpr char kAllowedDomain[] = "https://search.brave.com";
constexpr char kNotAllowedDomain[] = "https://brave.com";

}  // namespace

class SearchResultAdHandlerTest : public ::testing::Test {
 public:
  SearchResultAdHandlerTest() {
    feature_list_.InitAndEnableFeature(
        features::kSupportBraveSearchResultAdConfirmationEvents);
  }
  ~SearchResultAdHandlerTest() override = default;

  SearchResultAdHandlerTest(const SearchResultAdHandlerTest&) = delete;
  SearchResultAdHandlerTest& operator=(const SearchResultAdHandlerTest&) =
      delete;

  void SimulateOnRetrieveSearchResultAdEntities(
      SearchResultAdHandler* search_result_ad_handler,
      blink::mojom::WebPagePtr web_page) {
    mojo::Remote<blink::mojom::DocumentMetadata> remote;
    search_result_ad_handler->OnRetrieveSearchResultAdEntities(
        mojo::Remote<blink::mojom::DocumentMetadata>(), std::move(web_page));
  }

 protected:
  MockAdsService ads_service_;
  std::unique_ptr<SearchResultAdHandler> search_result_ad_handler_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(SearchResultAdHandlerTest,
       BraveAdsDisabledMaybeCreateSearchResultAdHandler) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillOnce(Return(false));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, GURL(kAllowedDomain),
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
  EXPECT_CALL(ads_service_, IsEnabled()).WillOnce(Return(true));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, GURL(kNotAllowedDomain),
          /*should_trigger_viewed_event*/ true);

  EXPECT_FALSE(search_result_ad_handler.get());
}

TEST_F(SearchResultAdHandlerTest, BraveAdsEmpty) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kViewed))
      .Times(0);
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kClicked))
      .Times(0);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           blink::mojom::WebPage::New());

  absl::optional<GURL> url =
      search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
          kCreativeInstanceId);
  EXPECT_FALSE(url);
}

TEST_F(SearchResultAdHandlerTest, BraveAdsBecomeDisabled) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillOnce(Return(true));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());
  Mock::VerifyAndClearExpectations(&ads_service_);

  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(false));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kViewed))
      .Times(0);
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kClicked))
      .Times(0);

  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           CreateTestWebPage());

  absl::optional<GURL> url =
      search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
          kCreativeInstanceId);
  EXPECT_FALSE(url);
}

TEST_F(SearchResultAdHandlerTest, BraveAdsViewedClicked) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kViewed));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kClicked))
      .Times(2);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           CreateTestWebPage());

  absl::optional<GURL> url =
      search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
          kCreativeInstanceId);
  EXPECT_EQ(url, GURL(kTargetUrl));

  url = search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      kCreativeInstanceId);
  EXPECT_EQ(url, GURL(kTargetUrl));
}

TEST_F(SearchResultAdHandlerTest, BraveAdsTabRestored) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kViewed))
      .Times(0);
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kClicked));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, GURL(kAllowedDomain),
          /*should_trigger_viewed_event*/ false);
  ASSERT_TRUE(search_result_ad_handler.get());

  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           CreateTestWebPage());

  absl::optional<GURL> url =
      search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
          kCreativeInstanceId);
  EXPECT_EQ(url, GURL(kTargetUrl));
}

}  // namespace brave_ads
