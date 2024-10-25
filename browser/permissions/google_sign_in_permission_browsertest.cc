// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/google_sign_in_permission/features.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/cookie_settings_base.h"
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
#include "services/network/public/mojom/cookie_manager.mojom.h"
#include "url/gurl.h"

using net::test_server::EmbeddedTestServer;

namespace {

constexpr char kAccountsGoogleUrl[] = "https://accounts.google.com";
constexpr char kEmbeddingPageUrl[] = "/google_sign_in_link.html";
constexpr char kTestDomain[] = "a.com";
constexpr char kThirdPartyTestDomain[] = "b.com";

// Used to identify the buttons on the test page.
constexpr char kGoogleAuthButtonHtmlId[] = "auth-button-google";
constexpr char kFirebaseAuthButtonHtmlId[] = "auth-button-firebase";
constexpr char kGoogleAuthButtonWithoutParamHtmlId[] =
    "auth-button-google-without-param";
constexpr char kFirebaseAuthButtonDiffParamHtmlId[] =
    "auth-button-firebase-diff-param";
constexpr char kGoogleAuthButtonPopupHtmlId[] = "auth-button-google-popup";

}  // namespace

class GoogleSignInBrowserTest : public InProcessBrowserTest {
 public:
  GoogleSignInBrowserTest() {
    feature_list_.InitAndEnableFeature(
        google_sign_in_permission::features::kBraveGoogleSignInPermission);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    current_browser_ = InProcessBrowserTest::browser();

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
    third_party_url_ = https_server_->GetURL(kThirdPartyTestDomain, "/");
    third_party_cookie_url_ = https_server_->GetURL(
        kThirdPartyTestDomain, "/set-cookie?test=true;SameSite=None;Secure");
    embedding_url_ = https_server_->GetURL(kTestDomain, kEmbeddingPageUrl);
    cookie_iframe_url_ =
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
    current_browser_ = InProcessBrowserTest::browser();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void TearDownOnMainThread() override { prompt_factory_.reset(); }

  permissions::MockPermissionPromptFactory* prompt_factory() {
    return prompt_factory_.get();
  }

  Browser* browser() { return current_browser_; }

  void SetBrowser(Browser* browser) { current_browser_ = browser; }

  void SetPromptFactory(permissions::PermissionRequestManager* manager) {
    prompt_factory_ =
        std::make_unique<permissions::MockPermissionPromptFactory>(manager);
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
    std::string click_script = content::JsReplace(
        R"(
        new Promise(async (resolve, reject) => {
            try {
              const button = document.getElementById($1);
              button.click();
              resolve(true);
            } catch(error) {
              reject(error);
            }
          })
      )",
        id);
    ASSERT_EQ(true, EvalJs(contents(), click_script));
  }

  void CheckCookiesAndContentSetting(ContentSetting content_setting,
                                     ContentSetting cookie_setting) {
    EXPECT_EQ(content_settings()->GetContentSetting(
                  embedding_url_, embedding_url_,
                  ContentSettingsType::BRAVE_GOOGLE_SIGN_IN),
              content_setting);
    EXPECT_EQ(cookie_settings()->GetCookieSetting(
                  GURL(kAccountsGoogleUrl), net::SiteForCookies(),
                  embedding_url_, net::CookieSettingOverrides(), nullptr),
              cookie_setting);
  }

