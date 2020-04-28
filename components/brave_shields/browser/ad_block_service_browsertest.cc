/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/vendor/adblock_rust_ffi/src/wrapper.hpp"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/test/extension_test_message_listener.h"
#include "net/dns/mock_host_resolver.h"

using brave_shields::features::kBraveAdblockCosmeticFiltering;
using content::BrowserThread;
using extensions::ExtensionBrowserTest;

const char kAdBlockTestPage[] = "/blocking.html";

const char kAdBlockEasyListFranceUUID[] =
    "9852EFC4-99E4-4F2D-A915-9C3196C7A1DE";

const char kDefaultAdBlockComponentTestId[] =
    "naccapggpomhlhoifnlebfoocegenbol";
const char kRegionalAdBlockComponentTestId[] =
    "dlpmaigjliompnelofkljgcmlenklieh";
const char kTrackingProtectionComponentTestId[] =
    "eclbkhjphkhalklhipiicaldjbnhdfkc";

const char kDefaultAdBlockComponentTestBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtV7Vr69kkvSvu2lhcMDh"
    "j4Jm3FKU1zpUkALaum5719/cccVvGpMKKFyy4WYXsmAfcIONmGO4ThK/q6jkgC5v"
    "8HrkjPOf7HHebKEnsJJucz/Z1t6dq0CE+UA2IWfbGfFM4nJ8AKIv2gqiw2d4ydAs"
    "QcL26uR9IHHrBk/zzkv2jO43Aw2kY3loqRf60THz4pfz5vOtI+BKOw1KHM0+y1Di"
    "Qdk+dZ9r8NRQnpjChQzwhMAkxyrdjT1N7NcfTufiYQTOyiFvxPAC9D7vAzkpGgxU"
    "Ikylk7cYRxqkRGS/AayvfipJ/HOkoBd0yKu1MRk4YcKGd/EahDAhUtd9t4+v33Qv"
    "uwIDAQAB";
const char kRegionalAdBlockComponentTestBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoKYkdDM8vWZXBbDJXTP6"
    "1m9yLuH9iL/TvqAqu1zOd91VJu4bpcCMZjfGPC1g+O+pZrCaFVv5NJeZxGqT6DUB"
    "RZUdXPkGGUC1ebS4LLJbggNQb152LFk8maR0/ItvMOW8eTcV8VFKHk4UrVhPTggf"
    "dU/teuAesUUJnhFchijBtAqO+nJ0wEcksY8ktrIyoNPzMj43a1OVJVXrPFDc+WT/"
    "G8XBq/Y8FbBt+u+7skWQy3lVyRwFjeFu6cXVF4tcc06PNx5yLsbHQtSv8R+h1bWw"
    "ieMF3JB9CZPr+qDKIap+RZUfsraV47QebRi/JA17nbDMlXOmK7mILfFU7Jhjx04F"
    "LwIDAQAB";
const char kTrackingProtectionComponentTestBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsleoSxQ3DN+6xym2P1uX"
    "mN6ArIWd9Oru5CSjS0SRE5upM2EnAl/C20TP8JdIlPi/3tk/SN6Y92K3xIhAby5F"
    "0rbPDSTXEWGy72tv2qb/WySGwDdvYQu9/J5sEDneVcMrSHcC0VWgcZR0eof4BfOy"
    "fKMEnHX98tyA3z+vW5ndHspR/Xvo78B3+6HX6tyVm/pNlCNOm8W8feyfDfPpK2Lx"
    "qRLB7PumyhR625txxolkGC6aC8rrxtT3oymdMfDYhB4BZBrzqdriyvu1NdygoEiF"
    "WhIYw/5zv1NyIsfUiG8wIs5+OwS419z7dlMKsg1FuB2aQcDyjoXx1habFfHQfQwL"
    "qwIDAQAB";

class AdBlockServiceTest : public ExtensionBrowserTest {
 public:
  AdBlockServiceTest() {}

  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void SetUp() override {
    InitEmbeddedTestServer();
    ExtensionBrowserTest::SetUp();
  }

  void PreRunTestOnMainThread() override {
    ExtensionBrowserTest::PreRunTestOnMainThread();
    WaitForAdBlockServiceThreads();
    ASSERT_TRUE(g_brave_browser_process->ad_block_service()->IsInitialized());
  }

