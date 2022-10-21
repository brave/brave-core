/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_ads/search_result_ad/search_result_ad_tab_helper.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/mock_ads_service.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/core/browser/search_result_ad/search_result_ad_util.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/request_handler_util.h"
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

brave_ads::SearchResultAdTabHelper* GetSearchResultAdTabHelper(
    Browser* browser) {
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();
  return brave_ads::SearchResultAdTabHelper::FromWebContents(web_contents);
}

class ScopedTestingAdsServiceSetter {
 public:
  explicit ScopedTestingAdsServiceSetter(brave_ads::AdsService* ads_service) {
    brave_ads::SearchResultAdTabHelper::SetAdsServiceForTesting(ads_service);
  }

  ~ScopedTestingAdsServiceSetter() {
    brave_ads::SearchResultAdTabHelper::SetAdsServiceForTesting(nullptr);
  }

  ScopedTestingAdsServiceSetter(const ScopedTestingAdsServiceSetter&) = delete;
  ScopedTestingAdsServiceSetter& operator=(
      const ScopedTestingAdsServiceSetter&) = delete;
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

  GURL CreateTargetURL() {
    const std::string target_url =
        "https://brave.com:" + base::NumberToString(https_server_->port()) +
        "/simple.html";
    return GURL(target_url);
  }

  bool CheckSampleSearchAdMetadata(
      const ads::mojom::SearchResultAdInfoPtr& search_result_ad,
      size_t ad_index) {
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
    EXPECT_EQ(search_result_ad->target_url, CreateTargetURL());
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

  content::WebContents* LoadAndCheckSampleSearchResultAdWebPage(
      brave_ads::MockAdsService* ads_service) {
    auto run_loop1 = std::make_unique<base::RunLoop>();
    auto run_loop2 = std::make_unique<base::RunLoop>();
    EXPECT_CALL(*ads_service, TriggerSearchResultAdEvent(_, _))
        .Times(2)
        .WillRepeatedly(
            [this, &run_loop1, &run_loop2](
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

    GURL url =
        CreateURL(kAllowedDomain, "/brave_ads/search_result_ad_sample.html");
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    EXPECT_EQ(url, web_contents->GetVisibleURL());

    run_loop1->Run();
    run_loop2->Run();

    return web_contents;
  }

  GURL CreateURL(base::StringPiece domain, const std::string& path) {
    base::StringPairs replacements;
    replacements.push_back(std::make_pair(
        "REPLACE_WITH_HTTP_PORT",
        base::NumberToString(https_server_->host_port_pair().port())));

    std::string replaced_path =
        net::test_server::GetFilePathWithReplacements(path, replacements);
    return https_server_->GetURL(domain, replaced_path);
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

 private:
  base::test::ScopedFeatureList feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, SampleSearchResultAd) {
  brave_ads::MockAdsService ads_service;
  ScopedTestingAdsServiceSetter scoped_setter(&ads_service);
  EXPECT_CALL(ads_service, IsEnabled()).WillRepeatedly(Return(true));

  content::WebContents* web_contents =
      LoadAndCheckSampleSearchResultAdWebPage(&ads_service);

  base::RunLoop run_loop;
  EXPECT_CALL(ads_service, TriggerSearchResultAdEvent(_, _))
      .WillOnce([this, &run_loop](
                    ads::mojom::SearchResultAdInfoPtr ad_mojom,
                    const ads::mojom::SearchResultAdEventType event_type) {
        EXPECT_EQ(event_type, ads::mojom::SearchResultAdEventType::kClicked);
        CheckSampleSearchAdMetadata(ad_mojom, 1);
        run_loop.Quit();
      });

  content::TestNavigationObserver observer(web_contents);
  EXPECT_TRUE(content::ExecJs(web_contents,
                              "document.getElementById('ad_link_1').click();"));
  observer.Wait();

  EXPECT_EQ(CreateTargetURL(), web_contents->GetVisibleURL());
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, SearchResultAdOpenedInNewTab) {
  brave_ads::MockAdsService ads_service;
  ScopedTestingAdsServiceSetter scoped_setter(&ads_service);
  EXPECT_CALL(ads_service, IsEnabled()).WillRepeatedly(Return(true));

  content::WebContents* web_contents =
      LoadAndCheckSampleSearchResultAdWebPage(&ads_service);

  EXPECT_CALL(ads_service, IsEnabled()).WillRepeatedly(Return(true));
  base::RunLoop run_loop;
  EXPECT_CALL(ads_service, TriggerSearchResultAdEvent(_, _))
      .WillOnce([this, &run_loop](
                    ads::mojom::SearchResultAdInfoPtr ad_mojom,
                    const ads::mojom::SearchResultAdEventType event_type) {
        EXPECT_EQ(event_type, ads::mojom::SearchResultAdEventType::kClicked);
        CheckSampleSearchAdMetadata(ad_mojom, 2);
        run_loop.Quit();
      });

  content::CreateAndLoadWebContentsObserver observer;
  EXPECT_TRUE(content::ExecJs(web_contents,
                              "document.getElementById('ad_link_2').click();"));
  content::WebContents* new_web_contents = observer.Wait();

  EXPECT_EQ(CreateTargetURL(), new_web_contents->GetVisibleURL());
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, AdsDisabled) {
  brave_ads::MockAdsService ads_service;
  ScopedTestingAdsServiceSetter scoped_setter(&ads_service);

  EXPECT_CALL(ads_service, IsEnabled()).WillRepeatedly(Return(false));
  EXPECT_CALL(ads_service, TriggerSearchResultAdEvent(_, _)).Times(0);

  GURL url =
      CreateURL(kAllowedDomain, "/brave_ads/search_result_ad_sample.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, web_contents->GetVisibleURL());
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, NotAllowedDomain) {
  brave_ads::MockAdsService ads_service;
  ScopedTestingAdsServiceSetter scoped_setter(&ads_service);

  EXPECT_CALL(ads_service, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service, TriggerSearchResultAdEvent(_, _)).Times(0);

  GURL url =
      CreateURL(kNotAllowedDomain, "/brave_ads/search_result_ad_sample.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, web_contents->GetVisibleURL());
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, BrokenSearchAdMetadata) {
  brave_ads::MockAdsService ads_service;
  ScopedTestingAdsServiceSetter scoped_setter(&ads_service);

  EXPECT_CALL(ads_service, IsEnabled()).WillRepeatedly(Return(true));
  EXPECT_CALL(ads_service, TriggerSearchResultAdEvent(_, _)).Times(0);

  GURL url =
      CreateURL(kAllowedDomain, "/brave_ads/search_result_ad_broken.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, web_contents->GetVisibleURL());
}

IN_PROC_BROWSER_TEST_F(SearchResultAdTest, IncognitoBrowser) {
  GURL url =
      CreateURL(kAllowedDomain, "/brave_ads/search_result_ad_sample.html");
  Browser* incognito_browser = OpenURLOffTheRecord(browser()->profile(), url);
  EXPECT_FALSE(GetSearchResultAdTabHelper(incognito_browser));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(incognito_browser, url));
  content::WebContents* web_contents =
      incognito_browser->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, web_contents->GetVisibleURL());

  content::CreateAndLoadWebContentsObserver observer;
  EXPECT_TRUE(content::ExecJs(web_contents,
                              "document.getElementById('ad_link_2').click();"));
  content::WebContents* new_web_contents = observer.Wait();

  EXPECT_EQ(
      "https://search.anonymous.ads.brave.com/v3/click?"
      "creativeInstanceId=data-creative-instance-id-2",
      new_web_contents->GetVisibleURL());
}