  void CheckIf3PCookiesCanBeSetFromAuthDomain(bool can_be_set) {
    std::string expected_cookie_string = "";
    if (can_be_set) {
      expected_cookie_string = "oauth=true";
    }
    NavigateToPageWithFrame(cookie_iframe_url_);
    ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");
    NavigateFrameTo(google_oauth_cookie_url_);
    ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), expected_cookie_string);
    // Try to set 3p cookies from non-auth domain, should never work.
    NavigateFrameTo(third_party_cookie_url_);
    ExpectCookiesOnHost(GURL(third_party_url_), "");
    // Delete set cookie
    DeleteCookies(contents()->GetBrowserContext(),
                  network::mojom::CookieDeletionFilter());
  }

  void CheckCurrentStatusIsAsk() {
    // Verify default.
    CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_ASK,
                                  ContentSetting::CONTENT_SETTING_BLOCK);
    CheckIf3PCookiesCanBeSetFromAuthDomain(false);
  }

  void CheckAskAndAcceptFlow(std::string button_id = kGoogleAuthButtonHtmlId) {
    EXPECT_EQ(0, prompt_factory()->show_count());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // Accept prompt.
    prompt_factory()->set_response_type(
        permissions::PermissionRequestManager::ACCEPT_ALL);
    // Have website issue request for Google auth URL.
    ClickButtonWithId(button_id);
    prompt_factory()->WaitForPermissionBubble();
    // Make sure prompt came up.
    EXPECT_EQ(1, prompt_factory()->show_count());
    // Check content settings and cookie settings are now ALLOWed
    CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_ALLOW,
                                  ContentSetting::CONTENT_SETTING_ALLOW);
    // Try to set 3p cookies from auth domain, should work.
    CheckIf3PCookiesCanBeSetFromAuthDomain(true);
  }

  void CheckAskAndDenyFlow(std::string button_id = kGoogleAuthButtonHtmlId) {
    EXPECT_EQ(0, prompt_factory()->show_count());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // Deny prompt.
    prompt_factory()->set_response_type(
        permissions::PermissionRequestManager::DENY_ALL);
    // Have website issue request for Google auth URL.
    ClickButtonWithId(button_id);
    prompt_factory()->WaitForPermissionBubble();
    // Make sure prompt comes up.
    EXPECT_EQ(1, prompt_factory()->show_count());
    CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_BLOCK,
                                  ContentSetting::CONTENT_SETTING_BLOCK);
    // Try to set 3p cookies from auth domain, should not work.
    CheckIf3PCookiesCanBeSetFromAuthDomain(false);
  }

  void CheckAllowedFlow(int initial_prompts_shown = 0,
                        std::string button_id = kGoogleAuthButtonHtmlId) {
    EXPECT_EQ(initial_prompts_shown, prompt_factory()->show_count());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // Have website issue request for Google auth URL.
    ClickButtonWithId(button_id);
    // Make sure prompt did not come up again.
    EXPECT_EQ(initial_prompts_shown, prompt_factory()->show_count());
    // Check content settings and cookie settings are ALLOWed
    CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_ALLOW,
                                  ContentSetting::CONTENT_SETTING_ALLOW);
    // Try to set 3p cookies from auth domain, should work.
    CheckIf3PCookiesCanBeSetFromAuthDomain(true);
  }

  void CheckBlockedFlow(int initial_prompts_shown = 0,
                        std::string button_id = kGoogleAuthButtonHtmlId) {
    EXPECT_EQ(initial_prompts_shown, prompt_factory()->show_count());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // Have website issue request for Google auth URL.
    ClickButtonWithId(button_id);
    // Make sure prompt did not come up again.
    EXPECT_EQ(initial_prompts_shown, prompt_factory()->show_count());
    // Check content settings and cookie settings are BLOCKed
    CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_BLOCK,
                                  ContentSetting::CONTENT_SETTING_BLOCK);
    // Try to set 3p cookies from auth domain, should NOT work.
    CheckIf3PCookiesCanBeSetFromAuthDomain(false);
  }

  void CheckPrefOffFlow(std::string button_id = kGoogleAuthButtonHtmlId) {
    EXPECT_EQ(0, prompt_factory()->show_count());
    // Try to set 3p cookies from auth domain, should not work.
    CheckIf3PCookiesCanBeSetFromAuthDomain(false);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    ClickButtonWithId(button_id);
    CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_ASK,
                                  ContentSetting::CONTENT_SETTING_BLOCK);
    // No prompt shown.
    EXPECT_EQ(0, prompt_factory()->show_count());
  }

 protected:
  GURL embedding_url_;
  GURL top_level_page_url_;
  GURL cookie_iframe_url_;
  GURL google_oauth_cookie_url_;
  GURL third_party_url_;
  GURL third_party_cookie_url_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList feature_list_;
  raw_ptr<Browser, DanglingUntriaged> current_browser_;

 private:
  std::unique_ptr<permissions::MockPermissionPromptFactory> prompt_factory_;
};

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PermissionAllowGoogle) {
  CheckAskAndAcceptFlow();
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PermissionDenyGoogle) {
  CheckAskAndDenyFlow();
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, Default) {
  CheckCurrentStatusIsAsk();
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PermissionAllowFirebase) {
  CheckAskAndAcceptFlow(kFirebaseAuthButtonHtmlId);
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PermissionDenyFirebase) {
  CheckAskAndDenyFlow(kFirebaseAuthButtonHtmlId);
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PermissionDismissGoogle) {
  EXPECT_EQ(0, prompt_factory()->show_count());
  prompt_factory()->set_response_type(
      permissions::PermissionRequestManager::DISMISS);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
  // Have website issue request for Google auth URL.
  ClickButtonWithId(kGoogleAuthButtonHtmlId);
  prompt_factory()->WaitForPermissionBubble();
  // Confirm that content and cookie settings did not change.
  CheckCurrentStatusIsAsk();
  EXPECT_EQ(1, prompt_factory()->show_count());
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PermissionDismissFirebase) {
  EXPECT_EQ(0, prompt_factory()->show_count());
  prompt_factory()->set_response_type(
      permissions::PermissionRequestManager::DISMISS);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
  // Have website issue request for Google auth URL.
  ClickButtonWithId(kFirebaseAuthButtonHtmlId);
  prompt_factory()->WaitForPermissionBubble();
  // Confirm that content and cookie settings did not change.
  CheckCurrentStatusIsAsk();
  EXPECT_EQ(1, prompt_factory()->show_count());
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest,
                       kGoogleLoginControlTypePrefIsOff) {
  CheckCurrentStatusIsAsk();
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, GoogleAuthButNoParam) {
  CheckPrefOffFlow(kGoogleAuthButtonWithoutParamHtmlId);
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, FirebaseAuthButNoParam) {
  CheckPrefOffFlow(kFirebaseAuthButtonDiffParamHtmlId);
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, IncognitoModeInheritAllow) {
  // Allowed permission for a website is inherited in incognito
  CheckAskAndAcceptFlow();
  Profile* profile = browser()->profile();
  Browser* incognito_browser = CreateIncognitoBrowser(profile);
  SetBrowser(incognito_browser);
  SetPromptFactory(GetPermissionRequestManager());
  CheckAllowedFlow();
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, IncognitoModeInheritBlock) {
  // Blocked permission for a website is inherited in incognito
  CheckAskAndDenyFlow();
  Profile* profile = browser()->profile();
  Browser* incognito_browser = CreateIncognitoBrowser(profile);
  SetBrowser(incognito_browser);
  SetPromptFactory(GetPermissionRequestManager());
  CheckBlockedFlow();
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest,
                       PopupAuthWindowAllowReloadsTab) {
  EXPECT_EQ(0, prompt_factory()->show_count());
  prompt_factory()->set_response_type(
      permissions::PermissionRequestManager::ACCEPT_ALL);
  // Wait for the page to reload after the popup window is closed.
  content::TestNavigationObserver reload_observer(contents(), 2);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
  // Have website issue request for Google auth URL in a popup window.
  ClickButtonWithId(kGoogleAuthButtonPopupHtmlId);
  reload_observer.Wait();
  EXPECT_TRUE(reload_observer.last_navigation_succeeded());
  EXPECT_EQ(embedding_url_, reload_observer.last_navigation_url());
  EXPECT_EQ(1, prompt_factory()->show_count());
  // Check current status is ALLOW.
  CheckAllowedFlow(1);
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest,
                       PopupAuthWindowDenyDoesNotReloadTab) {
  EXPECT_EQ(0, prompt_factory()->show_count());
  prompt_factory()->set_response_type(
      permissions::PermissionRequestManager::DENY_ALL);
  // Wait for the page to reload after the popup window is closed.
  content::TestNavigationObserver reload_observer(contents(), 1);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
  // Have website issue request for Google auth URL in a popup window.
  ClickButtonWithId(kGoogleAuthButtonPopupHtmlId);
  reload_observer.Wait();
  EXPECT_TRUE(reload_observer.last_navigation_succeeded());
  EXPECT_EQ(embedding_url_, reload_observer.last_navigation_url());
  EXPECT_EQ(1, prompt_factory()->show_count());
  // Check current status is DENY.
  CheckBlockedFlow(1);
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, IncognitoModeDoesNotLeak) {
  // Permission set in Incognito does not leak back to normal mode.
  Browser* original_browser = browser();
  Browser* incognito_browser = CreateIncognitoBrowser();
  SetBrowser(incognito_browser);
  SetPromptFactory(GetPermissionRequestManager());
  CheckAskAndAcceptFlow();
  // Check permission did not leak.
  SetBrowser(original_browser);
  SetPromptFactory(GetPermissionRequestManager());
  CheckCurrentStatusIsAsk();
}

