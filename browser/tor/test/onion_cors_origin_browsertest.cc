// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>

#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"
#include "base/values.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/tor/tor_navigation_throttle.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/base/host_port_pair.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "url/gurl.h"

namespace {

constexpr char kPagePath[] = "/page.html";
constexpr char kDenyCorsPath[] = "/deny-cors";
constexpr char kAllowNullOriginPath[] = "/allow-null-origin";
constexpr char kSameSiteCredentialsPath[] = "/same-site-credentials";
constexpr char kSameSiteCorpPath[] = "/same-site-corp";
constexpr char kRedirectPath[] = "/redirect";

constexpr char kFetchScript[] =
    "fetch($1, $2).then(() => 'allowed', () => 'blocked')";

}  // namespace

class OnionCorsOriginBrowserTest : public InProcessBrowserTest {
 public:
  OnionCorsOriginBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &OnionCorsOriginBrowserTest::RecordRequest, base::Unretained(this)));
    https_server_.RegisterRequestHandler(base::BindRepeating(
        &OnionCorsOriginBrowserTest::HandleRequest, base::Unretained(this)));
    ASSERT_TRUE(https_server_.Start());

    net::ProxyConfigServiceTor::SetBypassTorProxyConfigForTesting(true);
    tor::TorNavigationThrottle::SetSkipWaitForTorConnectedForTesting(true);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  // Loads a page from a .onion origin in a Tor window.
  content::WebContents* OpenOnionPage() {
    BrowserWindowInterface* tor_browser =
        TorProfileManager::SwitchToTorProfile(browser()->GetProfile());
    const GURL page_url = https_server_.GetURL("foobar.onion", kPagePath);
    EXPECT_TRUE(ui_test_utils::NavigateToURL(tor_browser, page_url));
    return tor_browser->tab_strip_model()->GetActiveWebContents();
  }

  GURL CrossOriginURL(std::string_view path, std::string_view host = "a.com") {
    return https_server_.GetURL(host, path);
  }

  // The Origin header of the last `method` request for `path` on `host`, or ""
  // if it had none.
  std::string LastOrigin(std::string_view method,
                         std::string_view path,
                         std::string_view host = "a.com") {
    return LastHeader("Origin", method, path, host);
  }

  std::string LastHeader(std::string_view header,
                         std::string_view method,
                         std::string_view path,
                         std::string_view host = "a.com") {
    base::AutoLock auto_lock(lock_);
    const auto& headers = last_headers_[{std::string(host), std::string(method),
                                         std::string(path)}];
    const auto it = headers.find(std::string(header));
    return it == headers.end() ? "" : it->second;
  }

  int RequestCount(std::string_view method,
                   std::string_view path,
                   std::string_view host = "a.com") {
    base::AutoLock auto_lock(lock_);
    return request_count_[{std::string(host), std::string(method),
                           std::string(path)}];
  }

 private:
  using RequestKey = std::tuple<std::string, std::string, std::string>;

  void RecordRequest(const net::test_server::HttpRequest& request) {
    const auto host_header = request.headers.find("Host");
    const std::string host =
        host_header == request.headers.end()
            ? ""
            : net::HostPortPair::FromString(host_header->second).host();
    const RequestKey key{host, request.method_string,
                         std::string(request.GetURL().path())};
    base::AutoLock auto_lock(lock_);
    ++request_count_[key];
    last_headers_[key] = request.headers;
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    const std::string path(request.GetURL().path());
    if (path == kPagePath) {
      response->set_content_type("text/html");
      response->set_content("<html></html>");
    } else if (path == kAllowNullOriginPath) {
      // Allows the null origin, including the preflight for a PUT, and asks
      // for the preflight to be cached.
      response->AddCustomHeader("Access-Control-Allow-Origin", "null");
      response->AddCustomHeader("Access-Control-Allow-Methods", "PUT");
      response->AddCustomHeader("Access-Control-Max-Age", "600");
    } else if (path == kSameSiteCredentialsPath) {
      response->AddCustomHeader("Access-Control-Allow-Origin", "null");
      response->AddCustomHeader("Access-Control-Allow-Credentials", "true");
    } else if (path == kSameSiteCorpPath) {
      response->set_content_type("application/javascript");
      response->set_content("// A same-site resource.\n");
      response->AddCustomHeader("Cross-Origin-Resource-Policy", "same-site");
    } else if (path == kRedirectPath) {
      // Redirects to the URL in the query, keeping the request's method.
      response->set_code(net::HTTP_TEMPORARY_REDIRECT);
      response->AddCustomHeader("Location",
                                std::string(request.GetURL().query()));
    } else if (path != kDenyCorsPath) {
      return nullptr;
    }
    return response;
  }

  content::ContentMockCertVerifier mock_cert_verifier_;
  base::Lock lock_;
  std::map<RequestKey, net::test_server::HttpRequest::HeaderMap> last_headers_
      GUARDED_BY(lock_);
  std::map<RequestKey, int> request_count_ GUARDED_BY(lock_);
  // Declared after the state its callbacks use, so the server shuts down
  // before that state is destroyed.
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(OnionCorsOriginBrowserTest, PreflightOrigin) {
  content::WebContents* web_contents = OpenOnionPage();

  // The server refuses CORS, so only the preflight reaches it.
  EXPECT_EQ("blocked",
            content::EvalJs(
                web_contents,
                content::JsReplace(kFetchScript, CrossOriginURL(kDenyCorsPath),
                                   base::DictValue().Set("method", "PUT"))));
  EXPECT_EQ(LastOrigin("OPTIONS", kDenyCorsPath), "null");
  EXPECT_EQ(LastOrigin("PUT", kDenyCorsPath), "");
}

