/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_paths.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "net/test/embedded_test_server/http_request.h"
#include "url/gurl.h"

using net::test_server::EmbeddedTestServer;

namespace {

bool NavigateRenderFrameToURL(content::RenderFrameHost* frame,
                              std::string iframe_id,
                              const GURL& url) {
  std::string script = base::StringPrintf(
      "setTimeout(\""
      "var iframes = document.getElementById('%s');iframes.src='%s';"
      "\",0)",
      iframe_id.c_str(), url.spec().c_str());

  content::TestNavigationManager navigation_manager(
      content::WebContents::FromRenderFrameHost(frame), url);
  bool result = content::ExecJs(frame, script);
  if (!navigation_manager.WaitForNavigationFinished()) {
    return false;
  }
  return result;
}

GURL GetHttpRequestURL(const net::test_server::HttpRequest& http_request) {
  return GURL(
      base::StrCat({http_request.base_url.scheme_piece(), "://",
                    http_request.headers.at(net::HttpRequestHeaders::kHost),
                    http_request.relative_url}));
}

}  // namespace

class BraveNetworkDelegateBrowserTest : public InProcessBrowserTest {
 public:
  BraveNetworkDelegateBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &BraveNetworkDelegateBrowserTest::MonitorHTTPRequest,
        base::Unretained(this)));
    content::SetupCrossSiteRedirector(&https_server_);
    ASSERT_TRUE(https_server_.Start());

    url_ = https_server_.GetURL("a.com", "/nested_iframe.html");
    nested_iframe_script_url_ =
        https_server_.GetURL("a.com", "/nested_iframe_script.html");

    top_level_page_url_ = https_server_.GetURL("a.com", "/");

    cookie_iframe_url_ = https_server_.GetURL("a.com", "/cookie_iframe.html");
    https_cookie_iframe_url_ =
        https_server_.GetURL("a.com", "/cookie_iframe.html");

    third_party_cookie_url_ = https_server_.GetURL(
        "b.com", "/set-cookie?name=bcom;SameSite=None;Secure");
    first_party_cookie_url_ = https_server_.GetURL(
        "a.com", "/set-cookie?name=acom;SameSite=None;Secure");
    subdomain_first_party_cookie_url_ = https_server_.GetURL(
        "subdomain.a.com",
        "/set-cookie?name=subdomainacom;SameSite=None;Secure");

    domain_registry_url_ =
        https_server_.GetURL("mobile.twitter.com", "/cookie_iframe.html");
    iframe_domain_registry_url_ =
        https_server_.GetURL("blah.twitter.com",
                             "/set-cookie?name=blahtwittercom;domain=twitter."
                             "com;SameSite=None;Secure");

    top_level_page_pattern_ =
        ContentSettingsPattern::FromString("https://a.com/*");
    first_party_pattern_ =
        ContentSettingsPattern::FromString("https://firstParty/*");

    wordpress_top_url_ =
        https_server_.GetURL("example.wordpress.com", "/cookie_iframe.html");
    wordpress_frame_url_ = https_server_.GetURL(
        "example.wordpress.com", "/set-cookie?frame=true;SameSite=None;Secure");
    wp_top_url_ = https_server_.GetURL("example.wp.com", "/cookie_iframe.html");
    wp_frame_url_ = https_server_.GetURL(
        "example.wp.com", "/set-cookie?frame=true;SameSite=None;Secure");
    a_frame_url_ = https_server_.GetURL(
        "a.com", "/set-cookie?frame=true;SameSite=None;Secure");
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
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

  void DefaultBlockAllCookies() {
    brave_shields::SetCookieControlType(
        content_settings(), browser()->profile()->GetPrefs(),
        brave_shields::ControlType::BLOCK, GURL());
  }

  void DefaultBlockThirdPartyCookies() {
    brave_shields::SetCookieControlType(
        content_settings(), browser()->profile()->GetPrefs(),
        brave_shields::ControlType::BLOCK_THIRD_PARTY, GURL());
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

  void ShieldsDown(const GURL& url) {
    brave_shields::SetBraveShieldsEnabled(content_settings(), false, url);
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

  void MonitorHTTPRequest(const net::test_server::HttpRequest& request) {
    auto cookie_it = request.headers.find(net::HttpRequestHeaders::kCookie);
    if (cookie_it != request.headers.end()) {
      seen_cookies_[GetHttpRequestURL(request)] = cookie_it->second;
    }
  }

  const base::flat_map<GURL, std::string>& seen_cookies() const {
    return seen_cookies_;
  }

 protected:
  GURL url_;
  GURL nested_iframe_script_url_;
  GURL top_level_page_url_;
  GURL cookie_iframe_url_;
  GURL https_cookie_iframe_url_;
  GURL third_party_cookie_url_;
  GURL first_party_cookie_url_;
  GURL subdomain_first_party_cookie_url_;
  GURL domain_registry_url_;
  GURL iframe_domain_registry_url_;
  GURL wordpress_top_url_;
  GURL wordpress_frame_url_;
  GURL wp_top_url_;
  GURL wp_frame_url_;
  GURL a_frame_url_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
  base::flat_map<GURL, std::string> seen_cookies_;

 private:
  ContentSettingsPattern top_level_page_pattern_;
  ContentSettingsPattern first_party_pattern_;
  ContentSettingsPattern iframe_pattern_;
};

// It is important that cookies in following tests are set by response headers,
// not by javascript. Fetching such cookies is controlled by NetworkDelegate.
IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest, Iframe3PCookieBlocked) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_));
  const std::string cookie = content::GetCookies(
      browser()->profile(), https_server_.GetURL("c.com", "/"));
  EXPECT_TRUE(cookie.empty()) << "Actual cookie: " << cookie;
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest, Iframe3PCookieAllowed) {
  AllowCookies(top_level_page_url_);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_));
  const std::string cookie = content::GetCookies(
      browser()->profile(), https_server_.GetURL("c.com", "/"));
  EXPECT_FALSE(cookie.empty());
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest, Iframe3PShieldsDown) {
  ShieldsDown(top_level_page_url_);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_));
  const std::string cookie =
      content::GetCookies(browser()->profile(), GURL("https://c.com/"));
  EXPECT_FALSE(cookie.empty());
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       Iframe3PShieldsDownOverridesCookieBlock) {
  // create an explicit override
  BlockCookies(top_level_page_url_);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_));
  std::string cookie =
      content::GetCookies(browser()->profile(), GURL("https://c.com/"));
  EXPECT_TRUE(cookie.empty()) << "Actual cookie: " << cookie;

  ShieldsDown(top_level_page_url_);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_));
  cookie = content::GetCookies(browser()->profile(), GURL("https://c.com/"));
  EXPECT_FALSE(cookie.empty());
}

