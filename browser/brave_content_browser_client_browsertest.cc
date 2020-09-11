/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "net/dns/mock_host_resolver.h"

class BraveContentBrowserClientTest : public InProcessBrowserTest {
 public:
  void SetUp() override {
    // This is needed because component extensions are not added by default
    // without it.  Theyu are found to interfere with tests otherwise. It's
    // needed for loading the hangouts extension of which there are tests for
    // below.
    extensions::ComponentLoader::EnableBackgroundExtensionsForTesting();
    InProcessBrowserTest::SetUp();
  }
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
    magnet_url_ = GURL(
        "magnet:?xt=urn:btih:dd8255ecdc7ca55fb0bbf81323d87062db1f6d1c&dn=Big+"
        "Buck+Bunny&tr=udp%3A%2F%2Fexplodie.org%3A6969&tr=udp%3A%2F%2Ftracker."
        "coppersurfer.tk%3A6969&tr=udp%3A%2F%2Ftracker.empire-js.us%3A1337&tr="
        "udp%3A%2F%2Ftracker.leechers-paradise.org%3A6969&tr=udp%3A%2F%"
        "2Ftracker.opentrackr.org%3A1337&tr=wss%3A%2F%2Ftracker.btorrent.xyz&"
        "tr=wss%3A%2F%2Ftracker.fastcast.nz&tr=wss%3A%2F%2Ftracker."
        "openwebtorrent.com&ws=https%3A%2F%2Fwebtorrent.io%2Ftorrents%2F&xs="
        "https%3A%2F%2Fwebtorrent.io%2Ftorrents%2Fbig-buck-bunny.torrent");
    extension_url_ = GURL(
        "chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/"
        "brave_webtorrent.html?magnet%3A%3Fxt%3Durn%3Abtih%"
        "3Add8255ecdc7ca55fb0bbf81323d87062db1f6d1c%26dn%3DBig%2BBuck%2BBunny%"
        "26tr%3Dudp%253A%252F%252Fexplodie.org%253A6969%26tr%3Dudp%253A%252F%"
        "252Ftracker.coppersurfer.tk%253A6969%26tr%3Dudp%253A%252F%252Ftracker."
        "empire-js.us%253A1337%26tr%3Dudp%253A%252F%252Ftracker.leechers-"
        "paradise.org%253A6969%26tr%3Dudp%253A%252F%252Ftracker.opentrackr.org%"
        "253A1337%26tr%3Dwss%253A%252F%252Ftracker.btorrent.xyz%26tr%3Dwss%"
        "253A%252F%252Ftracker.fastcast.nz%26tr%3Dwss%253A%252F%252Ftracker."
        "openwebtorrent.com%26ws%3Dhttps%253A%252F%252Fwebtorrent.io%"
        "252Ftorrents%252F%26xs%3Dhttps%253A%252F%252Fwebtorrent.io%"
        "252Ftorrents%252Fbig-buck-bunny.torrent");
    torrent_url_ = GURL("https://webtorrent.io/torrents/sintel.torrent#ix=5");
    torrent_extension_url_ = GURL(
        "chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/"
        "brave_webtorrent.html?https://webtorrent.io/torrents/"
        "sintel.torrent#ix=5");
    torrent_invalid_query_extension_url_ = GURL(
        "chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/"
        "brave_webtorrent.html?chrome://settings");
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

  const GURL& magnet_html_url() { return magnet_html_url_; }
  const GURL& magnet_url() { return magnet_url_; }
  const GURL& extension_url() { return extension_url_; }
  const GURL& torrent_url() { return torrent_url_; }
  const GURL& torrent_extension_url() { return torrent_extension_url_; }
  const GURL& torrent_invalid_query_extension_url() {
    return torrent_invalid_query_extension_url_;
  }

  content::ContentBrowserClient* client() {
    return browser_content_client_.get();
  }

