/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_request_headers.h"
#include "net/test/embedded_test_server/http_request.h"

const char kIframeID[] = "test";

const char kPointInPathScript[] =
  "var canvas = document.createElement('canvas');"
  "var ctx = canvas.getContext('2d');"
  "ctx.rect(10, 10, 100, 100);"
  "ctx.stroke();"
  "domAutomationController.send(ctx.isPointInPath(10, 10));";

const char kGetImageDataScript[] =
  "var canvas = document.createElement('canvas');"
  "var ctx = canvas.getContext('2d');"
  "ctx.rect(10, 10, 100, 100);"
  "ctx.fill();"
  "domAutomationController.send(ctx.getImageData(0, 0, 10, 10).data.length);";

#define COOKIE_STR "test=hi"
const char kCookieScript[] =
    "document.cookie = '"
    COOKIE_STR
    "'; domAutomationController.send(document.cookie);";

const char kReferrerScript[] =
    "domAutomationController.send(document.referrer);";

class BraveContentSettingsObserverBrowserTest : public InProcessBrowserTest {
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

      embedded_test_server()->RegisterRequestMonitor(
          base::BindRepeating(
              &BraveContentSettingsObserverBrowserTest::SaveReferrer,
              base::Unretained(this)));

      ASSERT_TRUE(embedded_test_server()->Start());

