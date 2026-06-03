// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/browser/backup_results_service.h"

#include "base/base64.h"
#include "base/containers/map_util.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/browser/brave_search/backup_results_service_factory.h"
#include "brave/browser/brave_search/backup_results_service_impl.h"
#include "brave/components/brave_search/browser/prefs.h"
#include "brave/components/brave_search/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_request_headers.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"
#include "ui/gfx/geometry/size.h"

namespace brave_search {

namespace {

constexpr char kTestCustomHeaderName[] = "X-Custom-Header";
constexpr char kTestCustomHeaderValue[] = "test-value";
constexpr char kTestUAOverride[] = "TestBrowser/1.0";

constexpr char kTestInitPath[] = "/test";
constexpr char kTestInitHtml[] = R"(
<!doctype html>
<html>
<body>
Test Content
<script>
document.cookie = "testcookie=value; path=/";
window.location.href = "/test2";
</script>
</body>
</html>
)";

constexpr char kTestInitInvalidRedirectHtml[] = R"(
<!doctype html>
<html>
<body>
Test Content
<script>
document.cookie = "testcookie=value; path=/";
window.location.href = "https://google.invalid/test2";
</script>
</body>
</html>
)";

constexpr char kTestFinalPath[] = "/test2";
constexpr char kTestFinalHtml[] =
    "<!doctype html><html><body>Test Content</body></html>";

}  // namespace

class BackupResultsServiceBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&BackupResultsServiceBrowserTest::HandleRequest,
                            base::Unretained(this)));

    ASSERT_TRUE(https_server_->Start());
    backup_results_service_ =
        BackupResultsServiceFactory::GetForBrowserContext(browser()->profile());
  }

  void TearDownOnMainThread() override { backup_results_service_ = nullptr; }

 protected:
  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_code(net::HTTP_OK);
    response->set_content_type("text/html");

    auto url = request.GetURL();
    if (auto* v = base::FindOrNull(request.headers, kTestCustomHeaderName)) {
      last_custom_header_ = *v;
    }
    if (auto* v = base::FindOrNull(request.headers,
                                   net::HttpRequestHeaders::kUserAgent)) {
      last_user_agent_ = *v;
    }
    if (url.path() == kTestInitPath) {
      response->set_content(redirect_to_invalid_domain_
                                ? kTestInitInvalidRedirectHtml
                                : kTestInitHtml);
    } else if (url.path() == kTestFinalPath) {
      auto cookie_it = request.headers.find(net::HttpRequestHeaders::kCookie);
      bool has_cookie =
          cookie_it != request.headers.end() &&
          cookie_it->second.find("testcookie=value") != std::string::npos;
      EXPECT_TRUE(has_cookie);
      if (has_cookie) {
        response->set_content(kTestFinalHtml);
      } else {
        response->set_content(
            "<html><body>Cookie validation failed</body></html>");
        response->set_code(net::HTTP_BAD_REQUEST);
      }
    } else {
      response->set_content("<html><body>Not Found</body></html>");
      response->set_code(net::HTTP_NOT_FOUND);
    }

    return response;
  }

  bool redirect_to_invalid_domain_ = false;
  base::test::ScopedFeatureList scoped_feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;

  raw_ptr<BackupResultsService> backup_results_service_;

  std::optional<std::string> last_custom_header_;
  std::optional<std::string> last_user_agent_;
};

