/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using content::NavigationHandle;
using content::WebContents;
using content::WebContentsObserver;

class RedirectObserver : public WebContentsObserver {
 public:
  explicit RedirectObserver(WebContents* web_contents)
      : WebContentsObserver(web_contents) {}
  RedirectObserver(const RedirectObserver&) = delete;
  RedirectObserver& operator=(const RedirectObserver&) = delete;
  ~RedirectObserver() override = default;

  void DidFinishNavigation(NavigationHandle* handle) override {
    const net::HttpResponseHeaders* response = handle->GetResponseHeaders();
    if (response) {
      const bool has_sts_header =
          response->HasHeader("Strict-Transport-Security");
      sts_header_for_url_.insert(
          std::pair<GURL, bool>(handle->GetURL(), has_sts_header));
    }
  }

  bool has_sts_header(GURL url) const {
    auto iter = sts_header_for_url_.find(url);
    DCHECK(iter != sts_header_for_url_.end());
    return iter->second;
  }

 private:
  std::map<GURL, bool> sts_header_for_url_;
};

class BraveNetworkDelegateBaseBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
    first_party_pattern_ = ContentSettingsPattern::FromString("http://a.com/*");
    iframe_pattern_ = ContentSettingsPattern::FromString("http://c.com/*");
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

 private:
  ContentSettingsPattern first_party_pattern_;
  ContentSettingsPattern iframe_pattern_;
};

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBaseBrowserTest, FirstPartySTS) {
  const GURL third_party =
      embedded_test_server()->GetURL("c.com", "/iframe_hsts.html");

  RedirectObserver redirect_observer(active_contents());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), third_party));

  EXPECT_TRUE(redirect_observer.has_sts_header(third_party));
}

IN_PROC_BROWSER_TEST_F(BraveNetworkDelegateBaseBrowserTest, ThirdPartySTS) {
  const GURL third_party =
      embedded_test_server()->GetURL("c.com", "/iframe_hsts.html");
  const GURL first_party =
      embedded_test_server()->GetURL("a.com", "/hsts.html");

  RedirectObserver redirect_observer(active_contents());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), first_party));

  EXPECT_FALSE(redirect_observer.has_sts_header(third_party));
}