      url_ = embedded_test_server()->GetURL("a.com", "/iframe.html");
      iframe_url_ = embedded_test_server()->GetURL("b.com", "/simple.html");
      image_url_ = embedded_test_server()->GetURL("b.com", "/logo.png");
      top_level_page_pattern_ =
          ContentSettingsPattern::FromString("http://a.com/*");
      iframe_pattern_ = ContentSettingsPattern::FromString("http://b.com/*");
      first_party_pattern_ =
          ContentSettingsPattern::FromString("https://firstParty/*");
    }

    void SaveReferrer(const net::test_server::HttpRequest& request) {
      base::AutoLock auto_lock(last_referrers_lock_);

      // Replace "127.0.0.1:<port>" with the hostnames used in this test.
      net::test_server::HttpRequest::HeaderMap::const_iterator pos =
          request.headers.find(net::HttpRequestHeaders::kHost);
      GURL::Replacements replace_host;
      if (pos != request.headers.end()) {
        replace_host.SetHostStr(pos->second);
        replace_host.SetPortStr("");  // Host header includes the port already.
      }
      GURL request_url = request.GetURL();
      request_url = request_url.ReplaceComponents(replace_host);

      pos = request.headers.find(net::HttpRequestHeaders::kReferer);
      if (pos == request.headers.end()) {
        last_referrers_[request_url] = "";  // no referrer
      } else {
        last_referrers_[request_url] = pos->second;
      }
    }

    std::string GetLastReferrer(const GURL& url) const {
      base::AutoLock auto_lock(last_referrers_lock_);
      auto pos = last_referrers_.find(url);
      if (pos == last_referrers_.end()) {
        return "(missing)";  // Fail test if we haven't seen this URL before.
      }
      return pos->second;
    }

    void TearDown() override {
      browser_content_client_.reset();
      content_client_.reset();
    }

    const GURL& url() { return url_; }
    const GURL& iframe_url() { return iframe_url_; }
    const GURL& image_url() { return image_url_; }

    std::string create_image_script() {
      std::string s;
      s = " var img = document.createElement('img');"
          " img.onload = function () {"
          "   domAutomationController.send(img.src);"
          " };"
          " img.src = '" + image_url().spec() + "';"
          " document.body.appendChild(img);";
      return s;
    }

    const ContentSettingsPattern& top_level_page_pattern() {
      return top_level_page_pattern_;
    }

    const ContentSettingsPattern& first_party_pattern() {
      return first_party_pattern_;
    }

    const ContentSettingsPattern& iframe_pattern() {
      return iframe_pattern_;
    }

    HostContentSettingsMap * content_settings() {
      return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
    }

    void BlockReferrers() {
      content_settings()->SetContentSettingCustomScope(top_level_page_pattern(),
          ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kReferrers, CONTENT_SETTING_BLOCK);
      ContentSettingsForOneType settings;
      content_settings()->GetSettingsForOneType(
          CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kReferrers, &settings);
      EXPECT_EQ(settings.size(), 1u);
    }

    void AllowReferrers() {
      content_settings()->SetContentSettingCustomScope(top_level_page_pattern(),
          ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kReferrers, CONTENT_SETTING_ALLOW);
      ContentSettingsForOneType settings;
      content_settings()->GetSettingsForOneType(
          CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kReferrers, &settings);
      EXPECT_EQ(settings.size(), 1u);
    }

    void Block3PCookies() {
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kCookies, CONTENT_SETTING_BLOCK);
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          first_party_pattern(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kCookies, CONTENT_SETTING_ALLOW);
    }

    void BlockCookies() {
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kCookies, CONTENT_SETTING_BLOCK);
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          first_party_pattern(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kCookies, CONTENT_SETTING_BLOCK);
    }

    void AllowCookies() {
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kCookies, CONTENT_SETTING_ALLOW);
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          first_party_pattern(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kCookies, CONTENT_SETTING_ALLOW);
    }

    void ShieldsDown() {
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kBraveShields, CONTENT_SETTING_BLOCK);
    }

    void ShieldsUp() {
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kBraveShields, CONTENT_SETTING_ALLOW);
    }

    void AllowFingerprinting() {
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kFingerprinting,
          CONTENT_SETTING_ALLOW);
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          first_party_pattern(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kFingerprinting,
          CONTENT_SETTING_ALLOW);
    }

    void BlockFingerprinting() {
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kFingerprinting,
          CONTENT_SETTING_BLOCK);
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          first_party_pattern(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kFingerprinting,
          CONTENT_SETTING_BLOCK);
    }

    void Block3PFingerprinting() {
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kFingerprinting,
          CONTENT_SETTING_BLOCK);
      content_settings()->SetContentSettingCustomScope(
          top_level_page_pattern(),
          first_party_pattern(),
          CONTENT_SETTINGS_TYPE_PLUGINS,
          brave_shields::kFingerprinting,
          CONTENT_SETTING_ALLOW);
    }

    void BlockScripts() {
      content_settings()->SetContentSettingCustomScope(
          ContentSettingsPattern::Wildcard(),
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_JAVASCRIPT,
          "",
          CONTENT_SETTING_BLOCK);
    }

    void AllowScripts() {
      content_settings()->SetContentSettingCustomScope(
          ContentSettingsPattern::Wildcard(),
          ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_JAVASCRIPT,
          "",
          CONTENT_SETTING_ALLOW);
    }

    content::WebContents* contents() {
      return browser()->tab_strip_model()->GetActiveWebContents();
    }

    content::RenderFrameHost* child_frame() {
      return ChildFrameAt(contents()->GetMainFrame(), 0);
    }

    template <typename T>
    std::string ExecScriptGetStr(const std::string &script, T* frame) {
      std::string value;
      EXPECT_TRUE(ExecuteScriptAndExtractString(frame, script, &value));
      return value;
    }

    void NavigateToPageWithIframe() {
      ui_test_utils::NavigateToURL(browser(), url());
      ASSERT_EQ(contents()->GetAllFrames().size(), 2u) <<
          "Two frames (main + iframe) should be created.";
      content::RenderFrameHost* main_frame = contents()->GetMainFrame();
      EXPECT_EQ(main_frame->GetLastCommittedURL(), url());
    }

    bool NavigateToURLUntilLoadStop(
        const std::string& origin, const std::string& path) {
      ui_test_utils::NavigateToURL(
          browser(),
          embedded_test_server()->GetURL(origin, path));

      return WaitForLoadStop(contents());
    }

 private:
    GURL url_;
    GURL iframe_url_;
    GURL image_url_;
    ContentSettingsPattern top_level_page_pattern_;
    ContentSettingsPattern first_party_pattern_;
    ContentSettingsPattern iframe_pattern_;
    std::unique_ptr<ChromeContentClient> content_client_;
    std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
    mutable base::Lock last_referrers_lock_;
    std::map<GURL, std::string> last_referrers_;

    base::ScopedTempDir temp_user_data_dir_;
};

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
    BlockThirdPartyFPByDefault) {
  ContentSettingsForOneType fp_settings;
  content_settings()->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      &fp_settings);
  EXPECT_EQ(fp_settings.size(), 0u) <<
      "There should not be any visible fingerprinting rules.";

  NavigateToPageWithIframe();

  bool isPointInPath;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents(),
      kPointInPathScript, &isPointInPath));
  EXPECT_TRUE(isPointInPath);

  EXPECT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  EXPECT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(
      child_frame(), kPointInPathScript, &isPointInPath));
  EXPECT_FALSE(isPointInPath);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest, BlockFP) {
  BlockFingerprinting();

  ContentSettingsForOneType fp_settings;
  content_settings()->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      &fp_settings);
  EXPECT_EQ(fp_settings.size(), 2u);

  NavigateToPageWithIframe();

  bool isPointInPath;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents(),
      kPointInPathScript, &isPointInPath));
  EXPECT_FALSE(isPointInPath);

  EXPECT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  EXPECT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(
      child_frame(), kPointInPathScript, &isPointInPath));
  EXPECT_FALSE(isPointInPath);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest, AllowFP) {
  AllowFingerprinting();

  ContentSettingsForOneType fp_settings;
  content_settings()->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      &fp_settings);
  EXPECT_EQ(fp_settings.size(), 2u);

  NavigateToPageWithIframe();

  bool isPointInPath;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents(),
      kPointInPathScript, &isPointInPath));
  EXPECT_TRUE(isPointInPath);

  EXPECT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  EXPECT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(
      child_frame(), kPointInPathScript, &isPointInPath));
  EXPECT_TRUE(isPointInPath);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
    BlockThirdPartyFP) {
  Block3PFingerprinting();

  ContentSettingsForOneType fp_settings;
  content_settings()->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      &fp_settings);
  EXPECT_EQ(fp_settings.size(), 2u);

  NavigateToPageWithIframe();

  bool isPointInPath;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents(),
      kPointInPathScript, &isPointInPath));
  EXPECT_TRUE(isPointInPath);

  EXPECT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  EXPECT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(
      child_frame(), kPointInPathScript, &isPointInPath));
  EXPECT_FALSE(isPointInPath);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
                       BlockFPShieldsDown) {
  BlockFingerprinting();
  ShieldsDown();

  ContentSettingsForOneType fp_settings;
  content_settings()->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      &fp_settings);
  EXPECT_EQ(fp_settings.size(), 2u);

  NavigateToPageWithIframe();

  bool isPointInPath;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents(),
      kPointInPathScript, &isPointInPath));
  EXPECT_TRUE(isPointInPath);

  EXPECT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  EXPECT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(
      child_frame(), kPointInPathScript, &isPointInPath));
  EXPECT_TRUE(isPointInPath);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
                       Block3PFPGetImageData) {
  Block3PFingerprinting();

  ContentSettingsForOneType fp_settings;
  content_settings()->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      &fp_settings);
  EXPECT_EQ(fp_settings.size(), 2u);

  NavigateToPageWithIframe();

  int bufLen = -1;
  EXPECT_TRUE(ExecuteScriptAndExtractInt(contents(),
      kGetImageDataScript, &bufLen));
  EXPECT_EQ(400, bufLen);

  EXPECT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  EXPECT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_TRUE(ExecuteScriptAndExtractInt(
      child_frame(), kGetImageDataScript, &bufLen));
  EXPECT_EQ(0, bufLen);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
                       BlockFPGetImageData) {
  BlockFingerprinting();

  ContentSettingsForOneType fp_settings;
  content_settings()->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      &fp_settings);
  EXPECT_EQ(fp_settings.size(), 2u);

  NavigateToPageWithIframe();

  int bufLen = -1;
  EXPECT_TRUE(ExecuteScriptAndExtractInt(contents(),
      kGetImageDataScript, &bufLen));
  EXPECT_EQ(0, bufLen);

  EXPECT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  EXPECT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_TRUE(ExecuteScriptAndExtractInt(
      child_frame(), kGetImageDataScript, &bufLen));
  EXPECT_EQ(0, bufLen);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
                       AllowFPGetImageData) {
  AllowFingerprinting();

  ContentSettingsForOneType fp_settings;
  content_settings()->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      &fp_settings);
  EXPECT_EQ(fp_settings.size(), 2u);

  NavigateToPageWithIframe();

  int bufLen = -1;
  EXPECT_TRUE(ExecuteScriptAndExtractInt(contents(),
      kGetImageDataScript, &bufLen));
  EXPECT_EQ(400, bufLen);

  EXPECT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  EXPECT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_TRUE(ExecuteScriptAndExtractInt(
      child_frame(), kGetImageDataScript, &bufLen));
  EXPECT_EQ(400, bufLen);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
    BlockReferrerByDefault) {
  ContentSettingsForOneType settings;
  content_settings()->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kReferrers, &settings);
  EXPECT_EQ(settings.size(), 0u) <<
      "There should not be any visible referrer rules.";

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_STREQ(ExecScriptGetStr(kReferrerScript,
      contents()).c_str(), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Sub-resources loaded within the page get their referrer spoofed.
  EXPECT_EQ(ExecScriptGetStr(create_image_script(), contents()),
            image_url().spec());
  EXPECT_EQ(GetLastReferrer(image_url()), iframe_url().GetOrigin().spec());

  // Cross-origin iframe navigations get their referrer spoofed.
  ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_STREQ(ExecScriptGetStr(kReferrerScript,
      child_frame()).c_str(), iframe_url().GetOrigin().spec().c_str());
  EXPECT_EQ(GetLastReferrer(iframe_url()), iframe_url().GetOrigin().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
                       BlockReferrer) {
  BlockReferrers();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_STREQ(ExecScriptGetStr(kReferrerScript,
      contents()).c_str(), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Sub-resources loaded within the page get their referrer spoofed.
  EXPECT_EQ(ExecScriptGetStr(create_image_script(), contents()),
            image_url().spec());
  EXPECT_EQ(GetLastReferrer(image_url()), image_url().GetOrigin().spec());

  // Cross-origin iframe navigations get their referrer spoofed.
  ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_STREQ(ExecScriptGetStr(kReferrerScript, child_frame()).c_str(),
      iframe_url().GetOrigin().spec().c_str());
  EXPECT_EQ(GetLastReferrer(iframe_url()), iframe_url().GetOrigin().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
                       AllowReferrer) {
  AllowReferrers();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_STREQ(ExecScriptGetStr(kReferrerScript,
      contents()).c_str(), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Sub-resources loaded within the page get the page URL as referrer.
  EXPECT_EQ(ExecScriptGetStr(create_image_script(), contents()),
            image_url().spec());
  EXPECT_EQ(GetLastReferrer(image_url()), url().spec());

  // A cross-origin iframe navigation gets the URL of the first one as
  // referrer.
  ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_EQ(GetLastReferrer(iframe_url()), url());
  EXPECT_STREQ(ExecScriptGetStr(kReferrerScript, child_frame()).c_str(),
      url().spec().c_str());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
                       BlockReferrerShieldsDown) {
  BlockReferrers();
  ShieldsDown();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_STREQ(ExecScriptGetStr(kReferrerScript,
      contents()).c_str(), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Sub-resources loaded within the page get the page URL as referrer.
  EXPECT_EQ(ExecScriptGetStr(create_image_script(), contents()),
            image_url().spec());
  EXPECT_EQ(GetLastReferrer(image_url()), url().spec());

  // A cross-origin iframe navigation gets the URL of the first one as
  // referrer.
  ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_EQ(GetLastReferrer(iframe_url()), url());
  EXPECT_STREQ(ExecScriptGetStr(kReferrerScript, child_frame()).c_str(),
      url().spec().c_str());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
    BlockThirdPartyCookieByDefault) {
  NavigateToPageWithIframe();
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript,
      contents()).c_str(), COOKIE_STR);
  ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, child_frame()).c_str(), "");
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
    ExplicitBlock3PCookies) {
  Block3PCookies();

  NavigateToPageWithIframe();

  EXPECT_STREQ(ExecScriptGetStr(kCookieScript,
      contents()).c_str(), COOKIE_STR);
  ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, child_frame()).c_str(), "");
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest, BlockCookies) {
  BlockCookies();
  NavigateToPageWithIframe();
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, contents()).c_str(), "");
  ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, child_frame()).c_str(), "");
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest, AllowCookies) {
  AllowCookies();
  NavigateToPageWithIframe();
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, contents()).c_str(),
               COOKIE_STR);
  ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, child_frame()).c_str(),
               COOKIE_STR);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
    ChromiumCookieBlockOverridesBraveAllowCookiesTopLevel) {
  AllowCookies();
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  content_settings->SetContentSettingCustomScope(
      top_level_page_pattern(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_COOKIES, std::string(), CONTENT_SETTING_BLOCK);

  NavigateToPageWithIframe();

  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, contents()).c_str(), "");
  ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, child_frame()).c_str(),
               COOKIE_STR);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
    ChromiumCookieBlockOverridesBraveAllowCookiesIframe) {
  AllowCookies();
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  content_settings->SetContentSettingCustomScope(
      iframe_pattern(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_COOKIES, std::string(), CONTENT_SETTING_BLOCK);

  NavigateToPageWithIframe();

  EXPECT_STREQ(ExecScriptGetStr(kCookieScript,
      contents()).c_str(), COOKIE_STR);
  ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, child_frame()).c_str(), "");
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
    ShieldsDownAllowsCookies) {
  BlockCookies();
  ShieldsDown();
  NavigateToPageWithIframe();
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, contents()).c_str(), COOKIE_STR);
  ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, child_frame()).c_str(),
               COOKIE_STR);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
    ShieldsUpBlockCookies) {
  BlockCookies();
  ShieldsUp();
  NavigateToPageWithIframe();
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, contents()).c_str(), "");
  ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
  ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  EXPECT_STREQ(ExecScriptGetStr(kCookieScript, child_frame()).c_str(), "");
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest, BlockScripts) {
  BlockScripts();

  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("a.com", "/load_js_from_origins.html"));
  EXPECT_EQ(contents()->GetAllFrames().size(), 1u);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest, AllowScripts) {
  AllowScripts();

  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("a.com", "/load_js_from_origins.html"));
  EXPECT_EQ(contents()->GetAllFrames().size(), 4u);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
                       BlockScriptsShieldsDown) {
  BlockScripts();
  ShieldsDown();

  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("a.com", "/load_js_from_origins.html"));
  EXPECT_EQ(contents()->GetAllFrames().size(), 4u);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest,
                       BlockScriptsShieldsDownInOtherTab) {
  // Turn off shields in a.com.
  ShieldsDown();
  // Block scripts in b.com.
  content_settings()->SetContentSettingCustomScope(
      iframe_pattern(),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_JAVASCRIPT,
      "",
      CONTENT_SETTING_BLOCK);

  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("b.com", "/load_js_from_origins.html"));
  EXPECT_EQ(contents()->GetAllFrames().size(), 1u);
}