IN_PROC_BROWSER_TEST_F(BackupResultsServiceBrowserTest, BasicRenderAndLoad) {
  GURL url = https_server_->GetURL("google.ca", kTestInitPath);

  {
    base::RunLoop run_loop;
    backup_results_service_->FetchBackupResults(
        url, std::nullopt,
        base::BindLambdaForTesting(
            [&](std::optional<BackupResultsService::BackupResults> result) {
              EXPECT_TRUE(result.has_value());
              if (result) {
                EXPECT_EQ(kTestFinalHtml, result->html);
                EXPECT_EQ(net::HTTP_OK, result->final_status_code);
              }
              EXPECT_FALSE(last_custom_header_);
              EXPECT_TRUE(last_user_agent_);
              EXPECT_NE(last_user_agent_, kTestUAOverride);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  BackupResultsServiceImpl::RecordLastViewSize(g_browser_process->local_state(),
                                               gfx::Size(1280, 720));

  {
    base::RunLoop run_loop;
    backup_results_service_->FetchBackupResults(
        url, std::nullopt,
        base::BindLambdaForTesting(
            [&](std::optional<BackupResultsService::BackupResults> result) {
              EXPECT_TRUE(result.has_value());
              if (result) {
                EXPECT_EQ(kTestFinalHtml, result->html);
                EXPECT_EQ(net::HTTP_OK, result->final_status_code);
              }
              run_loop.Quit();
            }));
    run_loop.Run();
  }
}

IN_PROC_BROWSER_TEST_F(BackupResultsServiceBrowserTest, InvalidDomain) {
  base::RunLoop run_loop;
  GURL url = https_server_->GetURL("google.invalid", kTestInitPath);

  backup_results_service_->FetchBackupResults(
      url, std::nullopt,
      base::BindLambdaForTesting(
          [&](std::optional<BackupResultsService::BackupResults> result) {
            EXPECT_FALSE(result.has_value());
            run_loop.Quit();
          }));

  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(BackupResultsServiceBrowserTest, InvalidRedirect) {
  redirect_to_invalid_domain_ = true;

  base::RunLoop run_loop;
  GURL url = https_server_->GetURL("google.ca", kTestInitPath);

  backup_results_service_->FetchBackupResults(
      url, std::nullopt,
      base::BindLambdaForTesting(
          [&](std::optional<BackupResultsService::BackupResults> result) {
            EXPECT_FALSE(result.has_value());
            run_loop.Quit();
          }));

  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(BackupResultsServiceBrowserTest, CookieHeader) {
  base::RunLoop run_loop;
  GURL url = https_server_->GetURL("google.co.uk", kTestFinalPath);

  net::HttpRequestHeaders headers;
  headers.SetHeader(net::HttpRequestHeaders::kCookie, "testcookie=value");

  backup_results_service_->FetchBackupResults(
      url, headers,
      base::BindLambdaForTesting(
          [&](std::optional<BackupResultsService::BackupResults> result) {
            EXPECT_TRUE(result.has_value());
            if (result) {
              EXPECT_EQ(kTestFinalHtml, result->html);
              EXPECT_EQ(net::HTTP_OK, result->final_status_code);
            }
            EXPECT_FALSE(last_custom_header_);
            EXPECT_TRUE(last_user_agent_);
            EXPECT_NE(last_user_agent_, kTestUAOverride);
            run_loop.Quit();
          }));

  run_loop.Run();
}

class BackupResultsServiceFullRenderBrowserTest
    : public BackupResultsServiceBrowserTest {
 public:
  BackupResultsServiceFullRenderBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        features::kBackupResultsFullRender);
  }
};

IN_PROC_BROWSER_TEST_F(BackupResultsServiceFullRenderBrowserTest, FullRender) {
  base::RunLoop run_loop;
  GURL url = https_server_->GetURL("google.com", kTestInitPath);

  backup_results_service_->FetchBackupResults(
      url, std::nullopt,
      base::BindLambdaForTesting(
          [&](std::optional<BackupResultsService::BackupResults> result) {
            EXPECT_TRUE(result.has_value());
            if (result) {
              EXPECT_EQ(kTestFinalHtml, result->html);
              EXPECT_EQ(net::HTTP_OK, result->final_status_code);
            }
            run_loop.Quit();
          }));

  run_loop.Run();
}

class BackupResultsServiceDisabledBrowserTest
    : public BackupResultsServiceBrowserTest {
 public:
  BackupResultsServiceDisabledBrowserTest() {
    scoped_feature_list_.InitAndDisableFeature(features::kBackupResults);
  }
};

IN_PROC_BROWSER_TEST_F(BackupResultsServiceDisabledBrowserTest,
                       FeatureDisabled) {
  base::RunLoop run_loop;
  GURL url = https_server_->GetURL("google.ca", kTestInitPath);

  backup_results_service_->FetchBackupResults(
      url, std::nullopt,
      base::BindLambdaForTesting(
          [&](std::optional<BackupResultsService::BackupResults> result) {
            EXPECT_FALSE(result.has_value());
            run_loop.Quit();
          }));

  run_loop.Run();
}

class BackupResultsServiceFeatureHeadersBrowserTest
    : public BackupResultsServiceBrowserTest {
 public:
  BackupResultsServiceFeatureHeadersBrowserTest() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBackupResults,
          {{"headers", absl::StrFormat("{\"%s\":\"%s\"}", kTestCustomHeaderName,
                                       kTestCustomHeaderValue)}}}},
        {});
  }
};

IN_PROC_BROWSER_TEST_F(BackupResultsServiceFeatureHeadersBrowserTest,
                       SimpleURLLoader) {
  base::RunLoop run_loop;
  GURL url = https_server_->GetURL("google.co.uk", kTestFinalPath);

  net::HttpRequestHeaders headers;
  headers.SetHeader(net::HttpRequestHeaders::kCookie, "testcookie=value");

  backup_results_service_->FetchBackupResults(
      url, headers,
      base::BindLambdaForTesting(
          [&](std::optional<BackupResultsService::BackupResults> result) {
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ(last_custom_header_, kTestCustomHeaderValue);
            EXPECT_TRUE(last_user_agent_);
            EXPECT_NE(last_user_agent_, kTestUAOverride);
            run_loop.Quit();
          }));

  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(BackupResultsServiceFeatureHeadersBrowserTest,
                       WebContents) {
  base::RunLoop run_loop;
  GURL url = https_server_->GetURL("google.ca", kTestInitPath);

  backup_results_service_->FetchBackupResults(
      url, std::nullopt,
      base::BindLambdaForTesting(
          [&](std::optional<BackupResultsService::BackupResults> result) {
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ(last_custom_header_, kTestCustomHeaderValue);
            EXPECT_TRUE(last_user_agent_);
            EXPECT_NE(last_user_agent_, kTestUAOverride);
            run_loop.Quit();
          }));

  run_loop.Run();
}

class BackupResultsServiceUAOverrideBrowserTest
    : public BackupResultsServiceBrowserTest {
 public:
  BackupResultsServiceUAOverrideBrowserTest() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBackupResults, {{"ua_override", kTestUAOverride}}}}, {});
  }
};