 private:
  GURL magnet_html_url_;
  GURL magnet_url_;
  GURL extension_url_;
  GURL torrent_url_;
  GURL torrent_extension_url_;
  GURL torrent_invalid_query_extension_url_;
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, CanLoadChromeURL) {
  std::vector<std::string> pages {
    chrome::kChromeUIWelcomeHost,
  };

  std::vector<std::string> schemes {
    "about:",
    "brave://",
    "chrome://",
  };

  for (const std::string& page : pages) {
    for (const std::string& scheme : schemes) {
      content::WebContents* contents =
          browser()->tab_strip_model()->GetActiveWebContents();
      ui_test_utils::NavigateToURL(browser(), GURL(scheme + page + "/"));
      ASSERT_TRUE(WaitForLoadStop(contents));

      EXPECT_STREQ(base::UTF16ToUTF8(browser()->location_bar_model()
                      ->GetFormattedFullURL()).c_str(),
                   ("brave://" + page).c_str());
      EXPECT_STREQ(contents->GetController().GetLastCommittedEntry()
                       ->GetVirtualURL().spec().c_str(),
                   ("chrome://" + page + "/").c_str());
      EXPECT_STREQ(contents->GetController().GetLastCommittedEntry()
                       ->GetURL().spec().c_str(),
                   ("chrome://" + page + "/").c_str());
    }
  }
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, CanLoadCustomBravePages) {
  std::vector<std::string> pages {
    "adblock",
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
        "rewards",
#endif
  };

  std::vector<std::string> schemes {
    "brave://",
    "chrome://",
  };

  for (const std::string& page : pages) {
    for (const std::string& scheme : schemes) {
      content::WebContents* contents =
          browser()->tab_strip_model()->GetActiveWebContents();
      ui_test_utils::NavigateToURL(browser(), GURL(scheme + page + "/"));
      ASSERT_TRUE(WaitForLoadStop(contents));

      EXPECT_STREQ(base::UTF16ToUTF8(browser()->location_bar_model()
                      ->GetFormattedFullURL()).c_str(),
                   ("brave://" + page).c_str());
      EXPECT_STREQ(contents->GetController().GetLastCommittedEntry()
                       ->GetVirtualURL().spec().c_str(),
                   ("chrome://" + page + "/").c_str());
      EXPECT_STREQ(contents->GetController().GetLastCommittedEntry()
                       ->GetURL().spec().c_str(),
                   ("chrome://" + page + "/").c_str());
    }
  }
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, CanLoadAboutHost) {
  std::vector<std::string> schemes {
    "chrome://",
    "brave://",
  };

  for (const std::string& scheme : schemes) {
    content::WebContents* contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    ui_test_utils::NavigateToURL(browser(), GURL(scheme + "about/"));
      ASSERT_TRUE(WaitForLoadStop(contents));

      EXPECT_STREQ(base::UTF16ToUTF8(browser()->location_bar_model()
                      ->GetFormattedFullURL()).c_str(),
                   "brave://about");
      EXPECT_STREQ(contents->GetController().GetLastCommittedEntry()
                       ->GetVirtualURL().spec().c_str(),
                   "chrome://about/");
      EXPECT_STREQ(contents->GetController().GetLastCommittedEntry()
                       ->GetURL().spec().c_str(),
                   "chrome://chrome-urls/");
  }
}
IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
                           RewriteChromeSync) {
  std::vector<std::string> schemes {
    "brave://",
    "chrome://",
  };

  for (const std::string& scheme : schemes) {
    content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
    ui_test_utils::NavigateToURL(browser(),
                                 GURL(scheme + chrome::kChromeUISyncHost));
    ASSERT_TRUE(WaitForLoadStop(contents));

    EXPECT_STREQ(base::UTF16ToUTF8(browser()->location_bar_model()
                                   ->GetFormattedFullURL()).c_str(),
                 "brave://sync");
    EXPECT_STREQ(contents->GetController().GetLastCommittedEntry()
                 ->GetVirtualURL().spec().c_str(),
                 "chrome://sync/");
    EXPECT_STREQ(contents->GetController().GetLastCommittedEntry()
                 ->GetURL().spec().c_str(),
                 "chrome://settings/braveSync");
  }
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, RewriteMagnetURLURLBar) {
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  ui_test_utils::NavigateToURL(browser(), magnet_url());
  ASSERT_TRUE(WaitForLoadStop(contents));
  EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
               magnet_url().spec().c_str())
      << "URL visible to users should stay as the magnet URL";
  content::NavigationEntry* entry =
      contents->GetController().GetLastCommittedEntry();
  EXPECT_STREQ(entry->GetURL().spec().c_str(), extension_url().spec().c_str())
      << "Real URL should be extension URL";
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, RewriteMagnetURLLink) {
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ui_test_utils::NavigateToURL(browser(), magnet_html_url());
  ASSERT_TRUE(WaitForLoadStop(contents));
  bool value;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents, "clickMagnetLink();", &value));
  EXPECT_TRUE(value);
  ASSERT_TRUE(WaitForLoadStop(contents));

  EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
               magnet_url().spec().c_str())
      << "URL visible to users should stay as the magnet URL";
  content::NavigationEntry* entry =
      contents->GetController().GetLastCommittedEntry();
  EXPECT_STREQ(entry->GetURL().spec().c_str(), extension_url().spec().c_str())
      << "Real URL should be extension URL";
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest, TypedMagnetURL) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::TestNavigationObserver observer(web_contents);
  ui_test_utils::SendToOmniboxAndSubmit(browser(), magnet_url().spec());
  observer.Wait();
  EXPECT_EQ(magnet_url(), web_contents->GetLastCommittedURL().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
                       ReverseRewriteTorrentURL) {
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Used to add the extension
  ui_test_utils::NavigateToURL(browser(), magnet_url());
  ASSERT_TRUE(WaitForLoadStop(contents));

  ui_test_utils::NavigateToURL(browser(), torrent_extension_url());
  ASSERT_TRUE(WaitForLoadStop(contents));

  EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
               torrent_url().spec().c_str())
      << "URL visible to users should stay as the torrent URL";
  content::NavigationEntry* entry =
      contents->GetController().GetLastCommittedEntry();
  EXPECT_STREQ(entry->GetURL().spec().c_str(),
               torrent_extension_url().spec().c_str())
      << "Real URL should be extension URL";
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
                       NoReverseRewriteTorrentURLForInvalidQuery) {
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Used to add the extension
  ui_test_utils::NavigateToURL(browser(), magnet_url());
  ASSERT_TRUE(WaitForLoadStop(contents));

  ui_test_utils::NavigateToURL(browser(),
                               torrent_invalid_query_extension_url());
  ASSERT_TRUE(WaitForLoadStop(contents));
  EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
               torrent_invalid_query_extension_url().spec().c_str())
      << "URL visible to users should stay as extension URL for invalid query";
  content::NavigationEntry* entry =
      contents->GetController().GetLastCommittedEntry();
  EXPECT_STREQ(entry->GetURL().spec().c_str(),
               torrent_invalid_query_extension_url().spec().c_str())
      << "Real URL should be extension URL";
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
                       WebTorrentExtensionEnabledAfterLoad) {
  ASSERT_TRUE(browser()->profile()->GetPrefs()->GetBoolean(kWebTorrentEnabled));

  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(browser()->profile());
  ASSERT_FALSE(
      registry->enabled_extensions().Contains(brave_webtorrent_extension_id));

  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ui_test_utils::NavigateToURL(browser(), magnet_url());
  WaitForLoadStop(contents);

  ASSERT_TRUE(
      registry->enabled_extensions().Contains(brave_webtorrent_extension_id));
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
                       PRE_NoRewriteMagnetURLURLBarWebTorrentDisabled) {
  browser()->profile()->GetPrefs()->SetBoolean(kWebTorrentEnabled, false);
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
                       NoRewriteMagnetURLURLBarWebTorrentDisabled) {
  ASSERT_FALSE(
      browser()->profile()->GetPrefs()->GetBoolean(kWebTorrentEnabled));
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(browser()->profile());
  ASSERT_FALSE(
      registry->enabled_extensions().Contains(brave_webtorrent_extension_id));

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
                       PRE_NoRewriteMagnetURLLinkWebTorrentDisabled) {
  browser()->profile()->GetPrefs()->SetBoolean(kWebTorrentEnabled, false);
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
                       NoRewriteMagnetURLLinkWebTorrentDisabled) {
  ASSERT_FALSE(
      browser()->profile()->GetPrefs()->GetBoolean(kWebTorrentEnabled));
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(browser()->profile());
  ASSERT_FALSE(
      registry->enabled_extensions().Contains(brave_webtorrent_extension_id));

  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ui_test_utils::NavigateToURL(browser(), magnet_html_url());
  ASSERT_TRUE(WaitForLoadStop(contents));
  bool value;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents, "clickMagnetLink();", &value));
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
                       PRE_NoReverseRewriteTorrentURLWebTorrentDisabled) {
  browser()->profile()->GetPrefs()->SetBoolean(kWebTorrentEnabled, false);
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
                       NoReverseRewriteTorrentURLWebTorrentDisabled) {
  ASSERT_FALSE(
      browser()->profile()->GetPrefs()->GetBoolean(kWebTorrentEnabled));
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(browser()->profile());
  ASSERT_FALSE(
      registry->enabled_extensions().Contains(brave_webtorrent_extension_id));

  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ui_test_utils::NavigateToURL(browser(), torrent_extension_url());
  WaitForLoadStop(contents);

  EXPECT_STREQ(contents->GetLastCommittedURL().spec().c_str(),
               torrent_extension_url().spec().c_str())
      << "No changes on the visible URL";
  content::NavigationEntry* entry =
      contents->GetController().GetLastCommittedEntry();
  EXPECT_STREQ(entry->GetURL().spec().c_str(),
               torrent_extension_url().spec().c_str())
      << "No changes on the real URL";
}

