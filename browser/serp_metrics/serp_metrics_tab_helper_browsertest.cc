/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/sessions/session_restore_test_helper.h"
#include "chrome/browser/sessions/session_restore_test_utils.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
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

constexpr char kSinglePageApplicationHtmlContent[] = R"HTML(
    <html>
    <head>
      <title>Single Page Application</title>
      <script>
        function updatePageHeader(pageHeader) {
          document.getElementById("pageHeader").textContent = pageHeader;
        }

        document.addEventListener("click", (event) => {
          const link =
              event.target.closest("a[data-navigation-type='same_document']");
          if (!link)
            return;

          event.preventDefault();
          history.pushState({ header: "same_document" },
                            "",
                            "same_document");
          updatePageHeader("same_document");
        });

        window.addEventListener("popstate", (event) => {
          if (event.state)
            updatePageHeader(event.state.header);
        });

        history.replaceState(
            { header: "Home" }, "", document.location.href);
      </script>
    </head>
    <body>
      <h1 id="pageHeader">Home</h1>
      <a href="same_document" data-navigation-type="same_document">
        Same Document
      </a>
    </body>
    </html>
  )HTML";

constexpr char kHtmlWithAnchorLinkContent[] = R"HTML(
    <html>
    <body>
      <a id='anchor_link' href='%s'>Link</a>
    </body>
    </html>
  )HTML";

// Builder class to help construct an HTTPS `EmbeddedTestServer` with various
// configurations for testing.
class TestHttpsServerBuilder {
 public:
  TestHttpsServerBuilder() = default;

  TestHttpsServerBuilder& WithStatusCode(net::HttpStatusCode http_status_code) {
    http_status_code_ = http_status_code;
    return *this;
  }

  TestHttpsServerBuilder& WithCertHostnames(
      std::vector<std::string> cert_hostnames) {
    cert_hostnames_ = std::move(cert_hostnames);
    return *this;
  }

  TestHttpsServerBuilder& WithContentOverrideForPath(
      const std::string& path,
      const std::string& content) {
    CHECK(!path.empty());
    CHECK(!content.empty());
    CHECK(override_http_response_content_.find(path) ==
          override_http_response_content_.cend())
        << "Duplicate content override for path: " << path;
    override_http_response_content_[path] = content;
    return *this;
  }

  TestHttpsServerBuilder& WithRedirect(std::string source_path,
                                       std::string destination_path) {
    source_path_ = std::move(source_path);
    destination_path_ = std::move(destination_path);
    return *this;
  }

  std::unique_ptr<net::EmbeddedTestServer> Build() const {
    auto https_server = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);

    if (!cert_hostnames_.empty()) {
      https_server->SetCertHostnames(cert_hostnames_);
    }

    https_server->RegisterRequestHandler(base::BindLambdaForTesting(
        [http_status_code = http_status_code_,
         override_http_response_content = override_http_response_content_,
         source_path = source_path_, destination_path = destination_path_](
            const net::test_server::HttpRequest& http_request)
            -> std::unique_ptr<net::test_server::HttpResponse> {
          auto http_response =
              std::make_unique<net::test_server::BasicHttpResponse>();

          if (source_path && destination_path &&
              http_request.relative_url.starts_with(*source_path)) {
            // Simulate redirect from source path to destination path.
            http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
            http_response->AddCustomHeader("Location", *destination_path);
            http_response->AddCustomHeader("Cache-Control", "no-cache");
            return http_response;
          }

          // Serve a HTML page with the specified status code.
          http_response->set_code(http_status_code);
          http_response->set_content_type("text/html");
          const auto iter =
              override_http_response_content.find(http_request.GetURL().path());
          if (iter != override_http_response_content.cend()) {
            // Override the default response content.
            http_response->set_content(iter->second);
          } else {
            // Default response content is a simple HTML page showing the status
            // code reason phrase.
            http_response->set_content(
                absl::StrFormat("<html><body>%s</body></html>",
                                net::GetHttpReasonPhrase(http_status_code)));
          }

          return http_response;
        }));

    return https_server;
  }

 private:
  std::vector<std::string> cert_hostnames_;

  std::optional<std::string> source_path_;
  std::optional<std::string> destination_path_;

  net::HttpStatusCode http_status_code_ = net::HTTP_OK;
  base::flat_map</*path*/ std::string, /*http_response_content*/ std::string>
      override_http_response_content_;
};

class SameDocumentNavigationWaiter : public content::WebContentsObserver {
 public:
  explicit SameDocumentNavigationWaiter(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}

  SameDocumentNavigationWaiter(const SameDocumentNavigationWaiter&) = delete;
  SameDocumentNavigationWaiter& operator=(const SameDocumentNavigationWaiter&) =
      delete;

