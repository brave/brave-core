/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/base/features.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_request_headers.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "net/test/embedded_test_server/http_request.h"
#include "testing/gmock/include/gmock/gmock.h"

using brave_shields::ControlType;

namespace {

const char kIframeID[] = "test";

const char kPointInPathScript[] =
    "var canvas = document.createElement('canvas');"
    "var ctx = canvas.getContext('2d');"
    "ctx.rect(10, 10, 100, 100);"
    "ctx.stroke();"
    "domAutomationController.send(ctx.isPointInPath(10, 10));";

const char kGetImageDataScript[] =
    "var adder = (a, x) => a + x;"
    "var canvas = document.createElement('canvas');"
    "canvas.width = 16;"
    "canvas.height = 16;"
    "var ctx = canvas.getContext('2d');"
    "var data = ctx.createImageData(canvas.width, canvas.height);"
    "ctx.putImageData(data, 0, 0);"
    "domAutomationController.send(ctx.getImageData(0, 0, canvas.width, "
    "canvas.height).data.reduce(adder));";

const int kExpectedImageDataHashFarblingBalanced = 204;
const int kExpectedImageDataHashFarblingOff = 0;
const int kExpectedImageDataHashFarblingMaximum =
    kExpectedImageDataHashFarblingBalanced;

const char kEmptyCookie[] = "";

#define COOKIE_STR "test=hi"

const char kTestCookie[] = COOKIE_STR;

const char kCookieScript[] =
    "document.cookie = '" COOKIE_STR "'"
    "; document.cookie;";

const char kCookie3PScript[] =
    "document.cookie = '" COOKIE_STR ";SameSite=None;Secure'"
    "; document.cookie;";

const char kReferrerScript[] =
    "domAutomationController.send(document.referrer);";

const char kTitleScript[] = "domAutomationController.send(document.title);";

}  // namespace

class BraveContentSettingsAgentImplBrowserTest : public InProcessBrowserTest {
 public:
  BraveContentSettingsAgentImplBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(net::features::kBraveEphemeralStorage);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    content_client_.reset(new ChromeContentClient);
    content::SetContentClient(content_client_.get());
    browser_content_client_.reset(new BraveContentBrowserClient());
    content::SetBrowserClientForTesting(browser_content_client_.get());

