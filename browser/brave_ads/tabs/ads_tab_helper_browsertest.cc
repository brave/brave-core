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
    ASSERT_TRUE(https_server_.Start());

    GetActiveAdsTabHelper(browser())->SetAdsServiceForTesting(&ads_service());
  }

  PrefService* GetPrefs() { return browser()->profile()->GetPrefs(); }

  net::EmbeddedTestServer& https_server() { return https_server_; }

  AdsServiceMock& ads_service() { return ads_service_mock_; }

 private:
  net::EmbeddedTestServer https_server_{
      net::test_server::EmbeddedTestServer::TYPE_HTTPS};
  AdsServiceMock ads_service_mock_;
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

IN_PROC_BROWSER_TEST_F(AdsTabHelperTest, IncognitoBrowser) {
  const GURL url = https_server().GetURL(kUrlDomain, kUrlPath);
  Browser* incognito_browser = OpenURLOffTheRecord(browser()->profile(), url);
  AdsTabHelper* incognito_ads_tab_helper =
      GetActiveAdsTabHelper(incognito_browser);
  EXPECT_FALSE(incognito_ads_tab_helper->ads_service());
}

}  // namespace brave_ads
