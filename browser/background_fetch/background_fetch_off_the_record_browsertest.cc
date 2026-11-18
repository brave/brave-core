/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <atomic>
#include <string>

#include "base/functional/bind.h"
#include "base/location.h"
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

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/tor/tor_navigation_throttle.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#endif

namespace {

constexpr char kHelperPage[] = "/background_fetch/background_fetch.html";
constexpr char kFetchedResource[] = "/background_fetch/types_of_cheese.txt";
constexpr char kPermissionDenied[] =
    "TypeError - This origin does not have permission to start a fetch.";

}  // namespace

// Off-the-record profiles must not run Background Fetch: their downloads leave
// without the initiator's isolation info, so they would carry the fetched
// site's first-party cookies.
class BackgroundFetchOffTheRecordBrowserTest : public InProcessBrowserTest {
 public:
  BackgroundFetchOffTheRecordBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromSourceDirectory(GetChromeTestDataDir());
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
  // Registers the helper page's service worker in `browser_window`, then
  // starts a Background Fetch and returns the page's result.
  std::string StartBackgroundFetch(
      BrowserWindowInterface* browser_window,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    EXPECT_TRUE(ui_test_utils::NavigateToURL(
        browser_window, https_server_.GetURL("a.test", kHelperPage)));
    content::WebContents* web_contents =
        browser_window->GetTabStripModel()->GetActiveWebContents();
    EXPECT_EQ("ok - service worker registered",
              content::EvalJs(web_contents, "RegisterServiceWorker()"));
    return content::EvalJs(web_contents, "StartSingleFileDownload()")
        .ExtractString();
  }

  int fetched_resource_count() const { return fetched_resource_count_; }

 private:
  void RecordRequest(const net::test_server::HttpRequest& request) {
    if (request.GetURL().path() == kFetchedResource) {
      ++fetched_resource_count_;
    }
  }

  std::atomic<int> fetched_resource_count_ = 0;
  // Declared after the state its monitor uses, so the server shuts down
  // before that state is destroyed.
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(BackgroundFetchOffTheRecordBrowserTest,
                       PrivateWindowDeniesBackgroundFetch) {
  BrowserWindowInterface* private_browser = CreateIncognitoBrowser();
  ASSERT_TRUE(private_browser->GetProfile()->IsOffTheRecord());
  EXPECT_EQ(kPermissionDenied, StartBackgroundFetch(private_browser));
  EXPECT_EQ(0, fetched_resource_count());

  // Control: a regular window may start the same fetch.
  EXPECT_EQ("ok", StartBackgroundFetch(browser()));
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
