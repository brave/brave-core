// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/base64.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/tor/pref_names.h"
#include "brave/components/tor/tor_navigation_throttle.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

class OnionDomainThrottleBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    auto request_handler = [](const net::test_server::HttpRequest& request)
        -> std::unique_ptr<net::test_server::HttpResponse> {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_content_type("image/png");
      std::string image;
      std::string base64_image =
          "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQYV2NIbbj6HwAF"
          "wgK6ho3LlwAAAABJRU5ErkJggg==";
      base::Base64Decode(base64_image, &image);
      http_response->set_content(image);
      return http_response;
    };

    https_server_->RegisterDefaultHandler(base::BindRepeating(request_handler));
    ASSERT_TRUE(https_server_->Start());
  }

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

  net::EmbeddedTestServer* test_server() { return https_server_.get(); }

  std::string image_script(const std::string& src) {
    return base::StringPrintf(R"(
        new Promise(resolve => {
          let img = document.createElement('img');
          img.src = '%s';
          img.onload = function () {
            resolve(true);
          };
          img.onerror = function() {
            resolve(false);
          };
        });
    )",
                              src.c_str());
  }

  Browser* OpenTorWindow() {
    return TorProfileManager::SwitchToTorProfile(browser()->profile());
  }

  void SubresourceRequest_testCases(bool only_in_tor_windows) {
    net::ProxyConfigServiceTor::SetBypassTorProxyConfigForTesting(true);
    tor::TorNavigationThrottle::SetSkipWaitForTorConnectedForTesting(true);
    auto* tor_browser = OpenTorWindow();
    ASSERT_TRUE(tor_browser);
    const GURL& url = https_server_->GetURL("example.com", "/favicon.ico");
    const GURL& onion_url =
        https_server_->GetURL("example.onion", "/favicon.ico");

    browser()->profile()->GetPrefs()->SetBoolean(
        tor::prefs::kOnionOnlyInTorWindows, only_in_tor_windows);
    struct {
      raw_ptr<Browser> browser;
      std::string src;
      bool result;
    } cases[] = {
        {browser(), onion_url.spec(), !only_in_tor_windows},
        {browser(), url.spec(), true},
        {tor_browser, onion_url.spec(), true},
        {tor_browser, url.spec(), true},
    };
    for (const auto& test_case : cases) {
      SCOPED_TRACE(testing::Message()
                   << test_case.src
                   << (test_case.browser == tor_browser ? "->Tor Window"
                                                        : "->Normal Window"));
      ASSERT_TRUE(ui_test_utils::NavigateToURL(
          test_case.browser,
          https_server_->GetURL("brave.com", "/simple.html")));
      content::WebContents* contents =
          test_case.browser->tab_strip_model()->GetActiveWebContents();
      auto loaded = EvalJs(contents, image_script(test_case.src));
      ASSERT_TRUE(loaded.error.empty());
      EXPECT_EQ(base::Value(test_case.result), loaded.value);
    }
  }

 protected:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
};

IN_PROC_BROWSER_TEST_F(OnionDomainThrottleBrowserTest,
                       SubresourceRequestsBlocked) {
  SubresourceRequest_testCases(true);
}

IN_PROC_BROWSER_TEST_F(OnionDomainThrottleBrowserTest,
                       SubresourceRequestsAllowed) {
  SubresourceRequest_testCases(false);
}