#if BUILDFLAG(ENABLE_HANGOUT_SERVICES_EXTENSION)
IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
                       HangoutsEnabledByDefault) {
  ASSERT_TRUE(browser()->profile()->GetPrefs()->GetBoolean(kHangoutsEnabled));
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(browser()->profile());
  ASSERT_TRUE(registry->enabled_extensions().Contains(hangouts_extension_id));
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
                       PRE_HangoutsDisabledDoesNotLoadComponent) {
  browser()->profile()->GetPrefs()->SetBoolean(kHangoutsEnabled, false);
}

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientTest,
                       HangoutsDisabledDoesNotLoadComponent) {
  ASSERT_FALSE(browser()->profile()->GetPrefs()->GetBoolean(kHangoutsEnabled));
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(browser()->profile());
  ASSERT_FALSE(registry->enabled_extensions().Contains(hangouts_extension_id));
}
#endif

class BraveContentBrowserClientReferrerTest
    : public BraveContentBrowserClientTest {
 public:
  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }
};

IN_PROC_BROWSER_TEST_F(BraveContentBrowserClientReferrerTest,
                       DefaultBehaviour) {
  const GURL kRequestUrl("http://request.com/path?query");
  const GURL kDocumentUrl("http://document.com/path?query");
  const GURL kSameSiteRequestUrl("http://sub.document.com/sub/path");
  const GURL kSameOriginRequestUrl("http://document.com/different/path");

  blink::mojom::ReferrerPtr kReferrer = blink::mojom::Referrer::New(
      kDocumentUrl, network::mojom::ReferrerPolicy::kDefault);

  // Cross-origin navigations don't get a referrer.
  blink::mojom::ReferrerPtr referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(),
                              kRequestUrl, kDocumentUrl, true, "GET",
                              &referrer);
  EXPECT_EQ(referrer->url, GURL());

  // Cross-origin navigations get a truncated referrer if method is not "GET" or
  // "HEAD".
  referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(),
                              kRequestUrl, kDocumentUrl, true, "POST",
                              &referrer);
  EXPECT_EQ(referrer->url, kDocumentUrl.GetOrigin());

  // Same-origin navigations get full referrers.
  referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(),
                              kSameOriginRequestUrl, kDocumentUrl, true,  "GET",
                              &referrer);
  EXPECT_EQ(referrer->url, kDocumentUrl);

  // Same-site navigations get truncated referrers.
  referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(),
                              kSameSiteRequestUrl, kDocumentUrl, true,  "GET",
                              &referrer);
  EXPECT_EQ(referrer->url, kDocumentUrl.GetOrigin());

  // Cross-origin iframe navigations get origins.
  referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(),
                              kRequestUrl, kDocumentUrl, false, "GET",
                              &referrer);
  EXPECT_EQ(referrer->url, kDocumentUrl.GetOrigin().spec());

  // Same-origin iframe navigations get full referrers.
  referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(),
                              kSameOriginRequestUrl, kDocumentUrl, false, "GET",
                              &referrer);
  EXPECT_EQ(referrer->url, kDocumentUrl);

  // Special rule for extensions.
  const GURL kExtensionUrl("chrome-extension://abc/path?query");
  referrer = kReferrer.Clone();
  referrer->url = kExtensionUrl;
  client()->MaybeHideReferrer(browser()->profile(),
                              kRequestUrl, kExtensionUrl, true, "GET",
                              &referrer);
  EXPECT_EQ(referrer->url, kExtensionUrl);

  // Allow referrers for certain URL.
  content_settings()->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString(kDocumentUrl.GetOrigin().spec() + "*"),
      ContentSettingsPattern::Wildcard(),
      ContentSettingsType::PLUGINS,
      brave_shields::kReferrers, CONTENT_SETTING_ALLOW);
  referrer = kReferrer.Clone();
  client()->MaybeHideReferrer(browser()->profile(),
                              kRequestUrl, kDocumentUrl, true, "GET",
                              &referrer);
  EXPECT_EQ(referrer->url, kDocumentUrl);
}