  void UpdateAdBlockInstanceWithRules(const std::string& rules,
                                      const std::string& resources = "") {
    g_brave_browser_process->ad_block_service()->ResetForTest(rules, resources);
  }

  void AssertTagExists(const std::string& tag, bool expected_exists) const {
    bool exists_default =
        g_brave_browser_process->ad_block_service()->TagExists(tag);
    ASSERT_EQ(exists_default, expected_exists);

    for (const auto& regional_service :
         g_brave_browser_process->ad_block_regional_service_manager()
             ->regional_services_) {
      bool exists_regional = regional_service.second->TagExists(tag);
      ASSERT_EQ(exists_regional, expected_exists);
    }
  }

  void InitEmbeddedTestServer() {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    content::SetupCrossSiteRedirector(embedded_test_server());
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void GetTestDataDir(base::FilePath* test_data_dir) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::PathService::Get(brave::DIR_TEST_DATA, test_data_dir);
  }

  void SetDefaultComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key) {
    brave_shields::AdBlockService::SetComponentIdAndBase64PublicKeyForTest(
        component_id, component_base64_public_key);
  }

  void InitTrackingProtectionService() {
    brave_component_updater::LocalDataFilesService::
        SetComponentIdAndBase64PublicKeyForTest(
            kTrackingProtectionComponentTestId,
            kTrackingProtectionComponentTestBase64PublicKey);
  }

  void SetRegionalComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key) {
    brave_shields::AdBlockRegionalService::
        SetComponentIdAndBase64PublicKeyForTest(component_id,
                                                component_base64_public_key);
  }

  bool InstallDefaultAdBlockExtension(
      const std::string& extension_dir = "adblock-default",
      int expected_change = 1) {
    base::FilePath test_data_dir;
    GetTestDataDir(&test_data_dir);
    const extensions::Extension* ad_block_extension = InstallExtension(
        test_data_dir.AppendASCII("adblock-data").AppendASCII(extension_dir),
        expected_change);
    if (!ad_block_extension)
      return false;

    g_brave_browser_process->ad_block_service()->OnComponentReady(
        ad_block_extension->id(), ad_block_extension->path(), "");
    WaitForAdBlockServiceThreads();

    return true;
  }

  bool InstallRegionalAdBlockExtension(const std::string& uuid) {
    base::FilePath test_data_dir;
    GetTestDataDir(&test_data_dir);
    const extensions::Extension* ad_block_extension =
        InstallExtension(test_data_dir.AppendASCII("adblock-data")
                             .AppendASCII("adblock-regional")
                             .AppendASCII(uuid),
                         1);
    if (!ad_block_extension)
      return false;

    g_brave_browser_process->ad_block_regional_service_manager()
        ->EnableFilterList(uuid, true);
    EXPECT_EQ(g_brave_browser_process->ad_block_regional_service_manager()
                  ->regional_services_.size(),
              1ULL);

    auto regional_service =
        g_brave_browser_process->ad_block_regional_service_manager()
            ->regional_services_.find(uuid);
    regional_service->second->OnComponentReady(ad_block_extension->id(),
                                               ad_block_extension->path(), "");
    WaitForAdBlockServiceThreads();

    return true;
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
    WaitForAdBlockServiceThreads();

    return true;
  }

  bool StartAdBlockRegionalServices() {
    g_brave_browser_process->ad_block_regional_service_manager()->Start();
    if (!g_brave_browser_process->ad_block_regional_service_manager()
             ->IsInitialized())
      return false;
    return true;
  }

  void WaitForAdBlockServiceThreads() {
    scoped_refptr<base::ThreadTestHelper> tr_helper(new base::ThreadTestHelper(
        g_brave_browser_process->local_data_files_service()->GetTaskRunner()));
    ASSERT_TRUE(tr_helper->Run());
    scoped_refptr<base::ThreadTestHelper> io_helper(new base::ThreadTestHelper(
        base::CreateSingleThreadTaskRunner({BrowserThread::IO}).get()));
    ASSERT_TRUE(io_helper->Run());
  }

  void WaitForBraveExtensionShieldsDataReady() {
    // Sometimes, the page can start loading before the Shields panel has
    // received information about the window and tab it's loaded in.
    ExtensionTestMessageListener extension_listener(
        "brave-extension-shields-data-ready",
        false);
    ASSERT_TRUE(extension_listener.WaitUntilSatisfied());
  }
};

