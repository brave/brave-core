/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/thread_test_helper.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_search/browser/brave_search_fallback_host.h"
#include "brave/components/brave_search/common/features.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/search_engines/template_url_service.h"
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
const char kScriptDefaultAPIExists[] =
    "window.domAutomationController.send("
    "  !!(window.brave && window.brave.getCanSetDefaultSearchProvider)"
    ")";
const char kScriptDefaultAPIGetValue[] =
    // Use setTimeout to allow opensearch xml to be fetched
    // and template url created.
    // If this is flakey, consider making TemplateURL manually,
    // or observing the TemplateURLService for changes.
    "setTimeout(function () {"
    "  brave.getCanSetDefaultSearchProvider()"
    "  .then(function (canSet) {"
    "    window.domAutomationController.send(canSet)"
    "  })"
    "}, 1200)";

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
  BraveSearchTest() = default;

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
    brave_search::BraveSearchFallbackHost::SetBackupProviderForTest(url);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    GURL url = request.GetURL();
    auto path = url.path_piece();

    if (path == "/" || path == "/sw.js" || path == "/bravesearch.html" ||
        path == "/search_provider_opensearch.xml")
      return nullptr;

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

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

class BraveSearchTestDisabled : public BraveSearchTest {
 public:
  BraveSearchTestDisabled() {
    feature_list_.InitAndDisableFeature(
        brave_search::features::kBraveSearchDefaultAPIFeature);
  }
};

class BraveSearchTestEnabled : public BraveSearchTest {
 public:
  BraveSearchTestEnabled() {
    feature_list_.InitAndEnableFeatureWithParameters(
        brave_search::features::kBraveSearchDefaultAPIFeature,
        {{brave_search::features::kBraveSearchDefaultAPIDailyLimitName, "3"},
         {brave_search::features::kBraveSearchDefaultAPITotalLimitName, "10"}});
  }
};

IN_PROC_BROWSER_TEST_F(BraveSearchTest, CheckForAFunction) {
  GURL url = https_server()->GetURL(kAllowedDomain, "/bravesearch.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);

  auto result_first = EvalJs(contents, GetChromeFetchBackupResultsAvailScript(),
                             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveSearchTest, CheckForAFunctionDev) {
  GURL url = https_server()->GetURL(kAllowedDomainDev, "/bravesearch.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);

  auto result_first = EvalJs(contents, GetChromeFetchBackupResultsAvailScript(),
                             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveSearchTest, CheckForAnUndefinedFunction) {
  GURL url = https_server()->GetURL(kNotAllowedDomain, "/bravesearch.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);

  auto result_first = EvalJs(contents, GetChromeFetchBackupResultsAvailScript(),
                             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(false), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveSearchTestEnabled, DefaultAPIVisibleKnownHost) {
  // Opensearch providers are only allowed in the root of a site,
  // See SearchEngineTabHelper::GenerateKeywordFromNavigationEntry.
  GURL url = https_server()->GetURL(kAllowedDomain, "/");
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(browser()->profile()));
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
  EXPECT_EQ(url, contents->GetURL());
  bool has_api;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents, kScriptDefaultAPIExists, &has_api));
  EXPECT_TRUE(has_api);
  bool can_set;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents, kScriptDefaultAPIGetValue,
                                          &can_set));
  EXPECT_TRUE(can_set);
}

IN_PROC_BROWSER_TEST_F(BraveSearchTestEnabled, DefaultAPIHiddenUnknownHost) {
  // Opensearch providers are only allowed in the root of a site,
  // See SearchEngineTabHelper::GenerateKeywordFromNavigationEntry.
  GURL url = https_server()->GetURL(kNotAllowedDomain, "/");
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(browser()->profile()));
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
  EXPECT_EQ(url, contents->GetURL());
  bool has_api;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents, kScriptDefaultAPIExists, &has_api));
  EXPECT_FALSE(has_api);
}

IN_PROC_BROWSER_TEST_F(BraveSearchTestEnabled,
                       DISABLED_DefaultAPIFalseNoOpenSearch) {
  // Opensearch providers are only allowed in the root of a site,
  // See SearchEngineTabHelper::GenerateKeywordFromNavigationEntry.
  GURL url = https_server()->GetURL(kAllowedDomain, "/bravesearch.html");
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(browser()->profile()));
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
  EXPECT_EQ(url, contents->GetURL());
  bool has_api;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents, kScriptDefaultAPIExists, &has_api));
  EXPECT_TRUE(has_api);
  bool can_set;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents, kScriptDefaultAPIGetValue,
                                          &can_set));
  EXPECT_FALSE(can_set);
}

IN_PROC_BROWSER_TEST_F(BraveSearchTestEnabled, DefaultAPIFalsePrivateWindow) {
  // Opensearch providers are only allowed in the root of a site,
  // See SearchEngineTabHelper::GenerateKeywordFromNavigationEntry.
  GURL url = https_server()->GetURL(kAllowedDomain, "/");
  Browser* private_browser = CreateIncognitoBrowser(nullptr);
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(private_browser->profile()));
  ui_test_utils::NavigateToURL(private_browser, url);
  content::WebContents* contents =
      private_browser->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
  EXPECT_EQ(url, contents->GetURL());
  bool has_api;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents, kScriptDefaultAPIExists, &has_api));
  EXPECT_TRUE(has_api);
  bool can_set;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents, kScriptDefaultAPIGetValue,
                                          &can_set));
  EXPECT_FALSE(can_set);
}

IN_PROC_BROWSER_TEST_F(BraveSearchTestDisabled, DefaultAPIInvisibleKnownHost) {
  // Opensearch providers are only allowed in the root of a site,
  // See SearchEngineTabHelper::GenerateKeywordFromNavigationEntry.
  GURL url = https_server()->GetURL(kAllowedDomain, "/");
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(browser()->profile()));
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
  EXPECT_EQ(url, contents->GetURL());
  bool has_api;
  EXPECT_TRUE(
      ExecuteScriptAndExtractBool(contents, kScriptDefaultAPIExists, &has_api));
  EXPECT_FALSE(has_api);
}
