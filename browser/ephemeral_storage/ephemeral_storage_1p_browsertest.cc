/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"

#include "base/strings/strcat.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "content/public/test/browser_test.h"
#include "net/base/features.h"

using content::RenderFrameHost;
using content::WebContents;

class EphemeralStorage1pBrowserTest : public EphemeralStorageBrowserTest {
 public:
  EphemeralStorage1pBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        net::features::kBraveFirstPartyEphemeralStorage);
  }
  ~EphemeralStorage1pBrowserTest() override {}

  void SetCookieSetting(const GURL& url, ContentSetting content_setting) {
    auto* host_content_settings_map =
        HostContentSettingsMapFactory::GetForProfile(browser()->profile());
    host_content_settings_map->SetContentSettingCustomScope(
        ContentSettingsPattern::FromString(
            base::StrCat({"[*.]", url.host_piece(), ":*"})),
        ContentSettingsPattern::Wildcard(), ContentSettingsType::COOKIES,
        content_setting);
  }

  // Helper to load easy-to-use Indexed DB API.
  void LoadIndexedDbHelper(RenderFrameHost* host) {
    const char kLoadIndexMinScript[] =
        "new Promise((resolve) => {"
        "  const script = document.createElement('script');"
        "  script.onload = () => {"
        "    resolve(true);"
        "  };"
        "  script.onerror = () => {"
        "    resolve(false);"
        "  };"
        "  script.src = '/ephemeral-storage/static/js/libs/index-min.js';"
        "  document.body.appendChild(script);"
        "});";

    ASSERT_EQ(true, content::EvalJs(host, kLoadIndexMinScript));
  }

  bool SetIDBValue(RenderFrameHost* host) {
    LoadIndexedDbHelper(host);
    content::EvalJsResult eval_js_result = content::EvalJs(
        host, "(async () => { await window.idbKeyval.set('a', 'a'); })()");
    return eval_js_result.error.empty();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(EphemeralStorage1pBrowserTest, FirstPartyIsEphemeral) {
  SetCookieSetting(a_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);

  WebContents* first_party_tab = LoadURLInNewTab(a_site_ephemeral_storage_url_);

  // We set a value in the page where all the frames are first-party.
  SetValuesInFrames(first_party_tab, "a.com", "from=a.com");

  {
    ValuesFromFrames first_party_values = GetValuesFromFrames(first_party_tab);
    EXPECT_EQ("a.com", first_party_values.main_frame.local_storage);
    EXPECT_EQ("a.com", first_party_values.iframe_1.local_storage);
    EXPECT_EQ("a.com", first_party_values.iframe_2.local_storage);

    EXPECT_EQ("a.com", first_party_values.main_frame.session_storage);
    EXPECT_EQ("a.com", first_party_values.iframe_1.session_storage);
    EXPECT_EQ("a.com", first_party_values.iframe_2.session_storage);

    EXPECT_EQ("from=a.com", first_party_values.main_frame.cookies);
    EXPECT_EQ("from=a.com", first_party_values.iframe_1.cookies);
    EXPECT_EQ("from=a.com", first_party_values.iframe_2.cookies);
  }

  // After keepalive values should be cleared.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), b_site_ephemeral_storage_url_));
  WaitForCleanupAfterKeepAlive();
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

  ExpectValuesFromFramesAreEmpty(FROM_HERE,
                                 GetValuesFromFrames(first_party_tab));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorage1pBrowserTest,
                       StorageIsPartitionedAndCleared) {
  SetCookieSetting(a_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);
  SetCookieSetting(b_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);
  SetCookieSetting(c_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);

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
  ExpectValuesFromFramesAreEmpty(FROM_HERE, site_c_tab_values);

  // Close 4 tabs.
  for (int i = 0; i < 4; ++i) {
    browser()->tab_strip_model()->CloseWebContentsAt(1,
                                                     TabStripModel::CLOSE_NONE);
  }

  WaitForCleanupAfterKeepAlive();

  ExpectValuesFromFramesAreEmpty(
      FROM_HERE,
      GetValuesFromFrames(LoadURLInNewTab(a_site_ephemeral_storage_url_)));
  ExpectValuesFromFramesAreEmpty(
      FROM_HERE,
      GetValuesFromFrames(LoadURLInNewTab(b_site_ephemeral_storage_url_)));
  ExpectValuesFromFramesAreEmpty(
      FROM_HERE,
      GetValuesFromFrames(LoadURLInNewTab(c_site_ephemeral_storage_url_)));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorage1pBrowserTest,
                       IndexedDbUnavailableInES) {
  SetCookieSetting(a_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);
  SetCookieSetting(b_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);

  WebContents* site_a = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  WebContents* site_b = LoadURLInNewTab(b_site_ephemeral_storage_url_);

  // Main frame and 1p frame.
  EXPECT_FALSE(SetIDBValue(site_a->GetMainFrame()));
  EXPECT_FALSE(SetIDBValue(content::ChildFrameAt(site_a->GetMainFrame(), 2)));
  // 3p frames.
  EXPECT_FALSE(SetIDBValue(content::ChildFrameAt(site_a->GetMainFrame(), 0)));
  EXPECT_FALSE(SetIDBValue(content::ChildFrameAt(site_a->GetMainFrame(), 1)));

  // 3p frame.
  EXPECT_FALSE(SetIDBValue(content::ChildFrameAt(site_b->GetMainFrame(), 2)));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorage1pBrowserTest,
                       ServiceWorkerUnavailable) {
  GURL a_site_ephemeral_storage_worker_url =
      https_server_.GetURL("a.com", "/workers/service_worker_setup.html");
  SetCookieSetting(a_site_ephemeral_storage_worker_url,
                   CONTENT_SETTING_SESSION_ONLY);

  // Ephemeral website should not allow service worker registration.
  WebContents* site_a_tab =
      LoadURLInNewTab(a_site_ephemeral_storage_worker_url);
  EXPECT_FALSE(content::ExecJs(site_a_tab, "setup();"));

  // Non ephemeral website should be fine.
  GURL b_site_ephemeral_storage_worker_url =
      https_server_.GetURL("b.com", "/workers/service_worker_setup.html");
  WebContents* site_b_tab =
      LoadURLInNewTab(b_site_ephemeral_storage_worker_url);
  EXPECT_EQ("ok", content::EvalJs(site_b_tab, "setup();"));
}

IN_PROC_BROWSER_TEST_F(EphemeralStorage1pBrowserTest,
                       LocalStorageWorksBetweenFrames) {
  SetCookieSetting(b_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);

  WebContents* site_b_tab1 = LoadURLInNewTab(b_site_ephemeral_storage_url_);
  WebContents* site_b_tab2 = LoadURLInNewTab(b_site_ephemeral_storage_url_);

  // Set values in the first tab.
  SetValuesInFrames(site_b_tab1, "b.com", "from=b.com");

  // Expect values are seen by the second tab in all frames.
  {
    ValuesFromFrames first_party_values = GetValuesFromFrames(site_b_tab2);
    EXPECT_EQ("b.com", first_party_values.main_frame.local_storage);
    EXPECT_EQ("b.com", first_party_values.iframe_1.local_storage);
    EXPECT_EQ("b.com", first_party_values.iframe_2.local_storage);

    EXPECT_EQ(nullptr, first_party_values.main_frame.session_storage);
    EXPECT_EQ(nullptr, first_party_values.iframe_1.session_storage);
    EXPECT_EQ(nullptr, first_party_values.iframe_2.session_storage);

    EXPECT_EQ("from=b.com", first_party_values.main_frame.cookies);
    EXPECT_EQ("from=b.com", first_party_values.iframe_1.cookies);
    EXPECT_EQ("from=b.com", first_party_values.iframe_2.cookies);
  }

  // Update values in the first tab.
  SetValuesInFrames(site_b_tab1, "b2.com", "from=b.com");

  {
    ValuesFromFrames first_party_values = GetValuesFromFrames(site_b_tab2);
    EXPECT_EQ("b2.com", first_party_values.main_frame.local_storage);
    EXPECT_EQ("b2.com", first_party_values.iframe_1.local_storage);
    EXPECT_EQ("b2.com", first_party_values.iframe_2.local_storage);

    EXPECT_EQ(nullptr, first_party_values.main_frame.session_storage);
    EXPECT_EQ(nullptr, first_party_values.iframe_1.session_storage);
    EXPECT_EQ(nullptr, first_party_values.iframe_2.session_storage);

    EXPECT_EQ("from=b.com", first_party_values.main_frame.cookies);
    EXPECT_EQ("from=b.com", first_party_values.iframe_1.cookies);
    EXPECT_EQ("from=b.com", first_party_values.iframe_2.cookies);
  }
}

IN_PROC_BROWSER_TEST_F(EphemeralStorage1pBrowserTest,
                       NavigationCookiesAreCleared) {
  SetCookieSetting(a_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);
  SetCookieSetting(b_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);

  GURL a_site_set_cookie_url = https_server_.GetURL(
      "a.com", "/set-cookie?name=acom;path=/;SameSite=None;Secure");
  GURL b_site_set_cookie_url = https_server_.GetURL(
      "b.com", "/set-cookie?name=bcom;path=/;SameSite=None;Secure");

  WebContents* site_a_set_cookies = LoadURLInNewTab(a_site_set_cookie_url);
  WebContents* site_b_set_cookies = LoadURLInNewTab(b_site_set_cookie_url);
  WebContents* site_a = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  WebContents* site_b = LoadURLInNewTab(b_site_ephemeral_storage_url_);

  // Default cookie storage request should return empty results.
  EXPECT_TRUE(content::GetCookies(browser()->profile(), GURL("https://a.com/"))
                  .empty());
  EXPECT_TRUE(content::GetCookies(browser()->profile(), GURL("https://b.com/"))
                  .empty());

  // JS cookie request should return valid results.
  EXPECT_EQ("name=acom", GetCookiesInFrame(site_a_set_cookies->GetMainFrame()));
  EXPECT_EQ("name=bcom", GetCookiesInFrame(site_b_set_cookies->GetMainFrame()));
  EXPECT_EQ("name=acom", GetCookiesInFrame(site_a->GetMainFrame()));

  // The third-party iframe should not have the b.com cookie that was set on the
  // main frame.
  RenderFrameHost* main_frame = site_a->GetMainFrame();
  RenderFrameHost* iframe_a = content::ChildFrameAt(main_frame, 0);
  RenderFrameHost* iframe_b = content::ChildFrameAt(main_frame, 1);
  EXPECT_EQ("", GetCookiesInFrame(iframe_a));
  EXPECT_EQ("", GetCookiesInFrame(iframe_b));

  // Setting the cookie directly on the third-party iframe should only set the
  // cookie in the ephemeral storage area for that frame.
  GURL b_site_set_ephemeral_cookie_url = https_server_.GetURL(
      "b.com", "/set-cookie?name=bcom_ephemeral;path=/;SameSite=None;Secure");
  NavigateIframeToURL(site_a, "third_party_iframe_a",
                      b_site_set_ephemeral_cookie_url);
  ASSERT_EQ("name=bcom_ephemeral", GetCookiesInFrame(iframe_a));
  ASSERT_EQ("name=bcom_ephemeral", GetCookiesInFrame(iframe_b));

  // The cookie set in the ephemeral area should not visible in the main
  // cookie storage.
  EXPECT_TRUE(content::GetCookies(browser()->profile(), GURL("https://b.com/"))
                  .empty());
  EXPECT_EQ("name=bcom", GetCookiesInFrame(site_b->GetMainFrame()));

  // Navigating to a new TLD should clear all ephemeral cookies after keep-alive
  // timeout.
  ASSERT_TRUE(content::NavigateToURL(site_a_set_cookies,
                                     c_site_ephemeral_storage_url_));
  ASSERT_TRUE(content::NavigateToURL(site_b_set_cookies,
                                     c_site_ephemeral_storage_url_));
  ASSERT_TRUE(content::NavigateToURL(site_a, c_site_ephemeral_storage_url_));
  ASSERT_TRUE(content::NavigateToURL(site_b, c_site_ephemeral_storage_url_));
  WaitForCleanupAfterKeepAlive();
  ASSERT_TRUE(content::NavigateToURL(site_a, a_site_ephemeral_storage_url_));
  ASSERT_TRUE(content::NavigateToURL(site_b, b_site_ephemeral_storage_url_));

  ValuesFromFrames values_site_a = GetValuesFromFrames(site_a);
  EXPECT_EQ("", values_site_a.main_frame.cookies);
  EXPECT_EQ("", values_site_a.iframe_1.cookies);
  EXPECT_EQ("", values_site_a.iframe_2.cookies);

  ValuesFromFrames values_site_b = GetValuesFromFrames(site_b);
  EXPECT_EQ("", values_site_b.main_frame.cookies);
  EXPECT_EQ("", values_site_b.iframe_1.cookies);
  EXPECT_EQ("", values_site_b.iframe_2.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorage1pBrowserTest,
                       FirstPartyNestedInThirdParty) {
  SetCookieSetting(a_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);

  auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();

  GURL a_site_set_cookie_url = https_server_.GetURL(
      "a.com", "/set-cookie?name=acom;path=/;SameSite=None;Secure");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), a_site_set_cookie_url));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

  RenderFrameHost* site_a_main_frame = web_contents->GetMainFrame();
  RenderFrameHost* nested_frames_tab =
      content::ChildFrameAt(site_a_main_frame, 3);
  ASSERT_NE(nested_frames_tab, nullptr);
  RenderFrameHost* first_party_nested_acom =
      content::ChildFrameAt(nested_frames_tab, 2);
  ASSERT_NE(first_party_nested_acom, nullptr);

  WebContents* site_b_tab = LoadURLInNewTab(b_site_ephemeral_storage_url_);
  RenderFrameHost* site_b_main_frame = site_b_tab->GetMainFrame();
  RenderFrameHost* third_party_nested_acom =
      content::ChildFrameAt(site_b_main_frame, 2);
  ASSERT_NE(first_party_nested_acom, nullptr);

  ASSERT_EQ("name=acom", GetCookiesInFrame(site_a_main_frame));
  ASSERT_EQ("name=acom", GetCookiesInFrame(first_party_nested_acom));
  ASSERT_EQ("", GetCookiesInFrame(third_party_nested_acom));

  SetValuesInFrame(site_a_main_frame, "first-party-a.com",
                   "name=first-party-a.com");
  SetValuesInFrame(third_party_nested_acom, "third-party-a.com",
                   "name=third-party-a.com");

  ValuesFromFrame first_party_values =
      GetValuesFromFrame(first_party_nested_acom);
  EXPECT_EQ("first-party-a.com", first_party_values.local_storage);
  EXPECT_EQ("first-party-a.com", first_party_values.session_storage);
  EXPECT_EQ("name=first-party-a.com", first_party_values.cookies);

  ValuesFromFrame third_party_values =
      GetValuesFromFrame(third_party_nested_acom);
  EXPECT_EQ("third-party-a.com", third_party_values.local_storage);
  EXPECT_EQ("third-party-a.com", third_party_values.session_storage);
  EXPECT_EQ("name=third-party-a.com", third_party_values.cookies);
}

