/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/prefs/pref_service.h"
#include "content/public/common/content_paths.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "url/gurl.h"

using net::test_server::EmbeddedTestServer;

class BraveNetworkDelegateBrowserTest : public InProcessBrowserTest {
 public:
  BraveNetworkDelegateBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    https_server_.ServeFilesFromDirectory(test_data_dir);
    net::test_server::RegisterDefaultHandlers(&https_server_);
    ASSERT_TRUE(https_server_.Start());

    url_ = embedded_test_server()->GetURL("a.com", "/nested_iframe.html");
    nested_iframe_script_url_ =
        embedded_test_server()->GetURL("a.com", "/nested_iframe_script.html");

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
    https_top_level_page_url_ = https_server_.GetURL("a.com", "/");

    cookie_iframe_url_ =
        embedded_test_server()->GetURL("a.com", "/cookie_iframe.html");
    https_cookie_iframe_url_ =
        https_server_.GetURL("a.com", "/cookie_iframe.html");

    third_party_cookie_url_ =
        embedded_test_server()->GetURL("b.com", "/set-cookie?name=Good");
    google_oauth_cookie_url_ =
        https_server_.GetURL("accounts.google.com", "/set-cookie?oauth=true");

    top_level_page_pattern_ =
        ContentSettingsPattern::FromString("http://a.com/*");
    first_party_pattern_ =
        ContentSettingsPattern::FromString("https://firstParty/*");
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // This is needed to load pages from "domain.com" without an interstitial.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void DefaultBlockAllCookies() {
    brave_shields::SetCookieControlType(
        browser()->profile(), brave_shields::ControlType::BLOCK, GURL());
  }

  void DefaultBlockThirdPartyCookies() {
    brave_shields::SetCookieControlType(
        browser()->profile(), brave_shields::ControlType::BLOCK_THIRD_PARTY,
        GURL());
  }

  void DefaultAllowAllCookies() {
    brave_shields::SetCookieControlType(
        browser()->profile(), brave_shields::ControlType::ALLOW, GURL());
  }

  void AllowCookies(const GURL url) {
    brave_shields::SetCookieControlType(browser()->profile(),
                                        brave_shields::ControlType::ALLOW,
                                        url);
  }

  void BlockThirdPartyCookies(const GURL url) {
    brave_shields::SetCookieControlType(
        browser()->profile(),
        brave_shields::ControlType::BLOCK_THIRD_PARTY,
        url);
  }

  void BlockCookies(const GURL url) {
    brave_shields::SetCookieControlType(browser()->profile(),
                                        brave_shields::ControlType::BLOCK,
                                        url);
  }

  void ShieldsDown(const GURL url) {
    brave_shields::SetBraveShieldsEnabled(browser()->profile(), false,
                                          url);
  }

  void NavigateToPageWithFrame(const GURL url) {
    ui_test_utils::NavigateToURL(browser(), url);
  }

  void ExpectCookiesOnHost(const GURL url,
                           const std::string& expected) {
    EXPECT_EQ(expected, content::GetCookies(browser()->profile(),
                                            url));
  }

  void NavigateFrameTo(const GURL url) {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    EXPECT_TRUE(NavigateIframeToURL(web_contents, "test", url));
  }

  void BlockGoogleOAuthCookies() {
    browser()->profile()->GetPrefs()->SetBoolean(kGoogleLoginControlType,
                                                 false);
  }

 protected:
  GURL url_;
  GURL nested_iframe_script_url_;
  GURL top_level_page_url_;
  GURL https_top_level_page_url_;
  GURL cookie_iframe_url_;
  GURL https_cookie_iframe_url_;
  GURL third_party_cookie_url_;
  GURL google_oauth_cookie_url_;

 private:
  ContentSettingsPattern top_level_page_pattern_;
  ContentSettingsPattern first_party_pattern_;
  ContentSettingsPattern iframe_pattern_;
  net::test_server::EmbeddedTestServer https_server_;
};

// It is important that cookies in following tests are set by response headers,
// not by javascript. Fetching such cookies is controlled by NetworkDelegate.
IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest, Iframe3PCookieBlocked) {
  ui_test_utils::NavigateToURL(browser(), url_);
  const std::string cookie =
      content::GetCookies(browser()->profile(), GURL("http://c.com/"));
  EXPECT_TRUE(cookie.empty()) << "Actual cookie: " << cookie;
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest, Iframe3PCookieAllowed) {
  AllowCookies(top_level_page_url_);
  ui_test_utils::NavigateToURL(browser(), url_);
  const std::string cookie =
      content::GetCookies(browser()->profile(), GURL("http://c.com/"));
  EXPECT_FALSE(cookie.empty());
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest, Iframe3PShieldsDown) {
  ShieldsDown(top_level_page_url_);
  ui_test_utils::NavigateToURL(browser(), url_);
  const std::string cookie =
      content::GetCookies(browser()->profile(), GURL("http://c.com/"));
  EXPECT_FALSE(cookie.empty());
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       Iframe3PShieldsDownOverridesCookieBlock) {
  // create an explicit override
  BlockCookies(top_level_page_url_);
  ui_test_utils::NavigateToURL(browser(), url_);
  std::string cookie =
      content::GetCookies(browser()->profile(), GURL("http://c.com/"));
  EXPECT_TRUE(cookie.empty()) << "Actual cookie: " << cookie;

  ShieldsDown(top_level_page_url_);
  ui_test_utils::NavigateToURL(browser(), url_);
  cookie = content::GetCookies(browser()->profile(), GURL("http://c.com/"));
  EXPECT_FALSE(cookie.empty());
}

