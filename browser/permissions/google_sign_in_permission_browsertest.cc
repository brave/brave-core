// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/path_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/cookie_settings_base.h"
#include "components/permissions/features.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/test/mock_permission_prompt_factory.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_paths.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "url/gurl.h"

using net::test_server::EmbeddedTestServer;

namespace {

const char kAccountsGoogleUrl[] = "https://accounts.google.com";
const char kEmbeddingPageUrl[] = "/google_sign_in_link.html";
const char kTestDomain[] = "a.com";
const char kThirdPartyTestDomain[] = "b.com";

// Used to identify the buttons on the test page
const char kAuthButtonHtmlId[] = "auth-button";
const char kReloadButtonHtmlId[] = "reload-button";

}  // namespace

class GoogleSignInBrowserTest : public InProcessBrowserTest {
 public:
  GoogleSignInBrowserTest() {
    feature_list_.InitAndEnableFeature(
        permissions::features::kBraveGoogleSignInPermission);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);

    https_server_->ServeFilesFromDirectory(test_data_dir);
    https_server_->AddDefaultHandlers(GetChromeTestDataDir());
    content::SetupCrossSiteRedirector(https_server_.get());
    ASSERT_TRUE(https_server_->Start());

    permissions::PermissionRequestManager* manager =
        GetPermissionRequestManager();
    prompt_factory_ =
        std::make_unique<permissions::MockPermissionPromptFactory>(manager);

