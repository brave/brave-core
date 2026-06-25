// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/base64.h"
#include "base/strings/strcat.h"
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
#include "net/base/url_util.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/embedded_test_server/install_default_websocket_handlers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace {

constexpr char kRedirectOnionPath[] = "/redirect-onion";
constexpr char kSimplePagePath[] = "/simple.html";

struct OnionAccessScenario {
  bool tor_window = false;
  bool onion_only_in_tor_windows = true;
  bool expect_blocked = true;
};

std::string OnionAccessScenarioName(
    const testing::TestParamInfo<OnionAccessScenario>& info) {
  const OnionAccessScenario& scenario = info.param;
  return base::StrCat(
      {scenario.tor_window ? "TorWindow" : "NormalWindow",
       scenario.onion_only_in_tor_windows ? "_PrefOn" : "_PrefOff",
       scenario.expect_blocked ? "_Blocked" : "_Allowed"});
}

}  // namespace

class OnionDomainThrottleBrowserTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<OnionAccessScenario> {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&OnionDomainThrottleBrowserTest::HandleRequest,
                            base::Unretained(this)));
    net::test_server::InstallDefaultWebSocketHandlers(https_server_.get());
    ASSERT_TRUE(https_server_->Start());

    net::ProxyConfigServiceTor::SetBypassTorProxyConfigForTesting(true);
    tor::TorNavigationThrottle::SetSkipWaitForTorConnectedForTesting(true);
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

  Browser* GetBrowser(bool tor_window) {
    if (!tor_window) {
      return browser();
    }
    return TorProfileManager::SwitchToTorProfile(browser()->profile());
  }

  content::WebContents* GetActiveWebContents(Browser* browser) {
    return (browser ? browser : this->browser())
        ->tab_strip_model()
        ->GetActiveWebContents();
  }

  Browser* SetUpScenario(const OnionAccessScenario& scenario) {
    browser()->profile()->GetPrefs()->SetBoolean(
        tor::prefs::kOnionOnlyInTorWindows, scenario.onion_only_in_tor_windows);

    Browser* browser = GetBrowser(scenario.tor_window);

    [&]() {
      ASSERT_TRUE(ui_test_utils::NavigateToURL(
          browser, https_server_->GetURL("brave.com", kSimplePagePath)));
    }();
    return browser;
  }

  GURL OnionResourceUrl() const {
    return https_server_->GetURL("example.onion", "/favicon.ico");
  }

  GURL RedirectToOnionUrl() const {
    return https_server_->GetURL("example.com", kRedirectOnionPath);
  }

  std::string ImageLoadScript(const std::string& src) {
    return absl::StrFormat(R"(
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
                           src);
  }

  std::string FetchLoadScript(const std::string& url) {
    return content::JsReplace(
        R"(fetch($1, {redirect: 'follow', mode: 'no-cors'})
            .then(() => true)
            .catch(() => false);)",
        url);
  }

  std::string WebSocketOpenScript(const GURL& ws_url) {
    return content::JsReplace(
        R"(new Promise(resolve => {
            let socket = new WebSocket($1);
            socket.addEventListener('open', () => resolve('open'));
            socket.addEventListener('error', () => resolve('error'));
          });)",
        ws_url);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    if (request.relative_url == kRedirectOnionPath) {
      auto http_response =
          std::make_unique<net::test_server::BasicHttpResponse>();
      http_response->set_code(net::HTTP_FOUND);
      http_response->AddCustomHeader("Location", OnionResourceUrl().spec());
      return http_response;
    }

    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_content_type("image/png");
    http_response->AddCustomHeader("Access-Control-Allow-Origin", "*");
    std::string image;
    std::string base64_image =
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVQYV2NIbbj6HwAF"
        "wgK6ho3LlwAAAABJRU5ErkJggg==";
    base::Base64Decode(base64_image, &image);
    http_response->set_content(image);
    return http_response;
  }

 protected:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
};

