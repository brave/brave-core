/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/greaselion/greaselion_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/greaselion/browser/greaselion_download_service.h"
#include "brave/components/greaselion/browser/greaselion_service.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;
using extensions::ExtensionBrowserTest;
using greaselion::GreaselionDownloadService;
using greaselion::GreaselionService;
using greaselion::GreaselionServiceFactory;

const char kLocalDataFilesComponentTestId[] =
    "eclbkhjphkhalklhipiicaldjbnhdfkc";

const char kLocalDataFilesComponentTestBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsleoSxQ3DN+6xym2P1uX"
    "mN6ArIWd9Oru5CSjS0SRE5upM2EnAl/C20TP8JdIlPi/3tk/SN6Y92K3xIhAby5F"
    "0rbPDSTXEWGy72tv2qb/WySGwDdvYQu9/J5sEDneVcMrSHcC0VWgcZR0eof4BfOy"
    "fKMEnHX98tyA3z+vW5ndHspR/Xvo78B3+6HX6tyVm/pNlCNOm8W8feyfDfPpK2Lx"
    "qRLB7PumyhR625txxolkGC6aC8rrxtT3oymdMfDYhB4BZBrzqdriyvu1NdygoEiF"
    "WhIYw/5zv1NyIsfUiG8wIs5+OwS419z7dlMKsg1FuB2aQcDyjoXx1habFfHQfQwL"
    "qwIDAQAB";

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

class GreaselionServiceTest : public ExtensionBrowserTest {
 public:
  GreaselionServiceTest() {}

  void SetUp() override {
    InitEmbeddedTestServer();
    InitService();
    ExtensionBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void PreRunTestOnMainThread() override {
    ExtensionBrowserTest::PreRunTestOnMainThread();
    ASSERT_TRUE(
        g_brave_browser_process->local_data_files_service()->IsInitialized());
  }

  void InitEmbeddedTestServer() {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void InitService() {
    brave_component_updater::LocalDataFilesService::
        SetComponentIdAndBase64PublicKeyForTest(
            kLocalDataFilesComponentTestId,
            kLocalDataFilesComponentTestBase64PublicKey);
  }

  void GetTestDataDir(base::FilePath* test_data_dir) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::PathService::Get(brave::DIR_TEST_DATA, test_data_dir);
  }

  bool InstallGreaselionExtension() {
    base::FilePath test_data_dir;
    GetTestDataDir(&test_data_dir);
    const extensions::Extension* mock_extension =
        InstallExtension(test_data_dir.AppendASCII("greaselion-data"), 1);
    if (!mock_extension)
      return false;

    greaselion::GreaselionDownloadService* download_service =
        g_brave_browser_process->greaselion_download_service();
    download_service->OnComponentReady(mock_extension->id(),
                                       mock_extension->path(), "");
    // wait for Greaselion download service to load and parse its configuration
    // file
    GreaselionDownloadServiceWaiter(download_service).Wait();
    GreaselionService* greaselion_service =
        GreaselionServiceFactory::GetForBrowserContext(profile());
    // wait for the Greaselion service to install all the extensions it creates
    GreaselionServiceWaiter(greaselion_service).Wait();
    return true;
  }

  int GetRulesSize() {
    return g_brave_browser_process->greaselion_download_service()
        ->rules()
        ->size();
  }

  void ClearRules() {
    g_brave_browser_process->greaselion_download_service()->rules()->clear();
  }

  void SetRewardsEnabled(bool enabled) {
    RewardsService* rewards_service =
        RewardsServiceFactory::GetForProfile(profile());
    rewards_service->SetRewardsMainEnabled(enabled);
    GreaselionService* greaselion_service =
        GreaselionServiceFactory::GetForBrowserContext(profile());
    // wait for the Greaselion service to install all the extensions it creates
    // after the rewards service is turned off or on
    GreaselionServiceWaiter(greaselion_service).Wait();
  }
};

// Ensure the site specific script service properly clears its cache of
// precompiled URLPatterns if initialized twice. (This can happen if
// the parent component is updated while Brave is running.)
IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, ClearCache) {
  ASSERT_TRUE(InstallGreaselionExtension());
  int size = GetRulesSize();
  // clear the cache manually to make sure we're actually
  // reinitializing it the second time
  ClearRules();
  ASSERT_TRUE(InstallGreaselionExtension());
  EXPECT_EQ(size, GetRulesSize());
  // now reinitialize without manually clearing (simulates an in-place
  // component update)
  ASSERT_TRUE(InstallGreaselionExtension());
  EXPECT_EQ(size, GetRulesSize());
}

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, ScriptInjection) {
  ASSERT_TRUE(InstallGreaselionExtension());
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

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, ScriptInjectionWithPrecondition) {
  ASSERT_TRUE(InstallGreaselionExtension());

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

  SetRewardsEnabled(true);
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