IN_PROC_BROWSER_TEST_F(OnionCorsOriginBrowserTest, AllowNullOrigin) {
  content::WebContents* web_contents = OpenOnionPage();

  // The CORS check runs against the same opaque origin the request sent.
  EXPECT_EQ(
      "allowed",
      content::EvalJs(
          web_contents,
          content::JsReplace(kFetchScript, CrossOriginURL(kAllowNullOriginPath),
                             base::DictValue().Set("method", "GET"))));
  EXPECT_EQ(LastOrigin("GET", kAllowNullOriginPath), "null");
}

IN_PROC_BROWSER_TEST_F(OnionCorsOriginBrowserTest,
                       RepeatedCrossOriginPutUsesTaintedOriginHandling) {
  content::WebContents* web_contents = OpenOnionPage();
  const GURL target = CrossOriginURL(kAllowNullOriginPath);

  for (int i = 0; i < 2; ++i) {
    ASSERT_EQ("allowed",
              content::EvalJs(
                  web_contents,
                  content::JsReplace(kFetchScript, target,
                                     base::DictValue().Set("method", "PUT"))));
    EXPECT_EQ(RequestCount("OPTIONS", kAllowNullOriginPath), i + 1);
    EXPECT_EQ(RequestCount("PUT", kAllowNullOriginPath), i + 1);
  }

  // Chromium bypasses the preflight cache for tainted origins, even with
  // Access-Control-Max-Age.
  EXPECT_EQ(LastOrigin("OPTIONS", kAllowNullOriginPath), "null");
  EXPECT_EQ(LastOrigin("PUT", kAllowNullOriginPath), "null");
}

IN_PROC_BROWSER_TEST_F(OnionCorsOriginBrowserTest,
                       SameOriginRedirectToCrossOrigin) {
  content::WebContents* web_contents = OpenOnionPage();
  const GURL redirect_url = web_contents->GetLastCommittedURL().Resolve(
      base::StrCat({kRedirectPath, "?", CrossOriginURL(kDenyCorsPath).spec()}));

  // The first hop is same-origin, so only the redirect makes it cross-origin.
  EXPECT_EQ("blocked",
            content::EvalJs(
                web_contents,
                content::JsReplace(kFetchScript, redirect_url,
                                   base::DictValue().Set("method", "PUT"))));
  EXPECT_EQ(LastOrigin("OPTIONS", kDenyCorsPath), "null");
  EXPECT_EQ(LastOrigin("PUT", kDenyCorsPath), "");
}

IN_PROC_BROWSER_TEST_F(OnionCorsOriginBrowserTest,
                       SameSiteCrossOriginRequestKeepsStrictCookies) {
  content::WebContents* web_contents = OpenOnionPage();
  ASSERT_EQ("strict_cookie=present", content::EvalJs(web_contents, R"(
              document.cookie = 'strict_cookie=present; Domain=foobar.onion;' +
                                ' Path=/; Secure; SameSite=Strict';
              document.cookie;
            )"));
  const GURL target =
      CrossOriginURL(kSameSiteCredentialsPath, "sub.foobar.onion");

  // Hide the CORS origin without changing the actual initiator's cookie site.
  ASSERT_EQ("allowed",
            content::EvalJs(web_contents,
                            content::JsReplace(kFetchScript, target,
                                               base::DictValue().Set(
                                                   "credentials", "include"))));
  EXPECT_EQ(RequestCount("GET", kSameSiteCredentialsPath, "sub.foobar.onion"),
            1);
  EXPECT_EQ(LastOrigin("GET", kSameSiteCredentialsPath, "sub.foobar.onion"),
            "null");
  EXPECT_EQ(
      LastHeader("Cookie", "GET", kSameSiteCredentialsPath, "sub.foobar.onion"),
      "strict_cookie=present");
  EXPECT_EQ(LastHeader("Sec-Fetch-Site", "GET", kSameSiteCredentialsPath,
                       "sub.foobar.onion"),
            "same-site");
}

IN_PROC_BROWSER_TEST_F(OnionCorsOriginBrowserTest,
                       SameSiteNoCorsRequestKeepsCorpAccess) {
  content::WebContents* web_contents = OpenOnionPage();
  const GURL target = CrossOriginURL(kSameSiteCorpPath, "sub.foobar.onion");

  // CORP and Fetch Metadata must use the document's real initiator.
  EXPECT_EQ("opaque",
            content::EvalJs(web_contents,
                            content::JsReplace(
                                "fetch($1, {mode: 'no-cors'}).then("
                                "response => response.type, () => 'blocked')",
                                target)));
  EXPECT_EQ(RequestCount("GET", kSameSiteCorpPath, "sub.foobar.onion"), 1);
  EXPECT_EQ(LastHeader("Sec-Fetch-Site", "GET", kSameSiteCorpPath,
                       "sub.foobar.onion"),
            "same-site");
}
