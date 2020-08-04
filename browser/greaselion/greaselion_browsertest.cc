/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/browser/greaselion/greaselion_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/greaselion/browser/greaselion_download_service.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/greaselion/browser/greaselion_service.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;
using extensions::ExtensionBrowserTest;
using greaselion::GreaselionDownloadService;
using greaselion::GreaselionService;
using greaselion::GreaselionServiceFactory;

const char kTestDataDirectory[] = "greaselion-data";
const char kEmbeddedTestServerDirectory[] = "greaselion";

class GreaselionDownloadServiceWaiter
    : public GreaselionDownloadService::Observer {
 public:
  explicit GreaselionDownloadServiceWaiter(
      GreaselionDownloadService* download_service)
      : download_service_(download_service), scoped_observer_(this) {
    scoped_observer_.Add(download_service_);
  }
  ~GreaselionDownloadServiceWaiter() override = default;

  void Wait() { run_loop_.Run(); }

 private:
  // GreaselionDownloadService::Observer:
  void OnRulesReady(GreaselionDownloadService* download_service) override {
    run_loop_.QuitWhenIdle();
  }

  GreaselionDownloadService* const download_service_;
  base::RunLoop run_loop_;
  ScopedObserver<GreaselionDownloadService, GreaselionDownloadService::Observer>
      scoped_observer_;

  DISALLOW_COPY_AND_ASSIGN(GreaselionDownloadServiceWaiter);
};

class GreaselionServiceWaiter : public GreaselionService::Observer {
 public:
  explicit GreaselionServiceWaiter(GreaselionService* greaselion_service)
      : greaselion_service_(greaselion_service), scoped_observer_(this) {
    scoped_observer_.Add(greaselion_service_);
  }
  ~GreaselionServiceWaiter() override = default;

  void Wait() {
    if (!greaselion_service_->ready())
      run_loop_.Run();
  }

 private:
  // GreaselionService::Observer:
  void OnExtensionsReady(GreaselionService* greaselion_service,
                         bool success) override {
    ASSERT_TRUE(success);
    run_loop_.QuitWhenIdle();
  }

  GreaselionService* const greaselion_service_;
  base::RunLoop run_loop_;
  ScopedObserver<GreaselionService, GreaselionService::Observer>
      scoped_observer_;

  DISALLOW_COPY_AND_ASSIGN(GreaselionServiceWaiter);
};

class GreaselionServiceTest : public BaseLocalDataFilesBrowserTest {
 public:
  GreaselionServiceTest(): https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    response_ =
        std::make_unique<rewards_browsertest::RewardsBrowserTestResponse>();
  }

  void SetUpOnMainThread() override {
    BaseLocalDataFilesBrowserTest::SetUpOnMainThread();
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
  }

  // BaseLocalDataFilesBrowserTest overrides
  const char* test_data_directory() override { return kTestDataDirectory; }
  const char* embedded_test_server_directory() override {
    return kEmbeddedTestServerDirectory;
  }
  LocalDataFilesObserver* service() override {
    return g_brave_browser_process->greaselion_download_service();
  }

  void WaitForService() override {
    // wait for Greaselion download service to load and parse its
    // configuration file
    greaselion::GreaselionDownloadService* download_service =
        g_brave_browser_process->greaselion_download_service();
    GreaselionDownloadServiceWaiter(download_service).Wait();
    GreaselionService* greaselion_service =
        GreaselionServiceFactory::GetForBrowserContext(profile());
    // wait for the Greaselion service to install all the extensions it creates
    GreaselionServiceWaiter(greaselion_service).Wait();
  }

  int GetRulesSize() {
    return g_brave_browser_process->greaselion_download_service()
        ->rules()
        ->size();
  }

  void ClearRules() {
    g_brave_browser_process->greaselion_download_service()->rules()->clear();
  }

  void StartRewards() {
    // HTTP resolver
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_.RegisterRequestHandler(
        base::BindRepeating(&rewards_browsertest_util::HandleRequest));
    ASSERT_TRUE(https_server_.Start());

    // Rewards service
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(profile()));

    // Response mock
    rewards_service_->ForTestingSetTestResponseCallback(
        base::BindRepeating(
            &GreaselionServiceTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();

    rewards_browsertest_util::EnableRewardsViaCode(browser(), rewards_service_);
    GreaselionService* greaselion_service =
        GreaselionServiceFactory::GetForBrowserContext(profile());
    // wait for the Greaselion service to install all the extensions it creates
    // after the rewards service is turned off or on
    GreaselionServiceWaiter(greaselion_service).Wait();
  }

  void GetTestResponse(
      const std::string& url,
      int32_t method,
      int* response_status_code,
      std::string* response,
      std::map<std::string, std::string>* headers) {
    response_->Get(
        url,
        method,
        response_status_code,
        response);
  }

  std::unique_ptr<rewards_browsertest::RewardsBrowserTestResponse> response_;
  net::test_server::EmbeddedTestServer https_server_;
  brave_rewards::RewardsServiceImpl* rewards_service_;
};

