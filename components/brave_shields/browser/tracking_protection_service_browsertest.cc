/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/net/url_request_mock_util.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using extensions::ExtensionBrowserTest;

const char kTrackingPage[] = "/tracking.html";

const char kTrackingScript[] =
    "const url = '%s';"
    "const img = document.createElement('img');"
    "img.src = url;"
    "img.onload = function() { window.domAutomationController.send(img.complete &&"
    "    (img.naturalHeight !== 1 || img.naturalWidth !== 1)); };"
    "img.onerror = function() { window.domAutomationController.send(false); };"
    "document.body.appendChild(img);";

const std::string kTrackingProtectionComponentTestId(
    "eclbkhjphkhalklhipiicaldjbnhdfkc");

const std::string kTrackingProtectionComponentTestBase64PublicKey =
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

  void SetUp() override {
    InitEmbeddedTestServer();
    ExtensionBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();
    base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::IO},
        base::BindOnce(&chrome_browser_net::SetUrlRequestMocksEnabled, true));
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void PreRunTestOnMainThread() override {
    ExtensionBrowserTest::PreRunTestOnMainThread();
    WaitForTrackingProtectionServiceThread();
    ASSERT_TRUE(g_brave_browser_process->tracking_protection_service()->IsInitialized());
  }

  void InitEmbeddedTestServer() {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void InitService() {
    brave_shields::TrackingProtectionService::
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
        InstallExtension(test_data_dir.AppendASCII("tracking-protection-data"), 1);
    if (!tracking_protection_extension)
      return false;

    g_brave_browser_process->tracking_protection_service()->OnComponentReady(
        tracking_protection_extension->id(), tracking_protection_extension->path(), "");
    WaitForTrackingProtectionServiceThread();

    return true;
  }

  void WaitForTrackingProtectionServiceThread() {
    scoped_refptr<base::ThreadTestHelper> io_helper(
        new base::ThreadTestHelper(
            g_brave_browser_process->tracking_protection_service()->GetTaskRunner()));
    ASSERT_TRUE(io_helper->Run());
  }
};

// Load a page that references a tracker from a trusted domain, and
// make sure it is not blocked.
IN_PROC_BROWSER_TEST_F(TrackingProtectionServiceTest, TrackerReferencedFromTrustedDomainNotBlocked) {
  ASSERT_TRUE(InstallTrackingProtectionExtension());

  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kTrackersBlocked), 0ULL);

  GURL url = embedded_test_server()->GetURL("365media.com", kTrackingPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  GURL test_url = embedded_test_server()->GetURL("365dm.com", "/logo.png");

  bool img_loaded;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      base::StringPrintf(kTrackingScript, test_url.spec().c_str()),
      &img_loaded));
  EXPECT_TRUE(img_loaded);

  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kTrackersBlocked), 0ULL);
}

// Load a page that references a tracker from an untrusted domain, and
// make sure it is blocked.
IN_PROC_BROWSER_TEST_F(TrackingProtectionServiceTest, TrackerReferencedFromUntrustedDomainGetsBlocked) {
  ASSERT_TRUE(InstallTrackingProtectionExtension());
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kTrackersBlocked), 0ULL);

  GURL url = embedded_test_server()->GetURL("google.com", kTrackingPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  GURL test_url = embedded_test_server()->GetURL("365dm.com", "/logo.png");

  bool img_loaded;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      base::StringPrintf(kTrackingScript, test_url.spec().c_str()),
      &img_loaded));
  EXPECT_FALSE(img_loaded);

  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kTrackersBlocked), 1ULL);
}