// Load a page with an ad image, and make sure it is blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdsGetBlockedByDefaultBlocker) {
  SetDefaultComponentIdAndBase64PublicKeyForTest(
      kDefaultAdBlockComponentTestId,
      kDefaultAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(0, 1, 0, 0, 0, 0);"
                                          "addImage('ad_banner.png')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Load a page with an image which is not an ad, and make sure it is NOT
// blocked by custom filters.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       NotAdsDoNotGetBlockedByCustomBlocker) {
  ASSERT_TRUE(g_brave_browser_process->ad_block_custom_filters_service()
                  ->UpdateCustomFilters("*ad_banner.png"));

  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(1, 0, 0, 0, 0, 0);"
                                          "addImage('logo.png')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Load a page with an ad image, and make sure it is blocked by custom
// filters.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdsGetBlockedByCustomBlocker) {
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
  ASSERT_TRUE(g_brave_browser_process->ad_block_custom_filters_service()
                  ->UpdateCustomFilters("*ad_banner.png"));

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(0, 1, 0, 0, 0, 0);"
                                          "addImage('ad_banner.png')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Load a page with an image which is not an ad, and make sure it is NOT
// blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       NotAdsDoNotGetBlockedByDefaultBlocker) {
  SetDefaultComponentIdAndBase64PublicKeyForTest(
      kDefaultAdBlockComponentTestId,
      kDefaultAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(1, 0, 0, 0, 0, 0);"
                                          "addImage('logo.png')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Load a page with an ad image, and make sure it is blocked by the
// regional blocker.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdsGetBlockedByRegionalBlocker) {
  g_browser_process->SetApplicationLocale("fr");
  ASSERT_STREQ(g_browser_process->GetApplicationLocale().c_str(), "fr");

  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  SetRegionalComponentIdAndBase64PublicKeyForTest(
      kRegionalAdBlockComponentTestId,
      kRegionalAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallRegionalAdBlockExtension(kAdBlockEasyListFranceUUID));
  ASSERT_TRUE(StartAdBlockRegionalServices());

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(0, 1, 0, 0, 0, 0);"
                                          "addImage('ad_fr.png')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Load a page with an image which is not an ad, and make sure it is
// NOT blocked by the regional blocker.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       NotAdsDoNotGetBlockedByRegionalBlocker) {
  g_browser_process->SetApplicationLocale("fr");
  ASSERT_STREQ(g_browser_process->GetApplicationLocale().c_str(), "fr");

  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  SetRegionalComponentIdAndBase64PublicKeyForTest(
      kRegionalAdBlockComponentTestId,
      kRegionalAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallRegionalAdBlockExtension(kAdBlockEasyListFranceUUID));
  ASSERT_TRUE(StartAdBlockRegionalServices());

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(1, 0, 0, 0, 0, 0);"
                                          "addImage('logo.png')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Upgrade from v3 to v4 format data file and make sure v4-specific ad
// is blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       AdsGetBlockedAfterDataFileVersionUpgrade) {
  SetDefaultComponentIdAndBase64PublicKeyForTest(
      kDefaultAdBlockComponentTestId,
      kDefaultAdBlockComponentTestBase64PublicKey);

  // Install AdBlock extension with a version 3 format data file and
  // expect a new install
  ASSERT_TRUE(InstallDefaultAdBlockExtension("adblock-v3", 1));

  // Install AdBlock extension with a version 4 format data file and
  // expect an upgrade install
  ASSERT_TRUE(InstallDefaultAdBlockExtension("adblock-v4", 0));

  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(0, 1, 0, 0, 0, 0);"
                                          "addImage('v4_specific_banner.png')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Load a page with several of the same adblocked xhr requests, it should only
// count 1.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, TwoSameAdsGetCountedAsOne) {
  SetDefaultComponentIdAndBase64PublicKeyForTest(
      kDefaultAdBlockComponentTestId,
      kDefaultAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(0, 0, 0, 1, 2, 0);"
                                          "xhr('adbanner.js');"
                                          "xhr('normal.js');"
                                          "xhr('adbanner.js')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Load a page with different adblocked xhr requests, it should count each.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, TwoDiffAdsGetCountedAsTwo) {
  SetDefaultComponentIdAndBase64PublicKeyForTest(
      kDefaultAdBlockComponentTestId,
      kDefaultAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(0, 0, 0, 1, 2, 0);"
                                          "xhr('adbanner.js?1');"
                                          "xhr('normal.js');"
                                          "xhr('adbanner.js?2')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);
}

// New tab continues to count blocking the same resource
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, NewTabContinuesToBlock) {
  SetDefaultComponentIdAndBase64PublicKeyForTest(
      kDefaultAdBlockComponentTestId,
      kDefaultAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(0, 0, 0, 0, 1, 0);"
                                          "xhr('adbanner.js');",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  ui_test_utils::NavigateToURL(browser(), url);
  contents = browser()->tab_strip_model()->GetActiveWebContents();

  as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(0, 0, 0, 0, 1, 0);"
                                          "xhr('adbanner.js');",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);

  ui_test_utils::NavigateToURL(browser(), url);
}

// XHRs and ads in a cross-site iframe are blocked as well.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, SubFrame) {
  SetDefaultComponentIdAndBase64PublicKeyForTest(
      kDefaultAdBlockComponentTestId,
      kDefaultAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallDefaultAdBlockExtension());
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  GURL url = embedded_test_server()->GetURL("a.com", "/iframe_blocking.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents->GetAllFrames()[1],
                                          "setExpectations(0, 0, 0, 0, 1, 0);"
                                          "xhr('adbanner.js?1');",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  // Check also an explicit request for a script since it is a common real-world
  // scenario.
  ASSERT_TRUE(ExecuteScript(contents->GetAllFrames()[1],
                            "var s = document.createElement('script');"
                            "s.setAttribute('src', 'adbanner.js?2');"
                            "document.head.appendChild(s);"));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);
}

// Load a page with an ad image which is matched on the regional blocker,
// but make sure it is saved by the default ad_block_client's exception.
// This test is the same as AdsGetBlockedByRegionalBlocker except for at
// the start it adds an exception rule to the non regional adblocker.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       ExceptionAdsAreAllowedAcrossClients) {
  UpdateAdBlockInstanceWithRules("*ad_fr*\n@@*ad_fr.png*");
  g_browser_process->SetApplicationLocale("fr");
  ASSERT_STREQ(g_browser_process->GetApplicationLocale().c_str(), "fr");

  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  SetRegionalComponentIdAndBase64PublicKeyForTest(
      kRegionalAdBlockComponentTestId,
      kRegionalAdBlockComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallRegionalAdBlockExtension(kAdBlockEasyListFranceUUID));
  ASSERT_TRUE(StartAdBlockRegionalServices());

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(1, 0, 0, 0, 0, 0);"
                                          "addImage('ad_fr.png')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Make sure the third-party flag is passed into the ad-block library properly
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdBlockThirdPartyWorksByETLDP1) {
  UpdateAdBlockInstanceWithRules("||a.com$third-party");
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  GURL tab_url = embedded_test_server()->GetURL("test.a.com", kAdBlockTestPage);
  GURL resource_url =
      embedded_test_server()->GetURL("test2.a.com", "/logo.png");
  ui_test_utils::NavigateToURL(browser(), tab_url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      base::StringPrintf("setExpectations(1, 0, 0, 0, 0, 0);"
                         "addImage('%s')",
                         resource_url.spec().c_str()),
      &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Make sure the third-party flag is passed into the ad-block library properly
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       AdBlockThirdPartyWorksForThirdPartyHost) {
  UpdateAdBlockInstanceWithRules("||a.com$third-party");
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  GURL resource_url = embedded_test_server()->GetURL("a.com", "/logo.png");
  ui_test_utils::NavigateToURL(browser(), tab_url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      base::StringPrintf("setExpectations(0, 1, 0, 0, 0, 0);"
                         "addImage('%s')",
                         resource_url.spec().c_str()),
      &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Load an image from a specific subdomain, and make sure it is blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, BlockNYP) {
  UpdateAdBlockInstanceWithRules("||sp1.nypost.com$third-party");
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  GURL resource_url =
      embedded_test_server()->GetURL("sp1.nypost.com", "/logo.png");
  ui_test_utils::NavigateToURL(browser(), tab_url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      base::StringPrintf("setExpectations(0, 1, 0, 0, 0, 0);"
                         "addImage('%s')",
                         resource_url.spec().c_str()),
      &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Tags for social buttons work
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, SocialButttonAdBLockTagTest) {
  UpdateAdBlockInstanceWithRules(
      base::StringPrintf("||example.com^$tag=%s",
                         brave_shields::kFacebookEmbeds)
          .c_str());
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  g_brave_browser_process->ad_block_service()->EnableTag(
      brave_shields::kFacebookEmbeds, true);
  WaitForAdBlockServiceThreads();
  GURL resource_url =
      embedded_test_server()->GetURL("example.com", "/logo.png");
  ui_test_utils::NavigateToURL(browser(), tab_url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      base::StringPrintf("setExpectations(0, 1, 0, 0, 0, 0);"
                         "addImage('%s')",
                         resource_url.spec().c_str()),
      &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Lack of tags for social buttons work
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, SocialButttonAdBlockDiffTagTest) {
  UpdateAdBlockInstanceWithRules("||example.com^$tag=sup");
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  g_brave_browser_process->ad_block_service()->EnableTag(
      brave_shields::kFacebookEmbeds, true);
  WaitForAdBlockServiceThreads();
  GURL resource_url =
      embedded_test_server()->GetURL("example.com", "/logo.png");
  ui_test_utils::NavigateToURL(browser(), tab_url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      base::StringPrintf("setExpectations(1, 0, 0, 0, 0, 0);"
                         "addImage('%s')",
                         resource_url.spec().c_str()),
      &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Tags are preserved after resetting
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, ResetPreservesTags) {
  g_brave_browser_process->ad_block_service()->EnableTag(
      brave_shields::kFacebookEmbeds, true);
  WaitForAdBlockServiceThreads();
  UpdateAdBlockInstanceWithRules("");
  AssertTagExists(brave_shields::kFacebookEmbeds, true);
}

// Setting prefs sets the right tags
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, TagPrefsControlTags) {
  auto* prefs = browser()->profile()->GetPrefs();

  // Default tags exist on startup
  AssertTagExists(brave_shields::kFacebookEmbeds, true);
  AssertTagExists(brave_shields::kTwitterEmbeds, true);
  AssertTagExists(brave_shields::kLinkedInEmbeds, false);

  // Toggling prefs once is reflected in the adblock client.
  prefs->SetBoolean(kLinkedInEmbedControlType, true);
  WaitForAdBlockServiceThreads();
  AssertTagExists(brave_shields::kFacebookEmbeds, true);
  AssertTagExists(brave_shields::kTwitterEmbeds, true);
  AssertTagExists(brave_shields::kLinkedInEmbeds, true);

  prefs->SetBoolean(kFBEmbedControlType, false);
  WaitForAdBlockServiceThreads();
  AssertTagExists(brave_shields::kFacebookEmbeds, false);
  AssertTagExists(brave_shields::kTwitterEmbeds, true);
  AssertTagExists(brave_shields::kLinkedInEmbeds, true);

  prefs->SetBoolean(kTwitterEmbedControlType, false);
  WaitForAdBlockServiceThreads();
  AssertTagExists(brave_shields::kFacebookEmbeds, false);
  AssertTagExists(brave_shields::kTwitterEmbeds, false);
  AssertTagExists(brave_shields::kLinkedInEmbeds, true);

  // Toggling prefs back is reflected in the adblock client.
  prefs->SetBoolean(kLinkedInEmbedControlType, false);
  WaitForAdBlockServiceThreads();
  AssertTagExists(brave_shields::kFacebookEmbeds, false);
  AssertTagExists(brave_shields::kTwitterEmbeds, false);
  AssertTagExists(brave_shields::kLinkedInEmbeds, false);

  prefs->SetBoolean(kFBEmbedControlType, true);
  WaitForAdBlockServiceThreads();
  AssertTagExists(brave_shields::kFacebookEmbeds, true);
  AssertTagExists(brave_shields::kTwitterEmbeds, false);
  AssertTagExists(brave_shields::kLinkedInEmbeds, false);

  prefs->SetBoolean(kTwitterEmbedControlType, true);
  WaitForAdBlockServiceThreads();
  AssertTagExists(brave_shields::kFacebookEmbeds, true);
  AssertTagExists(brave_shields::kTwitterEmbeds, true);
  AssertTagExists(brave_shields::kLinkedInEmbeds, false);
}

// Make sure that cancelrequest actually blocks
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CancelRequestOptionTest) {
  UpdateAdBlockInstanceWithRules("logo.png$explicitcancel");
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  GURL resource_url =
      embedded_test_server()->GetURL("example.com", "/logo.png");
  ui_test_utils::NavigateToURL(browser(), tab_url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      base::StringPrintf("setExpectations(0, 0, 1, 0, 0, 0);"
                         "addImage('%s')",
                         resource_url.spec().c_str()),
      &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Load a page with a script which uses a redirect data URL.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, RedirectRulesAreRespected) {
  UpdateAdBlockInstanceWithRules(
      "js_mock_me.js$redirect=noopjs",
      R"(
      [
        {
          "name": "noop.js",
          "aliases": ["noopjs"],
          "kind": {
            "mime":"application/javascript"
          },
          "content": "KGZ1bmN0aW9uKCkgewogICAgJ3VzZSBzdHJpY3QnOwp9KSgpOwo="
        }
      ])");
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  const GURL url = embedded_test_server()->GetURL("example.com",
                                                  kAdBlockTestPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  const std::string noopjs = "(function() {\\n    \\'use strict\\';\\n})();\\n";
  bool as_expected = false;
  const GURL resource_url =
      embedded_test_server()->GetURL("example.com", "/js_mock_me.js");
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      base::StringPrintf("setExpectations(0, 0, 0, 1, 0, 0);"
                         "xhr_expect_content('%s', '%s');",
                         resource_url.spec().c_str(), noopjs.c_str()),
      &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

class CosmeticFilteringDisabledTest : public AdBlockServiceTest {
 public:
  CosmeticFilteringDisabledTest() {
    feature_list_.InitAndDisableFeature(kBraveAdblockCosmeticFiltering);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Ensure no cosmetic filtering occurs when the feature flag is disabled
IN_PROC_BROWSER_TEST_F(CosmeticFilteringDisabledTest, CosmeticFilteringSimple) {
  UpdateAdBlockInstanceWithRules(
      "b.com###ad-banner\n"
      "##.ad");

  WaitForBraveExtensionShieldsDataReady();

  GURL tab_url = embedded_test_server()->GetURL("b.com",
                                                "/cosmetic_filtering.html");
  ui_test_utils::NavigateToURL(browser(), tab_url);

  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "checkSelector('#ad-banner', 'display', 'block')",
              &as_expected));
  EXPECT_TRUE(as_expected);

  as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "checkSelector('.ad-banner', 'display', 'block')",
              &as_expected));
  EXPECT_TRUE(as_expected);

  as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "checkSelector('.ad', 'display', 'block')",
              &as_expected));
  EXPECT_TRUE(as_expected);
}

// Test simple cosmetic filtering
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringSimple) {
  UpdateAdBlockInstanceWithRules(
      "b.com###ad-banner\n"
      "##.ad");

  WaitForBraveExtensionShieldsDataReady();

  GURL tab_url = embedded_test_server()->GetURL("b.com",
                                                "/cosmetic_filtering.html");
  ui_test_utils::NavigateToURL(browser(), tab_url);

  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "checkSelector('#ad-banner', 'display', 'none')",
              &as_expected));
  EXPECT_TRUE(as_expected);

  as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "checkSelector('.ad-banner', 'display', 'block')",
              &as_expected));
  EXPECT_TRUE(as_expected);

  as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "checkSelector('.ad', 'display', 'none')",
              &as_expected));
  EXPECT_TRUE(as_expected);
}

