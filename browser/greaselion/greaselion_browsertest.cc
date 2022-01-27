/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/browser/greaselion/greaselion_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/greaselion/browser/greaselion_download_service.h"
#include "brave/components/greaselion/browser/greaselion_service.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "ui/base/ui_base_switches.h"

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
      : download_service_(download_service) {
    scoped_observer_.Observe(download_service_);
  }
  GreaselionDownloadServiceWaiter(const GreaselionDownloadServiceWaiter&) =
      delete;
  GreaselionDownloadServiceWaiter& operator=(
      const GreaselionDownloadServiceWaiter&) = delete;
  ~GreaselionDownloadServiceWaiter() override = default;

  void Wait() { run_loop_.Run(); }

 private:
  // GreaselionDownloadService::Observer:
  void OnRulesReady(GreaselionDownloadService* download_service) override {
    run_loop_.QuitWhenIdle();
  }

  GreaselionDownloadService* const download_service_;
  base::RunLoop run_loop_;
  base::ScopedObservation<GreaselionDownloadService,
                          GreaselionDownloadService::Observer>
      scoped_observer_{this};
};

class GreaselionServiceWaiter : public GreaselionService::Observer {
 public:
  explicit GreaselionServiceWaiter(GreaselionService* greaselion_service)
      : greaselion_service_(greaselion_service) {
    scoped_observer_.Observe(greaselion_service_);
  }
  GreaselionServiceWaiter(const GreaselionServiceWaiter&) = delete;
  GreaselionServiceWaiter& operator=(const GreaselionServiceWaiter&) = delete;
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
  base::ScopedObservation<GreaselionService, GreaselionService::Observer>
      scoped_observer_{this};
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
    // Give a consistent browser version for testing.
    static const base::NoDestructor<base::Version> version("1.2.3.4");
    greaselion_service->SetBrowserVersionForTesting(*version);
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
    rewards_browsertest_util::StartProcess(rewards_service_);

    // Response mock
    rewards_service_->ForTestingSetTestResponseCallback(
        base::BindRepeating(
            &GreaselionServiceTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();
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
      base::flat_map<std::string, std::string>* headers) {
    response_->Get(
        url,
        method,
        response_status_code,
        response);
  }

  std::unique_ptr<rewards_browsertest::RewardsBrowserTestResponse> response_;
  net::test_server::EmbeddedTestServer https_server_;
  raw_ptr<brave_rewards::RewardsServiceImpl> rewards_service_ = nullptr;
};

#if !defined(OS_MAC)
class GreaselionServiceLocaleTest : public GreaselionServiceTest {
 public:
  explicit GreaselionServiceLocaleTest(const std::string& locale)
      : locale_(locale) {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(switches::kLang, locale_);
  }