    top_level_page_url_ = https_server_->GetURL(kTestDomain, "/");
    https_top_level_page_url_ = https_server_->GetURL(kTestDomain, "/");
    third_party_url_ = https_server_->GetURL(kThirdPartyTestDomain, "/");
    third_party_cookie_url_ = https_server_->GetURL(
        kThirdPartyTestDomain, "/set-cookie?test=true;SameSite=None;Secure");
    cookie_iframe_url_ =
        https_server_->GetURL(kTestDomain, "/cookie_iframe.html");
    embedding_url_ = https_server_->GetURL(kTestDomain, kEmbeddingPageUrl);
    https_cookie_iframe_url_ =
        https_server_->GetURL(kTestDomain, "/cookie_iframe.html");
    google_oauth_cookie_url_ = https_server_->GetURL(
        "accounts.google.com", "/set-cookie?oauth=true;SameSite=None;Secure");
  }

  content_settings::CookieSettings* cookie_settings() {
    return CookieSettingsFactory::GetForProfile(browser()->profile()).get();
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  permissions::PermissionRequestManager* GetPermissionRequestManager() {
    return permissions::PermissionRequestManager::FromWebContents(
        browser()->tab_strip_model()->GetActiveWebContents());
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

  void TearDownOnMainThread() override { prompt_factory_.reset(); }

  permissions::MockPermissionPromptFactory* prompt_factory() {
    return prompt_factory_.get();
  }

  void DefaultBlockAllCookies() {
    brave_shields::SetCookieControlType(
        content_settings(), browser()->profile()->GetPrefs(),
        brave_shields::ControlType::BLOCK, GURL());
  }

  void DefaultAllowAllCookies() {
    brave_shields::SetCookieControlType(
        content_settings(), browser()->profile()->GetPrefs(),
        brave_shields::ControlType::ALLOW, GURL());
  }

  void AllowCookies(const GURL& url) {
    brave_shields::SetCookieControlType(content_settings(),
                                        browser()->profile()->GetPrefs(),
                                        brave_shields::ControlType::ALLOW, url);
  }

  void BlockThirdPartyCookies(const GURL& url) {
    brave_shields::SetCookieControlType(
        content_settings(), browser()->profile()->GetPrefs(),
        brave_shields::ControlType::BLOCK_THIRD_PARTY, url);
  }

  void BlockCookies(const GURL& url) {
    brave_shields::SetCookieControlType(content_settings(),
                                        browser()->profile()->GetPrefs(),
                                        brave_shields::ControlType::BLOCK, url);
  }

  void NavigateToPageWithFrame(const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  }

  void ExpectCookiesOnHost(const GURL& url, const std::string& expected) {
    EXPECT_EQ(expected, content::GetCookies(browser()->profile(), url));
  }

  void NavigateFrameTo(const GURL& url, const std::string& id = "test") {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    EXPECT_TRUE(NavigateIframeToURL(web_contents, id, url));
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void SetGoogleSignInPref(bool value) {
    browser()->profile()->GetPrefs()->SetBoolean(kGoogleLoginControlType,
                                                 value);
  }

  void ClickButtonWithId(const std::string& id) {
    std::string click_script = base::StringPrintf(
        R"(
        new Promise(async (resolve, reject) => {
            try {
              const button = document.getElementById('%s');
              button.click();
              resolve(true);
            } catch(error) {
              reject(error);
            }
          })
      )",
        id.c_str());

    ASSERT_EQ(true, EvalJs(contents(), click_script));
  }

 protected:
  GURL embedding_url_;
  GURL top_level_page_url_;
  GURL https_top_level_page_url_;
  GURL cookie_iframe_url_;
  GURL https_cookie_iframe_url_;
  GURL google_oauth_cookie_url_;
  GURL third_party_url_;
  GURL third_party_cookie_url_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList feature_list_;

 private:
  std::unique_ptr<permissions::MockPermissionPromptFactory> prompt_factory_;
};

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PermissionAllow) {
  SetGoogleSignInPref(true);
  EXPECT_EQ(0, prompt_factory()->show_count());

  // Try to set 3p cookies from auth domain, should not work
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");
  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));

  // Verify that content settings are as expected for Google Sign-In and cookies
  EXPECT_EQ(content_settings()->GetContentSetting(
                GURL(kAccountsGoogleUrl), embedding_url_,
                ContentSettingsType::BRAVE_GOOGLE_SIGN_IN),
            ContentSetting::CONTENT_SETTING_ASK);
  EXPECT_EQ(cookie_settings()->GetCookieSetting(
                GURL(kAccountsGoogleUrl), embedding_url_, nullptr,
                content_settings::CookieSettingsBase::QueryReason::kCookies),
            ContentSetting::CONTENT_SETTING_BLOCK);
  // Accept prompt
  prompt_factory()->set_response_type(
      permissions::PermissionRequestManager::ACCEPT_ALL);

  // Have website issue request for Google auth URL
  ClickButtonWithId(kAuthButtonHtmlId);
  prompt_factory()->WaitForPermissionBubble();
  // Make sure prompt came up
  EXPECT_EQ(1, prompt_factory()->show_count());

  // Check content settings and cookie settings are as expected
  EXPECT_EQ(content_settings()->GetContentSetting(
                embedding_url_, embedding_url_,
                ContentSettingsType::BRAVE_GOOGLE_SIGN_IN),
            ContentSetting::CONTENT_SETTING_ALLOW);
  EXPECT_EQ(cookie_settings()->GetCookieSetting(
                GURL(kAccountsGoogleUrl), embedding_url_, nullptr,
                content_settings::CookieSettingsBase::QueryReason::kCookies),
            ContentSetting::CONTENT_SETTING_ALLOW);
  // Try to set 3p cookies from auth domain, should work
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");
  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
  // Try to set 3p cookies from non-auth domain, should not work
  NavigateFrameTo(third_party_cookie_url_);
  ExpectCookiesOnHost(GURL(third_party_url_), "");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PermissionDeny) {
  SetGoogleSignInPref(true);
  EXPECT_EQ(0, prompt_factory()->show_count());

  // Try to set 3p cookies from auth domain, should not work
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");
  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));

  EXPECT_EQ(content_settings()->GetContentSetting(
                GURL(kAccountsGoogleUrl), embedding_url_,
                ContentSettingsType::BRAVE_GOOGLE_SIGN_IN),
            ContentSetting::CONTENT_SETTING_ASK);
  EXPECT_EQ(cookie_settings()->GetCookieSetting(
                GURL(kAccountsGoogleUrl), embedding_url_, nullptr,
                content_settings::CookieSettingsBase::QueryReason::kCookies),
            ContentSetting::CONTENT_SETTING_BLOCK);

  prompt_factory()->set_response_type(
      permissions::PermissionRequestManager::DENY_ALL);

  // Have website issue request for Google auth URL
  ClickButtonWithId(kAuthButtonHtmlId);
  prompt_factory()->WaitForPermissionBubble();
  // Make sure prompt comes up
  EXPECT_EQ(1, prompt_factory()->show_count());

  EXPECT_EQ(content_settings()->GetContentSetting(
                embedding_url_, embedding_url_,
                ContentSettingsType::BRAVE_GOOGLE_SIGN_IN),
            ContentSetting::CONTENT_SETTING_BLOCK);
  EXPECT_EQ(cookie_settings()->GetCookieSetting(
                GURL(kAccountsGoogleUrl), embedding_url_, nullptr,
                content_settings::CookieSettingsBase::QueryReason::kCookies),
            ContentSetting::CONTENT_SETTING_BLOCK);

  // Try to set 3p cookies from auth domain, should not work
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");
  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");
  // Try to set 3p cookies from non-auth domain, should not work either
  NavigateFrameTo(third_party_cookie_url_);
  ExpectCookiesOnHost(GURL(third_party_url_), "");

  // Coming back to page, DENY is respected
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
  content::TestNavigationObserver error_observer(contents());

  ClickButtonWithId(kAuthButtonHtmlId);
  WaitForLoadStopWithoutSuccessCheck(contents());
  error_observer.Wait();
  // Navigation blocked
  EXPECT_EQ(false, error_observer.last_navigation_succeeded());
  EXPECT_EQ(net::ERR_BLOCKED_BY_CLIENT, error_observer.last_net_error_code());
  // No additional prompt shown
  EXPECT_EQ(1, prompt_factory()->show_count());
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PermissionDismiss) {
  SetGoogleSignInPref(true);
  EXPECT_EQ(0, prompt_factory()->show_count());

  // Try to set 3p cookies from auth domain, should not work
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");
  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));

  EXPECT_EQ(content_settings()->GetContentSetting(
                GURL(kAccountsGoogleUrl), embedding_url_,
                ContentSettingsType::BRAVE_GOOGLE_SIGN_IN),
            ContentSetting::CONTENT_SETTING_ASK);
  EXPECT_EQ(cookie_settings()->GetCookieSetting(
                GURL(kAccountsGoogleUrl), embedding_url_, nullptr,
                content_settings::CookieSettingsBase::QueryReason::kCookies),
            ContentSetting::CONTENT_SETTING_BLOCK);

  prompt_factory()->set_response_type(
      permissions::PermissionRequestManager::DISMISS);

  // Have website issue request for Google auth URL
  ClickButtonWithId(kAuthButtonHtmlId);
  prompt_factory()->WaitForPermissionBubble();

  // Confirm that content settings did not change
  EXPECT_EQ(content_settings()->GetContentSetting(
                embedding_url_, embedding_url_,
                ContentSettingsType::BRAVE_GOOGLE_SIGN_IN),
            ContentSetting::CONTENT_SETTING_ASK);
  EXPECT_EQ(cookie_settings()->GetCookieSetting(
                GURL(kAccountsGoogleUrl), embedding_url_, nullptr,
                content_settings::CookieSettingsBase::QueryReason::kCookies),
            ContentSetting::CONTENT_SETTING_BLOCK);

  EXPECT_EQ(1, prompt_factory()->show_count());
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PrefTurnedOff) {
  SetGoogleSignInPref(false);
  EXPECT_EQ(0, prompt_factory()->show_count());

  // Try to set 3p cookies from auth domain, should not work
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");
  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
  content::TestNavigationObserver error_observer(contents());

  ClickButtonWithId(kAuthButtonHtmlId);
  WaitForLoadStopWithoutSuccessCheck(contents());
  error_observer.Wait();
  // Navigation blocked
  EXPECT_EQ(false, error_observer.last_navigation_succeeded());
  EXPECT_EQ(net::ERR_BLOCKED_BY_CLIENT, error_observer.last_net_error_code());

  EXPECT_EQ(cookie_settings()->GetCookieSetting(
                GURL(kAccountsGoogleUrl), embedding_url_, nullptr,
                content_settings::CookieSettingsBase::QueryReason::kCookies),
            ContentSetting::CONTENT_SETTING_BLOCK);

  // No prompt shown
  EXPECT_EQ(0, prompt_factory()->show_count());
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, JSReloadOnBlockedPage) {
  SetGoogleSignInPref(true);
  EXPECT_EQ(0, prompt_factory()->show_count());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
  content::TestNavigationObserver error_observer(contents());
  prompt_factory()->set_response_type(
      permissions::PermissionRequestManager::DENY_ALL);

  ClickButtonWithId(kAuthButtonHtmlId);
  WaitForLoadStopWithoutSuccessCheck(contents());
  error_observer.Wait();
  // Navigation blocked
  EXPECT_EQ(false, error_observer.last_navigation_succeeded());
  EXPECT_EQ(net::ERR_BLOCKED_BY_CLIENT, error_observer.last_net_error_code());

  // Click reload button on blocked page
  ClickButtonWithId(kReloadButtonHtmlId);
  WaitForLoadStopWithoutSuccessCheck(contents());
  error_observer.Wait();

  // Navigation is still blocked
  EXPECT_EQ(false, error_observer.last_navigation_succeeded());
  EXPECT_EQ(net::ERR_BLOCKED_BY_CLIENT, error_observer.last_net_error_code());
}

