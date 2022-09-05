/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_ads/search_result_ad/search_result_ad_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/mock_ads_service.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/common/search_result_ad_util.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_service.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/url_loader_interceptor.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_browser_tests --filter=SearchResultAdTest*

using testing::_;
using testing::Mock;
using testing::Return;

namespace {

constexpr char kAllowedDomain[] = "search.brave.com";
constexpr char kNotAllowedDomain[] = "brave.com";

bool CheckSampleSearchAdMetadata(
    const ads::mojom::SearchResultAdInfoPtr& search_result_ad,
    size_t ad_index) {
  const std::string index = base::StrCat({"-", base::NumberToString(ad_index)});
  if (search_result_ad->placement_id !=
      base::StrCat({"data-placement-id", index})) {
    return false;
  }

  EXPECT_EQ(search_result_ad->creative_instance_id,
            base::StrCat({"data-creative-instance-id", index}));
  EXPECT_EQ(search_result_ad->creative_set_id,
            base::StrCat({"data-creative-set-id", index}));
  EXPECT_EQ(search_result_ad->campaign_id,
            base::StrCat({"data-campaign-id", index}));
  EXPECT_EQ(search_result_ad->advertiser_id,
            base::StrCat({"data-advertiser-id", index}));
  EXPECT_EQ(search_result_ad->target_url,
            GURL(base::StrCat({"https://brave.com/data-landing-page", index})));
  EXPECT_EQ(search_result_ad->headline_text,
            base::StrCat({"data-headline-text", index}));
  EXPECT_EQ(search_result_ad->description,
            base::StrCat({"data-description", index}));
  EXPECT_DOUBLE_EQ(search_result_ad->value, 0.5 + ad_index);

  EXPECT_EQ(search_result_ad->conversion->type,
            base::StrCat({"data-conversion-type-value", index}));
  EXPECT_EQ(search_result_ad->conversion->url_pattern,
            base::StrCat({"data-conversion-url-pattern-value", index}));
  EXPECT_EQ(
      search_result_ad->conversion->advertiser_public_key,
      base::StrCat({"data-conversion-advertiser-public-key-value", index}));
  EXPECT_EQ(
      static_cast<size_t>(search_result_ad->conversion->observation_window),
      ad_index);

  return true;
}

brave_ads::SearchResultAdService* GetSearchResultAdService(Profile* profile) {
  return brave_ads::SearchResultAdServiceFactory::GetForProfile(profile);
}

class ScopedTestingAdsServiceSetter {
 public:
  explicit ScopedTestingAdsServiceSetter(Profile* profile,
                                         brave_ads::AdsService* ads_service)
      : profile_(profile) {
    previous_ads_service_ =
        GetSearchResultAdService(profile_)->SetAdsServiceForTesting(
            ads_service);
  }

  ~ScopedTestingAdsServiceSetter() {
    GetSearchResultAdService(profile_)->SetAdsServiceForTesting(
        previous_ads_service_.get());
  }

  ScopedTestingAdsServiceSetter(const ScopedTestingAdsServiceSetter&) = delete;
  ScopedTestingAdsServiceSetter& operator=(
      const ScopedTestingAdsServiceSetter&) = delete;

 private:
  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<brave_ads::AdsService> previous_ads_service_ = nullptr;
};

class SentViewedEventsWaiter final {
 public:
  explicit SentViewedEventsWaiter(
      std::vector<std::string> creative_instance_ids) {
    sent_viewed_creative_instance_ids_ = std::move(creative_instance_ids);

    url_loader_interceptor_ =
        std::make_unique<content::URLLoaderInterceptor>(base::BindRepeating(
            &SentViewedEventsWaiter::URLLoaderInterceptorCallback,
            base::Unretained(this)));
  }
  ~SentViewedEventsWaiter() = default;

  bool URLLoaderInterceptorCallback(
      content::URLLoaderInterceptor::RequestParams* params) {
    const std::string creative_instance_id =
        brave_ads::GetViewedSearchResultAdCreativeInstanceId(
            params->url_request);
    if (!creative_instance_id.empty()) {
      auto it = base::ranges::find(sent_viewed_creative_instance_ids_,
                                   creative_instance_id);
      EXPECT_TRUE(it != sent_viewed_creative_instance_ids_.end())
          << "Not expected creative instance id: " << creative_instance_id;
      if (it != sent_viewed_creative_instance_ids_.end()) {
        sent_viewed_creative_instance_ids_.erase(it);
      }
    }

    if (sent_viewed_creative_instance_ids_.empty()) {
      run_loop_.Quit();
    }
    return false;
  }

  void WaitForViewedEvents() { run_loop_.Run(); }

 private:
  base::RunLoop run_loop_;
  std::vector<std::string> sent_viewed_creative_instance_ids_;
  std::unique_ptr<content::URLLoaderInterceptor> url_loader_interceptor_;
};

}  // namespace