    host_resolver()->AddRule("*", "127.0.0.1");

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    content::SetupCrossSiteRedirector(&https_server_);
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &BraveContentSettingsAgentImplBrowserTest::SaveReferrer,
        base::Unretained(this)));

    ASSERT_TRUE(https_server_.Start());

    url_ = https_server_.GetURL("a.com", "/iframe.html");
    cross_site_url_ = https_server_.GetURL("b.com", "/simple.html");
    cross_site_image_url_ =
        https_server_.GetURL("b.com", "/logo.png");
    link_url_ = https_server_.GetURL("a.com", "/simple_link.html");
    redirect_to_cross_site_url_ = https_server_.GetURL(
        "a.com", "/cross-site/b.com/simple.html");
    redirect_to_cross_site_image_url_ =
        https_server_.GetURL("a.com", "/cross-site/b.com/logo.png");
    same_site_url_ =
        https_server_.GetURL("sub.a.com", "/simple.html");
    same_origin_url_ = https_server_.GetURL("a.com", "/simple.html");
    same_origin_image_url_ =
        https_server_.GetURL("a.com", "/logo.png");
    top_level_page_url_ = https_server_.GetURL("a.com", "/");
    top_level_page_pattern_ =
        ContentSettingsPattern::FromString("https://a.com/*");
    iframe_pattern_ = ContentSettingsPattern::FromString("https://b.com/*");
    first_party_pattern_ =
        ContentSettingsPattern::FromString("https://firstParty/*");
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    // This is needed to load pages from "domain.com" without an interstitial.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
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

  const net::EmbeddedTestServer& https_server() { return https_server_; }
  const GURL& url() { return url_; }
  const GURL& cross_site_url() { return cross_site_url_; }
  const GURL& cross_site_image_url() { return cross_site_image_url_; }
  const GURL& link_url() { return link_url_; }
  const GURL& redirect_to_cross_site_url() {
    return redirect_to_cross_site_url_;
  }
  const GURL& redirect_to_cross_site_image_url() {
    return redirect_to_cross_site_image_url_;
  }
  const GURL& same_site_url() { return same_site_url_; }
  const GURL& same_origin_url() { return same_origin_url_; }

  const GURL& same_origin_image_url() { return same_origin_image_url_; }

  std::string create_image_script(const GURL& url) {
    std::string s;
    s = " var img = document.createElement('img');"
        " img.onload = function () {"
        "   domAutomationController.send(img.src);"
        " };"
        " img.src = '" +
        url.spec() +
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
        ContentSettingsType::BRAVE_REFERRERS, CONTENT_SETTING_BLOCK);
    ContentSettingsForOneType settings;
    content_settings()->GetSettingsForOneType(
        ContentSettingsType::BRAVE_REFERRERS, &settings);
    EXPECT_EQ(settings.size(), 1u);
  }

  void AllowReferrers() {
    content_settings()->SetContentSettingCustomScope(
        top_level_page_pattern(), ContentSettingsPattern::Wildcard(),
        ContentSettingsType::BRAVE_REFERRERS, CONTENT_SETTING_ALLOW);
    ContentSettingsForOneType settings;
    content_settings()->GetSettingsForOneType(
        ContentSettingsType::BRAVE_REFERRERS, &settings);
    EXPECT_EQ(settings.size(), 1u);
  }

  void Block3PCookies() {
    brave_shields::SetCookieControlType(content_settings(),
                                        ControlType::BLOCK_THIRD_PARTY,
                                        top_level_page_url());
  }

  void BlockCookies() {
    brave_shields::SetCookieControlType(
        content_settings(), ControlType::BLOCK, top_level_page_url());
  }

  void AllowCookies() {
    brave_shields::SetCookieControlType(
        content_settings(), ControlType::ALLOW, top_level_page_url());
  }

  void ShieldsDown() {
    brave_shields::SetBraveShieldsEnabled(content_settings(),
                                          false,
                                          top_level_page_url());
  }

  void ShieldsUp() {
    brave_shields::SetBraveShieldsEnabled(content_settings(),
                                          true,
                                          top_level_page_url());
  }

  void AllowFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW, top_level_page_url());
  }

  void BlockFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK, top_level_page_url());
  }

  void BlockThirdPartyFingerprinting() {
    brave_shields::SetFingerprintingControlType(content_settings(),
                                                ControlType::BLOCK_THIRD_PARTY,
                                                top_level_page_url());
  }

  void SetFingerprintingDefault() {
    brave_shields::SetFingerprintingControlType(content_settings(),
                                                ControlType::DEFAULT,
                                                top_level_page_url());
  }

  void BlockScripts() {
    brave_shields::SetNoScriptControlType(
        content_settings(), ControlType::BLOCK, top_level_page_url());
  }

  void AllowScripts() {
    brave_shields::SetNoScriptControlType(
        content_settings(), ControlType::ALLOW, top_level_page_url());
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

  // Returns the URL from which we are navigating away.
  GURL NavigateDirectlyToPageWithLink(const GURL& url,
                                      const std::string& referrer_policy = {}) {
    const std::string link_query =
        referrer_policy.empty() ? "" : "?policy=" + referrer_policy;
    GURL link(link_url().spec() + link_query);
    ui_test_utils::NavigateToURL(browser(), link);

    std::string clickLink =
        "domAutomationController.send(clickLink('" + url.spec() + "'));";
    bool success = false;
    EXPECT_TRUE(
        ExecuteScriptAndExtractBool(contents(), clickLink.c_str(), &success));
    EXPECT_TRUE(success);
    EXPECT_TRUE(WaitForLoadStop(contents()));

    return link;
  }

  void RedirectToPageWithLink(const GURL& url, const GURL& final_url) {
    NavigateDirectlyToPageWithLink(url);
    content::RenderFrameHost* main_frame = contents()->GetMainFrame();
    EXPECT_EQ(main_frame->GetLastCommittedURL(), final_url);
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
                                 https_server().GetURL(origin, path));
  }

  void NavigateIframe(const GURL& url) {
    ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID, url));
    ASSERT_EQ(child_frame()->GetLastCommittedURL(), url);
  }

  void NavigateCrossSiteRedirectIframe() {
    ASSERT_TRUE(NavigateIframeToURL(contents(), kIframeID,
                                    redirect_to_cross_site_url()));
    ASSERT_EQ(child_frame()->GetLastCommittedURL(), cross_site_url());
  }

  template <typename T>
  void CheckCookie(T* frame, base::StringPiece cookie) {
    EXPECT_EQ(cookie, EvalJs(frame, kCookieScript));
  }

  template <typename T>
  void Check3PCookie(T* frame, base::StringPiece cookie) {
    EXPECT_EQ(cookie, EvalJs(frame, kCookie3PScript));
  }

  template <typename T>
  void CheckLocalStorageAccessible(T* frame) {
    EXPECT_EQ(1, EvalJs(frame, "localStorage.test = 1"));
    EXPECT_EQ(1, EvalJs(frame, "sessionStorage.test = 1"));
  }

  template <typename T>
  void CheckLocalStorageAccessDenied(T* frame) {
    EXPECT_THAT(
        EvalJs(frame, "localStorage").error,
        ::testing::StartsWith(
            "a JavaScript error:\nError: Failed to read the 'localStorage' "
            "property from 'Window': Access is denied for this document.\n"));
    EXPECT_THAT(
        EvalJs(frame, "sessionStorage").error,
        ::testing::StartsWith(
            "a JavaScript error:\nError: Failed to read the 'sessionStorage' "
            "property from 'Window': Access is denied for this document.\n"));
  }

  template <typename T>
  void CheckLocalStorageThrows(T* frame) {
    EXPECT_FALSE(ExecJs(frame, "localStorage"));
    EXPECT_FALSE(ExecJs(frame, "sessionStorage"));
  }

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  GURL url_;
  GURL cross_site_url_;
  GURL cross_site_image_url_;
  GURL link_url_;
  GURL redirect_to_cross_site_url_;
  GURL redirect_to_cross_site_image_url_;
  GURL same_site_url_;
  GURL same_origin_url_;
  GURL same_origin_image_url_;
  GURL top_level_page_url_;
  ContentSettingsPattern top_level_page_pattern_;
  ContentSettingsPattern first_party_pattern_;
  ContentSettingsPattern iframe_pattern_;
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
  mutable base::Lock last_referrers_lock_;
  std::map<GURL, std::string> last_referrers_;

  base::ScopedTempDir temp_user_data_dir_;
  net::test_server::EmbeddedTestServer https_server_;
};