// No prompt shown when current website is a google.com domain
IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, GoogleDomain) {
  EXPECT_EQ(0, prompt_factory()->show_count());
  // Go to website that is a google.com domain.
  auto google_domain =
      https_server_->GetURL("developers.google.com", kEmbeddingPageUrl);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), google_domain));
  ClickButtonWithId(kGoogleAuthButtonHtmlId);
  // No prompt shown.
  EXPECT_EQ(0, prompt_factory()->show_count());
}

class GoogleSignInFlagDisabledTest : public GoogleSignInBrowserTest {
 public:
  GoogleSignInFlagDisabledTest() {
    // With feature flag for new Google Sign-In behaviour turned off,
    // behave like before. These tests are copied over from
    // browser/net/brave_network_delegate_browsertest.cc
    feature_list_.InitAndDisableFeature(
        google_sign_in_permission::features::kBraveGoogleSignInPermission);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieAllowed) {
  NavigateToPageWithFrame(cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieDefaultAllowSiteOverride) {
  AllowCookies(top_level_page_url_);
  NavigateToPageWithFrame(cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieDefaultBlock3pSiteOverride) {
  BlockThirdPartyCookies(top_level_page_url_);
  NavigateToPageWithFrame(cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieDefaultBlockSiteOverride) {
  BlockCookies(top_level_page_url_);
  NavigateToPageWithFrame(cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  // Cookies for accounts.google.com will be allowed since the exception
  // for google oauth will be parsed first.
  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieAllowAllAlowSiteOverride) {
  DefaultAllowAllCookies();
  AllowCookies(top_level_page_url_);
  NavigateToPageWithFrame(cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieAllowAllBlock3pSiteOverride) {
  DefaultAllowAllCookies();
  BlockThirdPartyCookies(top_level_page_url_);
  NavigateToPageWithFrame(cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieAllowAllBlockSiteOverride) {
  DefaultAllowAllCookies();
  BlockCookies(top_level_page_url_);
  NavigateToPageWithFrame(cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieBlockAllAllowSiteOverride) {
  DefaultBlockAllCookies();
  AllowCookies(top_level_page_url_);
  NavigateToPageWithFrame(cookie_iframe_url_);
  ExpectCookiesOnHost(top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieBlockAllBlock3pSiteOverride) {
  DefaultBlockAllCookies();
  BlockThirdPartyCookies(top_level_page_url_);
  NavigateToPageWithFrame(cookie_iframe_url_);
  ExpectCookiesOnHost(top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieBlockAllBlockSiteOverride) {
  DefaultBlockAllCookies();
  BlockCookies(top_level_page_url_);
  NavigateToPageWithFrame(cookie_iframe_url_);
  ExpectCookiesOnHost(top_level_page_url_, "");
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(GoogleSignInFlagDisabledTest,
                       ThirdPartyGoogleOauthCookieBlocked) {
  SetGoogleSignInPref(false);
  NavigateToPageWithFrame(cookie_iframe_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");
}