// Fetching not just a frame, but some other resource.
IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       IframeJs3PCookieBlocked) {
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), nested_iframe_script_url_));
  const std::string cookie =
      content::GetCookies(browser()->profile(), GURL("https://c.com/"));
  EXPECT_TRUE(cookie.empty()) << "Actual cookie: " << cookie;
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       IframeJs3PCookieAllowed) {
  AllowCookies(top_level_page_url_);
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), nested_iframe_script_url_));
  const std::string cookie =
      content::GetCookies(browser()->profile(), GURL("https://c.com/"));
  EXPECT_FALSE(cookie.empty());
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest, DefaultCookiesBlocked) {
  DefaultBlockAllCookies();
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), nested_iframe_script_url_));
  std::string cookie =
      content::GetCookies(browser()->profile(), GURL("https://c.com/"));
  EXPECT_TRUE(cookie.empty()) << "Actual cookie: " << cookie;
  cookie = content::GetCookies(browser()->profile(), GURL("https://a.com/"));
  EXPECT_TRUE(cookie.empty()) << "Actual cookie: " << cookie;
}

// 1stpartydomain.com -> 3rdpartydomain.com -> 1stpartydomain.com nested iframe
IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyCookiesBlockedNestedFirstPartyIframe) {
  DefaultBlockThirdPartyCookies();

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_));

  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  NavigateFrameTo(https_server_.GetURL("b.com", "/iframe_cookie.html"),
                  "nested_iframe");

  content::RenderFrameHost* child_frame =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  NavigateRenderFrameToURL(child_frame, "iframe_cookie",
                           subdomain_first_party_cookie_url_);

  ExpectCookiesOnHost(subdomain_first_party_cookie_url_, "name=subdomainacom");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       PrefToggleBlockAllToBlockThirdParty) {
  DefaultBlockAllCookies();
  DefaultBlockThirdPartyCookies();

  EXPECT_EQ(static_cast<content_settings::CookieControlsMode>(
                browser()->profile()->GetPrefs()->GetInteger(
                    prefs::kCookieControlsMode)),
            content_settings::CookieControlsMode::kBlockThirdParty);

  EXPECT_EQ(browser()->profile()->GetPrefs()->GetInteger(
                "profile.default_content_setting_values.cookies"),
            ContentSetting::CONTENT_SETTING_ALLOW);

  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(third_party_cookie_url_, "");

  NavigateFrameTo(subdomain_first_party_cookie_url_);
  ExpectCookiesOnHost(subdomain_first_party_cookie_url_, "name=subdomainacom");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       PrefToggleBlockAllToAllowAll) {
  DefaultBlockAllCookies();
  DefaultAllowAllCookies();

  EXPECT_EQ(static_cast<content_settings::CookieControlsMode>(
                browser()->profile()->GetPrefs()->GetInteger(
                    prefs::kCookieControlsMode)),
            content_settings::CookieControlsMode::kOff);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetInteger(
                "profile.default_content_setting_values.cookies"),
            ContentSetting::CONTENT_SETTING_ALLOW);

  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL("https://b.com"), "name=bcom");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       PrefToggleBlockThirdPartyToAllowAll) {
  DefaultBlockThirdPartyCookies();
  DefaultAllowAllCookies();

  EXPECT_EQ(static_cast<content_settings::CookieControlsMode>(
                browser()->profile()->GetPrefs()->GetInteger(
                    prefs::kCookieControlsMode)),
            content_settings::CookieControlsMode::kOff);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetInteger(
                "profile.default_content_setting_values.cookies"),
            ContentSetting::CONTENT_SETTING_ALLOW);

  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL("https://b.com"), "name=bcom");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       PrefToggleBlockThirdPartyToBlockAll) {
  DefaultBlockThirdPartyCookies();
  DefaultBlockAllCookies();

  EXPECT_EQ(static_cast<content_settings::CookieControlsMode>(
                browser()->profile()->GetPrefs()->GetInteger(
                    prefs::kCookieControlsMode)),
            content_settings::CookieControlsMode::kBlockThirdParty);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetInteger(
                "profile.default_content_setting_values.cookies"),
            ContentSetting::CONTENT_SETTING_BLOCK);

  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "");
  ExpectCookiesOnHost(GURL("https://b.com"), "");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       PrefToggleAllowAllToBlockThirdParty) {
  DefaultAllowAllCookies();
  DefaultBlockThirdPartyCookies();

  EXPECT_EQ(static_cast<content_settings::CookieControlsMode>(
                browser()->profile()->GetPrefs()->GetInteger(
                    prefs::kCookieControlsMode)),
            content_settings::CookieControlsMode::kBlockThirdParty);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetInteger(
                "profile.default_content_setting_values.cookies"),
            ContentSetting::CONTENT_SETTING_ALLOW);

  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(GURL("https://b.com"), "");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       PrefToggleAllowAllToBlockAll) {
  DefaultAllowAllCookies();
  DefaultBlockAllCookies();

  EXPECT_EQ(browser()->profile()->GetPrefs()->GetInteger(
                "profile.default_content_setting_values.cookies"),
            ContentSetting::CONTENT_SETTING_BLOCK);

  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "");
  ExpectCookiesOnHost(GURL("https://b.com"), "");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ShieldsToggleBlockThirdPartyWithDefaultAllowAll) {
  DefaultAllowAllCookies();

  BlockThirdPartyCookies(cookie_iframe_url_);
  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(cookie_iframe_url_, "name=Good");
  ExpectCookiesOnHost(third_party_cookie_url_, "");

  NavigateFrameTo(first_party_cookie_url_);
  ExpectCookiesOnHost(cookie_iframe_url_, "name=acom");
  ExpectCookiesOnHost(first_party_cookie_url_, "name=acom");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ShieldsToggleBlockThirdPartyWithDefaultBlockAll) {
  DefaultBlockAllCookies();

  BlockThirdPartyCookies(cookie_iframe_url_);
  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(third_party_cookie_url_);

  ExpectCookiesOnHost(cookie_iframe_url_, "name=Good");
  ExpectCookiesOnHost(third_party_cookie_url_, "");

  NavigateFrameTo(first_party_cookie_url_);
  ExpectCookiesOnHost(cookie_iframe_url_, "name=acom");
  ExpectCookiesOnHost(first_party_cookie_url_, "name=acom");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ShieldsToggleBlockThirdPartyAllowSubdomain) {
  DefaultBlockAllCookies();

  BlockThirdPartyCookies(cookie_iframe_url_);
  NavigateToPageWithFrame(cookie_iframe_url_);
  NavigateFrameTo(subdomain_first_party_cookie_url_);

  ExpectCookiesOnHost(top_level_page_url_, "name=Good");
  ExpectCookiesOnHost(subdomain_first_party_cookie_url_, "name=subdomainacom");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ShieldsToggleBlockThirdPartyAllowDomainRegistry) {
  DefaultBlockAllCookies();

  BlockThirdPartyCookies(domain_registry_url_);
  NavigateToPageWithFrame(domain_registry_url_);
  NavigateFrameTo(iframe_domain_registry_url_);

  ExpectCookiesOnHost(domain_registry_url_, "name=blahtwittercom");
  ExpectCookiesOnHost(iframe_domain_registry_url_, "name=blahtwittercom");
}