class SearchResultAdTest : public InProcessBrowserTest {
 public:
  SearchResultAdTest() {
    feature_list_.InitAndEnableFeature(
        brave_ads::features::kSupportBraveSearchResultAdConfirmationEvents);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_->Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* LoadTestDataUrl(base::StringPiece domain,
                                        const std::string& relative_path) {
    GURL url = https_server()->GetURL(domain, relative_path);

    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    EXPECT_TRUE(content::NavigateToURL(web_contents, url));
    EXPECT_EQ(url, web_contents->GetVisibleURL());

    return web_contents;
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  Profile* profile() { return browser()->profile(); }

 private:
  base::test::ScopedFeatureList feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, SampleSearchAdMetadata) {
  brave_ads::MockAdsService ads_service;
  ScopedTestingAdsServiceSetter scoped_setter(profile(), &ads_service);

  EXPECT_CALL(ads_service, IsEnabled()).WillRepeatedly(Return(true));
  auto run_loop1 = std::make_unique<base::RunLoop>();
  auto run_loop2 = std::make_unique<base::RunLoop>();
  EXPECT_CALL(ads_service, TriggerSearchResultAdEvent(_, _))
      .Times(2)
      .WillRepeatedly(
          [&run_loop1, &run_loop2](
              ads::mojom::SearchResultAdInfoPtr ad_mojom,
              const ads::mojom::SearchResultAdEventType event_type) {
            const bool is_search_result_ad_1 =
                CheckSampleSearchAdMetadata(ad_mojom, 1);
            const bool is_search_result_ad_2 =
                CheckSampleSearchAdMetadata(ad_mojom, 2);
            EXPECT_TRUE(is_search_result_ad_1 || is_search_result_ad_2);

            if (is_search_result_ad_1) {
              run_loop1->Quit();
            } else if (is_search_result_ad_2) {
              run_loop2->Quit();
            }
          });

  SentViewedEventsWaiter sent_viewed_events_waiter(
      {"data-creative-instance-id-1", "not-existant"});
  LoadTestDataUrl(kAllowedDomain, "/brave_ads/search_result_ad_sample.html");

  run_loop1->Run();
  run_loop2->Run();
  sent_viewed_events_waiter.WaitForViewedEvents();
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, AdsDisabled) {
  brave_ads::MockAdsService ads_service;
  ScopedTestingAdsServiceSetter scoped_setter(profile(), &ads_service);

  EXPECT_CALL(ads_service, IsEnabled()).WillRepeatedly(Return(false));
  EXPECT_CALL(ads_service, TriggerSearchResultAdEvent(_, _)).Times(0);

  base::RunLoop run_loop;
  GetSearchResultAdService(profile())
      ->SetMetadataRequestFinishedCallbackForTesting(base::BindOnce(
          [](base::OnceClosure run_loop_closure) {
            std::move(run_loop_closure).Run();
          },
          run_loop.QuitClosure()));

  SentViewedEventsWaiter sent_viewed_events_waiter(
      {"data-creative-instance-id-1", "data-creative-instance-id-1",
       "data-creative-instance-id-2", "not-existant"});
  LoadTestDataUrl(kAllowedDomain, "/brave_ads/search_result_ad_sample.html");

  run_loop.Run();
  sent_viewed_events_waiter.WaitForViewedEvents();
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, NotAllowedDomain) {
  brave_ads::MockAdsService ads_service;
  ScopedTestingAdsServiceSetter scoped_setter(profile(), &ads_service);

  EXPECT_CALL(ads_service, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service, TriggerSearchResultAdEvent(_, _)).Times(0);

  base::RunLoop run_loop;
  GetSearchResultAdService(profile())
      ->SetMetadataRequestFinishedCallbackForTesting(base::BindOnce(
          [](base::OnceClosure run_loop_closure) {
            std::move(run_loop_closure).Run();
          },
          run_loop.QuitClosure()));

  SentViewedEventsWaiter sent_viewed_events_waiter(
      {"data-creative-instance-id-1", "data-creative-instance-id-1",
       "data-creative-instance-id-2", "not-existant"});
  LoadTestDataUrl(kNotAllowedDomain, "/brave_ads/search_result_ad_sample.html");

  run_loop.Run();
  sent_viewed_events_waiter.WaitForViewedEvents();
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, NoSearchAdMetadata) {
  brave_ads::MockAdsService ads_service;
  ScopedTestingAdsServiceSetter scoped_setter(profile(), &ads_service);

  EXPECT_CALL(ads_service, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service, TriggerSearchResultAdEvent(_, _)).Times(0);

  base::RunLoop run_loop;
  GetSearchResultAdService(profile())
      ->SetMetadataRequestFinishedCallbackForTesting(base::BindOnce(
          [](base::OnceClosure run_loop_closure) {
            std::move(run_loop_closure).Run();
          },
          run_loop.QuitClosure()));

  LoadTestDataUrl(kAllowedDomain, "/simple.html");

  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, BrokenSearchAdMetadata) {
  brave_ads::MockAdsService ads_service;
  ScopedTestingAdsServiceSetter scoped_setter(profile(), &ads_service);

  EXPECT_CALL(ads_service, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service, TriggerSearchResultAdEvent(_, _)).Times(0);

  base::RunLoop run_loop;
  GetSearchResultAdService(profile())
      ->SetMetadataRequestFinishedCallbackForTesting(base::BindOnce(
          [](base::OnceClosure run_loop_closure) {
            std::move(run_loop_closure).Run();
          },
          run_loop.QuitClosure()));

  SentViewedEventsWaiter sent_viewed_events_waiter(
      {"data-creative-instance-id-1"});
  LoadTestDataUrl(kAllowedDomain, "/brave_ads/search_result_ad_broken.html");

  run_loop.Run();
  sent_viewed_events_waiter.WaitForViewedEvents();
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, IncognitoBrowser) {
  SentViewedEventsWaiter sent_viewed_events_waiter(
      {"data-creative-instance-id-1", "data-creative-instance-id-1",
       "data-creative-instance-id-2", "not-existant"});

  GURL url = https_server()->GetURL(kAllowedDomain,
                                    "/brave_ads/search_result_ad_sample.html");
  Browser* incognito_browser = OpenURLOffTheRecord(browser()->profile(), url);
  EXPECT_FALSE(GetSearchResultAdService(incognito_browser->profile()));

  content::WebContents* web_contents =
      incognito_browser->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(content::NavigateToURL(web_contents, url));
  EXPECT_EQ(url, web_contents->GetVisibleURL());

  sent_viewed_events_waiter.WaitForViewedEvents();
}