class BraveContentSettingsAgentImplNoEphemeralStorageBrowserTest
    : public BraveContentSettingsAgentImplBrowserTest {
 public:
  BraveContentSettingsAgentImplNoEphemeralStorageBrowserTest() {
    feature_list_.Reset();
    feature_list_.InitAndDisableFeature(net::features::kBraveEphemeralStorage);
  }
};

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       FarbleGetImageData) {
  // Farbling should be balanced by default
  NavigateToPageWithIframe();
  int hash = -1;
  EXPECT_TRUE(
      ExecuteScriptAndExtractInt(contents(), kGetImageDataScript, &hash));
  EXPECT_EQ(kExpectedImageDataHashFarblingBalanced, hash);

  // The iframe should have the same result as the top frame because farbling is
  // based on the top frame's session token.
  hash = -1;
  NavigateIframe(cross_site_url());
  EXPECT_TRUE(
      ExecuteScriptAndExtractInt(child_frame(), kGetImageDataScript, &hash));
  EXPECT_EQ(kExpectedImageDataHashFarblingBalanced, hash);

  // Farbling should be off if shields is down
  ShieldsDown();
  NavigateToPageWithIframe();
  hash = -1;
  EXPECT_TRUE(
      ExecuteScriptAndExtractInt(contents(), kGetImageDataScript, &hash));
  EXPECT_EQ(kExpectedImageDataHashFarblingOff, hash);

  // Farbling should be off if shields is up but fingerprinting is allowed
  // via content settings
  ShieldsUp();
  AllowFingerprinting();
  NavigateToPageWithIframe();
  hash = -1;
  EXPECT_TRUE(
      ExecuteScriptAndExtractInt(contents(), kGetImageDataScript, &hash));
  EXPECT_EQ(kExpectedImageDataHashFarblingOff, hash);
}

