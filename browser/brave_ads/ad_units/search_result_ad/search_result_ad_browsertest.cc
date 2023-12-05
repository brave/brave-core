/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_ads/ad_units/search_result_ad/search_result_ad_tab_helper.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_mock.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/request_handler_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_browser_tests --filter=SearchResultAdTest*

namespace brave_ads {

namespace {

using testing::_;
using testing::Mock;
using testing::Return;

constexpr char kAllowedDomain[] = "search.brave.com";
constexpr char kNotAllowedDomain[] = "brave.com";
constexpr char kClickRedirectPath[] = "/a/redirect";
constexpr char kTargetDomain[] = "example.com";
constexpr char kTargetPath[] = "/simple.html";
constexpr char kSearchResultUrlPath[] =
    "/brave_ads/search_result_ad_sample.html";

SearchResultAdTabHelper* GetSearchResultAdTabHelper(Browser* browser) {
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();
  return SearchResultAdTabHelper::FromWebContents(web_contents);
}

class ScopedTestingAdsServiceSetter {
 public:
  explicit ScopedTestingAdsServiceSetter(AdsService* ads_service) {
    SearchResultAdTabHelper::SetAdsServiceForTesting(ads_service);
  }

  ~ScopedTestingAdsServiceSetter() {
    SearchResultAdTabHelper::SetAdsServiceForTesting(nullptr);
  }

  ScopedTestingAdsServiceSetter(const ScopedTestingAdsServiceSetter&) = delete;
  ScopedTestingAdsServiceSetter& operator=(
      const ScopedTestingAdsServiceSetter&) = delete;
};

}  // namespace

class SearchResultAdTest : public InProcessBrowserTest {
 public:
  SearchResultAdTest() {
    scoped_feature_list_.InitAndEnableFeature(
        kShouldSupportSearchResultAdsFeature);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->RegisterRequestHandler(base::BindRepeating(
        &SearchResultAdTest::HandleRequest, base::Unretained(this)));

    brave::RegisterPathProvider();
    const base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
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

  GURL GetURL(std::string_view domain, const std::string& path) const {
    base::StringPairs replacements;
    replacements.emplace_back(std::make_pair(
        "REPLACE_WITH_HTTP_PORT",
        base::NumberToString(https_server_->host_port_pair().port())));

    std::string replaced_path =
        net::test_server::GetFilePathWithReplacements(path, replacements);
    return https_server_->GetURL(domain, replaced_path);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    const GURL url = request.GetURL();
    const std::string_view path = url.path_piece();

    if (!base::StartsWith(path, kClickRedirectPath)) {
      return nullptr;
    }

    const GURL target_url = https_server_->GetURL(kTargetDomain, kTargetPath);
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->AddCustomHeader("Access-Control-Allow-Origin", "*");
    http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
    http_response->AddCustomHeader("Location", target_url.spec());
    return http_response;
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  AdsServiceMock* ads_service() { return &ads_service_mock_; }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  AdsServiceMock ads_service_mock_;
};

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, UserHasNotJoinedBraveRewards) {
  browser()->profile()->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled,
                                               false);

  ScopedTestingAdsServiceSetter scoped_setter(ads_service());
  EXPECT_CALL(*ads_service(), TriggerSearchResultAdEvent).Times(0);

  GURL url = GetURL(kAllowedDomain, kSearchResultUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, web_contents->GetVisibleURL());
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, NotAllowedDomain) {
  ScopedTestingAdsServiceSetter scoped_setter(ads_service());

  browser()->profile()->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled,
                                               true);

  EXPECT_CALL(*ads_service(), TriggerSearchResultAdEvent).Times(0);

  GURL url = GetURL(kNotAllowedDomain, kSearchResultUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, web_contents->GetVisibleURL());
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, BrokenSearchAdMetadata) {
  ScopedTestingAdsServiceSetter scoped_setter(ads_service());

  browser()->profile()->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled,
                                               true);

  EXPECT_CALL(*ads_service(), TriggerSearchResultAdEvent).Times(0);

  GURL url = GetURL(kAllowedDomain, "/brave_ads/search_result_ad_broken.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, web_contents->GetVisibleURL());
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, IncognitoBrowser) {
  GURL url = GetURL(kAllowedDomain, kSearchResultUrlPath);
  Browser* incognito_browser = OpenURLOffTheRecord(browser()->profile(), url);
  EXPECT_FALSE(GetSearchResultAdTabHelper(incognito_browser));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(incognito_browser, url));
  content::WebContents* web_contents =
      incognito_browser->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, web_contents->GetVisibleURL());

  content::CreateAndLoadWebContentsObserver observer;
  EXPECT_TRUE(content::ExecJs(web_contents,
                              "document.getElementById('ad_link_2').click();"));
  observer.Wait();
}

