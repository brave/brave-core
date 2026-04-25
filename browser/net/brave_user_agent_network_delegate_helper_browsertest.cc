/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kSecCHUAHeader[] = "Sec-CH-UA";
constexpr char kSecCHUAFullVersionListHeader[] = "Sec-CH-UA-Full-Version-List";
constexpr char kBraveBrand[] = "Brave";
constexpr char kGoogleChromeBrand[] = "Google Chrome";

struct HeaderCapture {
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
    auto* command_line = base::CommandLine::ForCurrentProcess();
    if (command_line->HasSwitch("enable-brave-user-agent")) {
      feature_list_.InitAndEnableFeature(
          brave_user_agent::features::kUseBraveUserAgent);
    } else if (command_line->HasSwitch("disable-brave-user-agent")) {
      feature_list_.InitAndDisableFeature(
          brave_user_agent::features::kUseBraveUserAgent);
    }
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    if (GetParam()) {
      command_line->AppendSwitch("enable-brave-user-agent");
    } else {
      command_line->AppendSwitch("disable-brave-user-agent");
    }
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Add excepted domains for testing
    auto* exceptions =
        brave_user_agent::BraveUserAgentExceptions::GetInstance();
    exceptions->AddToExceptedDomainsForTesting("a.test");
    exceptions->SetIsReadyForTesting();
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &BraveUserAgentNetworkDelegateBrowserTest::HandleRequest,
        base::Unretained(this)));
    RegisterImagePageHandler();
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_.Start());
  }

 protected:
  void RegisterImagePageHandler();
  void HandleRequest(const net::test_server::HttpRequest& request);
  void StartTracking();
  HeaderCapture header_capture();
  net::EmbeddedTestServer& https_server();
  void RunBrandHeaderTest(const std::string& domain, const std::string& path);
  void NavigateAndWait(const GURL& url);
  base::test::ScopedFeatureList feature_list_;

  void ExpectHeaderBrands(const HeaderCapture& capture, bool feature_enabled) {
    ASSERT_TRUE(capture.sec_ch_ua.has_value());
    ASSERT_TRUE(capture.sec_ch_ua_full_version_list.has_value());
    if (feature_enabled) {
      // Excepted domain, feature enabled: expect Google Chrome
      EXPECT_EQ(!feature_enabled, capture.sec_ch_ua->contains(kBraveBrand));
      EXPECT_EQ(!feature_enabled,
                capture.sec_ch_ua_full_version_list->contains(kBraveBrand));
      EXPECT_EQ(feature_enabled,
                capture.sec_ch_ua->contains(kGoogleChromeBrand));
      EXPECT_EQ(feature_enabled, capture.sec_ch_ua_full_version_list->contains(
                                     kGoogleChromeBrand));
    }
  }

 private:
  base::Lock header_lock_;
  bool start_tracking_ = false;
  HeaderCapture header_capture_;
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

void BraveUserAgentNetworkDelegateBrowserTest::RegisterImagePageHandler() {
  https_server_.RegisterRequestHandler(base::BindRepeating(
      [](const net::test_server::HttpRequest& request)
          -> std::unique_ptr<::net::test_server::HttpResponse> {
        if (request.relative_url == "/page_with_image.html") {
          auto* response = new ::net::test_server::BasicHttpResponse();
          response->set_content(
              "<html><body><img src=\"https://b.test/image.png\" "
              "/></body></html>");
          response->set_content_type("text/html");
          response->AddCustomHeader("Accept-CH", "Sec-CH-UA-Full-Version-List");
          return std::unique_ptr<::net::test_server::HttpResponse>(response);
        }
        if (request.relative_url == "/simple.html") {
          auto* response = new ::net::test_server::BasicHttpResponse();
          response->set_content("<html><body>ok</body></html>");
          response->set_content_type("text/html");
          response->AddCustomHeader("Accept-CH", "Sec-CH-UA-Full-Version-List");
          return std::unique_ptr<::net::test_server::HttpResponse>(response);
        }
        if (request.relative_url == "/image.png") {
          return CreateBasicHttpResponse("fake image", "image/png");
        }
        return nullptr;
      }));
}

void BraveUserAgentNetworkDelegateBrowserTest::HandleRequest(
    const net::test_server::HttpRequest& request) {
  base::AutoLock auto_lock(header_lock_);
  if (!start_tracking_) {
    return;
  }
  auto it = request.headers.find(kSecCHUAHeader);
  if (it != request.headers.end()) {
    header_capture_.sec_ch_ua = it->second;
  }
  auto it2 = request.headers.find(kSecCHUAFullVersionListHeader);
  if (it2 != request.headers.end()) {
    header_capture_.sec_ch_ua_full_version_list = it2->second;
  }
}

void BraveUserAgentNetworkDelegateBrowserTest::StartTracking() {
  base::AutoLock auto_lock(header_lock_);
  start_tracking_ = true;
  header_capture_ = HeaderCapture{};
}

HeaderCapture BraveUserAgentNetworkDelegateBrowserTest::header_capture() {
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
  ExpectHeaderBrands(header_capture(), GetParam());
}

IN_PROC_BROWSER_TEST_P(BraveUserAgentNetworkDelegateBrowserTest,
                       SecCHUAHeadersBrandCheck) {
  RunBrandHeaderTest("a.test", "/simple.html");
}

IN_PROC_BROWSER_TEST_P(BraveUserAgentNetworkDelegateBrowserTest,
                       SecCHUAHeadersBrandCheckOnThirdPartyRequest) {
  RunBrandHeaderTest("a.test", "/page_with_image.html");
}

INSTANTIATE_TEST_SUITE_P(FeatureFlag,
                         BraveUserAgentNetworkDelegateBrowserTest,
                         ::testing::Values(true, false));
}  // namespace
