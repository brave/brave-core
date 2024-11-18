/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string_view>

#include "base/feature_list.h"
#include "base/path_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/google/core/common/google_switches.h"
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
#include "net/test/embedded_test_server/request_handler_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/origin.h"

using brave_shields::ControlType;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;

namespace {

constexpr char kIframeID[] = "test";

constexpr char kPointInPathScript[] = R"(
  var canvas = document.createElement('canvas');
  var ctx = canvas.getContext('2d');
  ctx.rect(10, 10, 100, 100);
  ctx.stroke();
  ctx.isPointInPath(10, 10);
)";

constexpr char kGetImageDataScript[] =
    "var adder = (a, x) => a + x;"
    "var canvas = document.createElement('canvas');"
    "canvas.width = 16;"
    "canvas.height = 16;"
    "var ctx = canvas.getContext('2d');"
    "var data = ctx.createImageData(canvas.width, canvas.height);"
    "ctx.putImageData(data, 0, 0);"
    "ctx.getImageData(0, 0, canvas.width, canvas.height).data.reduce(adder);";

constexpr char kImageScript[] = R"(
  let frame = document.createElement('img');
  frame.src = $1;
  new Promise(resolve => {
    frame.onload = () => { resolve(frame.src); };
    frame.onerror = (e) => {
      resolve('failure: ' + e.toString());
    };
    document.body.appendChild(frame);
  });
)";

constexpr int kExpectedImageDataHashFarblingBalanced = 172;
constexpr int kExpectedImageDataHashFarblingOff = 0;
constexpr int kExpectedImageDataHashFarblingMaximum =
    kExpectedImageDataHashFarblingBalanced;
constexpr int kExpectedImageDataHashFarblingBalancedGoogleCom = 182;

constexpr char kEmptyCookie[] = "";

#define COOKIE_STR "test=hi"

constexpr char kTestCookie[] = COOKIE_STR;

constexpr char kCookieScript[] = "document.cookie = '" COOKIE_STR
                                 "'"
                                 "; document.cookie;";

constexpr char kCookie3PScript[] = "document.cookie = '" COOKIE_STR
                                   ";SameSite=None;Secure'"
                                   "; document.cookie;";

constexpr char kReferrerScript[] = "document.referrer;";

constexpr char kTitleScript[] = "document.title;";

GURL GetOriginURL(const GURL& url) {
  return url::Origin::Create(url).GetURL();
}

// Remaps requests from /maps/simple.html to /simple.html
std::unique_ptr<HttpResponse> HandleGoogleMapsFileRequest(
    const base::FilePath& server_root,
    const HttpRequest& request) {
  HttpRequest new_request(request);
  if (!new_request.relative_url.starts_with("/maps")) {
    // This handler is only relevant for a Google Maps url.
    return nullptr;
  }
  new_request.relative_url = new_request.relative_url.substr(5);
  return HandleFileRequest(server_root, new_request);
}

}  // namespace

