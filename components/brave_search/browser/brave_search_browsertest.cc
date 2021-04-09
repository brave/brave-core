/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using extensions::ExtensionBrowserTest;

const char kAllowedDomain[] = "search.brave.com";
const char kAllowedDomainDev[] = "search-dev.brave.com";
const char kNotAllowedDomain[] = "brave.com";

class BraveSearchTest : public InProcessBrowserTest {
 public:
  BraveSearchTest() {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_->Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

 private:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(BraveSearchTest, DISABLED_CheckForAFunction) {
  GURL url = https_server()->GetURL(kAllowedDomain, "/simple.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);

  auto result_first = EvalJsWithManualReply(contents,
                                            R"(function waitForFunction() {
          if (window.chrome.fetchBackupResults != undefined) {
            window.domAutomationController.send(true);
          } else {
            console.log('still waiting for the function');
            setTimeout(waitForFunction, 200);
          }
        } waitForFunction();)");
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveSearchTest, DISABLED_CheckForAFunctionDev) {
  GURL url = https_server()->GetURL(kAllowedDomainDev, "/simple.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);

  auto result_first = EvalJsWithManualReply(contents,
                                            R"(function waitForFunction() {
          if (window.chrome.fetchBackupResults != undefined) {
            window.domAutomationController.send(true);
          } else {
            console.log('still waiting for the function');
            setTimeout(waitForFunction, 200);
          }
        } waitForFunction();)");
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveSearchTest, DISABLED_CheckForAnUndefinedFunction) {
  GURL url = https_server()->GetURL(kNotAllowedDomain, "/simple.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);

  auto result_first = EvalJsWithManualReply(contents,
                                            R"(function waitForFunction() {
          if (window.chrome.fetchBackupResults != undefined) {
            window.domAutomationController.send(false);
          } else {
            window.domAutomationController.send(true);
          }
        } setTimeout(waitForFunction, 1000);)");
  EXPECT_EQ(base::Value(true), result_first.value);
}
