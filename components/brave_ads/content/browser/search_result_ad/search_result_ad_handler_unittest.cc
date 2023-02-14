/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_piece.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/browser/mock_ads_service.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_handler.h"
#include "brave/components/brave_ads/core/browser/search_result_ad/test_web_page_util.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

using testing::_;
using testing::Mock;
using testing::Return;

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
    feature_list_.InitAndEnableFeature(
        features::kSupportBraveSearchResultAdConfirmationEvents);
  }

  static void SimulateOnRetrieveSearchResultAdEntities(
      SearchResultAdHandler* search_result_ad_handler,
      blink::mojom::WebPagePtr web_page) {
    search_result_ad_handler->OnRetrieveSearchResultAdEntities(
        mojo::Remote<blink::mojom::DocumentMetadata>(), std::move(web_page));
  }

  void SetUp() override {
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());
    settings_map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &prefs_, false /* is_off_the_record */, false /* store_last_modified */,
        false /* restore_session */, false /* should_record_metrics */);
  }

  void TearDown() override { settings_map_->ShutdownOnUIThread(); }

  HostContentSettingsMap* settings_map() { return settings_map_.get(); }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  MockAdsService ads_service_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  scoped_refptr<HostContentSettingsMap> settings_map_;
};

TEST_F(SearchResultAdHandlerTest,
       BraveAdsDisabledMaybeCreateSearchResultAdHandler) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillOnce(Return(false));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, GURL(kAllowedDomain), settings_map(),
          /*should_trigger_viewed_event*/ true);

  EXPECT_FALSE(search_result_ad_handler.get());
}

TEST_F(SearchResultAdHandlerTest,
       IncognitoModeMaybeCreateSearchResultAdHandler) {
  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          nullptr, GURL(kAllowedDomain), settings_map(),
          /*should_trigger_viewed_event*/ true);

  EXPECT_FALSE(search_result_ad_handler.get());
}

TEST_F(SearchResultAdHandlerTest,
       NotAllowedDomainMaybeCreateSearchResultAdHandler) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillOnce(Return(true));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, GURL(kNotAllowedDomain), settings_map(),
          /*should_trigger_viewed_event*/ true);

  EXPECT_FALSE(search_result_ad_handler.get());
}

TEST_F(SearchResultAdHandlerTest,
       CosmeticFilteringMaybeCreateSearchResultAdHandler) {
  const GURL url = GURL(kAllowedDomain);
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

  // Aggressively block ads.
  brave_shields::SetCosmeticFilteringControlType(
      settings_map(), brave_shields::ControlType::BLOCK, url);
  brave_shields::SetAdControlType(settings_map(),
                                  brave_shields::ControlType::BLOCK, url);
  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, url, settings_map(),
          /*should_trigger_viewed_event*/ true);
  EXPECT_FALSE(search_result_ad_handler.get());

  // Disable aggressive mode.
  brave_shields::SetCosmeticFilteringControlType(
      settings_map(), brave_shields::ControlType::BLOCK_THIRD_PARTY, url);
  search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, url, settings_map(),
          /*should_trigger_viewed_event*/ true);
  EXPECT_TRUE(search_result_ad_handler.get());
  brave_shields::SetCosmeticFilteringControlType(
      settings_map(), brave_shields::ControlType::BLOCK, url);

  // Allow ads.
  brave_shields::SetAdControlType(settings_map(),
                                  brave_shields::ControlType::ALLOW, url);
  search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, url, settings_map(),
          /*should_trigger_viewed_event*/ true);
  EXPECT_TRUE(search_result_ad_handler.get());
  brave_shields::SetAdControlType(settings_map(),
                                  brave_shields::ControlType::BLOCK, url);

  // Disable Brave Shields.
  brave_shields::SetAdControlType(settings_map(),
                                  brave_shields::ControlType::BLOCK, url);
  brave_shields::SetBraveShieldsEnabled(settings_map(), false, url);
  search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, url, settings_map(),
          /*should_trigger_viewed_event*/ true);
  EXPECT_TRUE(search_result_ad_handler.get());
}

TEST_F(SearchResultAdHandlerTest, NullWebPage) {
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
          &ads_service_, GURL(kAllowedDomain), settings_map(),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           blink::mojom::WebPagePtr());

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, EmptyWebPage) {
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
          &ads_service_, GURL(kAllowedDomain), settings_map(),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           blink::mojom::WebPage::New());

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, NotValidSearchResultAd) {
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
          &ads_service_, GURL(kAllowedDomain), settings_map(),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  // "data-rewards-value" is missed.
  SimulateOnRetrieveSearchResultAdEntities(
      search_result_ad_handler.get(),
      CreateTestWebPage({"data-rewards-value"}));

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, EmptyConversions) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kServed));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kViewed));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kClicked));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, GURL(kAllowedDomain), settings_map(),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  // "data-conversion-type-value" is missed.
  SimulateOnRetrieveSearchResultAdEntities(
      search_result_ad_handler.get(),
      CreateTestWebPage({"data-conversion-type-value"}));

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, BraveAdsBecomeDisabled) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillOnce(Return(true));

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, GURL(kAllowedDomain), settings_map(),
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

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, BraveAdsViewedClicked) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kServed));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kViewed));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kClicked))
      .Times(2);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, GURL(kAllowedDomain), settings_map(),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           CreateTestWebPage());

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
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
          &ads_service_, GURL(kAllowedDomain), settings_map(),
          /*should_trigger_viewed_event*/ false);
  ASSERT_TRUE(search_result_ad_handler.get());

  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           CreateTestWebPage());

  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(
      GetSearchResultAdClickedUrl());
}

TEST_F(SearchResultAdHandlerTest, WrongClickedUrl) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kServed));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kViewed));
  EXPECT_CALL(ads_service_,
              TriggerSearchResultAdEvent(
                  _, ads::mojom::SearchResultAdEventType::kClicked))
      .Times(0);

  auto search_result_ad_handler =
      SearchResultAdHandler::MaybeCreateSearchResultAdHandler(
          &ads_service_, GURL(kAllowedDomain), settings_map(),
          /*should_trigger_viewed_event*/ true);
  ASSERT_TRUE(search_result_ad_handler.get());

  SimulateOnRetrieveSearchResultAdEntities(search_result_ad_handler.get(),
                                           CreateTestWebPage());

  GURL url(base::StrCat({kSearchResultAdClickUrl, kPlacementId}));
  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(url);

  url = GURL(base::StrCat({kSearchResultAdClickUrl, "not_placement_id=id"}));
  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(url);

  url = GURL(base::StrCat({kSearchResultAdClickUrl}));
  search_result_ad_handler->MaybeTriggerSearchResultAdClickedEvent(url);
}

}  // namespace brave_ads
