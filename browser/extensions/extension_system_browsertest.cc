/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "services/network/public/cpp/network_switches.h"

using extensions::ExtensionBrowserTest;

class ExtensionSystemBrowserTest : public ExtensionBrowserTest {
 public:
  ExtensionSystemBrowserTest() = default;

  void SetUp() override {
    ASSERT_TRUE(https_server_.InitializeAndListen());
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();
    dir_test_data_ = base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(dir_test_data_);
    https_server_.StartAcceptingConnections();
    host_resolver()->AddRule("*", "127.0.0.1");
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        base::StringPrintf("MAP *:443 127.0.0.1:%d", https_server_.port()));
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    ExtensionBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    ExtensionBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void NavigateAndExpectErrorPage(const GURL& url, bool expect_error_page) {
    SCOPED_TRACE(url);
    auto* rfh = ui_test_utils::NavigateToURL(browser(), url);
    ASSERT_TRUE(rfh);
    EXPECT_EQ(rfh->IsErrorDocument(), expect_error_page);
  }

 protected:
  base::FilePath dir_test_data_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

IN_PROC_BROWSER_TEST_F(ExtensionSystemBrowserTest,
                       PRE_DeclarativeNetRequestWorksAfterRestart) {
  NavigateAndExpectErrorPage(GURL("https://a.com/simple.html"), false);
  NavigateAndExpectErrorPage(GURL("https://b.com/simple.html"), false);

  // Load extension that should block b.com main frame via declarative net
  // requests feature.
  ASSERT_TRUE(InstallExtensionWithPermissionsGranted(
      dir_test_data_.AppendASCII("extensions")
          .AppendASCII("declarative_net_request"),
      1));

  NavigateAndExpectErrorPage(GURL("https://a.com/simple.html"), false);
  NavigateAndExpectErrorPage(GURL("https://b.com/simple.html"), true);
}

IN_PROC_BROWSER_TEST_F(ExtensionSystemBrowserTest,
                       DeclarativeNetRequestWorksAfterRestart) {
  // After a browser restart the extensions should work as expected.
  NavigateAndExpectErrorPage(GURL("https://a.com/simple.html"), false);
  NavigateAndExpectErrorPage(GURL("https://b.com/simple.html"), true);
}