class GoogleSignInFlagDisabledTest : public GoogleSignInBrowserTest {
 public:
  GoogleSignInFlagDisabledTest() {
    // With feature flag for new Google Sign-In behaviour turned off,
    // behave like before. These tests are copied over from
    // browser/net/brave_network_delegate_browsertest.cc
    feature_list_.InitAndDisableFeature(
        permissions::features::kBraveGoogleSignInPermission);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieAllowed) {
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieDefaultAllowSiteOverride) {
  AllowCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieDefaultBlock3pSiteOverride) {
  BlockThirdPartyCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieDefaultBlockSiteOverride) {
  BlockCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  // Cookies for accounts.google.com will be allowed since the exception
  // for google oauth will be parsed first.
  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieAllowAllAlowSiteOverride) {
  DefaultAllowAllCookies();
  AllowCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieAllowAllBlock3pSiteOverride) {
  DefaultAllowAllCookies();
  BlockThirdPartyCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieAllowAllBlockSiteOverride) {
  DefaultAllowAllCookies();
  BlockCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieBlockAllAllowSiteOverride) {
  DefaultBlockAllCookies();
  AllowCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(https_top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieBlockAllBlock3pSiteOverride) {
  DefaultBlockAllCookies();
  BlockThirdPartyCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(https_top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieBlockAllBlockSiteOverride) {
  DefaultBlockAllCookies();
  BlockCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(https_top_level_page_url_, "");
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieBlocked) {
  SetGoogleSignInPref(false);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");
}
