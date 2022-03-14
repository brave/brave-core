/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/pref_names.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=SendSearchAdConfirmationApiTest*

namespace {

constexpr char kAllowedDomain[] = "search.brave.com";

constexpr char kNotAllowedDomain[] = "brave.com";

constexpr char kBraveSendSearchAdConfirmationExists[] =
    "!!(window.chrome && window.chrome.braveSendSearchAdConfirmation)";

constexpr char kSendSearchAdConfirmationViewed[] = R"(
    window.chrome.braveSendSearchAdConfirmation(
        'viewed',
        {
          'uuid': '21c2c195-91a4-4fce-8346-2a85f4478e26'
        }
    ).then(
        enabled => enabled
    )
)";

constexpr char kSendSearchAdConfirmationClicked[] = R"(
    window.chrome.braveSendSearchAdConfirmation(
        'clicked',
        {
          'uuid': '21c2c195-91a4-4fce-8346-2a85f4478e26'
        }
    ).then(
        enabled => enabled
    )
)";

constexpr char kGetSendSearchAdConfirmationPromiseRejectReason[] = R"(
    window.chrome.braveSendSearchAdConfirmation(
        'clicked',
        {
          'uuid': '21c2c195-91a4-4fce-8346-2a85f4478e26'
        }
    ).then(
        undefined,
        reason => reason
    )
)";

constexpr char kSendSearchAdConfirmationWithWrongConfirmationType[] = R"(
    window.chrome.braveSendSearchAdConfirmation(
        'sent',
        {
          'uuid': '21c2c195-91a4-4fce-8346-2a85f4478e26'
        }
    ).then(
        undefined,
        reason => reason
    )
)";

constexpr char kSendSearchAdConfirmationWithWrongAdArguments[] = R"(
    window.chrome.braveSendSearchAdConfirmation(
        'viewed',
        ''
    ).then(
        undefined,
        reason => reason
    )
)";

constexpr char kUserGestureRejectReason[] =
    "braveSendSearchAdConfirmation: Clicked confirmation can only be initiated "
    "by a user gesture.";

constexpr char kWrongConfirmRejectReason[] =
    "braveSendSearchAdConfirmation: Wrong confirmation type.";

constexpr char kWrongAdAttributesRejectReason[] =
    "braveSendSearchAdConfirmation: At attributes is not an object.";

}  // namespace

class SendSearchAdConfirmationApiTestBase : public InProcessBrowserTest {
 public:
  SendSearchAdConfirmationApiTestBase() {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));

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

  PrefService* GetPrefs() { return browser()->profile()->GetPrefs(); }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

class SendSearchAdConfirmationApiTestEnabled
    : public SendSearchAdConfirmationApiTestBase {
 public:
  SendSearchAdConfirmationApiTestEnabled() {
    feature_list_.InitAndEnableFeature(
        brave_ads::features::kSearchAdConfirmationApi);
  }
};

IN_PROC_BROWSER_TEST_F(SendSearchAdConfirmationApiTestEnabled,
                       BraveAdsEnabled) {
  brave_ads::AdsService* ads_service =
      brave_ads::AdsServiceFactory::GetForProfile(browser()->profile());
  ads_service->SetEnabled(true);

  GURL url = https_server()->GetURL(kAllowedDomain, "/simple.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, contents->GetVisibleURL());
  EXPECT_EQ(true,
            content::EvalJs(contents, kBraveSendSearchAdConfirmationExists));

  EXPECT_EQ(true, content::EvalJs(contents, kSendSearchAdConfirmationViewed));
  EXPECT_EQ(true, content::EvalJs(contents, kSendSearchAdConfirmationClicked));
}

IN_PROC_BROWSER_TEST_F(SendSearchAdConfirmationApiTestEnabled,
                       BraveAdsDisabled) {
  GURL url = https_server()->GetURL(kAllowedDomain, "/simple.html");
  GetPrefs()->SetBoolean(ads::prefs::kEnabled, false);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, contents->GetVisibleURL());
  EXPECT_EQ(true,
            content::EvalJs(contents, kBraveSendSearchAdConfirmationExists));

  EXPECT_EQ(false, content::EvalJs(contents, kSendSearchAdConfirmationViewed));
  EXPECT_EQ(false, content::EvalJs(contents, kSendSearchAdConfirmationClicked));
}

