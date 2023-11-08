// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/testing_browser_process.h"
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

const char kScript[] = R"(
      (() => {
        return new Promise((resolve) => {
          document.title = 'test';
          resolve(%s)
        });
      })();
      )";
      
}  // namespace

mojo::AssociatedRemote<script_injector::mojom::ScriptInjector> GetRemote(
    content::RenderFrameHost* rfh) {
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote;
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&script_injector_remote);
  return script_injector_remote;
}

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  response->set_content(R"(
                          <html>
                           <head><title>OK</title></head>
                          </html>
                        )");
  response->set_content_type("text/html; charset=utf-8");
  return response;
}

class ScriptInjectorBrowserTest : public PlatformBrowserTest {
 public:
  ScriptInjectorBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.RegisterRequestHandler(base::BindRepeating(HandleRequest));

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
  content::ContentMockCertVerifier mock_cert_verifier_;
};

// TESTS

IN_PROC_BROWSER_TEST_F(ScriptInjectorBrowserTest, ScriptInjected) {
  const GURL url = https_server_.GetURL("a.com", "/");
  auto cb = base::BindOnce([](base::Value value) {
    EXPECT_TRUE(value.GetIfBool().value_or(false));
  });
  auto script = base::StringPrintf(kScript, "true");
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  GetRemote(web_contents()->GetPrimaryMainFrame())
      ->RequestAsyncExecuteScript(
          ISOLATED_WORLD_ID_BRAVE_INTERNAL,
          base::UTF8ToUTF16(std::string(script)),
          blink::mojom::UserActivationOption::kDoNotActivate,
          blink::mojom::PromiseResultOption::kAwait, std::move(cb));
  std::u16string expected_title(u"test");
  content::TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(ScriptInjectorBrowserTest, ScriptInjectedReturnsFalse) {
  const GURL url = https_server_.GetURL("a.com", "/");
  auto cb = base::BindOnce([](base::Value value) {
    EXPECT_FALSE(value.GetIfBool().value_or(false));
  });
  auto script = base::StringPrintf(kScript, "false");
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  GetRemote(web_contents()->GetPrimaryMainFrame())
      ->RequestAsyncExecuteScript(
          ISOLATED_WORLD_ID_BRAVE_INTERNAL,
          base::UTF8ToUTF16(std::string(script)),
          blink::mojom::UserActivationOption::kDoNotActivate,
          blink::mojom::PromiseResultOption::kAwait, std::move(cb));
  std::u16string expected_title(u"test");
  content::TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(ScriptInjectorBrowserTest, ScriptInjectedDoNotAwait) {
  const GURL url = https_server_.GetURL("a.com", "/");
  auto cb = base::BindOnce([](base::Value value) {
    EXPECT_TRUE(value.GetIfBool().value_or(false));
  });
  auto script = base::StringPrintf(kScript, "true");
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  GetRemote(web_contents()->GetPrimaryMainFrame())
      ->RequestAsyncExecuteScript(
          ISOLATED_WORLD_ID_BRAVE_INTERNAL,
          base::UTF8ToUTF16(std::string(script)),
          blink::mojom::UserActivationOption::kDoNotActivate,
          blink::mojom::PromiseResultOption::kDoNotWait, std::move(cb));
  // Test will not wait for the promise.
  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}