// Test cosmetic filtering ignores content determined to be 1st party
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       CosmeticFilteringProtect1p) {
  UpdateAdBlockInstanceWithRules("b.com##.fpsponsored\n");

  WaitForBraveExtensionShieldsDataReady();

  GURL tab_url = embedded_test_server()->GetURL("b.com",
                                                "/cosmetic_filtering.html");
  ui_test_utils::NavigateToURL(browser(), tab_url);

  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "checkSelector('.fpsponsored', 'display', 'block')",
              &as_expected));
  EXPECT_TRUE(as_expected);
}

// Test cosmetic filtering on elements added dynamically
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringDynamic) {
  UpdateAdBlockInstanceWithRules("##.blockme");

  WaitForBraveExtensionShieldsDataReady();

  GURL tab_url = embedded_test_server()->GetURL("b.com",
                                                "/cosmetic_filtering.html");
  ui_test_utils::NavigateToURL(browser(), tab_url);

  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "addElementsDynamically();\n"
              "checkSelector('.blockme', 'display', 'none')",
              &as_expected));
  EXPECT_TRUE(as_expected);

  as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "checkSelector('.dontblockme', 'display', 'block')",
              &as_expected));
  EXPECT_TRUE(as_expected);
}

// Test custom style rules
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       CosmeticFilteringCustomStyle) {
  UpdateAdBlockInstanceWithRules("b.com##.ad:style(padding-bottom: 10px)");

  WaitForBraveExtensionShieldsDataReady();

  GURL tab_url = embedded_test_server()->GetURL("b.com",
                                                "/cosmetic_filtering.html");
  ui_test_utils::NavigateToURL(browser(), tab_url);

  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "checkSelector('.ad', 'padding-bottom', '10px')",
              &as_expected));
  EXPECT_TRUE(as_expected);
}

