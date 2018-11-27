/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_paths.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "net/dns/mock_host_resolver.h"

class BraveContentBrowserClientTest : public InProcessBrowserTest {
  public:
    void SetUpOnMainThread() override {
      InProcessBrowserTest::SetUpOnMainThread();

      content_client_.reset(new ChromeContentClient);
      content::SetContentClient(content_client_.get());
      browser_content_client_.reset(new BraveContentBrowserClient());
      content::SetBrowserClientForTesting(browser_content_client_.get());

      host_resolver()->AddRule("*", "127.0.0.1");
      content::SetupCrossSiteRedirector(embedded_test_server());

      brave::RegisterPathProvider();
      base::FilePath test_data_dir;
      base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
      embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

      ASSERT_TRUE(embedded_test_server()->Start());

      magnet_html_url_ = embedded_test_server()->GetURL("a.com", "/magnet.html");
      magnet_url_ = GURL("magnet:?xt=urn:btih:dd8255ecdc7ca55fb0bbf81323d87062db1f6d1c&dn=Big+Buck+Bunny&tr=udp%3A%2F%2Fexplodie.org%3A6969&tr=udp%3A%2F%2Ftracker.coppersurfer.tk%3A6969&tr=udp%3A%2F%2Ftracker.empire-js.us%3A1337&tr=udp%3A%2F%2Ftracker.leechers-paradise.org%3A6969&tr=udp%3A%2F%2Ftracker.opentrackr.org%3A1337&tr=wss%3A%2F%2Ftracker.btorrent.xyz&tr=wss%3A%2F%2Ftracker.fastcast.nz&tr=wss%3A%2F%2Ftracker.openwebtorrent.com&ws=https%3A%2F%2Fwebtorrent.io%2Ftorrents%2F&xs=https%3A%2F%2Fwebtorrent.io%2Ftorrents%2Fbig-buck-bunny.torrent");
      extension_url_ = GURL("chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/brave_webtorrent.html?magnet%3A%3Fxt%3Durn%3Abtih%3Add8255ecdc7ca55fb0bbf81323d87062db1f6d1c%26dn%3DBig%2BBuck%2BBunny%26tr%3Dudp%253A%252F%252Fexplodie.org%253A6969%26tr%3Dudp%253A%252F%252Ftracker.coppersurfer.tk%253A6969%26tr%3Dudp%253A%252F%252Ftracker.empire-js.us%253A1337%26tr%3Dudp%253A%252F%252Ftracker.leechers-paradise.org%253A6969%26tr%3Dudp%253A%252F%252Ftracker.opentrackr.org%253A1337%26tr%3Dwss%253A%252F%252Ftracker.btorrent.xyz%26tr%3Dwss%253A%252F%252Ftracker.fastcast.nz%26tr%3Dwss%253A%252F%252Ftracker.openwebtorrent.com%26ws%3Dhttps%253A%252F%252Fwebtorrent.io%252Ftorrents%252F%26xs%3Dhttps%253A%252F%252Fwebtorrent.io%252Ftorrents%252Fbig-buck-bunny.torrent");
      torrent_url_ = GURL("https://webtorrent.io/torrents/sintel.torrent#ix=5");
      torrent_extension_url_ = GURL("chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/brave_webtorrent.html?https://webtorrent.io/torrents/sintel.torrent#ix=5");
    }

    void TearDown() override {
      browser_content_client_.reset();
      content_client_.reset();
    }

    void DisableWebTorrent() {
      extensions::ExtensionService* service = extensions::ExtensionSystem::Get(
          browser()->profile())->extension_service();
      service->DisableExtension(brave_webtorrent_extension_id,
          extensions::disable_reason::DisableReason::DISABLE_BLOCKED_BY_POLICY);
    }

    const GURL& magnet_html_url() { return magnet_html_url_; }
    const GURL& magnet_url() { return magnet_url_; }
    const GURL& extension_url() { return extension_url_; }
    const GURL& torrent_url() { return torrent_url_; }
    const GURL& torrent_extension_url() { return torrent_extension_url_; }