class BraveContentSettingsAgentImplV2BrowserTest
    : public BraveContentSettingsAgentImplBrowserTest {
 public:
  void SetUp() override { BraveContentSettingsAgentImplBrowserTest::SetUp(); }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// This test currently fails on Linux platforms due to an upstream bug when
// SwANGLE is used, see upstream bug at http://crbug.com/1192632.
#if defined(OS_LINUX)
#define MAYBE_WebGLReadPixels DISABLED_WebGLReadPixels
#else
#define MAYBE_WebGLReadPixels WebGLReadPixels
#endif
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplV2BrowserTest,
                       MAYBE_WebGLReadPixels) {
  std::string origin = "a.com";
  std::string path = "/webgl/readpixels.html";

  // Farbling level: maximum
  // WebGL readPixels(): blocked
  BlockFingerprinting();
  NavigateToURLUntilLoadStop(origin, path);
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), "1");

  // Farbling level: balanced (default)
  // WebGL readPixels(): allowed
  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(origin, path);
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), "0");

  // Farbling level: off
  // WebGL readPixels(): allowed
  AllowFingerprinting();
  NavigateToURLUntilLoadStop(origin, path);
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), "0");
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplV2BrowserTest,
                       FarbleGetImageData) {
  // Farbling should be default when kBraveFingerprintingV2 is enabled
  // because it uses a different content setting
  NavigateToPageWithIframe();
  int hash = -1;
  EXPECT_TRUE(
      ExecuteScriptAndExtractInt(contents(), kGetImageDataScript, &hash));
  EXPECT_EQ(kExpectedImageDataHashFarblingBalanced, hash);

  // Farbling should be maximum if fingerprinting is blocked via content
  // settings and kBraveFingerprintingV2 is enabled
  BlockFingerprinting();
  NavigateToPageWithIframe();
  hash = -1;
  EXPECT_TRUE(
      ExecuteScriptAndExtractInt(contents(), kGetImageDataScript, &hash));
  EXPECT_EQ(kExpectedImageDataHashFarblingMaximum, hash);

  // Farbling should be balanced if fingerprinting is default via
  // content settings and kBraveFingerprintingV2 is enabled
  SetFingerprintingDefault();
  NavigateToPageWithIframe();
  hash = -1;
  EXPECT_TRUE(
      ExecuteScriptAndExtractInt(contents(), kGetImageDataScript, &hash));
  EXPECT_EQ(kExpectedImageDataHashFarblingBalanced, hash);

  // Farbling should be off if fingerprinting is allowed via
  // content settings and kBraveFingerprintingV2 is enabled
  AllowFingerprinting();
  NavigateToPageWithIframe();
  hash = -1;
  EXPECT_TRUE(
      ExecuteScriptAndExtractInt(contents(), kGetImageDataScript, &hash));
  EXPECT_EQ(kExpectedImageDataHashFarblingOff, hash);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplV2BrowserTest,
                       CanvasIsPointInPath) {
  bool isPointInPath;

  // Farbling level: maximum
  // Canvas isPointInPath(): blocked
  BlockFingerprinting();
  NavigateToPageWithIframe();
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents(), kPointInPathScript,
                                          &isPointInPath));
  EXPECT_FALSE(isPointInPath);
  NavigateIframe(cross_site_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(child_frame(), kPointInPathScript,
                                          &isPointInPath));
  EXPECT_FALSE(isPointInPath);

  // Farbling level: balanced (default)
  // Canvas isPointInPath(): allowed
  SetFingerprintingDefault();
  NavigateToPageWithIframe();
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents(), kPointInPathScript,
                                          &isPointInPath));
  EXPECT_TRUE(isPointInPath);
  NavigateIframe(cross_site_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(child_frame(), kPointInPathScript,
                                          &isPointInPath));
  EXPECT_TRUE(isPointInPath);

  // Farbling level: off
  // Canvas isPointInPath(): allowed
  AllowFingerprinting();
  NavigateToPageWithIframe();
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents(), kPointInPathScript,
                                          &isPointInPath));
  EXPECT_TRUE(isPointInPath);
  NavigateIframe(cross_site_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(child_frame(), kPointInPathScript,
                                          &isPointInPath));
  EXPECT_TRUE(isPointInPath);

  // Shields: down
  // Canvas isPointInPath(): allowed
  BlockFingerprinting();
  ShieldsDown();
  AllowFingerprinting();
  NavigateToPageWithIframe();
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents(), kPointInPathScript,
                                          &isPointInPath));
  EXPECT_TRUE(isPointInPath);
  NavigateIframe(cross_site_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(child_frame(), kPointInPathScript,
                                          &isPointInPath));
  EXPECT_TRUE(isPointInPath);
}

