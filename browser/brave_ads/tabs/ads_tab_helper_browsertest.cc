/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tabs/ads_tab_helper.h"

#include "base/path_service.h"
#include "base/test/gmock_callback_support.h"
#include "brave/components/brave_ads/browser/ads_service_mock.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_browser_tests --filter=AdsTabHelperTest*

namespace brave_ads {

namespace {

using base::test::RunClosure;
using testing::_;
using testing::IsEmpty;
using testing::Not;

constexpr char kUrlDomain[] = "example.com";
constexpr char kUrlPath[] = "/brave_ads/basic_page.html";
constexpr char kSinglePageApplicationUrlPath[] =
    "/brave_ads/single_page_application.html";
constexpr char k500ErrorPagePath[] = "/500_error_page.html";
constexpr char k404ErrorPagePath[] = "/404_error_page.html";

AdsTabHelper* GetActiveAdsTabHelper(Browser* browser) {
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();
  return AdsTabHelper::FromWebContents(web_contents);
}

}  // namespace

class AdsTabHelperTest : public CertVerifierBrowserTest {
 public:
  void SetUpOnMainThread() override {
    CertVerifierBrowserTest::SetUpOnMainThread();
    mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    const base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.RegisterRequestHandler(base::BindRepeating(
        &AdsTabHelperTest::HandleRequest, base::Unretained(this)));
    ASSERT_TRUE(https_server_.Start());

    GetActiveAdsTabHelper(browser())->SetAdsServiceForTesting(&ads_service());
  }

  PrefService* GetPrefs() { return browser()->profile()->GetPrefs(); }

  net::EmbeddedTestServer& https_server() { return https_server_; }

  AdsServiceMock& ads_service() { return ads_service_mock_; }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    if (base::Contains(request.relative_url, k500ErrorPagePath)) {
      // Return a 500 error without any content, causing the error page to be
      // displayed.
      auto response = std::make_unique<net::test_server::BasicHttpResponse>();
      response->set_code(net::HTTP_INTERNAL_SERVER_ERROR);
      return response;
    }

    if (base::Contains(request.relative_url, k404ErrorPagePath)) {
      // Return a 404 error with HTML content.
      auto response = std::make_unique<net::test_server::BasicHttpResponse>();
      response->set_code(net::HTTP_NOT_FOUND);
      response->set_content_type("text/html");
      response->set_content("<html><body>Not Found</body></html>");
      return response;
    }

    return nullptr;
  }

