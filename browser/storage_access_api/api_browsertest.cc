/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string_view>

#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/permissions/test/permission_request_observer.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_paths.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/test_data_directory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kHostA[] = "a.test";
constexpr char kHostB[] = "b.test";

// Path for URL of custom response
constexpr char kEchoCookiesWithCorsPath[] = "/echocookieswithcors";

// Responds to a request to /echocookieswithcors with the cookies that were
// sent with the request. We can't use the default handler /echoheader?Cookie
// here, because it doesn't send the appropriate Access-Control-Allow-Origin
// and Access-Control-Allow-Credentials headers (which are required for this
// to work for cross-origin requests in the tests).
std::unique_ptr<net::test_server::HttpResponse>
HandleEchoCookiesWithCorsRequest(const net::test_server::HttpRequest& request) {
  if (request.relative_url != kEchoCookiesWithCorsPath) {
    return nullptr;
  }
  auto http_response = std::make_unique<net::test_server::BasicHttpResponse>();
  std::string content = "None";
  // Get the 'Cookie' header that was sent in the request.
  if (auto it = request.headers.find(net::HttpRequestHeaders::kCookie);
      it != request.headers.end()) {
    content = it->second;
  }

  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/plain");
  // Set the cors enabled headers.
  if (auto it = request.headers.find(net::HttpRequestHeaders::kOrigin);
      it != request.headers.end()) {
    http_response->AddCustomHeader("Access-Control-Allow-Origin", it->second);
    http_response->AddCustomHeader("Vary", "origin");
    http_response->AddCustomHeader("Access-Control-Allow-Credentials", "true");
  }
  http_response->set_content(content);

  return http_response;
}

std::string CookieAttributes(std::string_view domain) {
  return base::StrCat({";SameSite=None;Secure;Domain=", domain, ";Path=/"});
}

}  // namespace

// The test is based on Chromium's StorageAccessAPIBrowserTest.
class StorageAccessAPIBrowserTest : public PlatformBrowserTest {
 public:
  StorageAccessAPIBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    base::FilePath path = GetChromeTestDataDir();
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromDirectory(path);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&HandleEchoCookiesWithCorsRequest));
    ASSERT_TRUE(https_server_.Start());

    // All the sites used during the test should have a cookie.
    SetCrossSiteCookieOnDomain(kHostA);
    SetCrossSiteCookieOnDomain(kHostB);

    // The test invokes document.requestStorageAccess from a kHostB
    // iframe. We pre-seed that site with user interaction, to avoid being
    // blocked by the top-level user interaction heuristic.
    EnsureUserInteractionOn(kHostB);
  }

  base::FilePath GetChromeTestDataDir() const {
    return base::FilePath(FILE_PATH_LITERAL("chrome/test/data"));
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  Profile* profile() { return chrome_test_utils::GetProfile(this); }

  void SetCrossSiteCookieOnDomain(std::string_view domain) {
    GURL domain_url = GetURL(domain);
    std::string cookie = base::StrCat({"cross-site=", domain});
    content::SetCookie(profile(), domain_url,
                       base::StrCat({cookie, CookieAttributes(domain)}));
    ASSERT_THAT(content::GetCookies(profile(), domain_url),
                testing::HasSubstr(cookie));
  }

  GURL GetURL(std::string_view host) { return https_server_.GetURL(host, "/"); }

  void EnsureUserInteractionOn(std::string_view host) {
    ASSERT_TRUE(content::NavigateToURL(
        web_contents(), https_server_.GetURL(host, "/empty.html")));
    // ExecJs runs with a synthetic user interaction (by default), which is all
    // we need, so our script is a no-op.
    ASSERT_TRUE(content::ExecJs(web_contents(), ""));
  }

  void SetBlockThirdPartyCookies() {
    profile()->GetPrefs()->SetInteger(
        prefs::kCookieControlsMode,
        static_cast<int>(
            content_settings::CookieControlsMode::kBlockThirdParty));
  }

  void NavigateToPageWithFrame(const std::string& host) {
    GURL main_url(https_server_.GetURL(host, "/iframe.html"));
    ASSERT_TRUE(content::NavigateToURL(web_contents(), main_url));
  }

  void NavigateFrameTo(const GURL& url) {
    EXPECT_TRUE(NavigateIframeToURL(web_contents(), "test", url));
  }

  GURL EchoCookiesURL(std::string_view host) {
    return https_server_.GetURL(host, "/echoheader?cookie");
  }

  content::RenderFrameHost* GetFrame() {
    return ChildFrameAt(GetPrimaryMainFrame(), 0);
  }

  content::RenderFrameHost* GetPrimaryMainFrame() {
    return web_contents()->GetPrimaryMainFrame();
  }

 private:
  net::test_server::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(StorageAccessAPIBrowserTest, EnsureNoPrompt) {
  SetBlockThirdPartyCookies();
  NavigateToPageWithFrame(kHostA);
  NavigateFrameTo(EchoCookiesURL(kHostB));

  // Because we set storage-access content setting to be CONTENT_SETTING_BLOCK
  // by default, we should not see a prompt.
  permissions::PermissionRequestObserver pre_observer(web_contents());
  ASSERT_FALSE(pre_observer.request_shown());
  ASSERT_FALSE(content::ExecJs(GetFrame(), "document.requestStorageAccess()"));
  ASSERT_FALSE(pre_observer.request_shown());
}
