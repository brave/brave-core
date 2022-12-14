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

// Used to identify the buttons on the test page.
const char kAuthButtonHtmlId[] = "auth-button";

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
    current_browser_ = InProcessBrowserTest::browser();

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

  bool GetGoogleSignInPref() {
    return browser()->profile()->GetPrefs()->GetBoolean(
        kGoogleLoginControlType);
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
                  GURL(kAccountsGoogleUrl), embedding_url_, nullptr,
                  content_settings::CookieSettingsBase::QueryReason::kCookies),
              cookie_setting);
  }

  void CheckIf3PCookiesCanBeSetFromAuthDomain(bool can_be_set) {
    std::string expected_cookie_string = "";
    if (can_be_set) {
      expected_cookie_string = "oauth=true";
    }
    NavigateToPageWithFrame(https_cookie_iframe_url_);
    ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), "");
    NavigateFrameTo(google_oauth_cookie_url_);
    ExpectCookiesOnHost(GURL(kAccountsGoogleUrl), expected_cookie_string);
    // Try to set 3p cookies from non-auth domain, should never work.
    NavigateFrameTo(third_party_cookie_url_);
    ExpectCookiesOnHost(GURL(third_party_url_), "");
  }

  void CheckAllowFlow() {
    EXPECT_EQ(0, prompt_factory()->show_count());
    // Try to set 3p cookies from auth domain, should not work.
    CheckIf3PCookiesCanBeSetFromAuthDomain(false);
    // Verify default.
    CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_ASK,
                                  ContentSetting::CONTENT_SETTING_BLOCK);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // Accept prompt.
    prompt_factory()->set_response_type(
        permissions::PermissionRequestManager::ACCEPT_ALL);
    // Have website issue request for Google auth URL.
    ClickButtonWithId(kAuthButtonHtmlId);
    prompt_factory()->WaitForPermissionBubble();
    // Make sure prompt came up.
    EXPECT_EQ(1, prompt_factory()->show_count());
    // Check content settings and cookie settings are now ALLOWed
    CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_ALLOW,
                                  ContentSetting::CONTENT_SETTING_ALLOW);
    // Try to set 3p cookies from auth domain, should work.
    CheckIf3PCookiesCanBeSetFromAuthDomain(true);
  }

  void CheckDenyFlow() {
    EXPECT_EQ(0, prompt_factory()->show_count());
    // Try to set 3p cookies from auth domain, should not work.
    CheckIf3PCookiesCanBeSetFromAuthDomain(false);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // Verify default.
    CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_ASK,
                                  ContentSetting::CONTENT_SETTING_BLOCK);
    prompt_factory()->set_response_type(
        permissions::PermissionRequestManager::DENY_ALL);
    // Have website issue request for Google auth URL.
    ClickButtonWithId(kAuthButtonHtmlId);
    prompt_factory()->WaitForPermissionBubble();
    // Make sure prompt comes up.
    EXPECT_EQ(1, prompt_factory()->show_count());
    CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_BLOCK,
                                  ContentSetting::CONTENT_SETTING_BLOCK);
    // Try to set 3p cookies from auth domain, should not work.
    CheckIf3PCookiesCanBeSetFromAuthDomain(false);
    // Coming back to page, DENY is respected.
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    ClickButtonWithId(kAuthButtonHtmlId);
    // Try to set 3p cookies from auth domain, should not work.
    CheckIf3PCookiesCanBeSetFromAuthDomain(false);
    // No additional prompt shown.
    EXPECT_EQ(1, prompt_factory()->show_count());
  }

  void CheckPrefOffFlow() {
    EXPECT_EQ(0, prompt_factory()->show_count());
    // Try to set 3p cookies from auth domain, should not work.
    CheckIf3PCookiesCanBeSetFromAuthDomain(false);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    content::TestNavigationObserver error_observer(contents());
    ClickButtonWithId(kAuthButtonHtmlId);
    CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_ASK,
                                  ContentSetting::CONTENT_SETTING_BLOCK);
    // No prompt shown.
    EXPECT_EQ(0, prompt_factory()->show_count());
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
  Browser* current_browser_;

 private:
  std::unique_ptr<permissions::MockPermissionPromptFactory> prompt_factory_;
};

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PermissionAllow) {
  SetGoogleSignInPref(true);
  CheckAllowFlow();
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PermissionDeny) {
  SetGoogleSignInPref(true);
  CheckDenyFlow();
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PermissionDismiss) {
  SetGoogleSignInPref(true);
  EXPECT_EQ(0, prompt_factory()->show_count());
  // Try to set 3p cookies from auth domain, should not work.
  CheckIf3PCookiesCanBeSetFromAuthDomain(false);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
  CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_ASK,
                                ContentSetting::CONTENT_SETTING_BLOCK);
  prompt_factory()->set_response_type(
      permissions::PermissionRequestManager::DISMISS);
  // Have website issue request for Google auth URL.
  ClickButtonWithId(kAuthButtonHtmlId);
  prompt_factory()->WaitForPermissionBubble();
  // Confirm that content and cookie settings did not change.
  CheckCookiesAndContentSetting(ContentSetting::CONTENT_SETTING_ASK,
                                ContentSetting::CONTENT_SETTING_BLOCK);
  EXPECT_EQ(1, prompt_factory()->show_count());
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, PrefTurnedOff) {
  SetGoogleSignInPref(false);
  CheckPrefOffFlow();
}

IN_PROC_BROWSER_TEST_F(GoogleSignInBrowserTest, Profiles) {
  // Set pref off in one profile and on in other and it should work correctly.
  // Launch incognito in each profile and it should work correctly.
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  size_t starting_number_of_profiles = profile_manager->GetNumberOfProfiles();
  SetGoogleSignInPref(false);
  Profile* profile = browser()->profile();
  Browser* incognito_browser = CreateIncognitoBrowser(profile);
  SetBrowser(incognito_browser);
  SetPromptFactory(GetPermissionRequestManager());
  CheckPrefOffFlow();
  base::FilePath new_path = profile_manager->GenerateNextProfileDirectoryPath();
  Profile* new_profile =
      profiles::testing::CreateProfileSync(profile_manager, new_path);
  EXPECT_TRUE(new_profile);
  ASSERT_EQ(starting_number_of_profiles + 1,
            profile_manager->GetNumberOfProfiles());
  Browser* new_browser = CreateBrowser(new_profile);
  SetBrowser(new_browser);
  SetPromptFactory(GetPermissionRequestManager());
  // Check that pref is ON for new profile.
  EXPECT_TRUE(GetGoogleSignInPref());
  // Incognito behaves same as launching profile.
  Browser* new_incognito_browser = CreateIncognitoBrowser(new_profile);
  SetBrowser(new_incognito_browser);
  SetPromptFactory(GetPermissionRequestManager());
  CheckAllowFlow();
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
