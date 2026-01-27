/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <vector>

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_restore_test_helper.h"
#include "chrome/browser/sessions/session_restore_test_utils.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace metrics {
std::unique_ptr<net::EmbeddedTestServer>
CreateTestHttpsServerWithStatusCodeResponse(
    net::HttpStatusCode http_status_code,
    const std::vector<std::string>& cert_hostnames) {
  auto https_server = std::make_unique<net::EmbeddedTestServer>(
      net::test_server::EmbeddedTestServer::TYPE_HTTPS);
  https_server->SetCertHostnames(cert_hostnames);
  https_server->RegisterRequestHandler(base::BindLambdaForTesting(
      [http_status_code](const net::test_server::HttpRequest& /*http_request*/)
          -> std::unique_ptr<net::test_server::HttpResponse> {
        auto http_response =
            std::make_unique<net::test_server::BasicHttpResponse>();
        http_response->set_code(http_status_code);
        http_response->set_content_type("text/html");
        http_response->set_content(
            "<html><body>" +
            std::string(net::GetHttpReasonPhrase(http_status_code)) +
            "</body></html>");
        return http_response;
      }));
  return https_server;
}

namespace {
constexpr char kBraveSearchEngineTimePeriodStorageDictKey[] =
    "brave_search_engine";
constexpr char kGoogleSearchEngineTimePeriodStorageDictKey[] =
    "google_search_engine";
constexpr char kOtherSearchEngineTimePeriodStorageDictKey[] =
    "other_search_engine";

uint64_t GetSearchCountForTimePeriodStorageDictKey(const char* dict_key) {
  TimePeriodStorage time_period_storage(
      g_browser_process->local_state(), prefs::kSerpMetricsTimePeriodStorage,
      dict_key, kSerpMetricsTimePeriodInDays.Get());
  return time_period_storage.GetPeriodSum();
}

}  // namespace

class SerpMetricsTabHelperTest : public PlatformBrowserTest {
 public:
  SerpMetricsTabHelperTest() {
    scoped_feature_list_.InitAndEnableFeature(kSerpMetricsFeature);
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    // Enable usage stats reporting by default for tests.
    g_browser_process->local_state()->SetBoolean(kStatsReportingEnabled, true);

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_ = CreateTestHttpsServerWithStatusCodeResponse(
        net::HTTP_OK, /*cert_hostnames=*/{"www.google.com", "search.brave.com",
                                          "duckduckgo.com", "plugh.xyzzy.com"});
    ASSERT_TRUE(https_server_);
    ASSERT_TRUE(https_server_->Start());
  }

 protected:
  content::WebContents* GetWebContents() const {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  void Reload() const {
    ASSERT_TRUE(GetWebContents()->GetController().CanGoBack());
    content::TestNavigationObserver observer(GetWebContents());
    GetWebContents()->GetController().Reload(content::ReloadType::NORMAL,
                                             /*check_for_repost=*/false);
    observer.Wait();
  }

  void GoBack() const {
    ASSERT_TRUE(GetWebContents()->GetController().CanGoBack());
    content::TestNavigationObserver observer(GetWebContents());
    GetWebContents()->GetController().GoBack();
    observer.Wait();
  }

  void GoForward() const {
    ASSERT_TRUE(GetWebContents()->GetController().CanGoForward());
    content::TestNavigationObserver observer(GetWebContents());
    GetWebContents()->GetController().GoForward();
    observer.Wait();
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  content::ContentMockCertVerifier mock_cert_verifier_;

  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordBraveSearchEngineResultPage) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("search.brave.com", "/search?q=test")));
  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordGoogleSearchEngineResultPage) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));
  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordOtherSearchEngineResultPage) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test")));
  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kOtherSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest, DoNotRecordForNonSearchUrl) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud")));
  EXPECT_EQ(0U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));
  EXPECT_EQ(0U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
  EXPECT_EQ(0U, GetSearchCountForTimePeriodStorageDictKey(
                    kOtherSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordIfUsagePingDisabled) {
  // Disable usage stats reporting, so no searches should be recorded.
  g_browser_process->local_state()->SetBoolean(kStatsReportingEnabled, false);

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("search.brave.com", "/search?q=test")));
  EXPECT_EQ(0U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));
  EXPECT_EQ(0U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test")));
  EXPECT_EQ(0U, GetSearchCountForTimePeriodStorageDictKey(
                    kOtherSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordWithoutUserGesture) {
  content::TestNavigationObserver observer(GetWebContents());
  ASSERT_TRUE(NavigateToURLFromRendererWithoutUserGesture(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));
  observer.Wait();

  EXPECT_EQ(0U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordForReloadNavigation) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));
  ASSERT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));

  Reload();

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordForBackForwardNavigation) {
  // Add a non-search engine results page to navigate back to.
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud")));

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));
  ASSERT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));

  GoBack();

  GoForward();

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest, DoNotRecordIfTabWasRestored) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));
  ASSERT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));

  Profile* profile = chrome_test_utils::GetProfile(this);

  const ScopedKeepAlive scoped_keep_alive(KeepAliveOrigin::SESSION_RESTORE,
                                          KeepAliveRestartOption::DISABLED);
  const ScopedProfileKeepAlive scoped_profile_keep_alive(
      profile, ProfileKeepAliveOrigin::kSessionRestore);

  // Close and restore browser to simulate session restore.
  CloseBrowserSynchronously(browser());

  ui_test_utils::BrowserCreatedObserver browser_created_observer;
  SessionRestoreTestHelper session_restore_test_helper;
  chrome::OpenWindowWithRestoredTabs(profile);
  if (SessionRestore::IsRestoring(profile)) {
    session_restore_test_helper.Wait();
  }
  SetBrowser(browser_created_observer.Wait());

  // At this point, restored tab navigation happens, but should not count.
  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordForHttp4xxResponse) {
  // Create a new embedded test server to simulate a 404 response.
  // `https_server_` always returns `HTTP_OK`, so it cannot be reused for this
  // test.
  auto https_server = CreateTestHttpsServerWithStatusCodeResponse(
      net::HTTP_NOT_FOUND, /*cert_hostnames=*/{"search.brave.com"});
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server->GetURL("search.brave.com", "/search?q=test")));
  EXPECT_EQ(0U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordForHttp5xxResponse) {
  // Create a new embedded test server to simulate a 5xx response.
  // `https_server_ always returns `HTTP_OK`, so it cannot be reused for this
  // test.
  auto https_server = CreateTestHttpsServerWithStatusCodeResponse(
      net::HTTP_INTERNAL_SERVER_ERROR, /*cert_hostnames=*/{"www.google.com"});
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server->GetURL("www.google.com", "/search?q=test")));
  EXPECT_EQ(0U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
}

}  // namespace metrics