 private:
  std::string locale_;
};

class GreaselionServiceLocaleTestEnglish : public GreaselionServiceLocaleTest {
 public:
  GreaselionServiceLocaleTestEnglish() : GreaselionServiceLocaleTest("en") {}
};

class GreaselionServiceLocaleTestGerman : public GreaselionServiceLocaleTest {
 public:
  GreaselionServiceLocaleTestGerman() : GreaselionServiceLocaleTest("de") {}
};

class GreaselionServiceLocaleTestFrench : public GreaselionServiceLocaleTest {
 public:
  GreaselionServiceLocaleTestFrench() : GreaselionServiceLocaleTest("fr") {}
};
#endif

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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest,
                       PRE_ScriptInjectionWithPrecondition) {
  ASSERT_TRUE(InstallMockExtension());

  GURL url = embedded_test_server()->GetURL("pre1.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  rewards_service_->SetAutoContributeEnabled(true);
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, ScriptInjectionWithPrecondition) {
  ASSERT_TRUE(InstallMockExtension());

  GURL url = embedded_test_server()->GetURL("pre1.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  // should be altered because rewards precondition matched, so the relevant
  // Greaselion rule is active
  EXPECT_EQ(title, "Altered");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, IsGreaselionExtension) {
  ASSERT_TRUE(InstallMockExtension());

  GreaselionService* greaselion_service =
      GreaselionServiceFactory::GetForBrowserContext(profile());
  ASSERT_TRUE(greaselion_service);

  auto extension_ids = greaselion_service->GetExtensionIdsForTesting();
  ASSERT_GT(extension_ids.size(), 0UL);

  EXPECT_TRUE(greaselion_service->IsGreaselionExtension(extension_ids[0]));
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, IsNotGreaselionExtension) {
  ASSERT_TRUE(InstallMockExtension());

  GreaselionService* greaselion_service =
      GreaselionServiceFactory::GetForBrowserContext(profile());
  ASSERT_TRUE(greaselion_service);

  EXPECT_FALSE(greaselion_service->IsGreaselionExtension("INVALID"));
}


IN_PROC_BROWSER_TEST_F(GreaselionServiceTest,
                      ScriptInjectionWithBrowserVersionConditionLowWild) {
  ASSERT_TRUE(InstallMockExtension());

  GURL url = embedded_test_server()->GetURL(
      "version-low-wild.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  // Should be altered because version is lower than current.
  EXPECT_EQ(title, "Altered");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest,
                      ScriptInjectionWithBrowserVersionConditionLowFormat) {
  ASSERT_TRUE(InstallMockExtension());

  GURL url = embedded_test_server()->GetURL(
      "version-low-format.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  // Should be altered because version is lower than current, even though it
  // omits last component.
  EXPECT_EQ(title, "Altered");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest,
                      ScriptInjectionWithBrowserVersionConditionMatchWild) {
  ASSERT_TRUE(InstallMockExtension());

  GURL url = embedded_test_server()->GetURL(
      "version-match-wild.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  // Should be altered because version is wild match.
  EXPECT_EQ(title, "Altered");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest,
                      ScriptInjectionWithBrowserVersionConditionMatchExact) {
  ASSERT_TRUE(InstallMockExtension());

  GURL url = embedded_test_server()->GetURL(
      "version-match-exact.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  // Should be altered because version is exact match.
  EXPECT_EQ(title, "Altered");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest,
                      ScriptInjectionWithBrowserVersionConditionHighWild) {
  ASSERT_TRUE(InstallMockExtension());

  GURL url = embedded_test_server()->GetURL(
      "version-high-wild.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  // Should be unaltered because version is too high.
  EXPECT_EQ(title, "OK");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest,
                      ScriptInjectionWithBrowserVersionConditionHighExact) {
  ASSERT_TRUE(InstallMockExtension());

  GURL url = embedded_test_server()->GetURL(
      "version-high-exact.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  // Should be unaltered because version is too high.
  EXPECT_EQ(title, "OK");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest,
                      ScriptInjectionWithBrowserVersionConditionEmpty) {
  ASSERT_TRUE(InstallMockExtension());

  GURL url = embedded_test_server()->GetURL(
      "version-empty.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  // Should be altered because version is not good format.
  EXPECT_EQ(title, "Altered");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest,
                      ScriptInjectionWithBrowserVersionConditionBadFormat) {
  ASSERT_TRUE(InstallMockExtension());

  GURL url = embedded_test_server()->GetURL(
      "version-bad-format.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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
  // Should be altered because version is not good format.
  EXPECT_EQ(title, "Altered");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, CleanShutdown) {
  ASSERT_TRUE(InstallMockExtension());

  GURL url = embedded_test_server()->GetURL("www.a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, contents->GetURL());
  std::string title;
  ASSERT_TRUE(
      ExecuteScriptAndExtractString(contents,
                                    "window.domAutomationController.send("
                                    "document.title)",
                                    &title));
  EXPECT_EQ(title, "Altered");

  CloseAllBrowsers();
  ui_test_utils::WaitForBrowserToClose(browser());
}

#if !defined(OS_MAC)
IN_PROC_BROWSER_TEST_F(GreaselionServiceLocaleTestEnglish,
                       ScriptInjectionWithMessagesDefaultLocale) {
  ASSERT_TRUE(InstallMockExtension());

  const GURL url =
      embedded_test_server()->GetURL("messages.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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

  // Ensure that English localization is correct
  EXPECT_EQ(title, "Hello, world!");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceLocaleTestGerman,
                       ScriptInjectionWithMessagesNonDefaultLocale) {
  ASSERT_TRUE(InstallMockExtension());

  const GURL url =
      embedded_test_server()->GetURL("messages.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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

  // Ensure that German localization is correct
  EXPECT_EQ(title, "Hallo, Welt!");
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceLocaleTestFrench,
                       ScriptInjectionWithMessagesUnsupportedLocale) {
  ASSERT_TRUE(InstallMockExtension());

  const GURL url =
      embedded_test_server()->GetURL("messages.example.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
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

  // We don't have a French localization, so ensure that the default
  // (English) localization is shown instead
  EXPECT_EQ(title, "Hello, world!");
}
#endif