  private:
    GURL magnet_html_url_;
    GURL magnet_url_;
    GURL extension_url_;
    GURL torrent_url_;
    GURL torrent_extension_url_;
    ContentSettingsPattern top_level_page_pattern_;
    ContentSettingsPattern empty_pattern_;
    std::unique_ptr<ChromeContentClient> content_client_;
    std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, CanLoadChromeURL) {
  GURL chrome_settings_url("chrome://settings/");
  std::vector<GURL> urls {
    chrome_settings_url,
    GURL("about:settings/"),
    GURL("brave://settings/")
  };
  std::for_each(urls.begin(), urls.end(), [this, &chrome_settings_url](const GURL& url) {
    content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
    ui_test_utils::NavigateToURL(browser(), url);
    ASSERT_TRUE(WaitForLoadStop(contents));
    EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
        chrome_settings_url.spec().c_str());
  });
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, CanLoadCustomBravePages) {
  std::vector<GURL> urls {
    GURL("chrome://adblock/"),
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
    GURL("chrome://rewards/"),
#endif
    GURL("chrome://welcome/")
  };
  std::for_each(urls.begin(), urls.end(), [this](const GURL& url) {
    content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
    ui_test_utils::NavigateToURL(browser(), url);
    ASSERT_TRUE(WaitForLoadStop(contents));
    EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
        url.spec().c_str());
  });
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, RewriteMagnetURLURLBar) {
  content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();

  ui_test_utils::NavigateToURL(browser(), magnet_url());
  ASSERT_TRUE(WaitForLoadStop(contents));
  EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
      magnet_url().spec().c_str()) << "URL visible to users should stay as the magnet URL";
  content::NavigationEntry* entry = contents->GetController().GetLastCommittedEntry();
  EXPECT_STREQ(entry->GetURL().spec().c_str(),
      extension_url().spec().c_str()) << "Real URL should be extension URL";
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, RewriteMagnetURLLink) {
  content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  ui_test_utils::NavigateToURL(browser(), magnet_html_url());
  ASSERT_TRUE(WaitForLoadStop(contents));
  bool value;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents, "clickMagnetLink();",
        &value));
  EXPECT_TRUE(value);
  ASSERT_TRUE(WaitForLoadStop(contents));

  EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
      magnet_url().spec().c_str()) << "URL visible to users should stay as the magnet URL";
  content::NavigationEntry* entry = contents->GetController().GetLastCommittedEntry();
  EXPECT_STREQ(entry->GetURL().spec().c_str(),
      extension_url().spec().c_str()) << "Real URL should be extension URL";
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, TypedMagnetURL) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::TestNavigationObserver observer(web_contents);
  LocationBar* location_bar = browser()->window()->GetLocationBar();
  ui_test_utils::SendToOmniboxAndSubmit(location_bar, magnet_url().spec());
  observer.Wait();
  EXPECT_EQ(magnet_url(), web_contents->GetLastCommittedURL().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, ReverseRewriteTorrentURL) {
  content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  ui_test_utils::NavigateToURL(browser(), torrent_extension_url());
  ASSERT_TRUE(WaitForLoadStop(contents));

  EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
      torrent_url().spec().c_str()) << "URL visible to users should stay as the torrent URL";
  content::NavigationEntry* entry = contents->GetController().GetLastCommittedEntry();
  EXPECT_STREQ(entry->GetURL().spec().c_str(),
      torrent_extension_url().spec().c_str()) << "Real URL should be extension URL";
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
    NoRewriteMagnetURLURLBarWebTorrentDisabled) {
  DisableWebTorrent();
  extensions::ExtensionRegistry* registry =
    extensions::ExtensionRegistry::Get(browser()->profile());
  ASSERT_TRUE(registry->disabled_extensions().Contains(
        brave_webtorrent_extension_id));

  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();
  ui_test_utils::NavigateToURL(browser(), magnet_url());
  ASSERT_TRUE(WaitForLoadStop(contents));
  EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(), "about:blank");
  content::NavigationEntry* entry =
    contents->GetController().GetLastCommittedEntry();
  EXPECT_STREQ(entry->GetURL().spec().c_str(), "about:blank");
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
    NoRewriteMagnetURLLinkWebTorrentDisabled) {
  DisableWebTorrent();
  extensions::ExtensionRegistry* registry =
    extensions::ExtensionRegistry::Get(browser()->profile());
  ASSERT_TRUE(registry->disabled_extensions().Contains(
        brave_webtorrent_extension_id));

  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();
  ui_test_utils::NavigateToURL(browser(), magnet_html_url());
  ASSERT_TRUE(WaitForLoadStop(contents));
  bool value;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents, "clickMagnetLink();",
        &value));
  EXPECT_TRUE(value);
  ASSERT_TRUE(WaitForLoadStop(contents));

  EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
      magnet_html_url().spec().c_str());
  content::NavigationEntry* entry =
    contents->GetController().GetLastCommittedEntry();
  EXPECT_STREQ(entry->GetURL().spec().c_str(),
      magnet_html_url().spec().c_str());
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
    NoReverseRewriteTorrentURLWebTorrentDisabled) {
  DisableWebTorrent();
  extensions::ExtensionRegistry* registry =
    extensions::ExtensionRegistry::Get(browser()->profile());
  ASSERT_TRUE(registry->disabled_extensions().Contains(
        brave_webtorrent_extension_id));

  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();
  ui_test_utils::NavigateToURL(browser(), torrent_extension_url());
  WaitForLoadStop(contents);

  EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
      torrent_extension_url().spec().c_str()) << "No changes on the visible URL";
  content::NavigationEntry* entry =
    contents->GetController().GetLastCommittedEntry();
  EXPECT_STREQ(entry->GetURL().spec().c_str(),
      torrent_extension_url().spec().c_str()) << "No changes on the real URL";
}
