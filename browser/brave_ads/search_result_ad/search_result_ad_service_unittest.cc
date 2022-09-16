/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/browser/mock_ads_service.h"
#include "brave/components/brave_ads/common/features.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/sessions/core/session_id.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::Return;

namespace {

constexpr char kAllowedDomain[] = "https://search.brave.com";
constexpr char kNotAllowedDomain[] = "https://brave.com";

class SearchResultAdServiceTest : public ChromeRenderViewHostTestHarness {
 public:
  SearchResultAdServiceTest() {
    feature_list_.InitAndEnableFeature(
        brave_ads::features::kSupportBraveSearchResultAdConfirmationEvents);
    search_result_ad_service_ =
        std::make_unique<brave_ads::SearchResultAdService>(&ads_service_);
  }
  ~SearchResultAdServiceTest() override = default;

  SearchResultAdServiceTest(const SearchResultAdServiceTest&) = delete;
  SearchResultAdServiceTest& operator=(const SearchResultAdServiceTest&) =
      delete;

 protected:
  brave_ads::MockAdsService ads_service_;
  std::unique_ptr<brave_ads::SearchResultAdService> search_result_ad_service_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(SearchResultAdServiceTest, BraveAdsDisabledTryTriggerAd) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillOnce(Return(false));

  SessionID session_id = SessionID::NewUnique();
  NavigateAndCommit(GURL(kAllowedDomain));
  search_result_ad_service_->OnDidFinishNavigation(session_id);

  base::RunLoop run_loop;
  search_result_ad_service_->MaybeTriggerSearchResultAdViewedEvent(
      "creative_instance_id", session_id,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool ad_was_triggered) {
            EXPECT_FALSE(ad_was_triggered);
            run_loop->Quit();
          },
          base::Unretained(&run_loop)));

  run_loop.Run();
}

TEST_F(SearchResultAdServiceTest, NotAllowedDomainTryTriggerAdBeforeRetrieve) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

  SessionID session_id = SessionID::NewUnique();
  NavigateAndCommit(GURL(kNotAllowedDomain));
  search_result_ad_service_->OnDidFinishNavigation(session_id);

  base::RunLoop run_loop;
  search_result_ad_service_->MaybeTriggerSearchResultAdViewedEvent(
      "creative_instance_id", session_id,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool ad_was_triggered) {
            EXPECT_FALSE(ad_was_triggered);
            run_loop->Quit();
          },
          base::Unretained(&run_loop)));

  search_result_ad_service_->MaybeRetrieveSearchResultAd(main_rfh(), session_id,
                                                         true);

  run_loop.Run();
}

TEST_F(SearchResultAdServiceTest, NotAllowedDomainTryTriggerAdAfterRetrieve) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

  SessionID session_id = SessionID::NewUnique();
  NavigateAndCommit(GURL(kNotAllowedDomain));
  search_result_ad_service_->OnDidFinishNavigation(session_id);

  search_result_ad_service_->MaybeRetrieveSearchResultAd(main_rfh(), session_id,
                                                         true);

  base::RunLoop run_loop;
  search_result_ad_service_->MaybeTriggerSearchResultAdViewedEvent(
      "creative_instance_id", session_id,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool ad_was_triggered) {
            EXPECT_FALSE(ad_was_triggered);
            run_loop->Quit();
          },
          base::Unretained(&run_loop)));

  run_loop.Run();
}

TEST_F(SearchResultAdServiceTest, TabRestoredTryTriggerAdBeforeRetrieve) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

  SessionID session_id = SessionID::NewUnique();
  NavigateAndCommit(GURL(kAllowedDomain));
  search_result_ad_service_->OnDidFinishNavigation(session_id);

  base::RunLoop run_loop;
  search_result_ad_service_->MaybeTriggerSearchResultAdViewedEvent(
      "creative_instance_id", session_id,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool ad_was_triggered) {
            EXPECT_FALSE(ad_was_triggered);
            run_loop->Quit();
          },
          base::Unretained(&run_loop)));

  search_result_ad_service_->MaybeRetrieveSearchResultAd(main_rfh(), session_id,
                                                         false);

  run_loop.Run();
}

TEST_F(SearchResultAdServiceTest, TabRestoredTryTriggerAdAfterRetrieve) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

  SessionID session_id = SessionID::NewUnique();
  NavigateAndCommit(GURL(kAllowedDomain));
  search_result_ad_service_->OnDidFinishNavigation(session_id);

  search_result_ad_service_->MaybeRetrieveSearchResultAd(main_rfh(), session_id,
                                                         false);

  base::RunLoop run_loop;
  search_result_ad_service_->MaybeTriggerSearchResultAdViewedEvent(
      "creative_instance_id", session_id,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool ad_was_triggered) {
            EXPECT_FALSE(ad_was_triggered);
            run_loop->Quit();
          },
          base::Unretained(&run_loop)));

  run_loop.Run();
}

TEST_F(SearchResultAdServiceTest, UnknownTabTryTriggerAd) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

  SessionID session_id = SessionID::NewUnique();

  base::RunLoop run_loop;
  search_result_ad_service_->MaybeTriggerSearchResultAdViewedEvent(
      "creative_instance_id", session_id,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool ad_was_triggered) {
            EXPECT_FALSE(ad_was_triggered);
            run_loop->Quit();
          },
          base::Unretained(&run_loop)));

  run_loop.Run();
}

TEST_F(SearchResultAdServiceTest, TryTriggerAdRepeatedNavigation) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

  SessionID session_id = SessionID::NewUnique();
  // Simulate navigation was finished.
  search_result_ad_service_->OnDidFinishNavigation(session_id);

  base::RunLoop run_loop;
  search_result_ad_service_->MaybeTriggerSearchResultAdViewedEvent(
      "creative_instance_id", session_id,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool ad_was_triggered) {
            EXPECT_FALSE(ad_was_triggered);
            run_loop->Quit();
          },
          base::Unretained(&run_loop)));

  // Simulate navigation.
  search_result_ad_service_->OnDidFinishNavigation(session_id);

  run_loop.Run();
}

TEST_F(SearchResultAdServiceTest, TryTriggerAdBeforeTabDeleted) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

  SessionID session_id = SessionID::NewUnique();
  // Simulate navigation was finished.
  search_result_ad_service_->OnDidFinishNavigation(session_id);

  base::RunLoop run_loop;
  search_result_ad_service_->MaybeTriggerSearchResultAdViewedEvent(
      "creative_instance_id", session_id,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool ad_was_triggered) {
            EXPECT_FALSE(ad_was_triggered);
            run_loop->Quit();
          },
          base::Unretained(&run_loop)));

  // Simulate tab deletion before search result JSON-ln loading.
  search_result_ad_service_->OnDidCloseTab(session_id);

  run_loop.Run();
}

TEST_F(SearchResultAdServiceTest, TryTriggerAdAfterTabDeleted) {
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

  SessionID session_id = SessionID::NewUnique();
  // Simulate navigation was finished.
  search_result_ad_service_->OnDidFinishNavigation(session_id);
  // Simulate tab deletion before search result JSON-ln loading.
  search_result_ad_service_->OnDidCloseTab(session_id);

  base::RunLoop run_loop;
  search_result_ad_service_->MaybeTriggerSearchResultAdViewedEvent(
      "creative_instance_id", session_id,
      base::BindOnce(
          [](base::RunLoop* run_loop, bool ad_was_triggered) {
            EXPECT_FALSE(ad_was_triggered);
            run_loop->Quit();
          },
          base::Unretained(&run_loop)));

  run_loop.Run();
}

}  // namespace
