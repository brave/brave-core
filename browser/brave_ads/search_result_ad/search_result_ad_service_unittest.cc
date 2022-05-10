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
  SessionID session_id = SessionID::NewUnique();
  NavigateAndCommit(GURL(kAllowedDomain));
  EXPECT_CALL(ads_service_, IsEnabled()).WillOnce(Return(false));

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
  SessionID session_id = SessionID::NewUnique();
  NavigateAndCommit(GURL(kNotAllowedDomain));
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

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
  SessionID session_id = SessionID::NewUnique();
  NavigateAndCommit(GURL(kNotAllowedDomain));
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

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
  SessionID session_id = SessionID::NewUnique();
  NavigateAndCommit(GURL(kAllowedDomain));
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

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
  SessionID session_id = SessionID::NewUnique();
  NavigateAndCommit(GURL(kAllowedDomain));
  EXPECT_CALL(ads_service_, IsEnabled()).WillRepeatedly(Return(true));

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

}  // namespace
