/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tabs/ads_tab_helper.h"

#include <memory>

#include "base/path_service.h"
#include "brave/components/brave_ads/browser/ads_service_mock.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_browser_tests --filter=AdsTabHelperTest*

namespace brave_ads {

namespace {

constexpr char kUrlDomain[] = "example.com";
constexpr char kUrlPath[] = "/brave_ads/not_empty.html";
constexpr char kSinglePageApplicationUrlPath[] =
    "/brave_ads/single_page_application.html";

AdsTabHelper* GetActiveAdsTabHelper(Browser* browser) {
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();
  return AdsTabHelper::FromWebContents(web_contents);
}

}  // namespace

class AdsTabHelperTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);

    const base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_->Start());

    GetActiveAdsTabHelper(browser())->SetAdsServiceForTesting(ads_service());
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

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  AdsServiceMock* ads_service() { return &ads_service_mock_; }

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  AdsServiceMock ads_service_mock_;
};

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest, UserHasNotJoinedBraveRewards) {
  browser()->profile()->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled,
                                               false);

  EXPECT_CALL(*ads_service(), NotifyTabTextContentDidChange).Times(0);

  base::RunLoop html_content_run_loop;
  EXPECT_CALL(*ads_service(), NotifyTabHtmlContentDidChange)
      .WillOnce([&html_content_run_loop](
                    int32_t tab_id, const std::vector<GURL>& redirect_chain,
                    const std::string& html) {
        EXPECT_TRUE(html.empty());
        html_content_run_loop.Quit();
      });

  GURL url = https_server()->GetURL(kUrlDomain, kUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  html_content_run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest,
                       UserHasJoinedBraveRewardsAndOptedInToNotificationAds) {
  browser()->profile()->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled,
                                               true);
  browser()->profile()->GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds,
                                               true);

  base::RunLoop text_content_run_loop;
  EXPECT_CALL(*ads_service(), NotifyTabTextContentDidChange)
      .WillOnce([&text_content_run_loop](
                    int32_t tab_id, const std::vector<GURL>& redirect_chain,
                    const std::string& text) {
        EXPECT_FALSE(text.empty());
        text_content_run_loop.Quit();
      });

  base::RunLoop html_content_run_loop;
  EXPECT_CALL(*ads_service(), NotifyTabHtmlContentDidChange)
      .WillOnce([&html_content_run_loop](
                    int32_t tab_id, const std::vector<GURL>& redirect_chain,
                    const std::string& html) {
        EXPECT_FALSE(html.empty());
        html_content_run_loop.Quit();
      });

  GURL url = https_server()->GetURL(kUrlDomain, kUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  text_content_run_loop.Run();
  html_content_run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest,
                       UserHasJoinedBraveRewardsAndOptedOutNotificationAds) {
  browser()->profile()->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled,
                                               true);
  browser()->profile()->GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds,
                                               false);

  EXPECT_CALL(*ads_service(), NotifyTabTextContentDidChange).Times(0);

  base::RunLoop html_content_run_loop;
  EXPECT_CALL(*ads_service(), NotifyTabHtmlContentDidChange)
      .WillOnce([&html_content_run_loop](
                    int32_t tab_id, const std::vector<GURL>& redirect_chain,
                    const std::string& html) {
        EXPECT_FALSE(html.empty());
        html_content_run_loop.Quit();
      });

  GURL url = https_server()->GetURL(kUrlDomain, kUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  html_content_run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest, LoadSinglePageApplication) {
  browser()->profile()->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled,
                                               true);
  browser()->profile()->GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds,
                                               true);

  base::RunLoop text_content_run_loop;
  EXPECT_CALL(*ads_service(), NotifyTabTextContentDidChange)
      .WillOnce([&text_content_run_loop](
                    int32_t tab_id, const std::vector<GURL>& redirect_chain,
                    const std::string& text) {
        EXPECT_FALSE(text.empty());
        text_content_run_loop.Quit();
      });

  base::RunLoop html_content_run_loop;
  EXPECT_CALL(*ads_service(), NotifyTabHtmlContentDidChange)
      .WillOnce([&html_content_run_loop](
                    int32_t tab_id, const std::vector<GURL>& redirect_chain,
                    const std::string& html) {
        EXPECT_FALSE(html.empty());
        html_content_run_loop.Quit();
      });

  GURL url = https_server()->GetURL(kUrlDomain, kSinglePageApplicationUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  text_content_run_loop.Run();
  html_content_run_loop.Run();
}

}  // namespace brave_ads
