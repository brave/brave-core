/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/synchronization/lock.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/query_filter/browser/test_support/query_filter_test_helper.h"
#include "brave/components/query_filter/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

// Serves one cached document for every navigation and echoes any tracking
// parameters it actually observed onto a same-site request. That is the
// service-worker bypass: the navigation never reaches the network stack.
// Mirrors https://r0hit0303.github.io/test-pages/sw-query-filter-bypass/.
constexpr char kServiceWorkerScript[] = R"JS(
const SHELL_HTML = `<!doctype html>
<html><head><meta charset="utf-8"><title>cached-shell</title></head>
<body>
<p id="source">from-service-worker</p>
<pre id="url"></pre>
<p id="verdict"></p>
<script>
  const href = location.href;
  document.getElementById('url').textContent = href;
  const params = new URL(href).searchParams;
  document.getElementById('verdict').textContent =
      (params.has('fbclid') || params.has('gclid'))
          ? 'tracking-params-survived'
          : 'tracking-params-removed';
</script>
</body></html>`;

self.addEventListener('install', (event) => {
  event.waitUntil((async () => {
    const cache = await caches.open('query-filter-shell');
    await cache.put('./shell', new Response(SHELL_HTML, {
      headers: {'content-type': 'text/html; charset=utf-8'},
    }));
    await self.skipWaiting();
  })());
});

self.addEventListener('activate', (event) => {
  event.waitUntil(self.clients.claim());
});

self.addEventListener('fetch', (event) => {
  if (event.request.mode !== 'navigate') {
    return;
  }
  const url = new URL(event.request.url);
  const beacon = new URL('/api/collect', self.location.origin);
  for (const name of ['fbclid', 'gclid']) {
    if (url.searchParams.has(name)) {
      beacon.searchParams.set(name, url.searchParams.get(name));
    }
  }
  event.waitUntil(fetch(beacon));
  event.respondWith((async () => {
    const cache = await caches.open('query-filter-shell');
    const cached = await cache.match('./shell');
    return cached || new Response(SHELL_HTML, {
      headers: {'content-type': 'text/html; charset=utf-8'},
    });
  })());
});
)JS";

constexpr char kNetworkBody[] = "from-network";

}  // namespace

class QueryFilterServiceWorkerBrowserTest : public InProcessBrowserTest {
 public:
  QueryFilterServiceWorkerBrowserTest()
      : feature_list_(query_filter::features::kQueryFilterComponent),
        https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

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

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&QueryFilterServiceWorkerBrowserTest::HandleRequest,
                            base::Unretained(this)));
    ASSERT_TRUE(https_server_.Start());
    rules_.emplace();
  }

  void TearDownOnMainThread() override {
    rules_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(
        browser()->GetProfile());
  }

  // Turns Brave Shields off for `url`'s origin, which disables query filtering
  // for navigations to it (see `ShouldInterceptNavigation`).
  void ShieldsDown(const GURL& url) {
    brave_shields::SetBraveShieldsEnabled(content_settings(), false, url);
  }

  GURL ServerURL(std::string_view path) {
    return https_server_.GetURL("a.com", path);
  }

  // Loads the test page from the network and installs the service worker, which
  // then controls every subsequent navigation to this origin. Registration must
  // run in the page world because `ServiceWorkerContainer.ready` is not
  // implemented for isolated worlds.
  void NavigateAndRegisterServiceWorker() {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), ServerURL("/")));
    EXPECT_EQ(kNetworkBody, EvalPage("document.body.textContent"));

    ASSERT_EQ(true, EvalPage(R"(
      (async () => {
        const registration = await navigator.serviceWorker.register('/sw.js');
        await navigator.serviceWorker.ready;
        return registration.active !== null;
      })()
    )",
                             content::ISOLATED_WORLD_ID_GLOBAL));
  }

  // Evaluates `script` in an isolated world by default so test code does not
  // interfere with page scripts (pass the page world where an API requires it).
  content::EvalJsResult EvalPage(
      std::string_view script,
      int32_t world_id = ISOLATED_WORLD_ID_BRAVE_INTERNAL) {
    return content::EvalJs(web_contents(), std::string(script),
                           content::EXECUTE_SCRIPT_DEFAULT_OPTIONS, world_id);
  }

  bool RequestedPath(std::string_view path) {
    base::AutoLock lock(requests_lock_);
    for (const GURL& url : requests_) {
      if (url.path() == path) {
        return true;
      }
    }
    return false;
  }

  bool AnyRequestHasTrackingQuery() {
    base::AutoLock lock(requests_lock_);
    for (const GURL& url : requests_) {
      if (url.query().find("fbclid") != std::string::npos ||
          url.query().find("gclid") != std::string::npos) {
        return true;
      }
    }
    return false;
  }

  void ExpectNoTrackingQueryInRequests(
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    base::AutoLock lock(requests_lock_);
    for (const GURL& url : requests_) {
      EXPECT_EQ(std::string::npos, url.query().find("fbclid")) << url;
      EXPECT_EQ(std::string::npos, url.query().find("gclid")) << url;
    }
  }

  // Confirms the committed page was served by the worker and reports `verdict`,
  // the worker's observation of whether tracking parameters reached it.
  void ExpectPageServedByWorkerWithVerdict(
      const GURL& url,
      std::string_view verdict,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    EXPECT_EQ(
        base::StrCat({"from-service-worker\n", url.spec(), "\n", verdict}),
        EvalPage(R"(
                document.getElementById('source').textContent + '\n' +
                document.getElementById('url').textContent + '\n' +
                document.getElementById('verdict').textContent
              )"))
        << url;
  }

 private:
  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    const GURL url = request.GetURL();
    {
      base::AutoLock lock(requests_lock_);
      requests_.push_back(url);
    }

    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    if (url.path() == "/sw.js") {
      response->set_content_type("text/javascript");
      response->set_content(kServiceWorkerScript);
      return response;
    }
    if (url.path() == "/api/collect") {
      response->set_code(net::HTTP_NO_CONTENT);
      return response;
    }
    if (url.path() == "/favicon.ico") {
      response->set_code(net::HTTP_NOT_FOUND);
      return response;
    }
    // The test only ever navigates to the origin root; serve the network
    // document there and fail loudly on anything unexpected.
    if (url.path() == "/") {
      response->set_content_type("text/html");
      response->set_content(
          base::StrCat({"<!doctype html><title>network</title><body>",
                        kNetworkBody, "</body>"}));
      return response;
    }
    NOTREACHED() << "Unhandled request: " << url;
  }

  base::test::ScopedFeatureList feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_;
  std::optional<query_filter::test::ScopedTestingQueryFilterRules> rules_;
  base::Lock requests_lock_;
  std::vector<GURL> requests_;
};

