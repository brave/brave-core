/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_user_agent/browser/brave_user_agent_exceptions.h"
#include "brave/components/brave_user_agent/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kSecCHUAHeader[] = "Sec-CH-UA";
constexpr char kSecCHUAFullVersionListHeader[] = "Sec-CH-UA-Full-Version-List";
constexpr char kBraveBrand[] = "Brave";
constexpr char kGoogleChromeBrand[] = "Google Chrome";

struct HeaderCapture {
  bool allows_brave_header = false;
  std::string path;
  std::optional<std::string> sec_ch_ua;
  std::optional<std::string> sec_ch_ua_full_version_list;
};

std::unique_ptr<::net::test_server::HttpResponse> CreateBasicHttpResponse(
    const std::string& content,
    const std::string& content_type) {
  auto* basic = new ::net::test_server::BasicHttpResponse();
  basic->set_content(content);
  basic->set_content_type(content_type);
  return std::unique_ptr<::net::test_server::HttpResponse>(basic);
}

class BraveUserAgentNetworkDelegateBrowserTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<bool> {
 public:
  BraveUserAgentNetworkDelegateBrowserTest() {
    if (GetParam()) {
      feature_list_.InitAndEnableFeature(
          brave_user_agent::features::kUseBraveUserAgent);
    } else {
      feature_list_.InitAndDisableFeature(
          brave_user_agent::features::kUseBraveUserAgent);
    }
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    if (GetParam()) {
      // Add excepted domains for testing
      auto* exceptions =
          brave_user_agent::BraveUserAgentExceptions::GetInstance();
      exceptions->AddToExceptedDomainsForTesting("a.test");
      exceptions->SetIsReadyForTesting();
    } else {
      ASSERT_FALSE(brave_user_agent::BraveUserAgentExceptions::GetInstance());
    }

    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &BraveUserAgentNetworkDelegateBrowserTest::HandleRequest,
        base::Unretained(this)));
    https_server_.RegisterRequestHandler(base::BindRepeating(
        &BraveUserAgentNetworkDelegateBrowserTest::HandleCustomRequest,
        base::Unretained(this)));
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_.Start());
  }

 protected:
  std::unique_ptr<::net::test_server::HttpResponse> HandleCustomRequest(
      const net::test_server::HttpRequest& request);
  void HandleRequest(const net::test_server::HttpRequest& request);
  void StartTracking();
  const std::vector<HeaderCapture>& header_capture();
  net::EmbeddedTestServer& https_server();
  void RunBrandHeaderTest(const std::string& domain, const std::string& path);
  void NavigateAndWait(const GURL& url);

  void ExpectHeaderBrands(const std::vector<HeaderCapture>& captures) {
    ASSERT_TRUE(!captures.empty());

    for (const auto& capture : captures) {
      SCOPED_TRACE(capture.path);

      ASSERT_TRUE(capture.sec_ch_ua.has_value());
      ASSERT_TRUE(capture.sec_ch_ua_full_version_list.has_value());

      const bool feature_enabled = GetParam();
      const bool contains_brave =
          capture.allows_brave_header || !feature_enabled;
      const bool contains_chrome =
          !capture.allows_brave_header && feature_enabled;

      EXPECT_NE(contains_brave, contains_chrome);

      EXPECT_EQ(contains_brave, capture.sec_ch_ua->contains(kBraveBrand));
      EXPECT_EQ(contains_brave,
                capture.sec_ch_ua_full_version_list->contains(kBraveBrand));
      EXPECT_EQ(contains_chrome,
                capture.sec_ch_ua->contains(kGoogleChromeBrand));
      EXPECT_EQ(contains_chrome, capture.sec_ch_ua_full_version_list->contains(
                                     kGoogleChromeBrand));
    }
  }

 private:
  base::Lock header_lock_;
  bool start_tracking_ = false;
  std::vector<HeaderCapture> header_capture_;
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
  base::test::ScopedFeatureList feature_list_;
};