class BraveContentSettingsAgentImplBrowserTest : public InProcessBrowserTest {
 public:
  BraveContentSettingsAgentImplBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(net::features::kBraveEphemeralStorage);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.RegisterDefaultHandler(
        base::BindRepeating(&HandleGoogleMapsFileRequest, test_data_dir));
    content::SetupCrossSiteRedirector(&https_server_);
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &BraveContentSettingsAgentImplBrowserTest::SaveReferrer,
        base::Unretained(this)));

    ASSERT_TRUE(https_server_.Start());

    url_ = https_server_.GetURL("a.test", "/iframe.html");
    cross_site_url_ = https_server_.GetURL("b.test", "/simple.html");
    cross_site_image_url_ = https_server_.GetURL("b.test", "/logo.png");
    link_url_ = https_server_.GetURL("a.test", "/simple_link.html");
    redirect_to_cross_site_url_ =
        https_server_.GetURL("a.test", "/cross-site/b.test/simple.html");
    redirect_to_cross_site_image_url_ =
        https_server_.GetURL("a.test", "/cross-site/b.test/logo.png");
    same_site_url_ = https_server_.GetURL("sub.a.test", "/simple.html");
    same_origin_url_ = https_server_.GetURL("a.test", "/simple.html");
    same_origin_image_url_ = https_server_.GetURL("a.test", "/logo.png");
    top_level_page_url_ = https_server_.GetURL("a.test", "/");
    top_level_page_pattern_ =
        ContentSettingsPattern::FromString("https://a.test/*");
    iframe_pattern_ = ContentSettingsPattern::FromString("https://b.test/*");
    first_party_pattern_ =
        ContentSettingsPattern::FromString("https://firstParty/*");
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Since the HTTPS server only serves a valid cert for localhost,
    // this is needed to load pages from "www.google.*" without an interstitial.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);

    // The production code only allows known ports (80 for http and 443 for
    // https), but the test server runs on a random port.
    command_line->AppendSwitch(switches::kIgnoreGooglePortNumbers);
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
    ContentSettingsForOneType settings =
        content_settings()->GetSettingsForOneType(
            ContentSettingsType::BRAVE_REFERRERS);
    // default plus new setting
    EXPECT_EQ(settings.size(), 2u);
  }

  void AllowReferrers() {
    content_settings()->SetContentSettingCustomScope(
        top_level_page_pattern(), ContentSettingsPattern::Wildcard(),
        ContentSettingsType::BRAVE_REFERRERS, CONTENT_SETTING_ALLOW);
    ContentSettingsForOneType settings =
        content_settings()->GetSettingsForOneType(
            ContentSettingsType::BRAVE_REFERRERS);
    // default plus new setting
    EXPECT_EQ(settings.size(), 2u);
  }

  void Block3PCookies() {
    brave_shields::SetCookieControlType(
        content_settings(), browser()->profile()->GetPrefs(),
        ControlType::BLOCK_THIRD_PARTY, top_level_page_url());
  }

  void BlockCookies() {
    brave_shields::SetCookieControlType(
        content_settings(), browser()->profile()->GetPrefs(),
        ControlType::BLOCK, top_level_page_url());
  }

  void AllowCookies() {
    brave_shields::SetCookieControlType(
        content_settings(), browser()->profile()->GetPrefs(),
        ControlType::ALLOW, top_level_page_url());
  }

  void ShieldsDown() {
    brave_shields::SetBraveShieldsEnabled(content_settings(), false,
                                          top_level_page_url());
  }

  void ShieldsUp() {
    brave_shields::SetBraveShieldsEnabled(content_settings(), true,
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
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT, top_level_page_url());
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
    return ChildFrameAt(contents()->GetPrimaryMainFrame(), 0);
  }

  // Returns the URL from which we are navigating away.
  GURL NavigateDirectlyToPageWithLink(const GURL& url,
                                      const std::string& referrer_policy = {}) {
    const std::string link_query =
        referrer_policy.empty() ? "" : "?policy=" + referrer_policy;
    GURL link(link_url().spec() + link_query);
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), link));

    EXPECT_EQ(true,
              content::EvalJs(contents(),
                              content::JsReplace("clickLink($1)", url.spec())));
    EXPECT_TRUE(WaitForLoadStop(contents()));

    return link;
  }

  void RedirectToPageWithLink(const GURL& url, const GURL& final_url) {
    NavigateDirectlyToPageWithLink(url);
    content::RenderFrameHost* main_frame = contents()->GetPrimaryMainFrame();
    EXPECT_EQ(main_frame->GetLastCommittedURL(), final_url);
  }

  void NavigateToPageWithIframe() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url()));
    ASSERT_EQ(CollectAllRenderFrameHosts(contents()).size(), 2u)
        << "Two frames (main + iframe) should be created.";
    content::RenderFrameHost* main_frame = contents()->GetPrimaryMainFrame();
    EXPECT_EQ(main_frame->GetLastCommittedURL(), url());
  }

  void NavigateToURLUntilLoadStop(const std::string& origin,
                                  const std::string& path) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), https_server().GetURL(origin, path)));
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
  void CheckCookie(T* frame, std::string_view cookie) {
    EXPECT_EQ(cookie, EvalJs(frame, kCookieScript));
  }

  template <typename T>
  void Check3PCookie(T* frame, std::string_view cookie) {
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
            "a JavaScript error: \"Error: Failed to read the 'localStorage' "
            "property from 'Window': Access is denied for this document.\n"));
    EXPECT_THAT(
        EvalJs(frame, "sessionStorage").error,
        ::testing::StartsWith(
            "a JavaScript error: \"Error: Failed to read the 'sessionStorage' "
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
  mutable base::Lock last_referrers_lock_;
  std::map<GURL, std::string> last_referrers_;

  base::ScopedTempDir temp_user_data_dir_;
  net::test_server::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       FarbleGetImageData) {
  // Farbling should be balanced by default
  NavigateToPageWithIframe();
  EXPECT_EQ(kExpectedImageDataHashFarblingBalanced,
            content::EvalJs(contents(), kGetImageDataScript));

  // The iframe should have the same result as the top frame because farbling is
  // based on the top frame's session token.
  NavigateIframe(cross_site_url());
  EXPECT_EQ(kExpectedImageDataHashFarblingBalanced,
            content::EvalJs(child_frame(), kGetImageDataScript));

  // Farbling should be off if shields is down
  ShieldsDown();
  NavigateToPageWithIframe();
  EXPECT_EQ(kExpectedImageDataHashFarblingOff,
            content::EvalJs(contents(), kGetImageDataScript));

  // Farbling should be off if shields is up but fingerprinting is allowed
  // via content settings
  ShieldsUp();
  AllowFingerprinting();
  NavigateToPageWithIframe();
  EXPECT_EQ(kExpectedImageDataHashFarblingOff,
            content::EvalJs(contents(), kGetImageDataScript));
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       FarbleGetImageDataGoogleMapsException) {
  // Farbling should be disabled on Google Maps
  SetFingerprintingDefault();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server().GetURL("google.com", "/maps/simple.html")));
  EXPECT_EQ(kExpectedImageDataHashFarblingOff,
            content::EvalJs(contents(), kGetImageDataScript));

  // Farbling should not be disabled on other Google things
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server().GetURL("google.com", "/simple.html")));
  EXPECT_EQ(kExpectedImageDataHashFarblingBalancedGoogleCom,
            content::EvalJs(contents(), kGetImageDataScript));

  // Farbling should be disabled on google.co.uk maps
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server().GetURL("google.co.uk", "/maps/simple.html")));
  EXPECT_EQ(kExpectedImageDataHashFarblingOff,
            content::EvalJs(contents(), kGetImageDataScript));

  // Farbling should be disabled on google.de maps
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server().GetURL("google.de", "/maps/simple.html")));
  EXPECT_EQ(kExpectedImageDataHashFarblingOff,
            content::EvalJs(contents(), kGetImageDataScript));
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
#if BUILDFLAG(IS_LINUX)
#define MAYBE_WebGLReadPixels DISABLED_WebGLReadPixels
#else
#define MAYBE_WebGLReadPixels WebGLReadPixels
#endif
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplV2BrowserTest,
                       MAYBE_WebGLReadPixels) {
  std::string origin = "a.test";
  std::string path = "/webgl/readpixels.html";

  // Farbling level: maximum
  // WebGL readPixels(): blocked
  BlockFingerprinting();
  NavigateToURLUntilLoadStop(origin, path);
  EXPECT_EQ(content::EvalJs(contents(), kTitleScript), "1");

  // Farbling level: balanced (default)
  // WebGL readPixels(): allowed
  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(origin, path);
  EXPECT_EQ(content::EvalJs(contents(), kTitleScript), "0");

  // Farbling level: off
  // WebGL readPixels(): allowed
  AllowFingerprinting();
  NavigateToURLUntilLoadStop(origin, path);
  EXPECT_EQ(content::EvalJs(contents(), kTitleScript), "0");
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplV2BrowserTest,
                       FarbleGetImageData) {
  // Farbling should be default when kBraveFingerprintingV2 is enabled
  // because it uses a different content setting
  NavigateToPageWithIframe();
  EXPECT_EQ(kExpectedImageDataHashFarblingBalanced,
            content::EvalJs(contents(), kGetImageDataScript));

  // Farbling should be maximum if fingerprinting is blocked via content
  // settings and kBraveFingerprintingV2 is enabled
  BlockFingerprinting();
  NavigateToPageWithIframe();
  EXPECT_EQ(kExpectedImageDataHashFarblingMaximum,
            content::EvalJs(contents(), kGetImageDataScript));

  // Farbling should be balanced if fingerprinting is default via
  // content settings and kBraveFingerprintingV2 is enabled
  SetFingerprintingDefault();
  NavigateToPageWithIframe();
  EXPECT_EQ(kExpectedImageDataHashFarblingBalanced,
            content::EvalJs(contents(), kGetImageDataScript));

  // Farbling should be off if fingerprinting is allowed via
  // content settings and kBraveFingerprintingV2 is enabled
  AllowFingerprinting();
  NavigateToPageWithIframe();
  EXPECT_EQ(kExpectedImageDataHashFarblingOff,
            content::EvalJs(contents(), kGetImageDataScript));
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplV2BrowserTest,
                       CanvasIsPointInPath) {
  // Farbling level: maximum
  // Canvas isPointInPath(): blocked
  BlockFingerprinting();
  NavigateToPageWithIframe();
  EXPECT_EQ(false, content::EvalJs(contents(), kPointInPathScript));
  NavigateIframe(cross_site_url());
  EXPECT_EQ(false, content::EvalJs(child_frame(), kPointInPathScript));

  // Farbling level: balanced (default)
  // Canvas isPointInPath(): allowed
  SetFingerprintingDefault();
  NavigateToPageWithIframe();
  EXPECT_EQ(true, content::EvalJs(contents(), kPointInPathScript));
  NavigateIframe(cross_site_url());
  EXPECT_EQ(true, content::EvalJs(child_frame(), kPointInPathScript));

  // Farbling level: off
  // Canvas isPointInPath(): allowed
  AllowFingerprinting();
  NavigateToPageWithIframe();
  EXPECT_EQ(true, content::EvalJs(contents(), kPointInPathScript));
  NavigateIframe(cross_site_url());
  EXPECT_EQ(true, content::EvalJs(child_frame(), kPointInPathScript));

  // Shields: down
  // Canvas isPointInPath(): allowed
  BlockFingerprinting();
  ShieldsDown();
  AllowFingerprinting();
  NavigateToPageWithIframe();
  EXPECT_EQ(true, content::EvalJs(contents(), kPointInPathScript));
  NavigateIframe(cross_site_url());
  EXPECT_EQ(true, content::EvalJs(child_frame(), kPointInPathScript));
}

