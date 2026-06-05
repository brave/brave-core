/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/containers/map_util.h"
#include "brave/components/constants/network_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "testing/gmock/include/gmock/gmock.h"

class BraveServiceKeyNetworkDelegateBrowserTest : public InProcessBrowserTest {
 public:
  BraveServiceKeyNetworkDelegateBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &BraveServiceKeyNetworkDelegateBrowserTest::HandleRequest,
        base::Unretained(this)));

    ASSERT_TRUE(https_server_.Start());
  }

  void HandleRequest(const net::test_server::HttpRequest& request) {
    base::AutoLock auto_lock(header_result_lock_);

    header_result_ = request.headers.contains("BraveServiceKey");
    const std::string* auth =
        base::FindOrNull(request.headers, "BraveServiceSignature");
    authorization_header_ = auth ? *auth : std::string();
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

  const net::EmbeddedTestServer& https_server() { return https_server_; }

  bool header_result() {
    base::AutoLock auto_lock(header_result_lock_);
    return header_result_;
  }

  std::string authorization_header() {
    base::AutoLock auto_lock(header_result_lock_);
    return authorization_header_;
  }

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
  mutable base::Lock header_result_lock_;
  bool header_result_ = false;
  std::string authorization_header_;
};

IN_PROC_BROWSER_TEST_F(BraveServiceKeyNetworkDelegateBrowserTest,
                       NotIncludesBraveServiceKey) {
  GURL target = https_server().GetURL("notbrave.com", "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_FALSE(header_result());

  target = https_server().GetURL("brave.com", "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_FALSE(header_result());

  target = https_server().GetURL("bravesoftware.com", "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_FALSE(header_result());

  target = https_server().GetURL("brave.demo.com", "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_FALSE(header_result());

  target = https_server().GetURL("demo.brave.com", "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_FALSE(header_result());

  target = https_server().GetURL("randomdomain.com", "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_FALSE(header_result());
}

IN_PROC_BROWSER_TEST_F(BraveServiceKeyNetworkDelegateBrowserTest,
                       IncludesBraveServiceKey) {
  GURL target = https_server().GetURL(kExtensionUpdaterDomain, "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_TRUE(header_result());

  target = https_server().GetURL("search.brave.com", "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_TRUE(header_result());

  target = https_server().GetURL("search.brave.software", "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_TRUE(header_result());
}

IN_PROC_BROWSER_TEST_F(BraveServiceKeyNetworkDelegateBrowserTest,
                       IncludesAuthorizationHeaderForSearchDomains) {
  GURL target = https_server().GetURL("search.brave.com", "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_THAT(authorization_header(),
              ::testing::StartsWith("Signature keyId="));
  EXPECT_THAT(authorization_header(),
              ::testing::HasSubstr("headers=\"(request-target)\""));

  target = https_server().GetURL("search.brave.software", "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_THAT(authorization_header(),
              ::testing::StartsWith("Signature keyId="));
  EXPECT_THAT(authorization_header(),
              ::testing::HasSubstr("headers=\"(request-target)\""));

  target = https_server().GetURL(kExtensionUpdaterDomain, "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_TRUE(authorization_header().empty());

  target = https_server().GetURL("brave.com", "/index.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_TRUE(authorization_header().empty());
}
