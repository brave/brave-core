/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_request_headers.h"
#include "net/test/embedded_test_server/http_request.h"

using brave_shields::ControlType;

namespace {

const char kIframeID[] = "test";

const char kGetImageDataScript[] =
    "var adder = (a, x) => a + x;"
    "var canvas = document.createElement('canvas');"
    "canvas.width = 16;"
    "canvas.height = 16;"
    "var ctx = canvas.getContext('2d');"
    "ctx.rect(0, 0, 16, 16);"
    "ctx.fillStyle = 'black';"
    "ctx.fill();"
    "ctx.rect(5, 10, 10, 4);"
    "ctx.fillStyle = 'white';"
    "ctx.fill();"
    "domAutomationController.send(ctx.getImageData(0, 0, canvas.width, "
    "canvas.height).data.reduce(adder));";

const int kExpectedImageDataHash = 261040;

const char kEmptyCookie[] = "";

#define COOKIE_STR "test=hi"

const char kTestCookie[] = COOKIE_STR;

const char kCookieScript[] =
    "document.cookie = '" COOKIE_STR
    "'; domAutomationController.send(document.cookie);";

const char kReferrerScript[] =
    "domAutomationController.send(document.referrer);";

}  // namespace

class BraveContentSettingsAgentImplBrowserTest : public InProcessBrowserTest {
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

    embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
        &BraveContentSettingsAgentImplBrowserTest::SaveReferrer,
        base::Unretained(this)));

    ASSERT_TRUE(embedded_test_server()->Start());

    url_ = embedded_test_server()->GetURL("a.com", "/iframe.html");
    iframe_url_ = embedded_test_server()->GetURL("b.com", "/simple.html");
    image_url_ = embedded_test_server()->GetURL("b.com", "/logo.png");
    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
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
        " img.src = '" +
        image_url().spec() +
        "';"
        " document.body.appendChild(img);";
    return s;
  }

  const GURL top_level_page_url() { return top_level_page_url_; }

  const ContentSettingsPattern& top_level_page_pattern() {
    return top_level_page_pattern_;
  }

  const ContentSettingsPattern& first_party_pattern() {
    return first_party_pattern_;
  }

  const ContentSettingsPattern& iframe_pattern() { return iframe_pattern_; }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void BlockReferrers() {
    content_settings()->SetContentSettingCustomScope(
        top_level_page_pattern(), ContentSettingsPattern::Wildcard(),
        ContentSettingsType::PLUGINS, brave_shields::kReferrers,
        CONTENT_SETTING_BLOCK);
    ContentSettingsForOneType settings;
    content_settings()->GetSettingsForOneType(
        ContentSettingsType::PLUGINS, brave_shields::kReferrers, &settings);
    EXPECT_EQ(settings.size(), 1u);
  }

  void AllowReferrers() {
    content_settings()->SetContentSettingCustomScope(
        top_level_page_pattern(), ContentSettingsPattern::Wildcard(),
        ContentSettingsType::PLUGINS, brave_shields::kReferrers,
        CONTENT_SETTING_ALLOW);
    ContentSettingsForOneType settings;
    content_settings()->GetSettingsForOneType(
        ContentSettingsType::PLUGINS, brave_shields::kReferrers, &settings);
    EXPECT_EQ(settings.size(), 1u);
  }

  void Block3PCookies() {
    brave_shields::SetCookieControlType(browser()->profile(),
                                        ControlType::BLOCK_THIRD_PARTY,
                                        top_level_page_url());
  }

  void BlockCookies() {
    brave_shields::SetCookieControlType(
        browser()->profile(), ControlType::BLOCK, top_level_page_url());
  }

  void AllowCookies() {
    brave_shields::SetCookieControlType(
        browser()->profile(), ControlType::ALLOW, top_level_page_url());
  }

  void ShieldsDown() {
    brave_shields::SetBraveShieldsEnabled(browser()->profile(), false,
                                          top_level_page_url());
  }

  void ShieldsUp() {
    brave_shields::SetBraveShieldsEnabled(browser()->profile(), true,
                                          top_level_page_url());
  }

  void AllowFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        browser()->profile(), ControlType::ALLOW, top_level_page_url());
  }

  void BlockFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        browser()->profile(), ControlType::BLOCK, top_level_page_url());
  }

  void Block3PFingerprinting() {
    brave_shields::SetFingerprintingControlType(browser()->profile(),
                                                ControlType::BLOCK_THIRD_PARTY,
                                                top_level_page_url());
  }

  void BlockScripts() {
    brave_shields::SetNoScriptControlType(
        browser()->profile(), ControlType::BLOCK, top_level_page_url());
  }

  void AllowScripts() {
    brave_shields::SetNoScriptControlType(
        browser()->profile(), ControlType::ALLOW, top_level_page_url());
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* child_frame() {
    return ChildFrameAt(contents()->GetMainFrame(), 0);
  }

  template <typename T>
  std::string ExecScriptGetStr(const std::string& script, T* frame) {
    std::string value;
    EXPECT_TRUE(ExecuteScriptAndExtractString(frame, script, &value));
    return value;
  }

  void NavigateToPageWithIframe() {
    ui_test_utils::NavigateToURL(browser(), url());
    ASSERT_EQ(contents()->GetAllFrames().size(), 2u)
        << "Two frames (main + iframe) should be created.";
    content::RenderFrameHost* main_frame = contents()->GetMainFrame();
    EXPECT_EQ(main_frame->GetLastCommittedURL(), url());
  }

  void NavigateToURLUntilLoadStop(const std::string& origin,
                                  const std::string& path) {
    ui_test_utils::NavigateToURL(browser(),
                                 embedded_test_server()->GetURL(origin, path));
  }

  void NavigateIframe() {
    ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, iframe_url()));
    ASSERT_EQ(child_frame()->GetLastCommittedURL(), iframe_url());
  }

  template <typename T>
  void CheckCookie(T* frame, base::StringPiece cookie) {
    EXPECT_EQ(ExecScriptGetStr(kCookieScript, frame), cookie);
  }

 private:
  GURL url_;
  GURL iframe_url_;
  GURL image_url_;
  GURL top_level_page_url_;
  ContentSettingsPattern top_level_page_pattern_;
  ContentSettingsPattern first_party_pattern_;
  ContentSettingsPattern iframe_pattern_;
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
  mutable base::Lock last_referrers_lock_;
  std::map<GURL, std::string> last_referrers_;

  base::ScopedTempDir temp_user_data_dir_;
};