// Fetching not just a frame, but some other resource.
IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       IframeJs3PCookieBlocked) {
  ui_test_utils::NavigateToURL(browser(), nested_iframe_script_url_);
  const std::string cookie =
      content::GetCookies(browser()->profile(), GURL("http://c.com/"));
  EXPECT_TRUE(cookie.empty()) << "Actual cookie: " << cookie;
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       IframeJs3PCookieAllowed) {
  AllowCookies(top_level_page_url_);
  ui_test_utils::NavigateToURL(browser(), nested_iframe_script_url_);
  const std::string cookie =
      content::GetCookies(browser()->profile(), GURL("http://c.com/"));
  EXPECT_FALSE(cookie.empty());
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest, DefaultCookiesBlocked) {
  DefaultBlockAllCookies();
  ui_test_utils::NavigateToURL(browser(), nested_iframe_script_url_);
  std::string cookie =
      content::GetCookies(browser()->profile(), GURL("http://c.com/"));
  EXPECT_TRUE(cookie.empty()) << "Actual cookie: " << cookie;
  cookie = content::GetCookies(browser()->profile(), GURL("http://a.com/"));
  EXPECT_TRUE(cookie.empty()) << "Actual cookie: " << cookie;
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       PrefToggleBlockAllToBlockThirdParty) {
  DefaultBlockAllCookies();
  DefaultBlockThirdPartyCookies();

  EXPECT_TRUE(browser()->profile()->GetPrefs()->GetBoolean(
      prefs::kBlockThirdPartyCookies));
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetInteger(
                "profile.default_content_setting_values.cookies"),
            ContentSetting::CONTENT_SETTING_ALLOW);

  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL(third_party_cookie_url_.host()), "");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       PrefToggleBlockAllToAllowAll) {
  DefaultBlockAllCookies();
  DefaultAllowAllCookies();

  EXPECT_FALSE(browser()->profile()->GetPrefs()->GetBoolean(
      prefs::kBlockThirdPartyCookies));
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetInteger(
                "profile.default_content_setting_values.cookies"),
            ContentSetting::CONTENT_SETTING_ALLOW);

  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL("http://b.com"), "name=Good");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       PrefToggleBlockThirdPartyToAllowAll) {
  DefaultBlockThirdPartyCookies();
  DefaultAllowAllCookies();

  EXPECT_FALSE(browser()->profile()->GetPrefs()->GetBoolean(
      prefs::kBlockThirdPartyCookies));
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetInteger(
                "profile.default_content_setting_values.cookies"),
            ContentSetting::CONTENT_SETTING_ALLOW);

  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL("http://b.com"), "name=Good");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       PrefToggleBlockThirdPartyToBlockAll) {
  DefaultBlockThirdPartyCookies();
  DefaultBlockAllCookies();

  EXPECT_FALSE(browser()->profile()->GetPrefs()->GetBoolean(
      prefs::kBlockThirdPartyCookies));
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetInteger(
                "profile.default_content_setting_values.cookies"),
            ContentSetting::CONTENT_SETTING_BLOCK);

  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "");
  ExpectCookiesOnHost(GURL("http://b.com"), "");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       PrefToggleAllowAllToBlockThirdParty) {
  DefaultAllowAllCookies();
  DefaultBlockThirdPartyCookies();

  EXPECT_TRUE(browser()->profile()->GetPrefs()->GetBoolean(
      prefs::kBlockThirdPartyCookies));
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetInteger(
                "profile.default_content_setting_values.cookies"),
            ContentSetting::CONTENT_SETTING_ALLOW);

  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL("http://b.com"), "");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       PrefToggleAllowAllToBlockAll) {
  DefaultAllowAllCookies();
  DefaultBlockAllCookies();

  EXPECT_FALSE(browser()->profile()->GetPrefs()->GetBoolean(
      prefs::kBlockThirdPartyCookies));
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetInteger(
                "profile.default_content_setting_values.cookies"),
            ContentSetting::CONTENT_SETTING_BLOCK);

  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "");
  ExpectCookiesOnHost(GURL("http://b.com"), "");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyGoogleOauthCookieAllowed) {
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyGoogleOauthCookieDefaultAllowSiteOverride) {
  AllowCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyGoogleOauthCookieDefaultBlock3pSiteOverride) {
  BlockThirdPartyCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyGoogleOauthCookieDefaultBlockSiteOverride) {
  BlockCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "");

  // Cookies for accounts.google.com will be allowed since the exception
  // for google oauth will be parsed first.
  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyGoogleOauthCookieAllowAllAlowSiteOverride) {
  DefaultAllowAllCookies();
  AllowCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyGoogleOauthCookieAllowAllBlock3pSiteOverride) {
  DefaultAllowAllCookies();
  BlockThirdPartyCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyGoogleOauthCookieAllowAllBlockSiteOverride) {
  DefaultAllowAllCookies();
  BlockCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyGoogleOauthCookieBlockAllAllowSiteOverride) {
  DefaultBlockAllCookies();
  AllowCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(https_top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyGoogleOauthCookieBlockAllBlock3pSiteOverride) {
  DefaultBlockAllCookies();
  BlockThirdPartyCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(https_top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyGoogleOauthCookieBlockAllBlockSiteOverride) {
  DefaultBlockAllCookies();
  BlockCookies(https_top_level_page_url_);
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(https_top_level_page_url_, "");
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "oauth=true");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyGoogleOauthCookieBlocked) {
  BlockGoogleOAuthCookies();
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "");

  NavigateFrameTo(google_oauth_cookie_url_);
  ExpectCookiesOnHost(GURL("https://accounts.google.com"), "");
}

