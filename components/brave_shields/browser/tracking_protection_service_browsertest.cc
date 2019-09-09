/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/buildflags/buildflags.h"  // For STP
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

#if BUILDFLAG(BRAVE_STP_ENABLED)
#include "brave/common/brave_switches.h"
#include "brave/components/brave_shields/browser/tracking_protection_helper.h"

using brave_shields::TrackingProtectionHelper;

const char kCancelledNavigation[] = "/cancelled_navigation.html";
const char kRedirectPage[] = "/client-redirect?";
const char kStoragePage[] = "/storage.html";
#endif

using content::BrowserThread;
using extensions::ExtensionBrowserTest;

const std::string kTrackingProtectionComponentTestId(
    "eclbkhjphkhalklhipiicaldjbnhdfkc");

const char kTrackingProtectionComponentTestBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsleoSxQ3DN+6xym2P1uX"
    "mN6ArIWd9Oru5CSjS0SRE5upM2EnAl/C20TP8JdIlPi/3tk/SN6Y92K3xIhAby5F"
    "0rbPDSTXEWGy72tv2qb/WySGwDdvYQu9/J5sEDneVcMrSHcC0VWgcZR0eof4BfOy"
    "fKMEnHX98tyA3z+vW5ndHspR/Xvo78B3+6HX6tyVm/pNlCNOm8W8feyfDfPpK2Lx"
    "qRLB7PumyhR625txxolkGC6aC8rrxtT3oymdMfDYhB4BZBrzqdriyvu1NdygoEiF"
    "WhIYw/5zv1NyIsfUiG8wIs5+OwS419z7dlMKsg1FuB2aQcDyjoXx1habFfHQfQwL"
    "qwIDAQAB";

class TrackingProtectionServiceTest : public ExtensionBrowserTest {
 public:
  TrackingProtectionServiceTest() {}

#if BUILDFLAG(BRAVE_STP_ENABLED)
  void SetUpDefaultCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpDefaultCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableSmartTrackingProtection);
  }
#endif

  void SetUp() override {
    InitEmbeddedTestServer();
    ExtensionBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void PreRunTestOnMainThread() override {
    ExtensionBrowserTest::PreRunTestOnMainThread();
    WaitForTrackingProtectionServiceThread();
    ASSERT_TRUE(
        g_brave_browser_process->local_data_files_service()->IsInitialized());
  }

  void InitEmbeddedTestServer() {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    content::SetupCrossSiteRedirector(embedded_test_server());
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void InitService() {
    brave_component_updater::LocalDataFilesService::
        SetComponentIdAndBase64PublicKeyForTest(
            kTrackingProtectionComponentTestId,
            kTrackingProtectionComponentTestBase64PublicKey);
  }

  void GetTestDataDir(base::FilePath* test_data_dir) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::PathService::Get(brave::DIR_TEST_DATA, test_data_dir);
  }

  bool InstallTrackingProtectionExtension() {
    base::FilePath test_data_dir;
    GetTestDataDir(&test_data_dir);
    const extensions::Extension* tracking_protection_extension =
        InstallExtension(test_data_dir.AppendASCII("tracking-protection-data"),
                         1);
    if (!tracking_protection_extension)
      return false;

    g_brave_browser_process->tracking_protection_service()->OnComponentReady(
        tracking_protection_extension->id(),
        tracking_protection_extension->path(), "");
    WaitForTrackingProtectionServiceThread();

    return true;
  }

  void WaitForTrackingProtectionServiceThread() {
    scoped_refptr<base::ThreadTestHelper> tr_helper(new base::ThreadTestHelper(
        g_brave_browser_process->local_data_files_service()->GetTaskRunner()));
    ASSERT_TRUE(tr_helper->Run());
    scoped_refptr<base::ThreadTestHelper> io_helper(new base::ThreadTestHelper(
        base::CreateSingleThreadTaskRunnerWithTraits({BrowserThread::IO})
            .get()));
    ASSERT_TRUE(io_helper->Run());
  }
};