// Test to ensure that we treat wp.com and wordpress.com as equal first parties
// for the purposes of ability to set / send storage.
// The following tests check each of the following.
//
// top level URL | iframe url    | iframe gets storage
// ---------------------------------------------------
// a.com         | wp.com        | no
// a.com         | wordpress.com | no
// wp.com        | a.com         | no
// wordpress.com | a.com         | no
// wp.com        | wordpress.com | yes
// wordpress.com | wp.com        | yes
IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyNoCookiesWpComInACom) {
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL("https://example.wp.com"), "");

  NavigateFrameTo(wp_frame_url_);
  ExpectCookiesOnHost(GURL("https://example.wp.com"), "");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyNoCookiesWordpressComInACom) {
  NavigateToPageWithFrame(https_cookie_iframe_url_);
  ExpectCookiesOnHost(GURL("https://example.wordpress.com"), "");

  NavigateFrameTo(wordpress_frame_url_);
  ExpectCookiesOnHost(GURL("https://example.wordpress.com"), "");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyNoCookiesAComInWpCom) {
  NavigateToPageWithFrame(wp_top_url_);
  ExpectCookiesOnHost(GURL("https://a.com"), "");

  NavigateFrameTo(a_frame_url_);
  ExpectCookiesOnHost(GURL("https://a.com"), "");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyNoCookiesAComInWordpressCom) {
  NavigateToPageWithFrame(wordpress_top_url_);
  ExpectCookiesOnHost(GURL("https://a.com"), "");

  NavigateFrameTo(a_frame_url_);
  ExpectCookiesOnHost(GURL("https://a.com"), "");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyYesCookiesWpComInWordpressCom) {
  NavigateToPageWithFrame(wordpress_top_url_);
  ExpectCookiesOnHost(GURL("https://example.wp.com"), "");

  NavigateFrameTo(wp_frame_url_);
  ExpectCookiesOnHost(GURL("https://example.wp.com"), "frame=true");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyYesCookiesnWordpressComInWpCom) {
  NavigateToPageWithFrame(wp_top_url_);
  ExpectCookiesOnHost(GURL("https://example.wordpress.com"), "");

  NavigateFrameTo(wordpress_frame_url_);
  ExpectCookiesOnHost(GURL("https://example.wordpress.com"), "frame=true");
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBrowserTest,
                       ThirdPartyYesNetworkCookieWpComInWordpressCom) {
  NavigateToPageWithFrame(wordpress_top_url_);
  ExpectCookiesOnHost(GURL("https://example.wp.com"), "");

  NavigateFrameTo(wp_frame_url_);
  ExpectCookiesOnHost(GURL("https://example.wp.com"), "frame=true");

  // No network cookie should be sent on first request.
  EXPECT_FALSE(seen_cookies().contains(wp_frame_url_));

  // Navigate from WordPress elsewhere.
  NavigateToPageWithFrame(cookie_iframe_url_);

  // Navigate to WordPress and to a friendly 3p frame to ensure network cookies
  // are sent from the frame.
  NavigateToPageWithFrame(wordpress_top_url_);
  NavigateFrameTo(wp_top_url_);

  ASSERT_TRUE(seen_cookies().contains(wp_top_url_));
  EXPECT_EQ(seen_cookies().at(wp_top_url_), "frame=true");
}
