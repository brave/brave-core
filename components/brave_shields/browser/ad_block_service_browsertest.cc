/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"

const char kAdsPage[] = "/blocking.html";
const char kAdsPageV4[] = "/blocking_v4.html";
const char kAdsPageRegional[] = "/blocking_regional.html";
const char kNoAdsPage[] = "/no_blocking.html";

const std::string kAdBlockEasyListFranceUUID("9852EFC4-99E4-4F2D-A915-9C3196C7A1DE");

const std::string kDefaultAdBlockComponentTestId("naccapggpomhlhoifnlebfoocegenbol");
const std::string kDefaultAdBlockComponentTestBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtV7Vr69kkvSvu2lhcMDh"
    "j4Jm3FKU1zpUkALaum5719/cccVvGpMKKFyy4WYXsmAfcIONmGO4ThK/q6jkgC5v"
    "8HrkjPOf7HHebKEnsJJucz/Z1t6dq0CE+UA2IWfbGfFM4nJ8AKIv2gqiw2d4ydAs"
    "QcL26uR9IHHrBk/zzkv2jO43Aw2kY3loqRf60THz4pfz5vOtI+BKOw1KHM0+y1Di"
    "Qdk+dZ9r8NRQnpjChQzwhMAkxyrdjT1N7NcfTufiYQTOyiFvxPAC9D7vAzkpGgxU"
    "Ikylk7cYRxqkRGS/AayvfipJ/HOkoBd0yKu1MRk4YcKGd/EahDAhUtd9t4+v33Qv"
    "uwIDAQAB";

const std::string kRegionalAdBlockComponentTestId("dlpmaigjliompnelofkljgcmlenklieh");
const std::string kRegionalAdBlockComponentTestBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoKYkdDM8vWZXBbDJXTP6"
    "1m9yLuH9iL/TvqAqu1zOd91VJu4bpcCMZjfGPC1g+O+pZrCaFVv5NJeZxGqT6DUB"
    "RZUdXPkGGUC1ebS4LLJbggNQb152LFk8maR0/ItvMOW8eTcV8VFKHk4UrVhPTggf"
    "dU/teuAesUUJnhFchijBtAqO+nJ0wEcksY8ktrIyoNPzMj43a1OVJVXrPFDc+WT/"
    "G8XBq/Y8FbBt+u+7skWQy3lVyRwFjeFu6cXVF4tcc06PNx5yLsbHQtSv8R+h1bWw"
    "ieMF3JB9CZPr+qDKIap+RZUfsraV47QebRi/JA17nbDMlXOmK7mILfFU7Jhjx04F"
    "LwIDAQAB";

class AdBlockServiceTest : public ExtensionBrowserTest {
public:
  AdBlockServiceTest() {}

  void SetUp() override {
    InitEmbeddedTestServer();
    ExtensionBrowserTest::SetUp();
  }

  void PreRunTestOnMainThread() override {
    ExtensionBrowserTest::PreRunTestOnMainThread();
    WaitForDefaultAdBlockServiceThread();
    ASSERT_TRUE(g_brave_browser_process->ad_block_service()->IsInitialized());
  }

  void InitEmbeddedTestServer() {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void SetDefaultComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key) {
    brave_shields::AdBlockService::SetComponentIdAndBase64PublicKeyForTest(
        component_id, component_base64_public_key);
  }

