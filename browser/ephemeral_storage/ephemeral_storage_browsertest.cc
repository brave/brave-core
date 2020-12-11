/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/features.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "url/gurl.h"

using content::RenderFrameHost;
using content::WebContents;
using net::test_server::EmbeddedTestServer;

namespace {

enum StorageType { Session, Local };

const char* ToString(StorageType storage_type) {
  switch (storage_type) {
    case StorageType::Session:
      return "session";
    case StorageType::Local:
      return "local";
  }
}

void SetStorageValueInFrame(RenderFrameHost* host,
                            std::string value,
                            StorageType storage_type) {
  std::string script =
      base::StringPrintf("%sStorage.setItem('storage_key', '%s');",
                         ToString(storage_type), value.c_str());
  ASSERT_TRUE(content::ExecuteScript(host, script));
}

content::EvalJsResult GetStorageValueInFrame(RenderFrameHost* host,
                                             StorageType storage_type) {
  std::string script = base::StringPrintf("%sStorage.getItem('storage_key');",
                                          ToString(storage_type));
  return content::EvalJs(host, script);
}

void SetCookieInFrame(RenderFrameHost* host, std::string cookie) {
  std::string script = base::StringPrintf(
      "document.cookie='%s; path=/; SameSite=None; Secure'", cookie.c_str());
  ASSERT_TRUE(content::ExecuteScript(host, script));
}

content::EvalJsResult GetCookiesInFrame(RenderFrameHost* host) {
  return content::EvalJs(host, "document.cookie");
}

}  // namespace

class EphemeralStorageBrowserTest : public InProcessBrowserTest {
 public:
  EphemeralStorageBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitAndEnableFeature(
        net::features::kBraveEphemeralStorage);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    content::SetupCrossSiteRedirector(&https_server_);