// TODO(iefremov): We should reduce the copy-paste amount in these tests.
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrerByDefault) {
  ContentSettingsForOneType settings;
  content_settings()->GetSettingsForOneType(
      ContentSettingsType::BRAVE_REFERRERS, &settings);
  EXPECT_EQ(settings.size(), 0u)
      << "There should not be any visible referrer rules.";

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Same-origin sub-resources within the page get the page URL as referrer.
  EXPECT_EQ(ExecScriptGetStr(create_image_script(same_origin_image_url()),
                             contents()),
            same_origin_image_url().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_image_url()), url().spec());

  // Cross-site sub-resources within the page should follow the default referrer
  // policy.
  EXPECT_EQ(
      ExecScriptGetStr(create_image_script(cross_site_image_url()), contents()),
      cross_site_image_url().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_image_url()), url().GetOrigin().spec());

  // Same-origin iframe navigations get the page URL as referrer.
  NavigateIframe(same_origin_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, child_frame()), url().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_url()), url().spec());

  // Cross-site iframe navigations should follow the default referrer policy.
  NavigateIframe(cross_site_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, child_frame()),
            url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), url().GetOrigin().spec());

  // Same-origin navigations get the original page origin as the referrer.
  NavigateDirectlyToPageWithLink(same_origin_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), link_url().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_url()), link_url().spec());

  // Same-site but cross-origin navigations get the original page origin as the
  // referrer.
  NavigateDirectlyToPageWithLink(same_site_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()),
            link_url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(same_site_url()), link_url().GetOrigin().spec());

  // Cross-site navigations should follow the default referrer policy.
  NavigateDirectlyToPageWithLink(cross_site_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()),
            link_url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), link_url().GetOrigin().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrerByDefaultRedirects) {
  ContentSettingsForOneType settings;
  content_settings()->GetSettingsForOneType(
      ContentSettingsType::BRAVE_REFERRERS, &settings);
  EXPECT_EQ(settings.size(), 0u)
      << "There should not be any visible referrer rules.";

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Cross-site sub-resources within the page should follow the default referrer
  // policy.
  EXPECT_EQ(
      ExecScriptGetStr(create_image_script(redirect_to_cross_site_image_url()),
                       contents()),
      redirect_to_cross_site_image_url().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_image_url()), url().GetOrigin().spec());

  // Cross-site iframe navigations should follow the default referrer policy.
  NavigateCrossSiteRedirectIframe();
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, child_frame()),
            url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), url().GetOrigin().spec());

  // Cross-site navigations  should follow the default referrer policy.
  RedirectToPageWithLink(redirect_to_cross_site_url(), cross_site_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()),
            redirect_to_cross_site_url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()),
            redirect_to_cross_site_url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(redirect_to_cross_site_url()),
            link_url().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrer) {
  BlockReferrers();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Same-origin sub-resources within the page get the page URL as referrer.
  EXPECT_EQ(ExecScriptGetStr(create_image_script(same_origin_image_url()),
                             contents()),
            same_origin_image_url().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_image_url()), url().spec());

  // Cross-site sub-resources within the page should follow the default referrer
  // policy.
  EXPECT_EQ(
      ExecScriptGetStr(create_image_script(cross_site_image_url()), contents()),
      cross_site_image_url().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_image_url()), url().GetOrigin().spec());

  // Same-origin iframe navigations get the page URL as referrer.
  NavigateIframe(same_origin_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, child_frame()), url().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_url()), url().spec());

  // Cross-site iframe navigations should follow the default referrer policy.
  NavigateIframe(cross_site_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, child_frame()),
            url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), url().GetOrigin().spec());

  // Same-origin navigations get the original page URL as the referrer.
  NavigateDirectlyToPageWithLink(same_origin_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), link_url().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_url()), link_url().spec());

  // Same-site but cross-origin navigations get the original page origin as the
  // referrer.
  const std::string expected_referrer = link_url().GetOrigin().spec();
  NavigateDirectlyToPageWithLink(same_site_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), expected_referrer);
  EXPECT_EQ(GetLastReferrer(same_site_url()), expected_referrer);

  // Cross-site navigations should follow the default referrer policy.
  NavigateDirectlyToPageWithLink(cross_site_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), expected_referrer);
  EXPECT_EQ(GetLastReferrer(cross_site_url()), expected_referrer);

  // Check that a less restrictive policy is not respected.
  NavigateDirectlyToPageWithLink(cross_site_url(),
                                 "no-referrer-when-downgrade");
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), expected_referrer);
  EXPECT_EQ(GetLastReferrer(cross_site_url()), expected_referrer);

  // Check that "no-referrer" policy is respected as more restrictive.
  NavigateDirectlyToPageWithLink(same_origin_url(), "no-referrer");
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_EQ(GetLastReferrer(same_origin_url()), "");

  NavigateDirectlyToPageWithLink(cross_site_url(), "no-referrer");
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_EQ(GetLastReferrer(cross_site_url()), "");

  // Check that "same-origin" policy is respected as more restrictive.
  NavigateDirectlyToPageWithLink(cross_site_url(), "same-origin");
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_EQ(GetLastReferrer(cross_site_url()), "");
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrerRedirects) {
  BlockReferrers();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Cross-site sub-resources within the page should follow the default referrer
  // policy.
  EXPECT_EQ(
      ExecScriptGetStr(create_image_script(redirect_to_cross_site_image_url()),
                       contents()),
      redirect_to_cross_site_image_url().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_image_url()), url().GetOrigin().spec());

  // Cross-site iframe navigations should follow the default referrer policy.
  NavigateCrossSiteRedirectIframe();
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, child_frame()),
            url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), url().GetOrigin().spec());

  // Cross-site navigations should follow the default referrer policy.
  RedirectToPageWithLink(redirect_to_cross_site_url(), cross_site_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()),
            redirect_to_cross_site_url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()),
            redirect_to_cross_site_url().GetOrigin().spec());
  // Intermidiate same-origin navigation gets full referrer.
  EXPECT_EQ(GetLastReferrer(redirect_to_cross_site_url()),
            link_url().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       AllowReferrer) {
  AllowReferrers();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Cross-site sub-resources within the page get the page origin as a referrer.
  EXPECT_EQ(
      ExecScriptGetStr(create_image_script(cross_site_image_url()), contents()),
      cross_site_image_url().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_image_url()), url().GetOrigin().spec());

  // A cross-site iframe navigation gets the origin of the first one as
  // referrer.
  NavigateIframe(cross_site_url());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), url().GetOrigin());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, child_frame()),
            url().GetOrigin().spec());

  // Same-site but cross-origin navigations get the original page origin as the
  // referrer.
  NavigateDirectlyToPageWithLink(same_site_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()),
            link_url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(same_site_url()), link_url().GetOrigin().spec());

  // Cross-site navigations get origin as a referrer.
  NavigateDirectlyToPageWithLink(cross_site_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()),
            url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), url().GetOrigin().spec());

  // Check that a less restrictive policy is respected.
  GURL link = NavigateDirectlyToPageWithLink(cross_site_url(),
                                             "no-referrer-when-downgrade");
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), link.spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), link.spec());

  // Check that "no-referrer" policy is respected.
  NavigateDirectlyToPageWithLink(same_origin_url(), "no-referrer");
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_EQ(GetLastReferrer(same_origin_url()), "");

  NavigateDirectlyToPageWithLink(cross_site_url(), "no-referrer");
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_EQ(GetLastReferrer(cross_site_url()), "");

  // Check that "same-origin" policy is respected.
  link = NavigateDirectlyToPageWithLink(same_origin_url(), "same-origin");
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), link.spec());
  EXPECT_EQ(GetLastReferrer(same_origin_url()), link.spec());

  NavigateDirectlyToPageWithLink(same_site_url(), "same-origin");
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_EQ(GetLastReferrer(same_site_url()), "");

  // Check that "strict-origin" policy is respected.
  link = NavigateDirectlyToPageWithLink(same_site_url(), "strict-origin");
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()),
            link.GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(same_site_url()), link.GetOrigin().spec());

  NavigateDirectlyToPageWithLink(same_origin_url(), "strict-origin");
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()),
            link.GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_url()), link.GetOrigin().spec());

  NavigateDirectlyToPageWithLink(cross_site_url(), "strict-origin");
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()),
            link.GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), link.GetOrigin().spec());

  // Check cross-site navigations with redirect.
  RedirectToPageWithLink(redirect_to_cross_site_url(), cross_site_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()),
            link.GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), link.GetOrigin().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrerShieldsDown) {
  BlockReferrers();
  ShieldsDown();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Cross-site sub-resources within the page get the page origin as referrer.
  EXPECT_EQ(
      ExecScriptGetStr(create_image_script(cross_site_image_url()), contents()),
      cross_site_image_url().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_image_url()), url().GetOrigin().spec());

  // A cross-origin iframe navigation gets the origin of the first one as
  // referrer.
  NavigateIframe(cross_site_url());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), url().GetOrigin());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, child_frame()),
            url().GetOrigin().spec());

  // Cross-site navigations get origin as a referrer.
  NavigateDirectlyToPageWithLink(cross_site_url());
  EXPECT_EQ(ExecScriptGetStr(kReferrerScript, contents()),
            url().GetOrigin().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), url().GetOrigin().spec());
}