IN_PROC_BROWSER_TEST_F(EphemeralStorage1pBrowserTest,
                       NetworkCookiesAreSetIn1p) {
  SetCookieSetting(a_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);
  GURL a_site_set_cookie_url = https_server_.GetURL(
      "a.com", "/set-cookie?name=acom;path=/;SameSite=None;Secure");

  WebContents* site_a_tab_network_cookies =
      LoadURLInNewTab(a_site_set_cookie_url);
  WebContents* site_a_tab = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  EXPECT_TRUE(http_request_monitor_.HasHttpRequestWithCookie(
      a_site_ephemeral_storage_url_, "name=acom"));

  ValuesFromFrames site_a_tab_values = GetValuesFromFrames(site_a_tab);
  EXPECT_EQ(nullptr, site_a_tab_values.main_frame.local_storage);
  EXPECT_EQ(nullptr, site_a_tab_values.iframe_1.local_storage);
  EXPECT_EQ(nullptr, site_a_tab_values.iframe_2.local_storage);

  EXPECT_EQ(nullptr, site_a_tab_values.main_frame.session_storage);
  EXPECT_EQ(nullptr, site_a_tab_values.iframe_1.session_storage);
  EXPECT_EQ(nullptr, site_a_tab_values.iframe_2.session_storage);

  EXPECT_EQ("name=acom", site_a_tab_values.main_frame.cookies);
  EXPECT_EQ("", site_a_tab_values.iframe_1.cookies);
  EXPECT_EQ("", site_a_tab_values.iframe_2.cookies);

  WebContents* site_b_tab = LoadURLInNewTab(b_site_ephemeral_storage_url_);
  ExpectValuesFromFramesAreEmpty(FROM_HERE, GetValuesFromFrames(site_b_tab));

  // Close a.com tabs.
  CloseWebContents(site_a_tab_network_cookies);
  CloseWebContents(site_a_tab);
  http_request_monitor_.Clear();

  // Load a.com tab again.
  WebContents* site_a_tab2 = LoadURLInNewTab(a_site_ephemeral_storage_url_);
  EXPECT_FALSE(http_request_monitor_.HasHttpRequestWithCookie(
      a_site_ephemeral_storage_url_, "name=acom"));

  // Cookie values should be empty after a cleanup.
  ExpectValuesFromFramesAreEmpty(FROM_HERE, GetValuesFromFrames(site_a_tab2));
}