  void SetRegionalComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key) {
    brave_shields::AdBlockRegionalService::
        SetComponentIdAndBase64PublicKeyForTest(component_id,
                                                component_base64_public_key);
  }

  void SetDATFileVersionForTest(const std::string& dat_file_version) {
    brave_shields::AdBlockService::SetDATFileVersionForTest(dat_file_version);
  }

  bool InstallDefaultAdBlockExtension(
      const std::string& extension_dir = "adblock-default",
      int expected_change = 1) {
    base::FilePath test_data_dir;
    PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    const extensions::Extension* ad_block_extension = InstallExtension(
        test_data_dir.AppendASCII("adblock-data").AppendASCII(extension_dir),
        expected_change);
    if (!ad_block_extension)
      return false;

    g_brave_browser_process->ad_block_service()->OnComponentReady(
        ad_block_extension->id(), ad_block_extension->path());
    WaitForDefaultAdBlockServiceThread();

    return true;
  }

  bool InstallRegionalAdBlockExtension(const std::string& uuid) {
    base::FilePath test_data_dir;
    PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    const extensions::Extension* ad_block_extension =
        InstallExtension(test_data_dir.AppendASCII("adblock-data")
                             .AppendASCII("adblock-regional")
                             .AppendASCII(uuid),
                         1);
    if (!ad_block_extension)
      return false;

    g_brave_browser_process->ad_block_regional_service()->OnComponentReady(
        ad_block_extension->id(), ad_block_extension->path());
    WaitForRegionalAdBlockServiceThread();

    return true;
  }

  bool StartAdBlockRegionalService() {
    g_brave_browser_process->ad_block_regional_service()->Start();
    if (!g_brave_browser_process->ad_block_regional_service()->IsInitialized())
      return false;
    return true;
  }

  void WaitForDefaultAdBlockServiceThread() {
    scoped_refptr<base::ThreadTestHelper> io_helper(
        new base::ThreadTestHelper(
            g_brave_browser_process->ad_block_service()->GetTaskRunner()));
    ASSERT_TRUE(io_helper->Run());
  }

  void WaitForRegionalAdBlockServiceThread() {
    scoped_refptr<base::ThreadTestHelper> io_helper(
        new base::ThreadTestHelper(
            g_brave_browser_process->ad_block_regional_service()->GetTaskRunner()));
    ASSERT_TRUE(io_helper->Run());
  }
};

// Load a page with an ad image, and make sure it is blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdsGetBlockedByDefaultBlocker) {
  SetDefaultComponentIdAndBase64PublicKeyForTest(
      kDefaultAdBlockComponentTestId,
      kDefaultAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallDefaultAdBlockExtension());

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
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, NotAdsDoNotGetBlockedByDefaultBlocker) {
  SetDefaultComponentIdAndBase64PublicKeyForTest(
      kDefaultAdBlockComponentTestId,
      kDefaultAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallDefaultAdBlockExtension());

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

// Load a page with an ad image, and make sure it is blocked by the
// regional blocker.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdsGetBlockedByRegionalBlocker) {
  g_browser_process->SetApplicationLocale("fr");
  ASSERT_EQ(g_browser_process->GetApplicationLocale(), "fr");

  ASSERT_TRUE(StartAdBlockRegionalService());

  SetRegionalComponentIdAndBase64PublicKeyForTest(
      kRegionalAdBlockComponentTestId,
      kRegionalAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallRegionalAdBlockExtension(kAdBlockEasyListFranceUUID));

  GURL url = embedded_test_server()->GetURL(kAdsPageRegional);
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

// Load a page with an image which is not an ad, and make sure it is
// NOT blocked by the regional blocker.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, NotAdsDoNotGetBlockedByRegionalBlocker) {
  g_browser_process->SetApplicationLocale("fr");
  ASSERT_EQ(g_browser_process->GetApplicationLocale(), "fr");

  ASSERT_TRUE(StartAdBlockRegionalService());

  SetRegionalComponentIdAndBase64PublicKeyForTest(
      kRegionalAdBlockComponentTestId,
      kRegionalAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallRegionalAdBlockExtension(kAdBlockEasyListFranceUUID));

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

// Upgrade from v3 to v4 format data file and make sure v4-specific ad
// is blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdsGetBlockedAfterDataFileVersionUpgrade) {
  SetDefaultComponentIdAndBase64PublicKeyForTest(
      kDefaultAdBlockComponentTestId,
      kDefaultAdBlockComponentTestBase64PublicKey);

  // Install AdBlock extension with a version 3 format data file and
  // expect a new install
  SetDATFileVersionForTest("3");
  ASSERT_TRUE(InstallDefaultAdBlockExtension("adblock-v3", 1));

  // Install AdBlock extension with a version 4 format data file and
  // expect an upgrade install
  SetDATFileVersionForTest("4");
  ASSERT_TRUE(InstallDefaultAdBlockExtension("adblock-v4", 0));

  GURL url = embedded_test_server()->GetURL(kAdsPageV4);
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