// TODO(iefremov): We should reduce the copy-paste amount in these tests.
IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrerByDefault) {
  ContentSettingsForOneType settings =
      content_settings()->GetSettingsForOneType(
          ContentSettingsType::BRAVE_REFERRERS);
  // default setting
  EXPECT_EQ(settings.size(), 1u)
      << "There should not be any visible referrer rules.";

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Same-origin sub-resources within the page get the page URL as referrer.
  EXPECT_EQ(
      content::EvalJs(contents(), content::JsReplace(kImageScript,
                                                     same_origin_image_url())),
      same_origin_image_url().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_image_url()), url().spec());

  // Cross-site sub-resources within the page should follow the default referrer
  // policy.
  EXPECT_EQ(
      content::EvalJs(contents(),
                      content::JsReplace(kImageScript, cross_site_image_url())),
      cross_site_image_url().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_image_url()),
            GetOriginURL(url()).spec());

  // Same-origin iframe navigations get the page URL as referrer.
  NavigateIframe(same_origin_url());
  EXPECT_EQ(content::EvalJs(child_frame(), kReferrerScript), url().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_url()), url().spec());

  // Cross-site iframe navigations should follow the default referrer policy.
  NavigateIframe(cross_site_url());
  EXPECT_EQ(content::EvalJs(child_frame(), kReferrerScript),
            GetOriginURL(url()).spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), GetOriginURL(url()).spec());

  // Same-origin navigations get the original page origin as the referrer.
  NavigateDirectlyToPageWithLink(same_origin_url());
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), link_url().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_url()), link_url().spec());

  // Same-site but cross-origin navigations get the original page origin as the
  // referrer.
  NavigateDirectlyToPageWithLink(same_site_url());
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript),
            GetOriginURL(link_url()).spec());
  EXPECT_EQ(GetLastReferrer(same_site_url()), GetOriginURL(link_url()).spec());

  // Cross-site navigations should follow the default referrer policy.
  NavigateDirectlyToPageWithLink(cross_site_url());
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript),
            GetOriginURL(link_url()).spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), GetOriginURL(link_url()).spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrerByDefaultRedirects) {
  ContentSettingsForOneType settings =
      content_settings()->GetSettingsForOneType(
          ContentSettingsType::BRAVE_REFERRERS);
  // default setting
  EXPECT_EQ(settings.size(), 1u)
      << "There should not be any visible referrer rules.";

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Cross-site sub-resources within the page should follow the default referrer
  // policy.
  EXPECT_EQ(
      content::EvalJs(
          contents(),
          content::JsReplace(kImageScript, redirect_to_cross_site_image_url())),
      redirect_to_cross_site_image_url().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_image_url()),
            GetOriginURL(url()).spec());

  // Cross-site iframe navigations should follow the default referrer policy.
  NavigateCrossSiteRedirectIframe();
  EXPECT_EQ(content::EvalJs(child_frame(), kReferrerScript),
            GetOriginURL(url()).spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), GetOriginURL(url()).spec());

  // Cross-site navigations  should follow the default referrer policy.
  RedirectToPageWithLink(redirect_to_cross_site_url(), cross_site_url());
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript),
            GetOriginURL(redirect_to_cross_site_url()).spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()),
            GetOriginURL(redirect_to_cross_site_url()).spec());
  EXPECT_EQ(GetLastReferrer(redirect_to_cross_site_url()), link_url().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrer) {
  BlockReferrers();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Same-origin sub-resources within the page get the page URL as referrer.
  EXPECT_EQ(
      content::EvalJs(contents(), content::JsReplace(kImageScript,
                                                     same_origin_image_url())),
      same_origin_image_url().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_image_url()), url().spec());

  // Cross-site sub-resources within the page should follow the default referrer
  // policy.
  EXPECT_EQ(
      content::EvalJs(contents(),
                      content::JsReplace(kImageScript, cross_site_image_url())),
      cross_site_image_url().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_image_url()),
            GetOriginURL(url()).spec());

  // Same-origin iframe navigations get the page URL as referrer.
  NavigateIframe(same_origin_url());
  EXPECT_EQ(content::EvalJs(child_frame(), kReferrerScript), url().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_url()), url().spec());

  // Cross-site iframe navigations should follow the default referrer policy.
  NavigateIframe(cross_site_url());
  EXPECT_EQ(content::EvalJs(child_frame(), kReferrerScript),
            GetOriginURL(url()).spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), GetOriginURL(url()).spec());

  // Same-origin navigations get the original page URL as the referrer.
  NavigateDirectlyToPageWithLink(same_origin_url());
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), link_url().spec());
  EXPECT_EQ(GetLastReferrer(same_origin_url()), link_url().spec());

  // Same-site but cross-origin navigations get the original page origin as the
  // referrer.
  const std::string expected_referrer = GetOriginURL(link_url()).spec();
  NavigateDirectlyToPageWithLink(same_site_url());
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), expected_referrer);
  EXPECT_EQ(GetLastReferrer(same_site_url()), expected_referrer);

  // Cross-site navigations should follow the default referrer policy.
  NavigateDirectlyToPageWithLink(cross_site_url());
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), expected_referrer);
  EXPECT_EQ(GetLastReferrer(cross_site_url()), expected_referrer);

  // Check that a less restrictive policy is not respected.
  NavigateDirectlyToPageWithLink(cross_site_url(),
                                 "no-referrer-when-downgrade");
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), expected_referrer);
  EXPECT_EQ(GetLastReferrer(cross_site_url()), expected_referrer);

  // Check that "no-referrer" policy is respected as more restrictive.
  NavigateDirectlyToPageWithLink(same_origin_url(), "no-referrer");
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), "");
  EXPECT_EQ(GetLastReferrer(same_origin_url()), "");

  NavigateDirectlyToPageWithLink(cross_site_url(), "no-referrer");
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), "");
  EXPECT_EQ(GetLastReferrer(cross_site_url()), "");

  // Check that "same-origin" policy is respected as more restrictive.
  NavigateDirectlyToPageWithLink(cross_site_url(), "same-origin");
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), "");
  EXPECT_EQ(GetLastReferrer(cross_site_url()), "");
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrerRedirects) {
  BlockReferrers();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Cross-site sub-resources within the page should follow the default referrer
  // policy.
  EXPECT_EQ(
      content::EvalJs(
          contents(),
          content::JsReplace(kImageScript, redirect_to_cross_site_image_url())),
      redirect_to_cross_site_image_url().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_image_url()),
            GetOriginURL(url()).spec());

  // Cross-site iframe navigations should follow the default referrer policy.
  NavigateCrossSiteRedirectIframe();
  EXPECT_EQ(content::EvalJs(child_frame(), kReferrerScript),
            GetOriginURL(url()).spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), GetOriginURL(url()).spec());

  // Cross-site navigations should follow the default referrer policy.
  RedirectToPageWithLink(redirect_to_cross_site_url(), cross_site_url());
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript),
            GetOriginURL(redirect_to_cross_site_url()).spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()),
            GetOriginURL(redirect_to_cross_site_url()).spec());
  // Intermidiate same-origin navigation gets full referrer.
  EXPECT_EQ(GetLastReferrer(redirect_to_cross_site_url()), link_url().spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       AllowReferrer) {
  AllowReferrers();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Cross-site sub-resources within the page get the page origin as a referrer.
  EXPECT_EQ(
      content::EvalJs(contents(),
                      content::JsReplace(kImageScript, cross_site_image_url())),
      cross_site_image_url().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_image_url()),
            GetOriginURL(url()).spec());

  // A cross-site iframe navigation gets the origin of the first one as
  // referrer.
  NavigateIframe(cross_site_url());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), GetOriginURL(url()));
  EXPECT_EQ(content::EvalJs(child_frame(), kReferrerScript),
            GetOriginURL(url()).spec());

  // Same-site but cross-origin navigations get the original page origin as the
  // referrer.
  NavigateDirectlyToPageWithLink(same_site_url());
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript),
            GetOriginURL(link_url()).spec());
  EXPECT_EQ(GetLastReferrer(same_site_url()), GetOriginURL(link_url()).spec());

  // Cross-site navigations get origin as a referrer.
  NavigateDirectlyToPageWithLink(cross_site_url());
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript),
            GetOriginURL(url()).spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), GetOriginURL(url()).spec());

  // Check that a less restrictive policy is respected.
  GURL link = NavigateDirectlyToPageWithLink(cross_site_url(),
                                             "no-referrer-when-downgrade");
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), link.spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), link.spec());

  // Check that "no-referrer" policy is respected.
  NavigateDirectlyToPageWithLink(same_origin_url(), "no-referrer");
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), "");
  EXPECT_EQ(GetLastReferrer(same_origin_url()), "");

  NavigateDirectlyToPageWithLink(cross_site_url(), "no-referrer");
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), "");
  EXPECT_EQ(GetLastReferrer(cross_site_url()), "");

  // Check that "same-origin" policy is respected.
  link = NavigateDirectlyToPageWithLink(same_origin_url(), "same-origin");
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), link.spec());
  EXPECT_EQ(GetLastReferrer(same_origin_url()), link.spec());

  NavigateDirectlyToPageWithLink(same_site_url(), "same-origin");
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), "");
  EXPECT_EQ(GetLastReferrer(same_site_url()), "");

  // Check that "strict-origin" policy is respected.
  link = NavigateDirectlyToPageWithLink(same_site_url(), "strict-origin");
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript),
            GetOriginURL(link).spec());
  EXPECT_EQ(GetLastReferrer(same_site_url()), GetOriginURL(link).spec());

  NavigateDirectlyToPageWithLink(same_origin_url(), "strict-origin");
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript),
            GetOriginURL(link).spec());
  EXPECT_EQ(GetLastReferrer(same_origin_url()), GetOriginURL(link).spec());

  NavigateDirectlyToPageWithLink(cross_site_url(), "strict-origin");
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript),
            GetOriginURL(link).spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), GetOriginURL(link).spec());

  // Check cross-site navigations with redirect.
  RedirectToPageWithLink(redirect_to_cross_site_url(), cross_site_url());
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript),
            GetOriginURL(link).spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), GetOriginURL(link).spec());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockReferrerShieldsDown) {
  BlockReferrers();
  ShieldsDown();

  // The initial navigation doesn't have a referrer.
  NavigateToPageWithIframe();
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript), "");
  EXPECT_TRUE(GetLastReferrer(url()).empty());

  // Cross-site sub-resources within the page get the page origin as referrer.
  EXPECT_EQ(
      content::EvalJs(contents(),
                      content::JsReplace(kImageScript, cross_site_image_url())),
      cross_site_image_url().spec());
  EXPECT_EQ(GetLastReferrer(cross_site_image_url()),
            GetOriginURL(url()).spec());

  // A cross-origin iframe navigation gets the origin of the first one as
  // referrer.
  NavigateIframe(cross_site_url());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), GetOriginURL(url()));
  EXPECT_EQ(content::EvalJs(child_frame(), kReferrerScript),
            GetOriginURL(url()).spec());

  // Cross-site navigations get origin as a referrer.
  NavigateDirectlyToPageWithLink(cross_site_url());
  EXPECT_EQ(content::EvalJs(contents(), kReferrerScript),
            GetOriginURL(url()).spec());
  EXPECT_EQ(GetLastReferrer(cross_site_url()), GetOriginURL(url()).spec());
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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), data_url));
  CheckLocalStorageThrows(contents());

  // Throws in a sandboxed iframe.
  const GURL sandboxed(
      https_server().GetURL("a.test", "/sandboxed_iframe.html"));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), sandboxed));
  CheckLocalStorageThrows(child_frame());
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest, BlockScripts) {
  BlockScripts();

  NavigateToURLUntilLoadStop("a.test", "/load_js_from_origins.html");
  EXPECT_EQ(CollectAllRenderFrameHosts(contents()).size(), 1u);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest, AllowScripts) {
  AllowScripts();

  NavigateToURLUntilLoadStop("a.test", "/load_js_from_origins.html");
  EXPECT_EQ(CollectAllRenderFrameHosts(contents()).size(), 4u);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockScriptsShieldsDown) {
  BlockScripts();
  ShieldsDown();

  NavigateToURLUntilLoadStop("a.test", "/load_js_from_origins.html");
  EXPECT_EQ(CollectAllRenderFrameHosts(contents()).size(), 4u);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsAgentImplBrowserTest,
                       BlockScriptsShieldsDownInOtherTab) {
  // Turn off shields in a.test.
  ShieldsDown();
  // Block scripts in b.test.
  content_settings()->SetContentSettingCustomScope(
      iframe_pattern(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_BLOCK);

  NavigateToURLUntilLoadStop("b.test", "/load_js_from_origins.html");
  EXPECT_EQ(CollectAllRenderFrameHosts(contents()).size(), 1u);
}
