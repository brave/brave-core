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
#include "brave/components/greaselion/browser/greaselion_download_service.h"
#include "brave/components/greaselion/browser/greaselion_service.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

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
  void OnAllScriptsLoaded(GreaselionDownloadService* download_service,
                          bool success) override {
    ASSERT_TRUE(success);
    run_loop_.QuitWhenIdle();
  }

  GreaselionDownloadService* const download_service_;
  base::RunLoop run_loop_;
  ScopedObserver<GreaselionDownloadService, GreaselionDownloadService::Observer>
      scoped_observer_;

  DISALLOW_COPY_AND_ASSIGN(GreaselionDownloadServiceWaiter);
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
    GreaselionDownloadServiceWaiter(download_service).Wait();

    return true;
  }

  bool ScriptsFor(const GURL& url, std::vector<std::string>* scripts) {
    GreaselionService* greaselion_service =
        GreaselionServiceFactory::GetForBrowserContext(profile());
    return greaselion_service->ScriptsFor(url, scripts);
  }

  int GetRulesSize() {
    return g_brave_browser_process->greaselion_download_service()
        ->rules()
        ->size();
  }

  void ClearRules() {
    g_brave_browser_process->greaselion_download_service()->rules()->clear();
  }
};

IN_PROC_BROWSER_TEST_F(GreaselionServiceTest, RuleParsing) {
  ASSERT_TRUE(InstallGreaselionExtension());
  std::vector<std::string> scripts;

  // URL should match two rules, each with a different script
  // (first rule is an exact match, second rule is a TLD match)
  ASSERT_TRUE(ScriptsFor(GURL("https://www.example.com/"), &scripts));
  ASSERT_EQ(scripts.size(), 2UL);
  EXPECT_EQ(scripts[0].find("example.com"), 7UL);
  EXPECT_EQ(scripts[1].find("example.*"), 7UL);

  // URL should match two rules, each with a different script
  // (first rule is still an exact match because 80 is the default port,
  // second rule is a TLD match)
  ASSERT_TRUE(ScriptsFor(GURL("https://www.example.com:80/"), &scripts));
  ASSERT_EQ(scripts.size(), 2UL);
  EXPECT_EQ(scripts[0].find("example.com"), 7UL);
  EXPECT_EQ(scripts[1].find("example.*"), 7UL);

  // URL should match one rule with one script (because of TLD matching)
  ASSERT_TRUE(ScriptsFor(GURL("https://www.example.org/"), &scripts));
  ASSERT_EQ(scripts.size(), 1UL);
  EXPECT_EQ(scripts[0].find("example.*"), 7UL);

  // URL should match one rule with one script (because 80 is the default port)
  ASSERT_TRUE(ScriptsFor(GURL("https://www.example.org:80/"), &scripts));
  ASSERT_EQ(scripts.size(), 1UL);
  EXPECT_EQ(scripts[0].find("example.*"), 7UL);

  // URL should match one rule with one script (because TLD matching works on
  // multi-dotted TLDs)
  ASSERT_TRUE(ScriptsFor(GURL("https://www.example.co.uk/"), &scripts));
  ASSERT_EQ(scripts.size(), 1UL);
  EXPECT_EQ(scripts[0].find("example.*"), 7UL);

  // URL should not match any rules (because of scheme)
  ASSERT_FALSE(ScriptsFor(GURL("http://www.example.com/"), &scripts));
  EXPECT_EQ(scripts.size(), 0UL);

  // URL should not match any rules (because of non-default port)
  ASSERT_FALSE(ScriptsFor(GURL("http://www.example.com:8000/"), &scripts));
  EXPECT_EQ(scripts.size(), 0UL);

  // URL should not match any rules (because wildcard at end of pattern only
  // matches TLDs, not arbitrary domains)
  ASSERT_FALSE(ScriptsFor(GURL("https://www.example.evil.com/"), &scripts));
  EXPECT_EQ(scripts.size(), 0UL);

  // URL should match one rule with one script (because of wildcard port
  // and wildcard path)
  ASSERT_TRUE(ScriptsFor(GURL("http://www.a.com:9876/simple.html"), &scripts));
  ASSERT_EQ(scripts.size(), 1UL);
  EXPECT_EQ(scripts[0].find("Altered"), 18UL);

  // URL should not match because rewards are disabled
  greaselion::GreaselionServiceFactory::GetForBrowserContext(profile())
      ->SetFeatureEnabled(greaselion::REWARDS, false);
  ASSERT_FALSE(ScriptsFor(GURL("https://pre1.example.com/"), &scripts));
  greaselion::GreaselionServiceFactory::GetForBrowserContext(profile())
      ->SetFeatureEnabled(greaselion::REWARDS, true);
  // URL should match one rule because rewards are enabled
  ASSERT_TRUE(ScriptsFor(GURL("https://pre1.example.com/"), &scripts));
  ASSERT_EQ(scripts.size(), 1UL);
}

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