INSTANTIATE_TEST_SUITE_P(
    ,
    OnionDomainThrottleBrowserTest,
    ::testing::Values(OnionAccessScenario{.tor_window = false,
                                          .onion_only_in_tor_windows = true,
                                          .expect_blocked = true},
                      OnionAccessScenario{.tor_window = false,
                                          .onion_only_in_tor_windows = false,
                                          .expect_blocked = false},
                      OnionAccessScenario{.tor_window = true,
                                          .onion_only_in_tor_windows = true,
                                          .expect_blocked = false},
                      OnionAccessScenario{.tor_window = true,
                                          .onion_only_in_tor_windows = false,
                                          .expect_blocked = false}),
    &OnionAccessScenarioName);

IN_PROC_BROWSER_TEST_P(OnionDomainThrottleBrowserTest,
                       DirectSubresourceToOnion) {
  const OnionAccessScenario scenario = GetParam();
  Browser* browser = SetUpScenario(scenario);
  content::WebContents* web_contents = GetActiveWebContents(browser);

  auto loaded =
      EvalJs(web_contents, ImageLoadScript(OnionResourceUrl().spec()));
  ASSERT_TRUE(loaded.is_ok());
  EXPECT_EQ(!scenario.expect_blocked, loaded.ExtractBool());
}

IN_PROC_BROWSER_TEST_P(OnionDomainThrottleBrowserTest, ImgRedirectToOnion) {
  const OnionAccessScenario scenario = GetParam();
  Browser* browser = SetUpScenario(scenario);
  content::WebContents* web_contents = GetActiveWebContents(browser);

  auto loaded =
      EvalJs(web_contents, ImageLoadScript(RedirectToOnionUrl().spec()));
  ASSERT_TRUE(loaded.is_ok());
  EXPECT_EQ(!scenario.expect_blocked, loaded.ExtractBool());
}

IN_PROC_BROWSER_TEST_P(OnionDomainThrottleBrowserTest, FetchRedirectToOnion) {
  const OnionAccessScenario scenario = GetParam();
  Browser* browser = SetUpScenario(scenario);
  content::WebContents* web_contents = GetActiveWebContents(browser);

  auto loaded =
      EvalJs(web_contents, FetchLoadScript(RedirectToOnionUrl().spec()));
  ASSERT_TRUE(loaded.is_ok());
  EXPECT_EQ(!scenario.expect_blocked, loaded.ExtractBool());
}

IN_PROC_BROWSER_TEST_P(OnionDomainThrottleBrowserTest, IframeOnionNavigation) {
  const OnionAccessScenario scenario = GetParam();
  Browser* browser = SetUpScenario(scenario);
  content::WebContents* web_contents = GetActiveWebContents(browser);

  const GURL onion_url = OnionResourceUrl().GetWithEmptyPath();
  ASSERT_TRUE(ExecJs(web_contents->GetPrimaryMainFrame(),
                     content::JsReplace(
                         R"(const iframe = document.createElement('iframe');
                            iframe.src = $1;
                            document.body.appendChild(iframe);)",
                         onion_url)));
  ASSERT_TRUE(content::WaitForLoadStop(web_contents));

  content::RenderFrameHost* child_frame =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(child_frame);
  EXPECT_EQ(scenario.expect_blocked,
            !net::IsOnion(child_frame->GetLastCommittedURL()));
}

IN_PROC_BROWSER_TEST_P(OnionDomainThrottleBrowserTest, WebSocketToOnion) {
  const OnionAccessScenario scenario = GetParam();
  Browser* browser = SetUpScenario(scenario);
  content::WebContents* web_contents = GetActiveWebContents(browser);

  const GURL ws_url = net::test_server::GetWebSocketURL(
      *https_server_, "example.onion", "/echo-with-no-extension");
  auto result = EvalJs(web_contents, WebSocketOpenScript(ws_url));
  ASSERT_TRUE(result.is_ok());
  EXPECT_EQ(scenario.expect_blocked ? "error" : "open", result);
}
