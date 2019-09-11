/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "url/gurl.h"

class BraveNetworkDelegateBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    url_ = embedded_test_server()->GetURL("a.com", "/nested_iframe.html");
    nested_iframe_script_url_ =
        embedded_test_server()->GetURL("a.com", "/nested_iframe_script.html");

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
    top_level_page_pattern_ =
        ContentSettingsPattern::FromString("http://a.com/*");
    first_party_pattern_ =
        ContentSettingsPattern::FromString("https://firstParty/*");
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowCookies() {
    brave_shields::SetCookieControlType(browser()->profile(),
                                        brave_shields::ControlType::ALLOW,
                                        top_level_page_url_);
  }

 protected:
  GURL url_;
  GURL nested_iframe_script_url_;

 private:
  GURL top_level_page_url_;
  ContentSettingsPattern top_level_page_pattern_;
  ContentSettingsPattern first_party_pattern_;
  ContentSettingsPattern iframe_pattern_;
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
  AllowCookies();
  ui_test_utils::NavigateToURL(browser(), url_);
  const std::string cookie =
      content::GetCookies(browser()->profile(), GURL("http://c.com/"));
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
  AllowCookies();
  ui_test_utils::NavigateToURL(browser(), nested_iframe_script_url_);
  const std::string cookie =
      content::GetCookies(browser()->profile(), GURL("http://c.com/"));
  EXPECT_FALSE(cookie.empty());
}