// Ensure the site specific script service properly clears its cache of
// precompiled URLPatterns if initialized twice. (This can happen if
// the parent component is updated while Brave is running.)
IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, ClearCache) {
  ASSERT_TRUE(InstallMockExtension());
  int size = GetRulesSize();
  // clear the cache manually to make sure we're actually
  // reinitializing it the second time
  ClearRules();
  ASSERT_TRUE(InstallMockExtension());
  EXPECT_EQ(size, GetRulesSize());
  // now reinitialize without manually clearing (simulates an in-place
  // component update)
  ASSERT_TRUE(InstallMockExtension());
  EXPECT_EQ(size, GetRulesSize());
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, ScriptInjection) {
  ASSERT_TRUE(InstallMockExtension());
  GURL url = embedded_test_server()->GetURL("www.a.com", "/simple.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());
  std::string title;
  ASSERT_TRUE(
      ExecuteScriptAndExtractString(contents,
                                    "window.domAutomationController.send("
                                    "document.title)",
                                    &title));
  EXPECT_EQ(title, "Altered");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, ScriptInjectionDocumentStart) {
  ASSERT_TRUE(InstallMockExtension());
  GURL url = embedded_test_server()->GetURL("runat1.b.com", "/intercept.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());
  std::string title;
  ASSERT_TRUE(
      ExecuteScriptAndExtractString(contents,
                                    "window.domAutomationController.send("
                                    "document.title)",
                                    &title));
  EXPECT_EQ(title, "SCRIPT_FIRST");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, ScriptInjectionDocumentEnd) {
  ASSERT_TRUE(InstallMockExtension());
  GURL url = embedded_test_server()->GetURL("runat2.b.com", "/intercept.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());
  std::string title;
  ASSERT_TRUE(
      ExecuteScriptAndExtractString(contents,
                                    "window.domAutomationController.send("
                                    "document.title)",
                                    &title));
  EXPECT_EQ(title, "PAGE_FIRST");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, ScriptInjectionRunAtDefault) {
  ASSERT_TRUE(InstallMockExtension());
  GURL url = embedded_test_server()->GetURL("runat3.b.com", "/intercept.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());
  std::string title;
  ASSERT_TRUE(
      ExecuteScriptAndExtractString(contents,
                                    "window.domAutomationController.send("
                                    "document.title)",
                                    &title));
  EXPECT_EQ(title, "PAGE_FIRST");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, ScriptInjectionWithPrecondition) {
  ASSERT_TRUE(InstallMockExtension());

  GURL url = embedded_test_server()->GetURL("pre1.example.com", "/simple.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());
  std::string title;
  ASSERT_TRUE(
      ExecuteScriptAndExtractString(contents,
                                    "window.domAutomationController.send("
                                    "document.title)",
                                    &title));
  // should be unaltered because precondition did not match, so no Greaselion
  // rules are active
  EXPECT_EQ(title, "OK");

  StartRewards();
  ui_test_utils::NavigateToURL(browser(), url);
  contents = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());
  ASSERT_TRUE(
      ExecuteScriptAndExtractString(contents,
                                    "window.domAutomationController.send("
                                    "document.title)",
                                    &title));
  // should be altered because rewards precondition matched, so the relevant
  // Greaselion rule is active
  EXPECT_EQ(title, "Altered");
}