IN_PROC_BROWSER_TEST_F(SendSearchAdConfirmationApiTestEnabled,
                       ApiForIncognitoBrowser) {
  GURL url = https_server()->GetURL(kAllowedDomain, "/simple.html");
  GetPrefs()->SetBoolean(ads::prefs::kEnabled, true);

  Browser* incognito_browser = OpenURLOffTheRecord(browser()->profile(), url);

  content::WebContents* contents =
      incognito_browser->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, contents->GetVisibleURL());
  EXPECT_EQ(true,
            content::EvalJs(contents, kBraveSendSearchAdConfirmationExists));

  EXPECT_EQ(false, content::EvalJs(contents, kSendSearchAdConfirmationViewed));
  EXPECT_EQ(false, content::EvalJs(contents, kSendSearchAdConfirmationClicked));
}

IN_PROC_BROWSER_TEST_F(SendSearchAdConfirmationApiTestEnabled,
                       RunApiForWithoutUserGesture) {
  GURL url = https_server()->GetURL(kAllowedDomain, "/simple.html");
  GetPrefs()->SetBoolean(ads::prefs::kEnabled, true);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, contents->GetVisibleURL());
  EXPECT_EQ(true,
            content::EvalJs(contents, kBraveSendSearchAdConfirmationExists,
                            content::EXECUTE_SCRIPT_NO_USER_GESTURE));

  EXPECT_EQ(
      kUserGestureRejectReason,
      content::EvalJs(contents, kGetSendSearchAdConfirmationPromiseRejectReason,
                      content::EXECUTE_SCRIPT_NO_USER_GESTURE));
  EXPECT_EQ(true, content::EvalJs(contents, kSendSearchAdConfirmationViewed,
                                  content::EXECUTE_SCRIPT_NO_USER_GESTURE));
}

IN_PROC_BROWSER_TEST_F(SendSearchAdConfirmationApiTestEnabled,
                       RunApiWithWrongArguments) {
  GURL url = https_server()->GetURL(kAllowedDomain, "/simple.html");
  GetPrefs()->SetBoolean(ads::prefs::kEnabled, true);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, contents->GetVisibleURL());
  EXPECT_EQ(true,
            content::EvalJs(contents, kBraveSendSearchAdConfirmationExists));

  EXPECT_EQ(kWrongConfirmRejectReason,
            content::EvalJs(
                contents, kSendSearchAdConfirmationWithWrongConfirmationType));

  EXPECT_EQ(
      kWrongAdAttributesRejectReason,
      content::EvalJs(contents, kSendSearchAdConfirmationWithWrongAdArguments));
}

IN_PROC_BROWSER_TEST_F(SendSearchAdConfirmationApiTestEnabled,
                       ApiNotAvailableForUnknownHost) {
  GURL url = https_server()->GetURL(kNotAllowedDomain, "/simple.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, contents->GetVisibleURL());
  EXPECT_EQ(false,
            content::EvalJs(contents, kBraveSendSearchAdConfirmationExists));
}

class SendSearchAdConfirmationApiTestDisabled
    : public SendSearchAdConfirmationApiTestBase {
 public:
  SendSearchAdConfirmationApiTestDisabled() {
    feature_list_.InitAndDisableFeature(
        brave_ads::features::kSearchAdConfirmationApi);
  }
};

IN_PROC_BROWSER_TEST_F(SendSearchAdConfirmationApiTestDisabled,
                       ApiNotAvailableWhenFeatureOff) {
  GURL url = https_server()->GetURL(kAllowedDomain, "/simple.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, contents->GetVisibleURL());
  EXPECT_EQ(false,
            content::EvalJs(contents, kBraveSendSearchAdConfirmationExists));
}
