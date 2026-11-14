/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <atomic>
#include <string>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/tor/tor_navigation_throttle.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#endif

namespace {

constexpr char kHelperPage[] = "/background_fetch/background_fetch.html";
constexpr char kFetchedResource[] =
    "/background_fetch/types_of_cheese_cors.txt";
constexpr char kPageSite[] = "a.test";
constexpr char kFetchedSite[] = "b.test";
constexpr char kPermissionDenied[] =
    "TypeError - This origin does not have permission to start a fetch.";

}  // namespace

// Background Fetch downloads in off-the-record profiles leave without the
// initiating page's isolation info. Private windows must still keep the
// fetched site's cookies out of them, and Tor windows must not run Background
// Fetch at all, since its Origin header would reveal a .onion page.
class BackgroundFetchOffTheRecordBrowserTest : public InProcessBrowserTest {
 public:
  BackgroundFetchOffTheRecordBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &BackgroundFetchOffTheRecordBrowserTest::RecordRequest,
        base::Unretained(this)));
    ASSERT_TRUE(https_server_.Start());
  }

#if BUILDFLAG(ENABLE_TOR)
  void TearDownOnMainThread() override {
    // Process-wide, so leave them as a fresh browser would have them.
    net::ProxyConfigServiceTor::SetBypassTorProxyConfigForTesting(false);
    tor::TorNavigationThrottle::SetSkipWaitForTorConnectedForTesting(false);
    InProcessBrowserTest::TearDownOnMainThread();
  }
#endif  // BUILDFLAG(ENABLE_TOR)

 protected:
  // Sets a SameSite=None cookie on the fetched site in `browser_window`, as a
  // first-party visit would.
  void SetFetchedSiteCookie(BrowserWindowInterface* browser_window) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser_window,
        https_server_.GetURL(kFetchedSite,
                             "/set-cookie?id=1;SameSite=None;Secure")));
  }

  // Registers the helper page's service worker in `browser_window`, then
  // starts a Background Fetch of a resource on the fetched site and returns
  // the page's result.
  std::string StartBackgroundFetch(
      BrowserWindowInterface* browser_window,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    EXPECT_TRUE(ui_test_utils::NavigateToURL(
        browser_window, https_server_.GetURL(kPageSite, kHelperPage)));
    content::WebContents* web_contents =
        browser_window->GetTabStripModel()->GetActiveWebContents();
    EXPECT_EQ("ok - service worker registered",
              content::EvalJs(web_contents, "RegisterServiceWorker()"));
    return content::EvalJs(
               web_contents,
               content::JsReplace(
                   "StartFetchFromWindowWithUrl($1)",
                   https_server_.GetURL(kFetchedSite, kFetchedResource)))
        .ExtractString();
  }

  int fetched_resource_count() const { return fetched_resource_count_; }

  GURL FetchedSiteURL() const {
    return https_server_.GetURL(kFetchedSite, "/");
  }

  // The Cookie header of the last request for the fetched resource.
  std::string last_fetched_resource_cookie() {
    base::AutoLock auto_lock(lock_);
    return last_fetched_resource_cookie_;
  }

 private:
  void RecordRequest(const net::test_server::HttpRequest& request) {
    if (request.GetURL().path() != kFetchedResource) {
      return;
    }
    const auto cookie = request.headers.find("Cookie");
    base::AutoLock auto_lock(lock_);
    last_fetched_resource_cookie_ =
        cookie == request.headers.end() ? "" : cookie->second;
    ++fetched_resource_count_;
  }

  std::atomic<int> fetched_resource_count_ = 0;
  base::Lock lock_;
  std::string last_fetched_resource_cookie_ GUARDED_BY(lock_);
  // Declared after the state its monitor uses, so the server shuts down
  // before that state is destroyed.
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(BackgroundFetchOffTheRecordBrowserTest,
                       PrivateWindowFetchOmitsFetchedSiteCookies) {
  BrowserWindowInterface* private_browser = CreateIncognitoBrowser();
  ASSERT_TRUE(private_browser->GetProfile()->IsOffTheRecord());
  ASSERT_NO_FATAL_FAILURE(SetFetchedSiteCookie(private_browser));
  ASSERT_EQ("id=1", content::GetCookies(private_browser->GetProfile(),
                                        FetchedSiteURL()));

  EXPECT_EQ("backgroundfetchsuccess", StartBackgroundFetch(private_browser));
  EXPECT_EQ(1, fetched_resource_count());
  EXPECT_EQ("", last_fetched_resource_cookie());
}

#if BUILDFLAG(ENABLE_TOR)
IN_PROC_BROWSER_TEST_F(BackgroundFetchOffTheRecordBrowserTest,
                       TorWindowDeniesBackgroundFetch) {
  net::ProxyConfigServiceTor::SetBypassTorProxyConfigForTesting(true);
  tor::TorNavigationThrottle::SetSkipWaitForTorConnectedForTesting(true);
  BrowserWindowInterface* tor_browser =
      TorProfileManager::SwitchToTorProfile(browser()->GetProfile());
  ASSERT_TRUE(tor_browser->GetProfile()->IsTor());
  EXPECT_EQ(kPermissionDenied, StartBackgroundFetch(tor_browser));
  EXPECT_EQ(0, fetched_resource_count());
}
#endif  // BUILDFLAG(ENABLE_TOR)