 private:
  net::EmbeddedTestServer https_server_{
      net::test_server::EmbeddedTestServer::TYPE_HTTPS};
  AdsServiceMock ads_service_mock_{nullptr};
};

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest, UserHasNotJoinedBraveRewards) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);

  EXPECT_CALL(ads_service(), NotifyTabTextContentDidChange).Times(0);

  base::RunLoop html_content_run_loop;
  EXPECT_CALL(ads_service(),
              NotifyTabHtmlContentDidChange(_, _, /*html=*/IsEmpty()))
      .WillOnce(RunClosure(html_content_run_loop.QuitClosure()));

  const GURL url = https_server().GetURL(kUrlDomain, kUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  html_content_run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest,
                       UserHasJoinedBraveRewardsAndOptedInToNotificationAds) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, true);

  base::RunLoop text_content_run_loop;
  EXPECT_CALL(ads_service(),
              NotifyTabTextContentDidChange(_, _, /*text=*/Not(IsEmpty())))
      .WillOnce(RunClosure(text_content_run_loop.QuitClosure()));

  base::RunLoop html_content_run_loop;
  EXPECT_CALL(ads_service(),
              NotifyTabHtmlContentDidChange(_, _, /*html=*/Not(IsEmpty())))
      .WillOnce(RunClosure(html_content_run_loop.QuitClosure()));

  const GURL url = https_server().GetURL(kUrlDomain, kUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  text_content_run_loop.Run();
  html_content_run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest,
                       UserHasJoinedBraveRewardsAndOptedOutNotificationAds) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, false);

  EXPECT_CALL(ads_service(), NotifyTabTextContentDidChange).Times(0);

  base::RunLoop html_content_run_loop;
  EXPECT_CALL(ads_service(),
              NotifyTabHtmlContentDidChange(_, _, /*html=*/Not(IsEmpty())))
      .WillOnce(RunClosure(html_content_run_loop.QuitClosure()));

  const GURL url = https_server().GetURL(kUrlDomain, kUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  html_content_run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest, LoadSinglePageApplication) {
  GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
  GetPrefs()->SetBoolean(prefs::kOptedInToNotificationAds, true);

  base::RunLoop text_content_run_loop;
  EXPECT_CALL(ads_service(),
              NotifyTabTextContentDidChange(_, _, /*text=*/Not(IsEmpty())))
      .WillOnce(RunClosure(text_content_run_loop.QuitClosure()));

  base::RunLoop html_content_run_loop;
  EXPECT_CALL(ads_service(),
              NotifyTabHtmlContentDidChange(_, _, /*html=*/Not(IsEmpty())))
      .WillOnce(RunClosure(html_content_run_loop.QuitClosure()));

  const GURL url =
      https_server().GetURL(kUrlDomain, kSinglePageApplicationUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  text_content_run_loop.Run();
  html_content_run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest, NotifyTabDidChange) {
  base::RunLoop tab_did_change_run_loop;
  EXPECT_CALL(
      ads_service(),
      NotifyTabDidChange(/*tab_id=*/_, /*redirect_chain=*/_,
                         /*is_new_navigation=*/true, /*is_restoring=*/false,
                         /*is_error_page_=*/false, /*is_visible=*/_))
      .WillRepeatedly(RunClosure(tab_did_change_run_loop.QuitClosure()));

  const GURL url = https_server().GetURL(kUrlDomain, kUrlPath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  tab_did_change_run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest,
                       NotifyTabDidChangeForServerErrorResponsePage) {
  // Mock unexpected NotifyTabDidChange calls to NotifyTabDidChange on browser
  // test startup.
  EXPECT_CALL(ads_service(),
              NotifyTabDidChange(/*tab_id=*/_, /*redirect_chain=*/_,
                                 /*is_new_navigation=*/_, /*is_restoring=*/_,
                                 /*is_error_page_=*/_, /*is_visible=*/_))
      .Times(testing::AnyNumber());

  base::RunLoop tab_did_change_run_loop;
  EXPECT_CALL(
      ads_service(),
      NotifyTabDidChange(/*tab_id=*/_, /*redirect_chain=*/_,
                         /*is_new_navigation=*/true, /*is_restoring=*/false,
                         /*is_error_page_=*/true, /*is_visible=*/_))
      .WillRepeatedly(RunClosure(tab_did_change_run_loop.QuitClosure()));

  const GURL url = https_server().GetURL(kUrlDomain, k500ErrorPagePath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  tab_did_change_run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest,
                       NotifyTabDidChangeForClientErrorResponsePage) {
  // Mock unexpected NotifyTabDidChange calls to NotifyTabDidChange on browser
  // test startup.
  EXPECT_CALL(ads_service(),
              NotifyTabDidChange(/*tab_id=*/_, /*redirect_chain=*/_,
                                 /*is_new_navigation=*/_, /*is_restoring=*/_,
                                 /*is_error_page_=*/_, /*is_visible=*/_))
      .Times(testing::AnyNumber());

  base::RunLoop tab_did_change_run_loop;
  EXPECT_CALL(
      ads_service(),
      NotifyTabDidChange(/*tab_id=*/_, /*redirect_chain=*/_,
                         /*is_new_navigation=*/true, /*is_restoring=*/false,
                         /*is_error_page_=*/true, /*is_visible=*/_))
      .WillRepeatedly(RunClosure(tab_did_change_run_loop.QuitClosure()));

  const GURL url = https_server().GetURL(kUrlDomain, k404ErrorPagePath);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  tab_did_change_run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest, IncognitoBrowser) {
  const GURL url = https_server().GetURL(kUrlDomain, kUrlPath);
  Browser* incognito_browser = OpenURLOffTheRecord(browser()->profile(), url);
  AdsTabHelper* incognito_ads_tab_helper =
      GetActiveAdsTabHelper(incognito_browser);
  EXPECT_FALSE(incognito_ads_tab_helper->ads_service());
}

}  // namespace brave_ads
