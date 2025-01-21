// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/browser/backup_results_service.h"

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_search/backup_results_service_factory.h"
#include "brave/components/brave_search/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace brave_search {

namespace {

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
  }

 protected:
  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_code(net::HTTP_OK);
    response->set_content_type("text/html");

    auto path = request.GetURL().path();
    if (path == kTestInitPath) {
      response->set_content(kTestInitHtml);
    } else if (path == kTestFinalPath) {
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

  base::test::ScopedFeatureList scoped_feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(BackupResultsServiceBrowserTest, BasicRenderAndLoad) {
  auto* backup_results_service =
      BackupResultsServiceFactory::GetForBrowserContext(browser()->profile());

  base::RunLoop run_loop;
  GURL url = https_server_->GetURL("a.com", kTestInitPath);

  backup_results_service->FetchBackupResults(
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

IN_PROC_BROWSER_TEST_F(BackupResultsServiceBrowserTest, CookieHeader) {
  auto* backup_results_service =
      BackupResultsServiceFactory::GetForBrowserContext(browser()->profile());

  base::RunLoop run_loop;
  GURL url = https_server_->GetURL("a.com", kTestFinalPath);

  net::HttpRequestHeaders headers;
  headers.SetHeader(net::HttpRequestHeaders::kCookie, "testcookie=value");

  backup_results_service->FetchBackupResults(
      url, headers,
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

class BackupResultsServiceFullRenderBrowserTest
    : public BackupResultsServiceBrowserTest {
 public:
  BackupResultsServiceFullRenderBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        features::kBackupResultsFullRender);
  }
};

IN_PROC_BROWSER_TEST_F(BackupResultsServiceFullRenderBrowserTest, FullRender) {
  auto* backup_results_service =
      BackupResultsServiceFactory::GetForBrowserContext(browser()->profile());

  base::RunLoop run_loop;
  GURL url = https_server_->GetURL("a.com", kTestInitPath);

  backup_results_service->FetchBackupResults(
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

}  // namespace brave_search