    ASSERT_TRUE(https_server_.Start());
    a_site_ephemeral_storage_url_ =
        https_server_.GetURL("a.com", "/ephemeral_storage.html");
    b_site_ephemeral_storage_url_ =
        https_server_.GetURL("b.com", "/ephemeral_storage.html");
    c_site_ephemeral_storage_url_ =
        https_server_.GetURL("c.com", "/ephemeral_storage.html");
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);

    // This is needed to load pages from "domain.com" without an interstitial.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void AllowAllCookies() {
    auto* content_settings =
        HostContentSettingsMapFactory::GetForProfile(browser()->profile());
    brave_shields::SetCookieControlType(
        content_settings, brave_shields::ControlType::ALLOW, GURL());
  }

  void SetValuesInFrames(WebContents* web_contents,
                         std::string storage_value,
                         std::string cookie_value) {
    auto set_values_in_frame = [&](RenderFrameHost* frame) {
      SetStorageValueInFrame(frame, storage_value, StorageType::Local);
      SetStorageValueInFrame(frame, storage_value, StorageType::Session);
      SetCookieInFrame(frame, cookie_value);
    };

    RenderFrameHost* main_frame = web_contents->GetMainFrame();
    set_values_in_frame(main_frame);
    set_values_in_frame(content::ChildFrameAt(main_frame, 0));
    set_values_in_frame(content::ChildFrameAt(main_frame, 1));
  }

  struct ValuesFromFrame {
    content::EvalJsResult local_storage;
    content::EvalJsResult session_storage;
    content::EvalJsResult cookies;
  };

  ValuesFromFrame GetValuesFromFrame(RenderFrameHost* frame) {
    return {
        GetStorageValueInFrame(frame, StorageType::Local),
        GetStorageValueInFrame(frame, StorageType::Session),
        GetCookiesInFrame(frame),
    };
  }

  struct ValuesFromFrames {
    ValuesFromFrame main_frame;
    ValuesFromFrame iframe_1;
    ValuesFromFrame iframe_2;
  };

  ValuesFromFrames GetValuesFromFrames(WebContents* web_contents) {
    RenderFrameHost* main_frame = web_contents->GetMainFrame();
    return {
        GetValuesFromFrame(main_frame),
        GetValuesFromFrame(content::ChildFrameAt(main_frame, 0)),
        GetValuesFromFrame(content::ChildFrameAt(main_frame, 1)),
    };
  }

  WebContents* LoadURLInNewTab(GURL url) {
    ui_test_utils::AllBrowserTabAddedWaiter add_tab;
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    return add_tab.Wait();
  }

 protected:
  net::test_server::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList scoped_feature_list_;
  GURL a_site_ephemeral_storage_url_;
  GURL b_site_ephemeral_storage_url_;
  GURL c_site_ephemeral_storage_url_;

 private:
  DISALLOW_COPY_AND_ASSIGN(EphemeralStorageBrowserTest);
};

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest, StorageIsPartitioned) {
  AllowAllCookies();

  WebContents* first_party_tab = LoadURLInNewTab(b_site_ephemeral_storage_url_);
  WebContents* site_a_tab1 = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  WebContents* site_a_tab2 = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  WebContents* site_c_tab = LoadURLInNewTab(c_site_ephemeral_storage_url_);

  EXPECT_EQ(browser()->tab_strip_model()->count(), 5);

  // We set a value in the page where all the frames are first-party.
  SetValuesInFrames(first_party_tab, "b.com - first party", "from=b.com");

  // The page this tab is loaded via a.com and has two b.com third-party
  // iframes. The third-party iframes should have ephemeral storage. That means
  // that their values should be shared by third-party b.com iframes loaded from
  // a.com.
  SetValuesInFrames(site_a_tab1, "a.com", "from=a.com");
  ValuesFromFrames site_a_tab1_values = GetValuesFromFrames(site_a_tab1);
  EXPECT_EQ("a.com", site_a_tab1_values.main_frame.local_storage);
  EXPECT_EQ("a.com", site_a_tab1_values.iframe_1.local_storage);
  EXPECT_EQ("a.com", site_a_tab1_values.iframe_2.local_storage);

  EXPECT_EQ("a.com", site_a_tab1_values.main_frame.session_storage);
  EXPECT_EQ("a.com", site_a_tab1_values.iframe_1.session_storage);
  EXPECT_EQ("a.com", site_a_tab1_values.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", site_a_tab1_values.main_frame.cookies);
  EXPECT_EQ("from=a.com", site_a_tab1_values.iframe_1.cookies);
  EXPECT_EQ("from=a.com", site_a_tab1_values.iframe_2.cookies);

  // The second tab is loaded on the same domain, so should see the same
  // storage for the third-party iframes.
  ValuesFromFrames site_a_tab2_values = GetValuesFromFrames(site_a_tab2);
  EXPECT_EQ("a.com", site_a_tab2_values.main_frame.local_storage);
  EXPECT_EQ("a.com", site_a_tab2_values.iframe_1.local_storage);
  EXPECT_EQ("a.com", site_a_tab2_values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, site_a_tab2_values.main_frame.session_storage);
  EXPECT_EQ(nullptr, site_a_tab2_values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, site_a_tab2_values.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", site_a_tab2_values.main_frame.cookies);
  EXPECT_EQ("from=a.com", site_a_tab2_values.iframe_1.cookies);
  EXPECT_EQ("from=a.com", site_a_tab2_values.iframe_2.cookies);

  // The storage in the first-party iframes should still reflect the
  // original value that was written in the non-ephemeral storage area.
  ValuesFromFrames first_party_values = GetValuesFromFrames(first_party_tab);
  EXPECT_EQ("b.com - first party", first_party_values.main_frame.local_storage);
  EXPECT_EQ("b.com - first party", first_party_values.iframe_1.local_storage);
  EXPECT_EQ("b.com - first party", first_party_values.iframe_2.local_storage);

  EXPECT_EQ("b.com - first party",
            first_party_values.main_frame.session_storage);
  EXPECT_EQ("b.com - first party", first_party_values.iframe_1.session_storage);
  EXPECT_EQ("b.com - first party", first_party_values.iframe_2.session_storage);

  EXPECT_EQ("from=b.com", first_party_values.main_frame.cookies);
  EXPECT_EQ("from=b.com", first_party_values.iframe_1.cookies);
  EXPECT_EQ("from=b.com", first_party_values.iframe_2.cookies);

  // Even though this page loads b.com iframes as third-party iframes, the TLD
  // differs, so it should get an entirely different ephemeral storage area.
  ValuesFromFrames site_c_tab_values = GetValuesFromFrames(site_c_tab);
  EXPECT_EQ(nullptr, site_c_tab_values.main_frame.local_storage);
  EXPECT_EQ(nullptr, site_c_tab_values.iframe_1.local_storage);
  EXPECT_EQ(nullptr, site_c_tab_values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, site_c_tab_values.main_frame.session_storage);
  EXPECT_EQ(nullptr, site_c_tab_values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, site_c_tab_values.iframe_2.session_storage);

  EXPECT_EQ("", site_c_tab_values.main_frame.cookies);
  EXPECT_EQ("", site_c_tab_values.iframe_1.cookies);
  EXPECT_EQ("", site_c_tab_values.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       NavigatingClearsEphemeralStorage) {
  AllowAllCookies();

  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  SetValuesInFrames(web_contents, "a.com value", "from=a.com");

  ValuesFromFrames values_before = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_before.main_frame.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_before.main_frame.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", values_before.main_frame.cookies);
  EXPECT_EQ("from=a.com", values_before.iframe_1.cookies);
  EXPECT_EQ("from=a.com", values_before.iframe_2.cookies);

  // Navigate away and then navigate back to the original site.
  ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_);
  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);

  ValuesFromFrames values_after = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_after.main_frame.local_storage);
  EXPECT_EQ(nullptr, values_after.iframe_1.local_storage);
  EXPECT_EQ(nullptr, values_after.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_after.main_frame.session_storage);
  EXPECT_EQ(nullptr, values_after.iframe_1.session_storage);
  EXPECT_EQ(nullptr, values_after.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", values_after.main_frame.cookies);
  EXPECT_EQ("", values_after.iframe_1.cookies);
  EXPECT_EQ("", values_after.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       ClosingTabClearsEphemeralStorage) {
  AllowAllCookies();

  WebContents* site_a_tab = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  EXPECT_EQ(browser()->tab_strip_model()->count(), 2);

  SetValuesInFrames(site_a_tab, "a.com value", "from=a.com");

  ValuesFromFrames values_before = GetValuesFromFrames(site_a_tab);
  EXPECT_EQ("a.com value", values_before.main_frame.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_before.main_frame.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", values_before.main_frame.cookies);
  EXPECT_EQ("from=a.com", values_before.iframe_1.cookies);
  EXPECT_EQ("from=a.com", values_before.iframe_2.cookies);

  // Close the new tab which we set ephemeral storage value in. This should
  // clear the ephemeral storage since this is the last tab which has a.com as
  // an eTLD.
  int tab_index =
      browser()->tab_strip_model()->GetIndexOfWebContents(site_a_tab);
  bool was_closed = browser()->tab_strip_model()->CloseWebContentsAt(
      tab_index, TabStripModel::CloseTypes::CLOSE_NONE);
  EXPECT_TRUE(was_closed);

  // Navigate the main tab to the same site.
  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  // Closing the tab earlier should have cleared the ephemeral storage area.
  ValuesFromFrames values_after = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_after.main_frame.local_storage);
  EXPECT_EQ(nullptr, values_after.iframe_1.local_storage);
  EXPECT_EQ(nullptr, values_after.iframe_2.local_storage);

  EXPECT_EQ(nullptr, values_after.main_frame.session_storage);
  EXPECT_EQ(nullptr, values_after.iframe_1.session_storage);
  EXPECT_EQ(nullptr, values_after.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", values_after.main_frame.cookies);
  EXPECT_EQ("", values_after.iframe_1.cookies);
  EXPECT_EQ("", values_after.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       ReloadDoesNotClearEphemeralStorage) {
  AllowAllCookies();

  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  SetValuesInFrames(web_contents, "a.com value", "from=a.com");

  ValuesFromFrames values_before = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_before.main_frame.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_before.main_frame.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", values_before.main_frame.cookies);
  EXPECT_EQ("from=a.com", values_before.iframe_1.cookies);
  EXPECT_EQ("from=a.com", values_before.iframe_2.cookies);

  // Reload the page.
  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);

  ValuesFromFrames values_after = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_after.main_frame.local_storage);
  EXPECT_EQ("a.com value", values_after.iframe_1.local_storage);
  EXPECT_EQ("a.com value", values_after.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_after.main_frame.session_storage);
  EXPECT_EQ("a.com value", values_after.iframe_1.session_storage);
  EXPECT_EQ("a.com value", values_after.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", values_after.main_frame.cookies);
  EXPECT_EQ("from=a.com", values_after.iframe_1.cookies);
  EXPECT_EQ("from=a.com", values_after.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       EphemeralStorageDoesNotLeakBetweenProfiles) {
  AllowAllCookies();

  ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_);
  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  SetValuesInFrames(web_contents, "a.com value", "from=a.com");

  ValuesFromFrames values_before = GetValuesFromFrames(web_contents);
  EXPECT_EQ("a.com value", values_before.main_frame.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.local_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.local_storage);

  EXPECT_EQ("a.com value", values_before.main_frame.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_1.session_storage);
  EXPECT_EQ("a.com value", values_before.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", values_before.main_frame.cookies);
  EXPECT_EQ("from=a.com", values_before.iframe_1.cookies);
  EXPECT_EQ("from=a.com", values_before.iframe_2.cookies);

  // A browser with the same profile should share all values with the
  // first browser, including ephemeral storage values.
  Browser* same_profile_browser = CreateBrowser(browser()->profile());
  ui_test_utils::NavigateToURL(same_profile_browser,
                               a_site_ephemeral_storage_url_);
  auto* same_profile_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  ValuesFromFrames same_profile_values =
      GetValuesFromFrames(same_profile_web_contents);
  EXPECT_EQ("a.com value", same_profile_values.main_frame.local_storage);
  EXPECT_EQ("a.com value", same_profile_values.iframe_1.local_storage);
  EXPECT_EQ("a.com value", same_profile_values.iframe_2.local_storage);

  EXPECT_EQ("a.com value", same_profile_values.main_frame.session_storage);
  EXPECT_EQ("a.com value", same_profile_values.iframe_1.session_storage);
  EXPECT_EQ("a.com value", same_profile_values.iframe_2.session_storage);

  EXPECT_EQ("from=a.com", same_profile_values.main_frame.cookies);
  EXPECT_EQ("from=a.com", same_profile_values.iframe_1.cookies);
  EXPECT_EQ("from=a.com", same_profile_values.iframe_2.cookies);

  // A browser with a different profile shouldn't share any values with
  // the first set of browsers.
  Browser* private_browser = CreateIncognitoBrowser(nullptr);
  ui_test_utils::NavigateToURL(private_browser, a_site_ephemeral_storage_url_);
  auto* private_web_contents =
      private_browser->tab_strip_model()->GetActiveWebContents();

  ValuesFromFrames private_values = GetValuesFromFrames(private_web_contents);
  EXPECT_EQ(nullptr, private_values.main_frame.local_storage);
  EXPECT_EQ(nullptr, private_values.iframe_1.local_storage);
  EXPECT_EQ(nullptr, private_values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, private_values.main_frame.session_storage);
  EXPECT_EQ(nullptr, private_values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, private_values.iframe_2.session_storage);

  EXPECT_EQ("", private_values.main_frame.cookies);
  EXPECT_EQ("", private_values.iframe_1.cookies);
  EXPECT_EQ("", private_values.iframe_2.cookies);
}
