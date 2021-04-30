/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_search/browser/brave_search_host.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

using extensions::ExtensionBrowserTest;

namespace {

const char kEmbeddedTestServerDirectory[] = "brave-search";
const char kAllowedDomain[] = "search.brave.com";
const char kAllowedDomainDev[] = "search-dev.brave.com";
const char kNotAllowedDomain[] = "brave.com";
const char kBackupSearchContent[] = "<html><body>results</body></html>";

std::string GetChromeFetchBackupResultsAvailScript() {
  return base::StringPrintf(R"(function waitForFunction() {
        setTimeout(waitForFunction, 200);
      }
      navigator.serviceWorker.addEventListener('message', msg => {
        if (msg.data && msg.data.result === 'INJECTED') {
          window.domAutomationController.send(msg.data.response === '%s');
        } else if (msg.data && msg.data.result === 'FAILED') {
          window.domAutomationController.send(false);
      }});
      waitForFunction();)",
                            kBackupSearchContent);
}

}  // namespace

class BraveSearchTest : public InProcessBrowserTest {
 public:
  BraveSearchTest() {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(base::BindRepeating(
        &BraveSearchTest::HandleRequest, base::Unretained(this)));

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    https_server_->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(https_server_->Start());
    GURL url = https_server()->GetURL("a.com", "/search");
    brave_search::BraveSearchHost::SetBackupProviderForTest(url);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    if (request.GetURL().path_piece() == "/sw.js" ||
        request.GetURL().path_piece() == "/bravesearch.html")
      return nullptr;

    GURL url = request.GetURL();
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    if (url.path() + "?" + url.query() ==
        "/search?q=test&hl=en&gl=us&safe=active") {
      http_response->set_code(net::HTTP_OK);
      http_response->set_content_type("text/html");
      http_response->set_content(kBackupSearchContent);
      return http_response;
    }
    http_response->set_code(net::HTTP_NOT_FOUND);
    return http_response;
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

 private:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(BraveSearchTest, CheckForAFunction) {
  GURL url = https_server()->GetURL(kAllowedDomain, "/bravesearch.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);

  auto result_first =
      EvalJsWithManualReply(contents, GetChromeFetchBackupResultsAvailScript());
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveSearchTest, CheckForAFunctionDev) {
  GURL url = https_server()->GetURL(kAllowedDomainDev, "/bravesearch.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);

  auto result_first =
      EvalJsWithManualReply(contents, GetChromeFetchBackupResultsAvailScript());
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveSearchTest, CheckForAnUndefinedFunction) {
  GURL url = https_server()->GetURL(kNotAllowedDomain, "/bravesearch.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);

  auto result_first =
      EvalJsWithManualReply(contents, GetChromeFetchBackupResultsAvailScript());
  EXPECT_EQ(base::Value(false), result_first.value);
}