// A typed navigation is restarted without tracking parameters before the
// service worker can answer it, so the worker's cached document and the
// beacon it sends both see the stripped URL.
IN_PROC_BROWSER_TEST_F(QueryFilterServiceWorkerBrowserTest,
                       ServiceWorkerDoesNotObserveTrackingQuery) {
  ASSERT_NO_FATAL_FAILURE(NavigateAndRegisterServiceWorker());

  const GURL tracked = ServerURL("/?fbclid=abcd1234&gclid=wxyz5678&keep=1");
  GURL::Replacements replacements;
  replacements.SetQueryStr("keep=1");
  const GURL expected = tracked.ReplaceComponents(replacements);

  ui_test_utils::UrlLoadObserver load_observer(expected);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), tracked));
  load_observer.Wait();

  EXPECT_EQ(expected, web_contents()->GetLastCommittedURL()) << tracked;
  ExpectPageServedByWorkerWithVerdict(expected, "tracking-params-removed");

  // The worker reports the parameters it saw. Wait for that request rather
  // than assuming it finished with the navigation.
  ASSERT_TRUE(base::test::RunUntil([&] {
    return RequestedPath("/api/collect");
  })) << tracked;
  ExpectNoTrackingQueryInRequests();
}

// Counterfactual for the test above: with query filtering disabled (Brave
// Shields off) nothing restarts the navigation, so the tracking parameters
// survive all the way to the service worker and its beacon. This proves the
// stripping above is caused by the query filter, not the test setup.
IN_PROC_BROWSER_TEST_F(
    QueryFilterServiceWorkerBrowserTest,
    ServiceWorkerObservesTrackingQueryWhenFilteringDisabled) {
  ASSERT_NO_FATAL_FAILURE(NavigateAndRegisterServiceWorker());

  const GURL tracked = ServerURL("/?fbclid=abcd1234&gclid=wxyz5678&keep=1");
  ShieldsDown(tracked);

  // The navigation is not restarted, so the tracked URL commits unchanged.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), tracked));
  EXPECT_EQ(tracked, web_contents()->GetLastCommittedURL());
  ExpectPageServedByWorkerWithVerdict(tracked, "tracking-params-survived");

  ASSERT_TRUE(base::test::RunUntil([&] {
    return RequestedPath("/api/collect");
  })) << tracked;
  EXPECT_TRUE(AnyRequestHasTrackingQuery());
}
