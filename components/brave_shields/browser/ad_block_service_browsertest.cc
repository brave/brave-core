/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"

const char kAdsPage[] = "/blocking.html";
const char kNoAdsPage[] = "/no_blocking.html";

const std::string kAdBlockUpdaterTestId("naccapggpomhlhoifnlebfoocegenbol");

const std::string kAdBlockUpdaterTestBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtV7Vr69kkvSvu2lhcMDh"
    "j4Jm3FKU1zpUkALaum5719/cccVvGpMKKFyy4WYXsmAfcIONmGO4ThK/q6jkgC5v"
    "8HrkjPOf7HHebKEnsJJucz/Z1t6dq0CE+UA2IWfbGfFM4nJ8AKIv2gqiw2d4ydAs"
    "QcL26uR9IHHrBk/zzkv2jO43Aw2kY3loqRf60THz4pfz5vOtI+BKOw1KHM0+y1Di"
    "Qdk+dZ9r8NRQnpjChQzwhMAkxyrdjT1N7NcfTufiYQTOyiFvxPAC9D7vAzkpGgxU"
    "Ikylk7cYRxqkRGS/AayvfipJ/HOkoBd0yKu1MRk4YcKGd/EahDAhUtd9t4+v33Qv"
    "uwIDAQAB";

class AdBlockServiceTest : public ExtensionBrowserTest {
public:
  AdBlockServiceTest() {}

  void SetUp() override {
    InitEmbeddedTestServer();
    InitService();
    ExtensionBrowserTest::SetUp();
  }

  void PreRunTestOnMainThread() override {
    ExtensionBrowserTest::PreRunTestOnMainThread();
    WaitForAdBlockServiceThread();
    ASSERT_TRUE(g_brave_browser_process->ad_block_service()->IsInitialized());
  }

  void InitEmbeddedTestServer() {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void InitService() {
    brave_shields::AdBlockService::SetIdAndBase64PublicKeyForTest(
        kAdBlockUpdaterTestId, kAdBlockUpdaterTestBase64PublicKey);
  }

  bool InstallAdBlockExtension() {
    base::FilePath test_data_dir;
    PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    const extensions::Extension* ad_block_extension =
        InstallExtension(test_data_dir.AppendASCII("adblock-data"), 1);
    if (!ad_block_extension)
      return false;

    g_brave_browser_process->ad_block_service()->OnComponentReady(
        ad_block_extension->id(), ad_block_extension->path());

    return true;
  }

  void WaitForAdBlockServiceThread() {
    scoped_refptr<base::ThreadTestHelper> io_helper(
        new base::ThreadTestHelper(
            g_brave_browser_process->ad_block_service()->GetTaskRunner()));
    ASSERT_TRUE(io_helper->Run());
  }
};

// Load a page with an ad image, and make sure it is blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdsGetBlocked) {
  ASSERT_TRUE(InstallAdBlockExtension());

  GURL url = embedded_test_server()->GetURL(kAdsPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  bool img_loaded;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(imgLoaded())",
      &img_loaded));
  EXPECT_FALSE(img_loaded);
}

// Load a page with an image which is not an ad, and make sure it is NOT blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, NotAdsDoNotGetBlocked) {
  ASSERT_TRUE(InstallAdBlockExtension());

  GURL url = embedded_test_server()->GetURL(kNoAdsPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  bool img_loaded;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(imgLoaded())",
      &img_loaded));
  EXPECT_TRUE(img_loaded);
}