#if BUILDFLAG(BRAVE_STP_ENABLED)
IN_PROC_BROWSER_TEST_F(TrackingProtectionServiceTest, StorageTrackingBlocked) {
  ASSERT_TRUE(TrackingProtectionHelper::IsSmartTrackingProtectionEnabled());
  ASSERT_TRUE(InstallTrackingProtectionExtension());

  std::string message;
  content::DOMMessageQueue message_queue;

  // tracker.com is in the StorageTrackingProtection list
  const GURL tracking_url =
      embedded_test_server()->GetURL("tracker.com", kStoragePage);

  const GURL url = embedded_test_server()->GetURL(
      "social.com", std::string(kRedirectPage) + tracking_url.spec());

  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(browser(), url, 2);

  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ("tracker.com", contents->GetURL().host());

  bool cookie_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents, "window.domAutomationController.send(!IsCookieAvailable())",
      &cookie_blocked));
  EXPECT_TRUE(cookie_blocked);

  bool session_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(!IsSessionStorageAvailable())",
      &session_blocked));
  EXPECT_TRUE(session_blocked);

  bool local_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(!IsLocalStorageAvailable())",
      &local_blocked));
  EXPECT_TRUE(local_blocked);

  EXPECT_TRUE(message_queue.WaitForMessage(&message));

  bool db_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents, "window.domAutomationController.send(!IsDBAvailable())",
      &db_blocked));
  EXPECT_TRUE(db_blocked);

  bool indexeddb_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents, "window.domAutomationController.send(!IsIndexedDBAvailable())",
      &indexeddb_blocked));
  EXPECT_TRUE(indexeddb_blocked);
}

IN_PROC_BROWSER_TEST_F(TrackingProtectionServiceTest, StorageTrackingAllowed) {
  ASSERT_TRUE(TrackingProtectionHelper::IsSmartTrackingProtectionEnabled());
  ASSERT_TRUE(InstallTrackingProtectionExtension());

  std::string message;
  content::DOMMessageQueue message_queue;

  // example.com is not in the StorageTrackingProtection list
  const GURL redirect_url =
      embedded_test_server()->GetURL("example.com", kStoragePage);

  const GURL url = embedded_test_server()->GetURL(
      "social.com", kRedirectPage + redirect_url.spec());

  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(browser(), url, 2);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ("example.com", contents->GetURL().host());

  bool cookie_allowed;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents, "window.domAutomationController.send(IsCookieAvailable())",
      &cookie_allowed));
  EXPECT_TRUE(cookie_allowed);

  bool session_allowed;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(IsSessionStorageAvailable())",
      &session_allowed));
  EXPECT_TRUE(session_allowed);

  bool local_allowed;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(IsLocalStorageAvailable())",
      &local_allowed));
  EXPECT_TRUE(local_allowed);

  EXPECT_TRUE(message_queue.WaitForMessage(&message));

  bool db_allowed;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents, "window.domAutomationController.send(IsDBAvailable())",
      &db_allowed));
  EXPECT_TRUE(db_allowed);

  bool indexeddb_allowed;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents, "window.domAutomationController.send(IsIndexedDBAvailable())",
      &indexeddb_allowed));
}

IN_PROC_BROWSER_TEST_F(TrackingProtectionServiceTest, CancelledNavigation) {
  ASSERT_TRUE(TrackingProtectionHelper::IsSmartTrackingProtectionEnabled());
  ASSERT_TRUE(InstallTrackingProtectionExtension());

  std::string message;
  content::DOMMessageQueue message_queue;

  // tracker.com is in the StorageTrackingProtection list
  const GURL tracking_url =
      embedded_test_server()->GetURL("tracker.com", kCancelledNavigation);

  const GURL url = embedded_test_server()->GetURL(
      "social.com", std::string(kRedirectPage) + tracking_url.spec());

  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(browser(), url, 2);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ("tracker.com", contents->GetURL().host());

  bool cookie_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(!IsIntialCookieAvailable())",
      &cookie_blocked));
  EXPECT_TRUE(cookie_blocked);

  ASSERT_TRUE(ExecuteScript(
      contents,
      base::StringPrintf(
          "window.domAutomationController.send(StartNavigation(\"%s\"))",
          tracking_url.spec().c_str())));

  // Cancel Navigation
  ASSERT_TRUE(ExecuteScript(
      contents, base::StringPrintf(
                    "window.domAutomationController.send(window.stop())")));

  // If the starting site is updated to the tracking site, then cookies
  // will be allowed
  bool cookie_allowed;
  ASSERT_TRUE(
      ExecuteScriptAndExtractBool(contents,
                                  "window.domAutomationController.send("
                                  "TryCookiesAfterCancelledNavigation())",
                                  &cookie_allowed));
  EXPECT_FALSE(cookie_allowed);
}
#endif