class SampleSearchResultAdTest : public SearchResultAdTest {
 public:
  GURL GetSearchResultUrl() const {
    return GetURL(kAllowedDomain, kSearchResultUrlPath);
  }

  bool CheckSampleSearchAdMetadata(
      const mojom::SearchResultAdInfoPtr& search_result_ad,
      size_t ad_index) {
    EXPECT_TRUE(search_result_ad);

    const std::string index =
        base::StrCat({"-", base::NumberToString(ad_index)});
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
              GURL(base::StrCat({"https://foo.com/page", index})));
    EXPECT_EQ(search_result_ad->headline_text,
              base::StrCat({"data-headline-text", index}));
    EXPECT_EQ(search_result_ad->description,
              base::StrCat({"data-description", index}));
    EXPECT_DOUBLE_EQ(search_result_ad->value, 0.5 + ad_index);

    EXPECT_TRUE(search_result_ad->conversion);
    EXPECT_EQ(search_result_ad->conversion->url_pattern,
              base::StrCat({"data-conversion-url-pattern-value", index}));
    if (ad_index == 2) {
      EXPECT_FALSE(search_result_ad->conversion
                       ->verifiable_advertiser_public_key_base64);
    } else {
      EXPECT_EQ(
          search_result_ad->conversion->verifiable_advertiser_public_key_base64,
          base::StrCat({"data-conversion-advertiser-public-key-value", index}));
    }
    EXPECT_EQ(static_cast<size_t>(
                  search_result_ad->conversion->observation_window.InDays()),
              ad_index);

    return true;
  }

  content::WebContents* LoadAndCheckSampleSearchResultAdWebPage(
      const GURL& url) {
    auto run_loop1 = std::make_unique<base::RunLoop>();
    auto run_loop2 = std::make_unique<base::RunLoop>();
    EXPECT_CALL(*ads_service(),
                TriggerSearchResultAdEvent(
                    _, mojom::SearchResultAdEventType::kViewed, _))
        .Times(2)
        .WillRepeatedly([this, &run_loop1, &run_loop2](
                            mojom::SearchResultAdInfoPtr ad_mojom,
                            const mojom::SearchResultAdEventType event_type,
                            TriggerAdEventCallback callback) {
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

    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    EXPECT_EQ(url, web_contents->GetVisibleURL());

    run_loop1->Run();
    run_loop2->Run();

    return web_contents;
  }
};

IN_PROC_BROWSER_TEST_F(SampleSearchResultAdTest,
                       SearchResultAdOpenedInSameTab) {
  ScopedTestingAdsServiceSetter scoped_setter(ads_service());

  browser()->profile()->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled,
                                               true);

  content::WebContents* web_contents =
      LoadAndCheckSampleSearchResultAdWebPage(GetSearchResultUrl());

  base::RunLoop run_loop;
  EXPECT_CALL(*ads_service(), TriggerSearchResultAdEvent)
      .WillOnce(
          [this, &run_loop](mojom::SearchResultAdInfoPtr ad_mojom,
                            const mojom::SearchResultAdEventType event_type,
                            TriggerAdEventCallback callback) {
            EXPECT_EQ(event_type, mojom::SearchResultAdEventType::kClicked);
            CheckSampleSearchAdMetadata(ad_mojom, 1);
            run_loop.Quit();
          });

  EXPECT_TRUE(content::ExecJs(web_contents,
                              "document.getElementById('ad_link_1').click();"));
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(SampleSearchResultAdTest, SearchResultAdOpenedInNewTab) {
  ScopedTestingAdsServiceSetter scoped_setter(ads_service());

  browser()->profile()->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled,
                                               true);

  content::WebContents* web_contents =
      LoadAndCheckSampleSearchResultAdWebPage(GetSearchResultUrl());

  base::RunLoop run_loop;
  EXPECT_CALL(*ads_service(), TriggerSearchResultAdEvent)
      .WillOnce(
          [this, &run_loop](mojom::SearchResultAdInfoPtr ad_mojom,
                            const mojom::SearchResultAdEventType event_type,
                            TriggerAdEventCallback callback) {
            EXPECT_EQ(event_type, mojom::SearchResultAdEventType::kClicked);
            CheckSampleSearchAdMetadata(ad_mojom, 2);
            run_loop.Quit();
          });

  EXPECT_TRUE(content::ExecJs(web_contents,
                              "document.getElementById('ad_link_2').click();"));
  run_loop.Run();
}

}  // namespace brave_ads