  void Wait() {
    if (seen_) {
      return;
    }
    run_loop_.Run();
  }

  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    if (!navigation_handle->HasCommitted()) {
      return;
    }

    if (!navigation_handle->IsSameDocument()) {
      return;
    }

    seen_ = true;
    if (run_loop_.running()) {
      run_loop_.Quit();
    }
  }

 private:
  bool seen_ = false;
  base::RunLoop run_loop_;
};

}  // namespace

// TODO(https://github.com/brave/brave-browser/issues/52599): Migrate SERP
// metrics tab helper browser tests to unit tests.

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
    https_server_ =
        TestHttpsServerBuilder()
            .WithCertHostnames({"www.google.com", "search.brave.com",
                                "duckduckgo.com", "plugh.xyzzy.com"})
            .Build();
    ASSERT_TRUE(https_server_);
    ASSERT_TRUE(https_server_->Start());

    // Wait for TemplateURLService to finish loading so that SerpClassifier can
    // classify search URLs. Without this, navigations that complete before the
    // service is loaded will not be classified.
    search_test_utils::WaitForTemplateURLServiceToLoad(
        TemplateURLServiceFactory::GetForProfile(
            chrome_test_utils::GetProfile(this)));
  }

 protected:
  content::WebContents* GetWebContents() const {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  void SimulateClickingAnchorLink() const {
    content::TestNavigationObserver observer(GetWebContents());
    ASSERT_TRUE(content::ExecJs(
        GetWebContents(), "document.getElementById('anchor_link').click();"));
    observer.Wait();
  }

  void SimulateClickingAnchorLinkWithSamePageNavigation() const {
    SameDocumentNavigationWaiter waiter(GetWebContents());
    ASSERT_TRUE(content::ExecJs(
        GetWebContents(),
        R"JS(document.querySelector("[data-navigation-type='same_document']").click();)JS"));
    waiter.Wait();
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

  SerpMetrics* GetSerpMetrics() const {
    auto* profile_misc_metrics_service =
        misc_metrics::ProfileMiscMetricsServiceFactory::GetServiceForContext(
            GetProfile());
    CHECK(profile_misc_metrics_service);
    return profile_misc_metrics_service->GetSerpMetrics();
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordBraveSearchEngineResultsPage) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("search.brave.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordGoogleSearchEngineResultsPage) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  EXPECT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordOtherSearchEngineResultsPage) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordSearchWhenNavigatingViaLinkClick) {
  auto https_server =
      TestHttpsServerBuilder()
          .WithCertHostnames({"search.brave.com", "plugh.xyzzy.com"})
          .WithContentOverrideForPath(
              "/thud",
              absl::StrFormat(
                  kHtmlWithAnchorLinkContent,
                  https_server_->GetURL("search.brave.com", "/search?q=test")
                      .spec()))
          .Build();
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server->GetURL("plugh.xyzzy.com", "/thud"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(0U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));

  SimulateClickingAnchorLink();

  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest, RecordForHttp4xxResponse) {
  auto https_server = TestHttpsServerBuilder()
                          .WithStatusCode(net::HTTP_NOT_FOUND)
                          .WithCertHostnames({"search.brave.com"})
                          .Build();
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server->GetURL("search.brave.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest, RecordForHttp5xxResponse) {
  auto https_server = TestHttpsServerBuilder()
                          .WithStatusCode(net::HTTP_INTERNAL_SERVER_ERROR)
                          .WithCertHostnames({"www.google.com"})
                          .Build();
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  EXPECT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest, DoNotRecordNonSearchUrl) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  EXPECT_EQ(0U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
  EXPECT_EQ(
      0U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordSameSearchWhenConsecutive) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordRedirectNavigationForSameSearch) {
  auto https_server =
      TestHttpsServerBuilder()
          .WithCertHostnames({"search.brave.com"})
          .WithRedirect(/*source_path=*/"/search?q=test",
                        /*destination_path=*/"/search?q=test&t=web")
          .Build();
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server->GetURL("search.brave.com", "/search?q=test"),
      /*number_of_navigations=*/1,
      /*ignore_uncommitted_navigations=*/true);

  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordSearchAfterRedirectNavigation) {
  auto https_server = TestHttpsServerBuilder()
                          .WithCertHostnames({"search.brave.com"})
                          .WithRedirect(/*source_path=*/"/a/redirect",
                                        /*destination_path=*/"/search?q=test")
                          .Build();
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server->GetURL("search.brave.com", "/a/redirect"),
      /*number_of_navigations=*/1,
      /*ignore_uncommitted_navigations=*/true);

  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordPagingThroughSameSearchResults) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("search.brave.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("search.brave.com", "/search?q=test&page=2"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordSameSearchAfterNavigatingAwayAndReturning) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  EXPECT_EQ(2U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordSameSearchWithDifferentSerpInBetween) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=other"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  EXPECT_EQ(3U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest, DoNotRecordReloadNavigation) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Reload();

  EXPECT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordAfterMultipleReloadNavigations) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Reload();
  Reload();

  EXPECT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

IN_PROC_BROWSER_TEST_F(
    SerpMetricsTabHelperTest,
    DoNotRecordSameSearchAfterReloadNavigationForNewNavigation) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Reload();

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  EXPECT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordSameSearchAfterReloadNavigation) {
  auto https_server =
      TestHttpsServerBuilder()
          .WithCertHostnames({"search.brave.com", "plugh.xyzzy.com"})
          .WithContentOverrideForPath(
              "/thud",
              absl::StrFormat(
                  kHtmlWithAnchorLinkContent,
                  https_server_->GetURL("search.brave.com", "/search?q=test")
                      .spec()))
          .Build();
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server->GetURL("plugh.xyzzy.com", "/thud"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server->GetURL("search.brave.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));

  Reload();

  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

IN_PROC_BROWSER_TEST_F(
    SerpMetricsTabHelperTest,
    RecordDifferentSearchAfterReloadNavigationForNewNavigation) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  Reload();

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("search.brave.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordBackForwardNavigation) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  GoBack();
  GoForward();

  EXPECT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordAfterMultipleBackForwardNavigations) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  GoBack();
  GoForward();
  GoBack();
  GoForward();

  EXPECT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

IN_PROC_BROWSER_TEST_F(
    SerpMetricsTabHelperTest,
    DoNotRecordSameSearchAfterBackForwardNavigationForNewNavigation) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  GoBack();
  GoForward();

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  EXPECT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

IN_PROC_BROWSER_TEST_F(
    SerpMetricsTabHelperTest,
    DoNotRecordSameSearchAfterBackForwardNavigationAndLinkClick) {
  auto https_server =
      TestHttpsServerBuilder()
          .WithCertHostnames({"search.brave.com", "plugh.xyzzy.com"})
          .WithContentOverrideForPath(
              "/thud",
              absl::StrFormat(
                  kHtmlWithAnchorLinkContent,
                  https_server_->GetURL("search.brave.com", "/search?q=test")
                      .spec()))
          .Build();
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server->GetURL("plugh.xyzzy.com", "/thud"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server->GetURL("search.brave.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));

  GoBack();
  GoForward();
  GoBack();

  SimulateClickingAnchorLink();

  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

IN_PROC_BROWSER_TEST_F(
    SerpMetricsTabHelperTest,
    RecordDifferentSearchAfterBackForwardNavigationForNewNavigation) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("plugh.xyzzy.com", "/thud"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  GoBack();
  GoForward();

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("search.brave.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);

  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordWithoutUserGesture) {
  content::TestNavigationObserver observer(GetWebContents());
  ASSERT_TRUE(NavigateToURLFromRendererWithoutUserGesture(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test")));
  observer.Wait();

  EXPECT_EQ(
      0U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordIfStatsReportingIsDisabled) {
  g_browser_process->local_state()->SetBoolean(kStatsReportingEnabled, false);

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("search.brave.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  EXPECT_EQ(0U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  EXPECT_EQ(
      0U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(), https_server_->GetURL("duckduckgo.com", "/?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  EXPECT_EQ(0U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kOther));
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest, DoNotRecordSameDocument) {
  auto https_server = TestHttpsServerBuilder()
                          .WithCertHostnames({"search.brave.com"})
                          .WithContentOverrideForPath(
                              "/search", kSinglePageApplicationHtmlContent)
                          .Build();
  ASSERT_TRUE(https_server);
  ASSERT_TRUE(https_server->Start());

  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server->GetURL("search.brave.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));

  SimulateClickingAnchorLinkWithSamePageNavigation();

  EXPECT_EQ(1U,
            GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kBrave));
}

#if !BUILDFLAG(IS_ANDROID)
IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest, DoNotRecordIfTabWasRestored) {
  content::NavigateToURLBlockUntilNavigationsComplete(
      GetWebContents(),
      https_server_->GetURL("www.google.com", "/search?q=test"),
      /*number_of_navigations=*/1, /*ignore_uncommitted_navigations=*/true);
  ASSERT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));

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

  EXPECT_EQ(
      1U, GetSerpMetrics()->GetSearchCountForTesting(SerpMetricType::kGoogle));
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace serp_metrics
