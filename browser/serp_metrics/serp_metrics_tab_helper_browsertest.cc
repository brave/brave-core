/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_restore_test_helper.h"
#include "chrome/browser/sessions/session_restore_test_utils.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
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
#include "url/gurl.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#endif  // !BUILDFLAG(IS_ANDROID)

namespace serp_metrics {

namespace {

// Creates an HTTPS embedded test server. `http_status_code` is the status code
// the server should return. `cert_hostnames` are the hostnames used in the
// server’s HTTPS certificate to match host names in the test. `link` is an
// optional `<a href>` in the response body to simulate a click.
std::unique_ptr<net::EmbeddedTestServer>
CreateTestHttpsServerWithStatusCodeResponse(
    net::HttpStatusCode http_status_code,
    const std::vector<std::string>& cert_hostnames,
    std::optional<GURL> link) {
  auto https_server = std::make_unique<net::EmbeddedTestServer>(
      net::test_server::EmbeddedTestServer::TYPE_HTTPS);
  https_server->SetCertHostnames(cert_hostnames);
  https_server->RegisterRequestHandler(base::BindLambdaForTesting(
      [link,
       http_status_code](const net::test_server::HttpRequest& /*http_request*/)
          -> std::unique_ptr<net::test_server::HttpResponse> {
        auto http_response =
            std::make_unique<net::test_server::BasicHttpResponse>();
        http_response->set_code(http_status_code);
        http_response->set_content_type("text/html");
        std::string_view http_reason_phrase =
            net::GetHttpReasonPhrase(http_status_code);
        std::string content;
        if (link) {
          content = absl::StrFormat(
              "<html><body>%s<a id='link' href='%s'>Link</a></body></html>",
              http_reason_phrase, link->spec());
        } else {
          content = absl::StrFormat("<html><body>%s</body></html>",
                                    http_reason_phrase);
        }
        http_response->set_content(content);
        return http_response;
      }));
  return https_server;
}

// Creates an HTTPS embedded test server. `http_status_code` is the status code
// the server should return. `cert_hostnames` are the hostnames used in the
// server’s HTTPS certificate to match host names in the test.
std::unique_ptr<net::EmbeddedTestServer>
CreateTestHttpsServerWithStatusCodeResponse(
    net::HttpStatusCode http_status_code,
    const std::vector<std::string>& cert_hostnames) {
  return CreateTestHttpsServerWithStatusCodeResponse(
      http_status_code, cert_hostnames, /*link=*/std::nullopt);
}

constexpr char kBraveSearchEngineTimePeriodStorageDictKey[] =
    "brave_search_engine";
constexpr char kGoogleSearchEngineTimePeriodStorageDictKey[] =
    "google_search_engine";
constexpr char kOtherSearchEngineTimePeriodStorageDictKey[] =
    "other_search_engine";

}  // namespace

class SerpMetricsTabHelperTest : public PlatformBrowserTest {
 public:
  SerpMetricsTabHelperTest() {
    scoped_feature_list_.InitAndEnableFeature(kSerpMetricsFeature);
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

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

  uint64_t GetSearchCountForTimePeriodStorageDictKey(
      const char* dict_key) const {
    TimePeriodStorage time_period_storage(
        GetProfile()->GetPrefs(), prefs::kSerpMetricsTimePeriodStorage,
        dict_key, kSerpMetricsTimePeriodInDays.Get(),
        /*should_offset_dst=*/false);
    return time_period_storage.GetPeriodSum();
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

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordSearchWhenNavigatingViaLinkClick) {
  const GURL link_url =
      https_server_->GetURL("search.brave.com", "/search?q=test");
  auto https_server = CreateTestHttpsServerWithStatusCodeResponse(
      net::HTTP_OK, /*cert_hostnames=*/{"search.brave.com", "plugh.xyzzy.com"},
      link_url);
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server->GetURL("plugh.xyzzy.com", "/thud")));
  ASSERT_EQ(0U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));

