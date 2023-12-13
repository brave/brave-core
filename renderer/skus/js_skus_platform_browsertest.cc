// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "brave/components/skus/common/features.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  response->set_content(R"({})");
  response->set_content_type("application/json");
  return response;
}

}  // namespace

namespace skus {

class JsSkusPlatformBrowserTest : public PlatformBrowserTest {
 public:
  JsSkusPlatformBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitWithFeatures({skus::features::kSkusFeature}, {});
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.RegisterRequestHandler(base::BindRepeating(HandleRequest));
    content::SetBrowserClientForTesting(&client_);

    ASSERT_TRUE(https_server_.Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

 protected:
  net::EmbeddedTestServer https_server_;

 private:
  BraveContentBrowserClient client_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(JsSkusPlatformBrowserTest, FetchOrderCredentialsError) {
  const std::string script = R"((async () => {
      try {
          await window.chrome.braveSkus.fetch_order_credentials('', '');
      } catch (error) {
          document.title = error;
      }
  })();)";
  const GURL url = https_server_.GetURL("account.brave.software", "/");
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  content::ExecuteScriptAsync(web_contents()->GetPrimaryMainFrame(), script);
  // This message comes from an error by the SKUs SDK in Rust.
  std::u16string expected_title(u"HTTP request failed");
  content::TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

}  // namespace skus