// See https://github.com/brave/brave-browser/issues/8937
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       DISABLED_FarbleGetImageData) {
  NavigateToPageWithIframe();

  int hash = -1;
  EXPECT_TRUE(
      ExecuteScriptAndExtractInt(contents(), kGetImageDataScript, &hash));
  EXPECT_EQ(kExpectedImageDataHash, hash);

  // The iframe should have the same result as the top frame because farbling is
  // based on the top frame's session token.
  hash = -1;
  NavigateIframe();
  EXPECT_TRUE(
      ExecuteScriptAndExtractInt(child_frame(), kGetImageDataScript, &hash));
  EXPECT_EQ(kExpectedImageDataHash, hash);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrerByDefault) {
  ContentSettingsForOneType settings;
  content_settings()->GetSettingsForOneType(
      ContentSettingsType::PLUGINS, brave_shields::kReferrers, &settings);
  EXPECT_EQ(settings.size(), 0u)
      << "There should not be any visible referrer rules.";

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Sub-resources loaded within the page get their referrer spoofed.
  EXPECT_EQ(ExecScriptGetStr(create_image_script(), contents()),
            image_url().spec());
  EXPECT_EQ(GetLastReferrer(image_url()), iframe_url().GetOrigin().spec());

  // Cross-origin iframe navigations get their referrer spoofed.
  NavigateIframe();

  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, child_frame()),
            iframe_url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(iframe_url()), iframe_url().GetOrigin().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrer) {
  BlockReferrers();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Sub-resources loaded within the page get their referrer spoofed.
  EXPECT_EQ(ExecScriptGetStr(create_image_script(), contents()),
            image_url().spec());
  EXPECT_EQ(GetLastReferrer(image_url()), image_url().GetOrigin().spec());

  // Cross-origin iframe navigations get their referrer spoofed.
  NavigateIframe();

  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, child_frame()),
            iframe_url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(iframe_url()), iframe_url().GetOrigin().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       AllowReferrer) {
  AllowReferrers();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Sub-resources loaded within the page get the page URL as referrer.
  EXPECT_EQ(ExecScriptGetStr(create_image_script(), contents()),
            image_url().spec());
  EXPECT_EQ(GetLastReferrer(image_url()), url().spec());

  // A cross-origin iframe navigation gets the URL of the first one as
  // referrer.
  NavigateIframe();

  EXPECT_EQ(GetLastReferrer(iframe_url()), url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, child_frame()), url().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrerShieldsDown) {
  BlockReferrers();
  ShieldsDown();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Sub-resources loaded within the page get the page URL as referrer.
  EXPECT_EQ(ExecScriptGetStr(create_image_script(), contents()),
            image_url().spec());
  EXPECT_EQ(GetLastReferrer(image_url()), url().spec());

  // A cross-origin iframe navigation gets the URL of the first one as
  // referrer.
  NavigateIframe();

  EXPECT_EQ(GetLastReferrer(iframe_url()), url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, child_frame()), url().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockThirdPartyCookieByDefault) {
  NavigateToPageWithIframe();
  CheckCookie(child_frame(), kTestCookie);

  NavigateIframe();
  CheckCookie(child_frame(), kEmptyCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       ExplicitBlock3PCookies) {
  Block3PCookies();

  NavigateToPageWithIframe();
  CheckCookie(child_frame(), kTestCookie);

  NavigateIframe();
  CheckCookie(child_frame(), kEmptyCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest, BlockCookies) {
  BlockCookies();

  NavigateToPageWithIframe();
  CheckCookie(contents(), kEmptyCookie);

  NavigateIframe();
  CheckCookie(child_frame(), kEmptyCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest, AllowCookies) {
  AllowCookies();

  NavigateToPageWithIframe();
  CheckCookie(contents(), kTestCookie);

  NavigateIframe();
  CheckCookie(child_frame(), kTestCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       ChromiumCookieBlockOverridesBraveAllowCookiesTopLevel) {
  AllowCookies();
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  content_settings->SetContentSettingCustomScope(
      top_level_page_pattern(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::COOKIES, std::string(), CONTENT_SETTING_BLOCK);

  NavigateToPageWithIframe();
  CheckCookie(contents(), kEmptyCookie);

  NavigateIframe();
  CheckCookie(child_frame(), kTestCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       ChromiumCookieBlockOverridesBraveAllowCookiesIframe) {
  AllowCookies();
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  content_settings->SetContentSettingCustomScope(
      iframe_pattern(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::COOKIES, std::string(), CONTENT_SETTING_BLOCK);

  NavigateToPageWithIframe();
  CheckCookie(contents(), kTestCookie);

  NavigateIframe();
  CheckCookie(child_frame(), kEmptyCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       ShieldsDownOverridesBlockedCookies) {
  BlockCookies();
  ShieldsDown();

  NavigateToPageWithIframe();
  CheckCookie(contents(), kTestCookie);

  NavigateIframe();
  CheckCookie(child_frame(), kTestCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       ShieldsDownAllowsCookies) {
  ShieldsDown();

  NavigateToPageWithIframe();
  CheckCookie(contents(), kTestCookie);

  NavigateIframe();
  CheckCookie(child_frame(), kTestCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       ShieldsUpBlockCookies) {
  BlockCookies();
  ShieldsUp();

  NavigateToPageWithIframe();
  CheckCookie(contents(), kEmptyCookie);

  NavigateIframe();
  CheckCookie(child_frame(), kEmptyCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest, BlockScripts) {
  BlockScripts();

  NavigateToURLUntilLoadStop("a.com", "/load_js_from_origins.html");
  EXPECT_EQ(contents()->GetAllFrames().size(), 1u);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest, AllowScripts) {
  AllowScripts();

  NavigateToURLUntilLoadStop("a.com", "/load_js_from_origins.html");
  EXPECT_EQ(contents()->GetAllFrames().size(), 4u);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockScriptsShieldsDown) {
  BlockScripts();
  ShieldsDown();

  NavigateToURLUntilLoadStop("a.com", "/load_js_from_origins.html");
  EXPECT_EQ(contents()->GetAllFrames().size(), 4u);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockScriptsShieldsDownInOtherTab) {
  // Turn off shields in a.com.
  ShieldsDown();
  // Block scripts in b.com.
  content_settings()->SetContentSettingCustomScope(
      iframe_pattern(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, "", CONTENT_SETTING_BLOCK);

  NavigateToURLUntilLoadStop("b.com", "/load_js_from_origins.html");
  EXPECT_EQ(contents()->GetAllFrames().size(), 1u);
}