IN_PROC_BROWSER_TEST_F(
    BraveContentSettingsAgentImplNoEphemeralStorageBrowserTest,
    BlockThirdPartyCookieByDefault) {
  NavigateToPageWithIframe();
  CheckCookie(child_frame(), kTestCookie);

  NavigateIframe(cross_site_url());
  Check3PCookie(child_frame(), kEmptyCookie);
}

// With ephemeral storage enabled, the 3p cookie should still appear to be set
// correctly.
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockThirdPartyCookieByDefault) {
  NavigateToPageWithIframe();
  CheckCookie(child_frame(), kTestCookie);

  NavigateIframe(cross_site_url());
  Check3PCookie(child_frame(), kTestCookie);
}

IN_PROC_BROWSER_TEST_F(
    BraveContentSettingsAgentImplNoEphemeralStorageBrowserTest,
    ExplicitBlock3PCookies) {
  Block3PCookies();

  NavigateToPageWithIframe();
  CheckCookie(child_frame(), kTestCookie);

  NavigateIframe(cross_site_url());
  Check3PCookie(child_frame(), kEmptyCookie);
}

// With ephemeral storage enabled, the 3p cookie should still appear to be
// set correctly.
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       ExplicitBlock3PCookies) {
  Block3PCookies();

  NavigateToPageWithIframe();
  CheckCookie(child_frame(), kTestCookie);

  NavigateIframe(cross_site_url());
  Check3PCookie(child_frame(), kTestCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest, BlockCookies) {
  BlockCookies();

  NavigateToPageWithIframe();
  CheckCookie(contents(), kEmptyCookie);

  NavigateIframe(cross_site_url());
  Check3PCookie(child_frame(), kEmptyCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest, AllowCookies) {
  AllowCookies();

  NavigateToPageWithIframe();
  CheckCookie(contents(), kTestCookie);
  EXPECT_EQ(COOKIE_STR, content::GetCookies(browser()->profile(), url()));

  NavigateIframe(cross_site_url());
  Check3PCookie(child_frame(), kTestCookie);
  EXPECT_EQ(COOKIE_STR,
            content::GetCookies(browser()->profile(), cross_site_url()));
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       ChromiumCookieBlockOverridesBraveAllowCookiesTopLevel) {
  AllowCookies();
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  content_settings->SetContentSettingCustomScope(
      top_level_page_pattern(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::COOKIES, CONTENT_SETTING_BLOCK);

  NavigateToPageWithIframe();
  CheckCookie(contents(), kEmptyCookie);

  NavigateIframe(cross_site_url());
  Check3PCookie(child_frame(), kTestCookie);
}

IN_PROC_BROWSER_TEST_F(
    BraveContentSettingsAgentImplNoEphemeralStorageBrowserTest,
    ChromiumCookieBlockOverridesBraveAllowCookiesIframe) {
  AllowCookies();
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  content_settings->SetContentSettingCustomScope(
      iframe_pattern(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::COOKIES, CONTENT_SETTING_BLOCK);

  NavigateToPageWithIframe();
  CheckCookie(contents(), kTestCookie);

  NavigateIframe(cross_site_url());
  Check3PCookie(child_frame(), kEmptyCookie);
}

// Ephemeral storage still works with the Chromium cookie blocking content
// setting.
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       ChromiumCookieBlockOverridesBraveAllowCookiesIframe) {
  AllowCookies();
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  content_settings->SetContentSettingCustomScope(
      iframe_pattern(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::COOKIES, CONTENT_SETTING_BLOCK);

  NavigateToPageWithIframe();
  CheckCookie(contents(), kTestCookie);

  NavigateIframe(cross_site_url());
  Check3PCookie(child_frame(), kTestCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       ShieldsDownOverridesBlockedCookies) {
  BlockCookies();
  ShieldsDown();

  NavigateToPageWithIframe();
  CheckCookie(contents(), kTestCookie);

  NavigateIframe(cross_site_url());
  Check3PCookie(child_frame(), kTestCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       ShieldsDownAllowsCookies) {
  ShieldsDown();

  NavigateToPageWithIframe();
  CheckCookie(contents(), kTestCookie);

  NavigateIframe(cross_site_url());
  Check3PCookie(child_frame(), kTestCookie);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       ShieldsUpBlockCookies) {
  BlockCookies();
  ShieldsUp();

  NavigateToPageWithIframe();
  CheckCookie(contents(), kEmptyCookie);

  NavigateIframe(cross_site_url());
  Check3PCookie(child_frame(), kEmptyCookie);
}

IN_PROC_BROWSER_TEST_F(
    BraveContentSettingsAgentImplNoEphemeralStorageBrowserTest,
    LocalStorageTest) {
  NavigateToPageWithIframe();

  // Local storage is null, accessing it shouldn't throw.
  NavigateIframe(cross_site_url());
  CheckLocalStorageAccessDenied(child_frame());

  // Local storage is null, accessing it doesn't throw.
  NavigateIframe(cross_site_url());
  CheckLocalStorageAccessDenied(child_frame());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       LocalStorageTest) {
  // Brave defaults:
  // Main frame storage is always accessible.
  NavigateToPageWithIframe();
  CheckLocalStorageAccessible(contents());

  // Local storage is null, accessing it shouldn't throw.
  NavigateIframe(cross_site_url());
  CheckLocalStorageAccessible(child_frame());

  // Cookies allowed:
  AllowCookies();
  // Main frame storage is always accessible.
  NavigateToPageWithIframe();
  CheckLocalStorageAccessible(contents());

  // Local storage is accessible.
  NavigateIframe(cross_site_url());
  CheckLocalStorageAccessible(child_frame());

  // Thirdy-part cookies blocked:
  Block3PCookies();
  // Main frame storage is always accessible.
  NavigateToPageWithIframe();
  CheckLocalStorageAccessible(contents());

  // Local storage is null, accessing it doesn't throw.
  NavigateIframe(cross_site_url());
  CheckLocalStorageAccessible(child_frame());

  // Shields down, third-party cookies still blocked:
  ShieldsDown();
  // Main frame storage is always accessible.
  NavigateToPageWithIframe();
  CheckLocalStorageAccessible(contents());

  // Local storage is accessible.
  NavigateIframe(cross_site_url());
  CheckLocalStorageAccessible(child_frame());

  // Throws when used on a data url.
  const GURL data_url("data:text/html,<title>Data URL</title>");
  ui_test_utils::NavigateToURL(browser(), data_url);
  CheckLocalStorageThrows(contents());

  // Throws in a sandboxed iframe.
  const GURL sandboxed(
      https_server().GetURL("a.com", "/sandboxed_iframe.html"));
  ui_test_utils::NavigateToURL(browser(), sandboxed);
  CheckLocalStorageThrows(child_frame());
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
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_BLOCK);

  NavigateToURLUntilLoadStop("b.com", "/load_js_from_origins.html");
  EXPECT_EQ(contents()->GetAllFrames().size(), 1u);
}