std::unique_ptr<::net::test_server::HttpResponse>
BraveUserAgentNetworkDelegateBrowserTest::HandleCustomRequest(
    const net::test_server::HttpRequest& request) {
  if (request.relative_url == "/a.test/redirect_a_to_b.html") {
    auto* response = new ::net::test_server::BasicHttpResponse();
    response->set_code(net::HTTP_FOUND);
    response->AddCustomHeader(
        "Location",
        https_server_.GetURL("b.test", "/b.test/simple.html").spec());
    response->AddCustomHeader("Accept-CH", "Sec-CH-UA-Full-Version-List");
    return std::unique_ptr<::net::test_server::HttpResponse>(response);
  }
  if (request.relative_url == "/b.test/redirect_b_to_a.html") {
    auto* response = new ::net::test_server::BasicHttpResponse();
    response->set_code(net::HTTP_FOUND);
    response->AddCustomHeader(
        "Location",
        https_server_.GetURL("a.test", "/a.test/simple.html").spec());
    response->AddCustomHeader("Accept-CH", "Sec-CH-UA-Full-Version-List");
    return std::unique_ptr<::net::test_server::HttpResponse>(response);
  }
  if (request.relative_url == "/a.test/page_with_image.html") {
    auto* response = new ::net::test_server::BasicHttpResponse();
    response->set_content(
        "<html><body><img src=\"https://b.test/b.test/image.png\" "
        "/></body></html>");
    response->set_content_type("text/html");
    response->AddCustomHeader("Accept-CH", "Sec-CH-UA-Full-Version-List");
    return std::unique_ptr<::net::test_server::HttpResponse>(response);
  }
  if (request.relative_url == "/a.test/simple.html" ||
      request.relative_url == "/b.test/simple.html") {
    auto* response = new ::net::test_server::BasicHttpResponse();
    response->set_content("<html><body>ok</body></html>");
    response->set_content_type("text/html");
    response->AddCustomHeader("Accept-CH", "Sec-CH-UA-Full-Version-List");
    return std::unique_ptr<::net::test_server::HttpResponse>(response);
  }
  if (request.relative_url == "/b.test/image.png") {
    return CreateBasicHttpResponse("fake image", "image/png");
  }
  return nullptr;
}

void BraveUserAgentNetworkDelegateBrowserTest::HandleRequest(
    const net::test_server::HttpRequest& request) {
  base::AutoLock auto_lock(header_lock_);
  if (!start_tracking_) {
    return;
  }

  if (request.relative_url.starts_with("/favicon")) {
    return;
  }

  HeaderCapture capture;
  capture.path = request.relative_url;
  capture.allows_brave_header = !request.relative_url.starts_with("/a.test/");

  auto it = request.headers.find(kSecCHUAHeader);
  if (it != request.headers.end()) {
    capture.sec_ch_ua = it->second;
  }
  auto it2 = request.headers.find(kSecCHUAFullVersionListHeader);
  if (it2 != request.headers.end()) {
    capture.sec_ch_ua_full_version_list = it2->second;
  }

  header_capture_.push_back(std::move(capture));
}

void BraveUserAgentNetworkDelegateBrowserTest::StartTracking() {
  base::AutoLock auto_lock(header_lock_);
  start_tracking_ = true;
  header_capture_.clear();
}

const std::vector<HeaderCapture>&
BraveUserAgentNetworkDelegateBrowserTest::header_capture() {
  base::AutoLock auto_lock(header_lock_);
  return header_capture_;
}

net::EmbeddedTestServer&
BraveUserAgentNetworkDelegateBrowserTest::https_server() {
  return https_server_;
}

void BraveUserAgentNetworkDelegateBrowserTest::NavigateAndWait(
    const GURL& url) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  auto* rfh = browser()
                  ->tab_strip_model()
                  ->GetActiveWebContents()
                  ->GetPrimaryMainFrame();
  ASSERT_TRUE(content::ExecJs(rfh, R"(
    new Promise(resolve => {
      if (document.readyState === 'complete') resolve();
      else window.onload = () => resolve();
    });
  )"));
}

void BraveUserAgentNetworkDelegateBrowserTest::RunBrandHeaderTest(
    const std::string& domain,
    const std::string& path) {
  const GURL url = https_server().GetURL(domain, path);
  NavigateAndWait(url);  // Prime client hint cache
  StartTracking();
  NavigateAndWait(url);  // Actual test navigation
  ExpectHeaderBrands(header_capture());
}

IN_PROC_BROWSER_TEST_P(BraveUserAgentNetworkDelegateBrowserTest,
                       SecCHUAHeadersBrandCheck) {
  RunBrandHeaderTest("a.test", "/a.test/simple.html");
}

IN_PROC_BROWSER_TEST_P(BraveUserAgentNetworkDelegateBrowserTest,
                       SecCHUAHeadersBrandCheckOnThirdPartyRequest) {
  RunBrandHeaderTest("a.test", "/a.test/page_with_image.html");
}

IN_PROC_BROWSER_TEST_P(BraveUserAgentNetworkDelegateBrowserTest,
                       SecCHUAHeadersAfterRedirectFromExceptedToNonExcepted) {
  RunBrandHeaderTest("a.test", "/a.test/redirect_a_to_b.html");
}

IN_PROC_BROWSER_TEST_P(BraveUserAgentNetworkDelegateBrowserTest,
                       SecCHUAHeadersAfterRedirectFromNonExceptedToExcepted) {
  RunBrandHeaderTest("b.test", "/b.test/redirect_b_to_a.html");
}

INSTANTIATE_TEST_SUITE_P(FeatureFlag,
                         BraveUserAgentNetworkDelegateBrowserTest,
                         ::testing::Bool());
}  // namespace