  content::TestNavigationObserver test_navigation_observer(GetWebContents());
  ASSERT_TRUE(content::ExecJs(GetWebContents(),
                              "document.getElementById('link').click();"));
  test_navigation_observer.Wait();

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest, RecordForHttp4xxResponse) {
  auto https_server = CreateTestHttpsServerWithStatusCodeResponse(
      net::HTTP_NOT_FOUND, /*cert_hostnames=*/{"search.brave.com"});
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server->GetURL("search.brave.com", "/search?q=test")));
  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest, RecordForHttp5xxResponse) {
  auto https_server = CreateTestHttpsServerWithStatusCodeResponse(
      net::HTTP_INTERNAL_SERVER_ERROR, /*cert_hostnames=*/{"www.google.com"});
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server->GetURL("www.google.com", "/search?q=test")));
  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
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
                       DoNotRecordSameSearchWhenConsecutive) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test")));
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test")));
  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kOtherSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordPagingThroughSameSearchResults) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("search.brave.com", "/search?q=test")));

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("search.brave.com", "/search?q=test&page=2")));

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordSameSearchWhenNotConsecutive) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test")));
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud")));
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test")));
  EXPECT_EQ(2U, GetSearchCountForTimePeriodStorageDictKey(
                    kOtherSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordSameSearchAfterNavigatingAwayAndReturning) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test")));

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud")));

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test")));

  EXPECT_EQ(2U, GetSearchCountForTimePeriodStorageDictKey(
                    kOtherSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordSameSearchWithDifferentSerpInBetween) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test")));

  // Different SERP (same engine, different query).
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=other")));

  // Original SERP again.
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test")));

  EXPECT_EQ(3U, GetSearchCountForTimePeriodStorageDictKey(
                    kOtherSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest, DoNotRecordReloadNavigation) {
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
                       DoNotRecordAfterMultipleReloadNavigations) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));
  ASSERT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));

  Reload();
  Reload();

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(
    SerpMetricsTabHelperTest,
    DoNotRecordSameSearchAfterReloadNavigationForNewNavigation) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));
  ASSERT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));

  Reload();

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordSameSearchAfterReloadNavigationAndLinkClick) {
  const GURL link_url =
      https_server_->GetURL("search.brave.com", "/search?q=test");
  auto https_server = CreateTestHttpsServerWithStatusCodeResponse(
      net::HTTP_OK, /*cert_hostnames=*/{"search.brave.com", "plugh.xyzzy.com"},
      link_url);
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server->GetURL("plugh.xyzzy.com", "/thud")));

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server->GetURL("search.brave.com", "/search?q=test")));
  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));

  Reload();

  content::TestNavigationObserver test_navigation_observer(GetWebContents());
  ASSERT_TRUE(content::ExecJs(GetWebContents(),
                              "document.getElementById('link').click();"));
  test_navigation_observer.WaitForNavigationFinished();

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(
    SerpMetricsTabHelperTest,
    RecordDifferentSearchAfterReloadNavigationForNewNavigation) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));

  Reload();

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("search.brave.com", "/search?q=test")));

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordBackForwardNavigation) {
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

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordAfterMultipleBackForwardNavigations) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud")));

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));
  ASSERT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));

  GoBack();
  GoForward();
  GoBack();
  GoForward();

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(
    SerpMetricsTabHelperTest,
    DoNotRecordSameSearchAfterBackForwardNavigationForNewNavigation) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud")));

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));
  ASSERT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));

  GoBack();
  GoForward();

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(
    SerpMetricsTabHelperTest,
    DoNotRecordSameSearchAfterBackForwardNavigationAndLinkClick) {
  const GURL link_url =
      https_server_->GetURL("search.brave.com", "/search?q=test");
  auto https_server = CreateTestHttpsServerWithStatusCodeResponse(
      net::HTTP_OK, /*cert_hostnames=*/{"search.brave.com", "plugh.xyzzy.com"},
      link_url);
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server->GetURL("plugh.xyzzy.com", "/thud")));

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server->GetURL("search.brave.com", "/search?q=test")));
  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));

  GoBack();
  GoForward();

  content::TestNavigationObserver test_navigation_observer(GetWebContents());
  ASSERT_TRUE(content::ExecJs(GetWebContents(),
                              "document.getElementById('link').click();"));
  test_navigation_observer.WaitForNavigationFinished();

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));
}

IN_PROC_BROWSER_TEST_F(
    SerpMetricsTabHelperTest,
    RecordDifferentSearchAfterBackForwardNavigationForNewNavigation) {
  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud")));

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));

  GoBack();
  GoForward();

  ASSERT_TRUE(content::NavigateToURL(
      GetWebContents(),
      https_server_->GetURL("search.brave.com", "/search?q=test")));

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kBraveSearchEngineTimePeriodStorageDictKey));
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
                       DoNotRecordIfStatsReportingIsDisabled) {
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

#if !BUILDFLAG(IS_ANDROID)
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

  CloseBrowserSynchronously(browser());

  ui_test_utils::BrowserCreatedObserver browser_created_observer;
  SessionRestoreTestHelper session_restore_test_helper;
  chrome::OpenWindowWithRestoredTabs(profile);
  if (SessionRestore::IsRestoring(profile)) {
    session_restore_test_helper.Wait();
  }
  SetBrowser(browser_created_observer.Wait());

  EXPECT_EQ(1U, GetSearchCountForTimePeriodStorageDictKey(
                    kGoogleSearchEngineTimePeriodStorageDictKey));
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace serp_metrics