// Test rules overridden by hostname-specific exception rules
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringUnhide) {
  UpdateAdBlockInstanceWithRules(
      "##.ad\n"
      "b.com#@#.ad\n"
      "###ad-banner\n"
      "a.com#@##ad-banner");

  WaitForBraveExtensionShieldsDataReady();

  GURL tab_url = embedded_test_server()->GetURL("b.com",
                                                "/cosmetic_filtering.html");
  ui_test_utils::NavigateToURL(browser(), tab_url);

  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "checkSelector('.ad', 'display', 'block')",
              &as_expected));
  EXPECT_TRUE(as_expected);

  as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "checkSelector('#ad-banner', 'display', 'none')",
              &as_expected));
  EXPECT_TRUE(as_expected);
}

// Test scriptlet injection that modifies window attributes
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       CosmeticFilteringWindowScriptlet) {
  /* "content" below corresponds to the following scriptlet:
   * ```
   * (function() {
   *   const send = window.getComputedStyle;
   *   window.getComputedStyle = function(selector) {
   *     return { 'color': 'Impossible value' };
   *   }
   * })();
   * ```
   */
  UpdateAdBlockInstanceWithRules("b.com##+js(hjt)", "[{"
          "\"name\": \"hijacktest\","
          "\"aliases\": [\"hjt\"],"
          "\"kind\": {\"mime\": \"application/javascript\"},"
          "\"content\": \"KGZ1bmN0aW9uKCkgewogIGNvbnN0IHNlbmQgPSB3aW5kb3cuZ2V0"
          "Q29tcHV0ZWRTdHlsZTsKICB3aW5kb3cuZ2V0Q29tcHV0ZWRTdHlsZSA9IGZ1bmN0aW9"
          "uKHNlbGVjdG9yKSB7CiAgICByZXR1cm4geyAnY29sb3InOiAnSW1wb3NzaWJsZSB2YW"
          "x1ZScgfTsKICB9Cn0pKCk7Cg==\"}]");

  WaitForBraveExtensionShieldsDataReady();

  GURL tab_url = embedded_test_server()->GetURL("b.com",
                                                "/cosmetic_filtering.html");
  ui_test_utils::NavigateToURL(browser(), tab_url);

  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
              contents,
              "checkSelector('.ad', 'color', 'Impossible value')",
              &as_expected));
  EXPECT_TRUE(as_expected);
}