IN_PROC_BROWSER_TEST_F(BackupResultsServiceUAOverrideBrowserTest, WebContents) {
  base::RunLoop run_loop;
  GURL url = https_server_->GetURL("google.ca", kTestInitPath);

  backup_results_service_->FetchBackupResults(
      url, std::nullopt,
      base::BindLambdaForTesting(
          [&](std::optional<BackupResultsService::BackupResults> result) {
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ(last_user_agent_, kTestUAOverride);
            run_loop.Quit();
          }));

  run_loop.Run();
}

class BackupResultsServiceUAOverrideWithMetadataBrowserTest
    : public BackupResultsServiceBrowserTest {
 public:
  BackupResultsServiceUAOverrideWithMetadataBrowserTest() {
    blink::UserAgentMetadata ua_metadata;
    ua_metadata.brand_version_list = {{"TestBrowser", "1"}};
    ua_metadata.brand_full_version_list = {{"TestBrowser", "1.0"}};
    ua_metadata.full_version = "1.0";
    ua_metadata.platform = "Linux";
    ua_metadata.platform_version = "1.0";
    ua_metadata.architecture = "x86";
    ua_metadata.model = "";
    ua_metadata.mobile = false;

    auto marshalled = blink::UserAgentMetadata::Marshal(ua_metadata);
    std::string encoded =
        marshalled ? base::Base64Encode(*marshalled) : std::string();

    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBackupResults,
          {{"ua_override", kTestUAOverride}, {"ua_metadata", encoded}}}},
        {});
  }
};

IN_PROC_BROWSER_TEST_F(BackupResultsServiceUAOverrideWithMetadataBrowserTest,
                       WebContents) {
  base::RunLoop run_loop;
  GURL url = https_server_->GetURL("google.ca", kTestInitPath);

  backup_results_service_->FetchBackupResults(
      url, std::nullopt,
      base::BindLambdaForTesting(
          [&](std::optional<BackupResultsService::BackupResults> result) {
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ(last_user_agent_, kTestUAOverride);
            run_loop.Quit();
          }));

  run_loop.Run();
}

class BackupResultsServiceDailyLimitBrowserTest
    : public BackupResultsServiceBrowserTest {
 public:
  BackupResultsServiceDailyLimitBrowserTest() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBackupResults, {{"max_daily_requests", "2"}}}}, {});
  }
};

// Verifies that once the daily limit is reached, subsequent fetches fail
// immediately without hitting the network.
IN_PROC_BROWSER_TEST_F(BackupResultsServiceDailyLimitBrowserTest,
                       DailyLimitEnforced) {
  GURL url = https_server_->GetURL("google.co.uk", kTestFinalPath);
  net::HttpRequestHeaders headers;
  headers.SetHeader(net::HttpRequestHeaders::kCookie, "testcookie=value");

  // First two requests should succeed (limit is 2).
  for (int i = 0; i < 2; i++) {
    base::RunLoop run_loop;
    backup_results_service_->FetchBackupResults(
        url, headers,
        base::BindLambdaForTesting(
            [&](std::optional<BackupResultsService::BackupResults> result) {
              EXPECT_TRUE(result.has_value());
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  // Third request should be rejected immediately.
  {
    base::RunLoop run_loop;
    backup_results_service_->FetchBackupResults(
        url, headers,
        base::BindLambdaForTesting(
            [&](std::optional<BackupResultsService::BackupResults> result) {
              EXPECT_FALSE(result.has_value());
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  // Simulate a day passing by backdating the window start pref.
  g_browser_process->local_state()->SetTime(
      prefs::kBackupResultsDailyRequestWindowStart,
      base::Time::Now() - base::Days(1) - base::Seconds(1));

  // First request of the new window should succeed.
  {
    base::RunLoop run_loop;
    backup_results_service_->FetchBackupResults(
        url, headers,
        base::BindLambdaForTesting(
            [&](std::optional<BackupResultsService::BackupResults> result) {
              EXPECT_TRUE(result.has_value());
              run_loop.Quit();
            }));
    run_loop.Run();
  }
}

// Verifies that the default param value (-1) does not impose any daily limit.
IN_PROC_BROWSER_TEST_F(BackupResultsServiceBrowserTest, NoDailyLimitByDefault) {
  GURL url = https_server_->GetURL("google.co.uk", kTestFinalPath);
  net::HttpRequestHeaders headers;
  headers.SetHeader(net::HttpRequestHeaders::kCookie, "testcookie=value");

  for (int i = 0; i < 5; i++) {
    base::RunLoop run_loop;
    backup_results_service_->FetchBackupResults(
        url, headers,
        base::BindLambdaForTesting(
            [&](std::optional<BackupResultsService::BackupResults> result) {
              EXPECT_TRUE(result.has_value());
              run_loop.Quit();
            }));
    run_loop.Run();
  }
}

}  // namespace brave_search
