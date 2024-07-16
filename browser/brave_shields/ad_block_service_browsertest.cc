/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/ad_block_service_browsertest.h"

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/thread_test_helper.h"
#include "base/threading/thread_restrictions.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"
#include "brave/components/brave_shields/content/browser/ad_block_custom_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_service_manager.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_service_manager_observer.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/content/test/engine_test_observer.h"
#include "brave/components/brave_shields/content/test/test_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_default_resource_provider.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/test_data_directory.h"
#include "services/network/host_resolver.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/browser/playlist_background_web_contentses.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/common/features.h"
#endif

const char kAdBlockTestPage[] = "/blocking.html";

const char kAdBlockEasyListFranceUUID[] =
    "9852EFC4-99E4-4F2D-A915-9C3196C7A1DE";

const char kDefaultAdBlockComponentTestId[] =
    "naccapggpomhlhoifnlebfoocegenbol";
const char kRegionalAdBlockComponentTestId[] =
    "dlpmaigjliompnelofkljgcmlenklieh";

const char kDefaultAdBlockComponentTest64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtV7Vr69kkvSvu2lhcMDh"
    "j4Jm3FKU1zpUkALaum5719/cccVvGpMKKFyy4WYXsmAfcIONmGO4ThK/q6jkgC5v"
    "8HrkjPOf7HHebKEnsJJucz/Z1t6dq0CE+UA2IWfbGfFM4nJ8AKIv2gqiw2d4ydAs"
    "QcL26uR9IHHrBk/zzkv2jO43Aw2kY3loqRf60THz4pfz5vOtI+BKOw1KHM0+y1Di"
    "Qdk+dZ9r8NRQnpjChQzwhMAkxyrdjT1N7NcfTufiYQTOyiFvxPAC9D7vAzkpGgxU"
    "Ikylk7cYRxqkRGS/AayvfipJ/HOkoBd0yKu1MRk4YcKGd/EahDAhUtd9t4+v33Qv"
    "uwIDAQAB";
const char kRegionalAdBlockComponentTest64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoKYkdDM8vWZXBbDJXTP6"
    "1m9yLuH9iL/TvqAqu1zOd91VJu4bpcCMZjfGPC1g+O+pZrCaFVv5NJeZxGqT6DUB"
    "RZUdXPkGGUC1ebS4LLJbggNQb152LFk8maR0/ItvMOW8eTcV8VFKHk4UrVhPTggf"
    "dU/teuAesUUJnhFchijBtAqO+nJ0wEcksY8ktrIyoNPzMj43a1OVJVXrPFDc+WT/"
    "G8XBq/Y8FbBt+u+7skWQy3lVyRwFjeFu6cXVF4tcc06PNx5yLsbHQtSv8R+h1bWw"
    "ieMF3JB9CZPr+qDKIap+RZUfsraV47QebRi/JA17nbDMlXOmK7mILfFU7Jhjx04F"
    "LwIDAQAB";

using brave_shields::features::kBraveAdblockCnameUncloaking;
using brave_shields::features::kBraveAdblockCollapseBlockedElements;
using brave_shields::features::kBraveAdblockCosmeticFiltering;
using brave_shields::features::kBraveAdblockDefault1pBlocking;
using brave_shields::features::kBraveAdblockScriptletDebugLogs;
using brave_shields::features::kCosmeticFilteringJsPerformance;

namespace {
void WaitForSelectorBlocked(const content::ToRenderFrameHost& target,
                            const std::string& selector) {
  const char kTemplate[] = R"(waitCSSSelector($1, 'display', 'none'))";

  ASSERT_TRUE(
      EvalJs(target, content::JsReplace(kTemplate, selector)).ExtractBool());
}
}  // namespace

AdBlockServiceTest::AdBlockServiceTest()
    : https_server_(net::EmbeddedTestServer::Type::TYPE_HTTPS) {}
AdBlockServiceTest::~AdBlockServiceTest() = default;

void AdBlockServiceTest::SetUpCommandLine(base::CommandLine* command_line) {
  PlatformBrowserTest::SetUpCommandLine(command_line);
  mock_cert_verifier_.SetUpCommandLine(command_line);
}

void AdBlockServiceTest::SetUpInProcessBrowserTestFixture() {
  PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
  mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
}

void AdBlockServiceTest::SetUpOnMainThread() {
  PlatformBrowserTest::SetUpOnMainThread();
  mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
  host_resolver()->AddRule("*", "127.0.0.1");
  InitEmbeddedTestServer();
  // Most tests are written for aggressive mode. Individual tests should reset
  // this using `DisableAggressiveMode` if they are testing standard mode
  // behavior.
  brave_shields::SetCosmeticFilteringControlType(
      content_settings(), brave_shields::ControlType::BLOCK, GURL());
}

void AdBlockServiceTest::PreRunTestOnMainThread() {
  PlatformBrowserTest::PreRunTestOnMainThread();
  WaitForAdBlockServiceThreads();
  histogram_tester_.ExpectTotalCount(
      "Brave.Adblock.MakeEngineWithRules.Default", 1);
  InstallDefaultAdBlockComponent();
  histogram_tester_.ExpectTotalCount(
      "Brave.Adblock.MakeEngineWithRules.Default", 2);
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

void AdBlockServiceTest::TearDownOnMainThread() {
  source_providers_.clear();
  temp_dirs_.clear();
  // Unset the host resolver so as not to interfere with later tests.
  brave::SetAdblockCnameHostResolverForTesting(nullptr);
  PlatformBrowserTest::TearDownOnMainThread();
}

void AdBlockServiceTest::TearDownInProcessBrowserTestFixture() {
  mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
}

content::WebContents* AdBlockServiceTest::web_contents() {
  return chrome_test_utils::GetActiveWebContents(this);
}

Profile* AdBlockServiceTest::profile() {
  return chrome_test_utils::GetProfile(this);
}

HostContentSettingsMap* AdBlockServiceTest::content_settings() {
  return HostContentSettingsMapFactory::GetForProfile(profile());
}

void AdBlockServiceTest::AddNewRules(const std::string& rules,
                                     uint8_t permission_mask,
                                     bool first_party_protections) {
  auto source_provider = std::make_unique<brave_shields::TestFiltersProvider>(
      rules, first_party_protections, permission_mask);
  source_providers_.push_back(std::move(source_provider));

  brave_shields::AdBlockService* ad_block_service =
      g_brave_browser_process->ad_block_service();
  auto* engine = first_party_protections
                     ? ad_block_service->default_engine_.get()
                     : ad_block_service->additional_filters_engine_.get();
  EngineTestObserver engine_observer(engine);
  engine_observer.Wait();
}

// Returns the path of the new directory, not the file. Intended for use with
// `OnComponentReady`.
base::FilePath AdBlockServiceTest::MakeFileInTempDir(
    const std::string& name,
    const std::string& contents) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  auto dir = std::make_unique<base::ScopedTempDir>();
  EXPECT_TRUE(dir->CreateUniqueTempDir());
  auto path = dir->GetPath();
  base::File list_file(path.AppendASCII(name), base::File::FLAG_CREATE |
                                                   base::File::FLAG_WRITE |
                                                   base::File::FLAG_READ);
  EXPECT_TRUE(list_file.IsValid());
  list_file.Write(0, contents.c_str(), contents.size());
  list_file.Close();

  temp_dirs_.push_back(std::move(dir));

  return path;
}

// `AdBlockComponentFiltersProvider` deletes outdated component data when a new
// version is installed. This method allows files from the test data directory
// to be used without removing the on-disk source.
//
// Returns the path of the new directory, not the file.
// Intended for use with `OnComponentReady`.
base::FilePath AdBlockServiceTest::MakeTestDataCopy(
    const base::FilePath& source_location) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  auto dir = std::make_unique<base::ScopedTempDir>();
  EXPECT_TRUE(dir->CreateUniqueTempDir());
  auto temp_path = dir->GetPath();

  EXPECT_TRUE(base::CopyDirectoryExcl(source_location, temp_path, true));

  temp_dirs_.push_back(std::move(dir));

  return temp_path.Append(source_location.BaseName());
}

void AdBlockServiceTest::UpdateAdBlockResources(const std::string& resources) {
  auto component_path = MakeFileInTempDir("resources.json", resources);

  brave_shields::AdBlockService* service =
      g_brave_browser_process->ad_block_service();

  static_cast<brave_shields::AdBlockDefaultResourceProvider*>(
      service->resource_provider())
      ->OnComponentReady(component_path);
}

void AdBlockServiceTest::UpdateAdBlockInstanceWithRules(
    const std::string& rules) {
  auto component_path = MakeFileInTempDir("list.txt", rules);

  brave_shields::AdBlockService* service =
      g_brave_browser_process->ad_block_service();

  auto& component_providers =
      service->component_service_manager()->component_filters_providers();

  std::string uuid = "default";
  auto& provider = component_providers.at(uuid);
  EXPECT_TRUE(provider);
  provider->OnComponentReady(component_path);

  auto* engine = service->default_engine_.get();
  EngineTestObserver engine_observer(engine);
  engine_observer.Wait();
}

void AdBlockServiceTest::UpdateCustomAdBlockInstanceWithRules(
    const std::string& rules) {
  brave_shields::AdBlockService* ad_block_service =
      g_brave_browser_process->ad_block_service();
  ad_block_service->custom_filters_provider()->UpdateCustomFilters(rules);

  auto* engine = ad_block_service->additional_filters_engine_.get();
  EngineTestObserver engine_observer(engine);
  engine_observer.Wait();
}

void AdBlockServiceTest::AssertTagExists(const std::string& tag,
                                         bool expected_exists) const {
  g_brave_browser_process->ad_block_service()->TagExistsForTest(
      tag, base::BindOnce(
               [](bool expected_exists, bool actual_exists) {
                 ASSERT_EQ(expected_exists, actual_exists);
               },
               expected_exists));
}

void AdBlockServiceTest::InitEmbeddedTestServer() {
  base::FilePath test_data_dir = GetTestDataDir();

  https_server_.ServeFilesFromDirectory(test_data_dir);
  content::SetupCrossSiteRedirector(&https_server_);
  ASSERT_TRUE(https_server_.Start());

  embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
  content::SetupCrossSiteRedirector(embedded_test_server());
  ASSERT_TRUE(embedded_test_server()->Start());
}

base::FilePath AdBlockServiceTest::GetTestDataDir() {
  base::ScopedAllowBlockingForTesting allow_blocking;
  return base::PathService::CheckedGet(brave::DIR_TEST_DATA);
}

void AdBlockServiceTest::NavigateToURL(GURL url) {
  content::NavigateToURLBlockUntilNavigationsComplete(web_contents(), url, 1,
                                                      true);
}

void AdBlockServiceTest::InstallComponent(
    const brave_shields::FilterListCatalogEntry& catalog_entry) {
  auto* service = g_brave_browser_process->ad_block_service();
  std::vector<brave_shields::FilterListCatalogEntry> filter_list_catalog(
      service->component_service_manager()->GetFilterListCatalog());
  filter_list_catalog.push_back(catalog_entry);
  service->component_service_manager()->SetFilterListCatalog(
      filter_list_catalog);

  if (catalog_entry.default_enabled) {
    base::FilePath test_data_dir = GetTestDataDir();
    auto original_path = test_data_dir.AppendASCII("adblock-components")
                             .AppendASCII(catalog_entry.uuid);
    auto component_path = MakeTestDataCopy(original_path);

    auto& component_providers =
        service->component_service_manager()->component_filters_providers();

    auto& provider = component_providers.at(catalog_entry.uuid);
    EXPECT_TRUE(provider);
    provider->OnComponentReady(component_path);

    auto* engine = catalog_entry.first_party_protections
                       ? service->default_engine_.get()
                       : service->additional_filters_engine_.get();
    EngineTestObserver engine_observer(engine);
    engine_observer.Wait();
  }
}

void AdBlockServiceTest::InstallDefaultAdBlockComponent() {
  auto catalog_entry = brave_shields::FilterListCatalogEntry(
      "default", "", "Brave Ad Block Updater", {}, "",
      "Default lists for Brave Browser", true, true, true, 0, {},
      kDefaultAdBlockComponentTestId, kDefaultAdBlockComponentTest64PublicKey);

  InstallComponent(catalog_entry);
}

void AdBlockServiceTest::InstallRegionalAdBlockComponent(
    const std::string& uuid,
    bool enable_list) {
  auto catalog_entry = brave_shields::FilterListCatalogEntry(
      uuid, "https://easylist-downloads.adblockplus.org/liste_fr.txt",
      "EasyList Liste FR", {"fr"}, "https://forums.lanik.us/viewforum.php?f=91",
      "Removes advertisements from French websites", false, enable_list, false,
      0, {}, kRegionalAdBlockComponentTestId,
      kRegionalAdBlockComponentTest64PublicKey);

  InstallComponent(catalog_entry);
}

void AdBlockServiceTest::SetSubscriptionIntervals() {
  auto initial_delay = base::Seconds(2);
  auto retry_interval = base::Seconds(2);

  auto* ad_block_service = g_brave_browser_process->ad_block_service();
  auto* subscription_service_manager =
      ad_block_service->subscription_service_manager();

  subscription_service_manager->SetUpdateIntervalsForTesting(&initial_delay,
                                                             &retry_interval);
}

void AdBlockServiceTest::WaitForAdBlockServiceThreads() {
  scoped_refptr<base::ThreadTestHelper> tr_helper(new base::ThreadTestHelper(
      g_brave_browser_process->ad_block_service()->GetTaskRunner()));
  ASSERT_TRUE(tr_helper->Run());
}

void AdBlockServiceTest::ShieldsDown(const GURL& url) {
  brave_shields::SetBraveShieldsEnabled(content_settings(), false, url);
}

void AdBlockServiceTest::DisableAggressiveMode() {
  brave_shields::SetCosmeticFilteringControlType(
      content_settings(), brave_shields::ControlType::BLOCK_THIRD_PARTY,
      GURL());
}

// Load a page with an ad image, and make sure it is blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdsGetBlockedByDefaultBlocker) {
  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Load a page with an image which is not an ad, and make sure it is NOT
// blocked by custom filters.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       NotAdsDoNotGetBlockedByCustomBlocker) {
  UpdateCustomAdBlockInstanceWithRules("*ad_banner.png");

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(1, 0, 0, 0);"
                         "addImage('logo.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Load a page with an ad image, and make sure it is blocked by custom
// filters.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdsGetBlockedByCustomBlocker) {
  UpdateAdBlockInstanceWithRules("");

  UpdateCustomAdBlockInstanceWithRules("*ad_banner.png");

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Load a page with an ad image, with a corresponding exception installed in
// the custom filters, and make sure it is not blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, DefaultBlockCustomException) {
  UpdateAdBlockInstanceWithRules("*ad_banner.png");
  UpdateCustomAdBlockInstanceWithRules("@@ad_banner.png");

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(contents,
                         "setExpectations(1, 0, 0, 0);"
                         "addImage('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Load a page with an image blocked by custom filters, with a corresponding
// exception installed in the default filters, and make sure it is not blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CustomBlockDefaultException) {
  UpdateAdBlockInstanceWithRules("@@ad_banner.png");
  UpdateCustomAdBlockInstanceWithRules("*ad_banner.png");

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(1, 0, 0, 0);"
                         "addImage('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Load a page with an image blocked by custom filters, with a corresponding
// exception installed in the default filters, and make sure it is not blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       CustomBlockDefaultExceptionStandardMode) {
  DisableAggressiveMode();

  UpdateAdBlockInstanceWithRules("@@ad_banner.png");
  UpdateCustomAdBlockInstanceWithRules("*ad_banner.png");

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(1, 0, 0, 0);"
                         "addImage('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Load a page with an image which is not an ad, and make sure it is NOT
// blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       NotAdsDoNotGetBlockedByDefaultBlocker) {
  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(1, 0, 0, 0);"
                         "addImage('logo.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// The test verifies the number of expected engine updating during normal
// startup.
// Warning: each engine updating is a CPU-heavy thing and degrades startup
// performance.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, DefaultStartupWithCookieList) {
  // The empty ruleset building until the components are loaded.
  // TODO(matuchin): remove that excessive work.
  histogram_tester_.ExpectTotalCount(
      "Brave.Adblock.MakeEngineWithRules.Default", 2);
  histogram_tester_.ExpectTotalCount(
      "Brave.Adblock.MakeEngineWithRules.Additional", 1);

  // Loads the default list first, then the additional cookie list.
  InstallRegionalAdBlockComponent(brave_shields::kCookieListUuid, true);

  // Only one new rebuild is expected. Loading the extra list must not
  // trigger another rebuilding of the default engine and vice versa.
  histogram_tester_.ExpectTotalCount(
      "Brave.Adblock.MakeEngineWithRules.Default", 2);
  histogram_tester_.ExpectTotalCount(
      "Brave.Adblock.MakeEngineWithRules.Additional", 2);
}

// Load a page with an ad image, and make sure it is blocked by a filter from a
// regional list component.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdsGetBlockedByRegionalBlocker) {
  g_browser_process->SetApplicationLocale("fr");
  ASSERT_STREQ(g_browser_process->GetApplicationLocale().c_str(), "fr");

  InstallRegionalAdBlockComponent(kAdBlockEasyListFranceUUID);

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('ad_fr.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Load a page with an image which is not an ad, and make sure it is
// NOT blocked by a filter from a regional list.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       NotAdsDoNotGetBlockedByRegionalBlocker) {
  g_browser_process->SetApplicationLocale("fr");
  ASSERT_STREQ(g_browser_process->GetApplicationLocale().c_str(), "fr");

  InstallRegionalAdBlockComponent(kAdBlockEasyListFranceUUID);

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(1, 0, 0, 0);"
                         "addImage('logo.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Load a page with several of the same adblocked xhr requests, it should only
// count 1.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, TwoSameAdsGetCountedAsOne) {
  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 0, 0, 1);"
                         "xhr('adbanner.js')"));
  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 0, 1, 1);"
                         "xhr('normal.js')"));
  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 0, 1, 2);"
                         "xhr('adbanner.js')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Load a page with different adblocked xhr requests, it should count each.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, TwoDiffAdsGetCountedAsTwo) {
  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 0, 0, 1);"
                         "xhr('adbanner.js?1')"));
  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 0, 1, 1);"
                         "xhr('normal.js')"));
  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 0, 1, 2);"
                         "xhr('adbanner.js?2')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);
}

// New tab continues to count blocking the same resource
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, NewTabContinuesToBlock) {
  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 0, 0, 1);"
                         "xhr('adbanner.js')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  NavigateToURL(url);
  contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 0, 0, 1);"
                         "xhr('adbanner.js')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);

  NavigateToURL(url);
}

// XHRs and ads in a cross-site iframe are blocked as well.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, SubFrame) {
  GURL url = embedded_test_server()->GetURL("a.com", "/iframe_blocking.html");
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(ChildFrameAt(contents, 0),
                         "setExpectations(0, 0, 0, 1);"
                         "xhr('adbanner.js?1')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  // Check also an explicit request for a script since it is a common real-world
  // scenario.
  ASSERT_EQ(true, EvalJs(ChildFrameAt(contents, 0),
                         R"(
                           new Promise(function (resolve, reject) {
                             var s = document.createElement('script');
                             s.onload = reject;
                             s.onerror = () => resolve(true);
                             s.src = 'adbanner.js?2';
                             document.head.appendChild(s);
                           })
                         )"));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);
}

// Checks nothing is blocked if shields are off.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, SubFrameShieldsOff) {
  GURL url = embedded_test_server()->GetURL("a.com", "/iframe_blocking.html");

  brave_shields::SetBraveShieldsEnabled(content_settings(), false, url);

  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(ChildFrameAt(contents, 0),
                         "setExpectations(0, 0, 1, 0);"
                         "xhr('adbanner.js?1')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  // Check also an explicit request for a script since it is a common real-world
  // scenario.
  EXPECT_EQ(true, EvalJs(ChildFrameAt(contents, 0),
                         R"(
                           new Promise(function (resolve, reject) {
                             var s = document.createElement('script');
                             s.onload = () => resolve(true);
                             s.onerror = reject;
                             s.src = 'adbanner.js?2';
                             document.head.appendChild(s);
                           })
                         )"));
  content::RunAllTasksUntilIdle();
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
  brave_shields::ResetBraveShieldsEnabled(content_settings(), url);
}

// Requests made by a service worker should be blocked as well.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, ServiceWorkerRequest) {
  UpdateAdBlockInstanceWithRules("adbanner.js");

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 0, 0, 1);"
                         "installBlockingServiceWorker()"));
  // https://github.com/brave/brave-browser/issues/14087
  // EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// See crbug.com/1372291.
#if BUILDFLAG(IS_ANDROID)
#define MAYBE_WebSocketBlocking DISABLED_WebSocketBlocking
#else
#define MAYBE_WebSocketBlocking WebSocketBlocking
#endif

IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, MAYBE_WebSocketBlocking) {
  UpdateAdBlockInstanceWithRules("*$websocket");

  net::SpawnedTestServer ws_server(net::SpawnedTestServer::TYPE_WS,
                                   net::GetWebSocketTestDataDirectory());
  ASSERT_TRUE(ws_server.Start());

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  GURL ws_url = ws_server.GetURL("echo-with-no-extension");

  EXPECT_EQ(false, EvalJs(contents,
                          base::StringPrintf("checkWebsocketConnection(\"%s\")",
                                             ws_url.spec().c_str())));
}

// Load a page with an ad image which is matched by a filter in the additional
// engine, but make sure it is saved by an exception in the default engine.
// This test is the same as AdsGetBlockedByRegionalBlocker except for at the
// start it adds an exception rule to the default engine.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       ExceptionAdsAreAllowedAcrossClients) {
  g_browser_process->SetApplicationLocale("fr");
  ASSERT_STREQ(g_browser_process->GetApplicationLocale().c_str(), "fr");

  InstallRegionalAdBlockComponent(kAdBlockEasyListFranceUUID);
  UpdateAdBlockInstanceWithRules("*ad_fr*\n@@*ad_fr.png*");

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();
  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(1, 0, 0, 0);"
                         "addImage('ad_fr.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Make sure the third-party flag is passed into the ad-block library properly
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdBlockThirdPartyWorksByETLDP1) {
  UpdateAdBlockInstanceWithRules("||a.com$third-party");

  GURL tab_url = embedded_test_server()->GetURL("test.a.com", kAdBlockTestPage);
  GURL resource_url =
      embedded_test_server()->GetURL("test2.a.com", "/logo.png");
  NavigateToURL(tab_url);
  content::WebContents* contents = web_contents();
  ASSERT_EQ(true,
            EvalJs(contents, base::StringPrintf("setExpectations(1, 0, 0, 0);"
                                                "addImage('%s')",
                                                resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Make sure the third-party flag is passed into the ad-block library properly
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       AdBlockThirdPartyWorksForThirdPartyHost) {
  UpdateAdBlockInstanceWithRules("||a.com$third-party");
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  GURL resource_url = embedded_test_server()->GetURL("a.com", "/logo.png");
  NavigateToURL(tab_url);
  content::WebContents* contents = web_contents();
  ASSERT_EQ(true,
            EvalJs(contents, base::StringPrintf("setExpectations(0, 1, 0, 0);"
                                                "addImage('%s')",
                                                resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// These tests fail intermittently on macOS; see
// https://github.com/brave/brave-browser/issues/15912
#if BUILDFLAG(IS_MAC)
#define MAYBE_CnameCloakedRequestsGetBlocked \
  DISABLED_CnameCloakedRequestsGetBlocked
#define MAYBE_CnameCloakedRequestsCanBeExcepted \
  DISABLED_CnameCloakedRequestsCanBeExcepted
#else
#define MAYBE_CnameCloakedRequestsGetBlocked CnameCloakedRequestsGetBlocked
#define MAYBE_CnameCloakedRequestsCanBeExcepted \
  CnameCloakedRequestsCanBeExcepted
#endif

// A test observer that allows blocking waits for the
// AdBlockSubscriptionServiceManager to update the status of any registered
// subscriptions.
class TestAdBlockSubscriptionServiceManagerObserver
    : public brave_shields::AdBlockSubscriptionServiceManagerObserver {
 public:
  // Constructs a TestAdBlockSubscriptionServiceManagerObserver which will
  // observe |sub_service_manager| for updates to the status of any registered
  // subscriptions.
  explicit TestAdBlockSubscriptionServiceManagerObserver(
      brave_shields::AdBlockSubscriptionServiceManager* sub_service_manager)
      : sub_service_manager_(sub_service_manager) {
    sub_service_manager_->AddObserver(this);
  }
  ~TestAdBlockSubscriptionServiceManagerObserver() override {
    sub_service_manager_->RemoveObserver(this);
  }

  TestAdBlockSubscriptionServiceManagerObserver(
      const TestAdBlockSubscriptionServiceManagerObserver& other) = delete;
  TestAdBlockSubscriptionServiceManagerObserver& operator=(
      const TestAdBlockSubscriptionServiceManagerObserver& other) = delete;

  // Waits for the notification from the subscription service manager to happen.
  void Wait() { run_loop_.Run(); }

 private:
  void OnServiceUpdateEvent() override { run_loop_.Quit(); }

  base::RunLoop run_loop_;
  raw_ptr<brave_shields::AdBlockSubscriptionServiceManager>
      sub_service_manager_ = nullptr;
};

// Make sure a list added as a custom subscription works correctly
// The download in this test fails intermittently with a network error code,
// although it doesn't seem to occur in real usage.
// TODO(https://github.com/brave/brave-browser/issues/33506)
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       DISABLED_SubscribeToCustomSubscription) {
  GURL subscription_url =
      embedded_test_server()->GetURL("lists.com", "/list.txt");
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  GURL resource_url = embedded_test_server()->GetURL("b.com", "/logo.png");

  SetSubscriptionIntervals();

  auto* sub_service_manager = g_brave_browser_process->ad_block_service()
                                  ->subscription_service_manager();

  ASSERT_EQ(sub_service_manager->GetSubscriptions().size(), 0ULL);

  // Create a new subscription
  sub_service_manager->CreateSubscription(subscription_url);

  // Ensure the subscription is registered correctly
  {
    const auto subscriptions = sub_service_manager->GetSubscriptions();
    ASSERT_EQ(subscriptions.size(), 1ULL);
    ASSERT_EQ(subscriptions[0].subscription_url, subscription_url);
    ASSERT_EQ(subscriptions[0].last_update_attempt, base::Time());
    ASSERT_EQ(subscriptions[0].last_successful_update_attempt, base::Time());
    ASSERT_EQ(subscriptions[0].enabled, true);
    ASSERT_EQ(subscriptions[0].homepage, std::nullopt);
    ASSERT_EQ(subscriptions[0].expires, 7 * 24);
    ASSERT_EQ(subscriptions[0].title, std::nullopt);
  }

  // Ensure that the subscription gets update attempts, and ultimately is
  // successfully updated. It may fail initially due to the download service
  // not being initialized in time, but this is an expected side effect of
  // using the download service.
  base::Time first_successful_update = base::Time();
  while (first_successful_update == base::Time()) {
    // Wait for the subscription to be updated using an observer
    TestAdBlockSubscriptionServiceManagerObserver sub_observer(
        sub_service_manager);
    sub_observer.Wait();
    WaitForAdBlockServiceThreads();

    const auto subscriptions = sub_service_manager->GetSubscriptions();
    ASSERT_EQ(subscriptions.size(), 1ULL);
    ASSERT_EQ(subscriptions[0].subscription_url, subscription_url);
    ASSERT_NE(subscriptions[0].last_update_attempt, base::Time());
    ASSERT_EQ(subscriptions[0].enabled, true);

    if (subscriptions[0].last_successful_update_attempt ==
        subscriptions[0].last_update_attempt) {
      first_successful_update = subscriptions[0].last_successful_update_attempt;
    }
  }

  // Make sure the list is applied during browsing
  NavigateToURL(tab_url);
  content::WebContents* contents = web_contents();
  EXPECT_EQ(true,
            EvalJs(contents, base::StringPrintf("setExpectations(0, 0, 0, 1);"
                                                "xhr('%s')",
                                                resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  // Disable the list and ensure it is no longer applied
  sub_service_manager->EnableSubscription(subscription_url, false);
  {
    const auto subscriptions = sub_service_manager->GetSubscriptions();
    ASSERT_EQ(subscriptions.size(), 1ULL);
    ASSERT_EQ(subscriptions[0].subscription_url, subscription_url);
    ASSERT_NE(subscriptions[0].last_update_attempt, base::Time());
    ASSERT_EQ(subscriptions[0].last_successful_update_attempt,
              subscriptions[0].last_update_attempt);
    ASSERT_EQ(subscriptions[0].enabled, false);
    ASSERT_EQ(subscriptions[0].homepage, "https://example.com/list.txt");
    ASSERT_EQ(subscriptions[0].expires, 3 * 24);
    ASSERT_EQ(subscriptions[0].title, "Test list");
  }

  EXPECT_EQ(true,
            EvalJs(contents, base::StringPrintf("setExpectations(0, 0, 1, 1);"
                                                "xhr('%s')",
                                                resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  // Refresh the subscription and ensure that it gets updated
  TestAdBlockSubscriptionServiceManagerObserver sub_observer(
      sub_service_manager);
  sub_service_manager->RefreshSubscription(subscription_url, true);
  sub_observer.Wait();
  {
    const auto subscriptions = sub_service_manager->GetSubscriptions();
    ASSERT_EQ(subscriptions.size(), 1ULL);
    ASSERT_EQ(subscriptions[0].subscription_url, subscription_url);
    ASSERT_GT(subscriptions[0].last_update_attempt, first_successful_update);
    ASSERT_EQ(subscriptions[0].last_successful_update_attempt,
              subscriptions[0].last_update_attempt);
    ASSERT_EQ(subscriptions[0].enabled, false);
  }

  // Remove the list and ensure it is completely gone
  sub_service_manager->DeleteSubscription(subscription_url);
  {
    const auto subscriptions = sub_service_manager->GetSubscriptions();
    ASSERT_EQ(subscriptions.size(), 0ULL);
  }
}

// The subscription observer never fires on Android, for an unknown reason.
#if BUILDFLAG(IS_ANDROID)
#define MAYBE_SubscribeTo404List DISABLED_SubscribeTo404List
#else
#define MAYBE_SubscribeTo404List SubscribeTo404List
#endif

// Make sure the state of a list that cannot be fetched is as expected
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, MAYBE_SubscribeTo404List) {
  GURL subscription_url =
      embedded_test_server()->GetURL("lists.com", "/this/list/does/not/exist");
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  GURL resource_url = embedded_test_server()->GetURL("b.com", "/logo.png");

  auto* sub_service_manager = g_brave_browser_process->ad_block_service()
                                  ->subscription_service_manager();

  ASSERT_EQ(sub_service_manager->GetSubscriptions().size(), 0ULL);

  // Register an observer for the subscription service manager
  TestAdBlockSubscriptionServiceManagerObserver sub_observer(
      sub_service_manager);

  // Create a new subscription
  sub_service_manager->CreateSubscription(subscription_url);

  // Ensure the subscription is registered correctly
  {
    const auto subscriptions = sub_service_manager->GetSubscriptions();
    ASSERT_EQ(subscriptions.size(), 1ULL);
    ASSERT_EQ(subscriptions[0].subscription_url, subscription_url);
    ASSERT_EQ(subscriptions[0].last_update_attempt, base::Time());
    ASSERT_EQ(subscriptions[0].last_successful_update_attempt, base::Time());
    ASSERT_EQ(subscriptions[0].enabled, true);
  }

  // Wait for the subscription to be updated for the first time
  sub_observer.Wait();

  // Ensure that the status of the subscription has been updated accordingly
  {
    const auto subscriptions = sub_service_manager->GetSubscriptions();
    ASSERT_EQ(subscriptions.size(), 1ULL);
    ASSERT_EQ(subscriptions[0].subscription_url, subscription_url);
    ASSERT_NE(subscriptions[0].last_update_attempt, base::Time());
    ASSERT_EQ(subscriptions[0].last_successful_update_attempt, base::Time());
    ASSERT_EQ(subscriptions[0].enabled, true);
  }
}

// Make sure that a list cannot be subscribed to twice
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, SubscribeToListUrlTwice) {
  GURL subscription_url =
      embedded_test_server()->GetURL("lists.com", "/this/list/does/not/exist");
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  GURL resource_url = embedded_test_server()->GetURL("b.com", "/logo.png");

  auto* sub_service_manager = g_brave_browser_process->ad_block_service()
                                  ->subscription_service_manager();

  ASSERT_EQ(sub_service_manager->GetSubscriptions().size(), 0ULL);

  // Register an observer for the subscription service manager
  TestAdBlockSubscriptionServiceManagerObserver sub_observer(
      sub_service_manager);

  // Create a new subscription
  sub_service_manager->CreateSubscription(subscription_url);

  // Ensure the subscription is registered correctly
  {
    const auto subscriptions = sub_service_manager->GetSubscriptions();
    ASSERT_EQ(subscriptions.size(), 1ULL);
    ASSERT_EQ(subscriptions[0].subscription_url, subscription_url);
    ASSERT_EQ(subscriptions[0].last_update_attempt, base::Time());
    ASSERT_EQ(subscriptions[0].last_successful_update_attempt, base::Time());
    ASSERT_EQ(subscriptions[0].enabled, true);
  }

  // Create a subscription with the same URL again
  sub_service_manager->CreateSubscription(subscription_url);

  // Ensure there's still only the original subscription
  {
    const auto subscriptions = sub_service_manager->GetSubscriptions();
    ASSERT_EQ(subscriptions.size(), 1ULL);
  }
}

// Make sure that CNAME cloaked network requests get blocked correctly and
// issue the correct number of DNS resolutions
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       MAYBE_CnameCloakedRequestsGetBlocked) {
  UpdateAdBlockInstanceWithRules("||cname-cloak-endpoint.tracking.com^");
  GURL tab_url = embedded_test_server()->GetURL("a.com", kAdBlockTestPage);
  GURL direct_resource_url =
      embedded_test_server()->GetURL("a83idbka2e.a.com", "/logo.png");
  GURL chain_resource_url =
      embedded_test_server()->GetURL("b94jeclb3f.a.com", "/logo.png");
  GURL safe_resource_url = embedded_test_server()->GetURL("a.com", "/logo.png");
  GURL bad_resource_url = embedded_test_server()->GetURL(
      "cname-cloak-endpoint.tracking.com", "/logo.png");

  auto inner_resolver = std::make_unique<net::MockHostResolver>();

  const std::set<std::string> kDnsAliasesDirect(
      {"cname-cloak-endpoint.tracking.com"});
  const std::set<std::string> kDnsAliasesChain(
      {"cname-cloak-endpoint.tracking.com",
       "cname-cloak-endpoint.tracking.com.redirectservice.net", "cname.a.com"});
  const std::set<std::string> kDnsAliasesSafe({"assets.cdn.net"});
  inner_resolver->rules()->AddIPLiteralRuleWithDnsAliases(
      "a83idbka2e.a.com", "127.0.0.1", kDnsAliasesDirect);
  inner_resolver->rules()->AddIPLiteralRuleWithDnsAliases(
      "b94jeclb3f.a.com", "127.0.0.1", kDnsAliasesChain);
  inner_resolver->rules()->AddIPLiteralRuleWithDnsAliases("a.com", "127.0.0.1",
                                                          kDnsAliasesSafe);
  inner_resolver->rules()->AddIPLiteralRuleWithDnsAliases(
      "cname-cloak-endpoint.tracking.com", "127.0.0.1",
      /*dns_aliases=*/std::set<std::string>());

  network::HostResolver resolver(inner_resolver.get(), net::NetLog::Get());

  brave::SetAdblockCnameHostResolverForTesting(&resolver);

  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  // Image request to an unblocked first-party endpoint that is CNAME cloaked
  // with 1 alias. The alias has a matching rule, so the request should be
  // blocked.
  ASSERT_EQ(true, EvalJs(contents, base::StringPrintf(
                                       "setExpectations(0, 1, 0, 0);"
                                       "addImage('%s')",
                                       direct_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
  // Note one resolution for the root document
  ASSERT_EQ(2ULL, inner_resolver->num_resolve());

  // XHR request to an unblocked first-party endpoint that is CNAME cloaked with
  // multiple intermediate aliases. The canonical alias has a matching rule, so
  // the request should be blocked.
  ASSERT_EQ(true, EvalJs(contents, base::StringPrintf(
                                       "setExpectations(0, 1, 0, 1);"
                                       "xhr('%s')",
                                       chain_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);
  ASSERT_EQ(3ULL, inner_resolver->num_resolve());

  // XHR request to an unblocked first-party endpoint that is CNAME cloaked.
  // The canonical alias has no matching rule, so the request should be allowed.
  ASSERT_EQ(true, EvalJs(contents,
                         base::StringPrintf("setExpectations(0, 1, 1, 1);"
                                            "xhr('%s')",
                                            safe_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);
  ASSERT_EQ(4ULL, inner_resolver->num_resolve());

  // XHR request directly to a blocked third-party endpoint.
  // The resolver should not be queried for this request.
  ASSERT_EQ(true, EvalJs(contents,
                         base::StringPrintf("setExpectations(0, 1, 1, 2);"
                                            "xhr('%s')",
                                            bad_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 3ULL);
  ASSERT_EQ(4ULL, inner_resolver->num_resolve());
}

// Make sure that an exception for a URL can apply to a blocking decision made
// to its CNAME-uncloaked equivalent.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       MAYBE_CnameCloakedRequestsCanBeExcepted) {
  UpdateAdBlockInstanceWithRules(
      "||cname-cloak-endpoint.tracking.com^\n"
      "@@a.com*/logo.png?unblock^");
  GURL tab_url = embedded_test_server()->GetURL("a.com", kAdBlockTestPage);
  GURL direct_resource_url =
      embedded_test_server()->GetURL("a83idbka2e.a.com", "/logo.png");
  GURL chain_resource_url =
      embedded_test_server()->GetURL("b94jeclb3f.a.com", "/logo.png");
  GURL safe_resource_url = embedded_test_server()->GetURL("a.com", "/logo.png");
  GURL bad_resource_url = embedded_test_server()->GetURL(
      "cname-cloak-endpoint.tracking.com", "/logo.png");
  GURL excepted_resource_url =
      embedded_test_server()->GetURL("a.com", "/logo.png?unblock");

  auto inner_resolver = std::make_unique<net::MockHostResolver>();

  const std::set<std::string> kDnsAliasesDirect(
      {"cname-cloak-endpoint.tracking.com"});
  const std::set<std::string> kDnsAliasesChain(
      {"cname-cloak-endpoint.tracking.com",
       "cname-cloak-endpoint.tracking.com.redirectservice.net", "cname.a.com"});
  const std::set<std::string> kDnsAliasesSafe({"assets.cdn.net"});
  inner_resolver->rules()->AddIPLiteralRuleWithDnsAliases(
      "a83idbka2e.a.com", "127.0.0.1", kDnsAliasesDirect);
  inner_resolver->rules()->AddIPLiteralRuleWithDnsAliases(
      "b94jeclb3f.a.com", "127.0.0.1", kDnsAliasesChain);
  inner_resolver->rules()->AddIPLiteralRuleWithDnsAliases("a.com", "127.0.0.1",
                                                          kDnsAliasesSafe);
  inner_resolver->rules()->AddIPLiteralRuleWithDnsAliases(
      "cname-cloak-endpoint.tracking.com", "127.0.0.1",
      /*dns_aliases=*/std::set<std::string>());

  network::HostResolver resolver(inner_resolver.get(), net::NetLog::Get());

  brave::SetAdblockCnameHostResolverForTesting(&resolver);

  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  // Image request to an unblocked first-party endpoint that is CNAME cloaked
  // with 1 alias. The alias has a matching rule, so the request should be
  // blocked.
  ASSERT_EQ(true, EvalJs(contents, base::StringPrintf(
                                       "setExpectations(0, 1, 0, 0);"
                                       "addImage('%s')",
                                       direct_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
  // Note one resolution for the root document
  ASSERT_EQ(2ULL, inner_resolver->num_resolve());

  // XHR request to an unblocked first-party endpoint that is CNAME cloaked with
  // multiple intermediate aliases. The canonical alias has a matching rule, so
  // the request should be blocked.
  ASSERT_EQ(true, EvalJs(contents, base::StringPrintf(
                                       "setExpectations(0, 1, 0, 1);"
                                       "xhr('%s')",
                                       chain_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);
  ASSERT_EQ(3ULL, inner_resolver->num_resolve());

  // XHR request to an unblocked first-party endpoint that is CNAME cloaked.
  // The canonical alias has no matching rule, so the request should be allowed.
  ASSERT_EQ(true, EvalJs(contents,
                         base::StringPrintf("setExpectations(0, 1, 1, 1);"
                                            "xhr('%s')",
                                            safe_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);
  ASSERT_EQ(4ULL, inner_resolver->num_resolve());

  // XHR request directly to a blocked third-party endpoint.
  // The resolver should not be queried for this request.
  ASSERT_EQ(true, EvalJs(contents,
                         base::StringPrintf("setExpectations(0, 1, 1, 2);"
                                            "xhr('%s')",
                                            bad_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 3ULL);
  ASSERT_EQ(4ULL, inner_resolver->num_resolve());

  // The original URL only matches an exception.
  // The CNAME'd URL only matches a blocking rule.
  // The resolver should be queried for this request, and the resource should
  // not be blocked.
  ASSERT_EQ(true, EvalJs(contents, base::StringPrintf(
                                       "setExpectations(0, 1, 2, 2);"
                                       "xhr('%s')",
                                       excepted_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 3ULL);
  ASSERT_EQ(5ULL, inner_resolver->num_resolve());
}

class CnameUncloakingFlagDisabledTest : public AdBlockServiceTest {
 public:
  CnameUncloakingFlagDisabledTest() {
    feature_list_.InitAndDisableFeature(kBraveAdblockCnameUncloaking);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Make sure that CNAME uncloaking does not occur when the CNAME uncloaking
// flag is disabled.
IN_PROC_BROWSER_TEST_F(CnameUncloakingFlagDisabledTest, NoDnsQueriesIssued) {
  UpdateAdBlockInstanceWithRules("||cname-cloak-endpoint.tracking.com^");
  GURL tab_url = embedded_test_server()->GetURL("a.com", kAdBlockTestPage);
  GURL direct_resource_url =
      embedded_test_server()->GetURL("a83idbka2e.a.com", "/logo.png");
  GURL chain_resource_url =
      embedded_test_server()->GetURL("b94jeclb3f.a.com", "/logo.png");
  GURL safe_resource_url = embedded_test_server()->GetURL("a.com", "/logo.png");
  GURL bad_resource_url = embedded_test_server()->GetURL(
      "cname-cloak-endpoint.tracking.com", "/logo.png");

  auto inner_resolver = std::make_unique<net::MockHostResolver>();

  const std::set<std::string> kDnsAliasesDirect(
      {"cname-cloak-endpoint.tracking.com"});
  const std::set<std::string> kDnsAliasesChain(
      {"cname-cloak-endpoint.tracking.com",
       "cname-cloak-endpoint.tracking.com.redirectservice.net", "cname.a.com"});
  const std::set<std::string> kDnsAliasesSafe({"assets.cdn.net"});
  inner_resolver->rules()->AddIPLiteralRuleWithDnsAliases(
      "a83idbka2e.a.com", "127.0.0.1", kDnsAliasesDirect);
  inner_resolver->rules()->AddIPLiteralRuleWithDnsAliases(
      "b94jeclb3f.a.com", "127.0.0.1", kDnsAliasesChain);
  inner_resolver->rules()->AddIPLiteralRuleWithDnsAliases("a.com", "127.0.0.1",
                                                          kDnsAliasesSafe);
  inner_resolver->rules()->AddIPLiteralRuleWithDnsAliases(
      "cname-cloak-endpoint.tracking.com", "127.0.0.1",
      /*dns_aliases=*/std::set<std::string>());

  network::HostResolver resolver(inner_resolver.get(), net::NetLog::Get());

  brave::SetAdblockCnameHostResolverForTesting(&resolver);

  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  // Image request to an unblocked first-party endpoint that is CNAME cloaked
  // with 1 alias. Nothing should be blocked.
  ASSERT_EQ(true, EvalJs(contents, base::StringPrintf(
                                       "setExpectations(1, 0, 0, 0);"
                                       "addImage('%s')",
                                       direct_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
  // Note one resolution for the root document
  ASSERT_EQ(0ULL, inner_resolver->num_resolve());

  // Image request to an unblocked first-party endpoint that is CNAME cloaked
  // with multiple intermediate aliases. Nothing should be blocked.
  ASSERT_EQ(true, EvalJs(contents, base::StringPrintf(
                                       "setExpectations(2, 0, 0, 0);"
                                       "addImage('%s')",
                                       chain_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
  ASSERT_EQ(0ULL, inner_resolver->num_resolve());

  // XHR request to an unblocked first-party endpoint that is CNAME cloaked.
  // Nothing should be blocked.
  ASSERT_EQ(true, EvalJs(contents,
                         base::StringPrintf("setExpectations(2, 0, 1, 0);"
                                            "xhr('%s')",
                                            safe_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
  ASSERT_EQ(0ULL, inner_resolver->num_resolve());

  // XHR request directly to a blocked third-party endpoint. It should be
  // blocked, but the resolver still should not be queried.
  ASSERT_EQ(true, EvalJs(contents,
                         base::StringPrintf("setExpectations(2, 0, 1, 1);"
                                            "xhr('%s')",
                                            bad_resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
  ASSERT_EQ(0ULL, inner_resolver->num_resolve());
}

// Load an image from a specific subdomain, and make sure it is blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, BlockNYP) {
  UpdateAdBlockInstanceWithRules("||sp1.nypost.com$third-party");
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  GURL resource_url =
      embedded_test_server()->GetURL("sp1.nypost.com", "/logo.png");
  NavigateToURL(tab_url);
  content::WebContents* contents = web_contents();
  ASSERT_EQ(true,
            EvalJs(contents, base::StringPrintf("setExpectations(0, 1, 0, 0);"
                                                "addImage('%s')",
                                                resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Frame root URL is used for context rather than the tab URL
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, FrameSourceURL) {
  UpdateAdBlockInstanceWithRules("adbanner.js$domain=a.com");
  GURL url = embedded_test_server()->GetURL("a.com", "/iframe_blocking.html");
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(ChildFrameAt(contents, 0),
                         "setExpectations(0, 0, 1, 0);"
                         "xhr('adbanner.js?1')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  UpdateAdBlockInstanceWithRules("adbanner.js$domain=b.com");
  NavigateToURL(url);
  contents = web_contents();

  ASSERT_EQ(true, EvalJs(ChildFrameAt(contents, 0),
                         "setExpectations(0, 0, 0, 1);"
                         "xhr('adbanner.js?1')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Tags for social buttons work
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, SocialButttonAdBlockTagTest) {
  UpdateAdBlockInstanceWithRules(
      base::StringPrintf("||example.com^$tag=%s",
                         brave_shields::kFacebookEmbeds)
          .c_str());
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  g_brave_browser_process->ad_block_service()->EnableTag(
      brave_shields::kFacebookEmbeds, true);
  WaitForAdBlockServiceThreads();
  GURL resource_url =
      embedded_test_server()->GetURL("example.com", "/logo.png");
  NavigateToURL(tab_url);
  content::WebContents* contents = web_contents();
  ASSERT_EQ(true,
            EvalJs(contents, base::StringPrintf("setExpectations(0, 1, 0, 0);"
                                                "addImage('%s')",
                                                resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Lack of tags for social buttons work
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, SocialButttonAdBlockDiffTagTest) {
  UpdateAdBlockInstanceWithRules("||example.com^$tag=sup");
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  g_brave_browser_process->ad_block_service()->EnableTag(
      brave_shields::kFacebookEmbeds, true);
  WaitForAdBlockServiceThreads();
  GURL resource_url =
      embedded_test_server()->GetURL("example.com", "/logo.png");
  NavigateToURL(tab_url);
  content::WebContents* contents = web_contents();
  ASSERT_EQ(true,
            EvalJs(contents, base::StringPrintf("setExpectations(1, 0, 0, 0);"
                                                "addImage('%s')",
                                                resource_url.spec().c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
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
  auto* prefs = profile()->GetPrefs();

  // Default tags exist on startup
  AssertTagExists(brave_shields::kFacebookEmbeds, true);
  AssertTagExists(brave_shields::kTwitterEmbeds, true);
  AssertTagExists(brave_shields::kLinkedInEmbeds, false);

  // Toggling prefs once is reflected in the adblock client.
  prefs->SetBoolean(brave_shields::prefs::kLinkedInEmbedControlType, true);
  WaitForAdBlockServiceThreads();
  AssertTagExists(brave_shields::kFacebookEmbeds, true);
  AssertTagExists(brave_shields::kTwitterEmbeds, true);
  AssertTagExists(brave_shields::kLinkedInEmbeds, true);

  prefs->SetBoolean(brave_shields::prefs::kFBEmbedControlType, false);
  WaitForAdBlockServiceThreads();
  AssertTagExists(brave_shields::kFacebookEmbeds, false);
  AssertTagExists(brave_shields::kTwitterEmbeds, true);
  AssertTagExists(brave_shields::kLinkedInEmbeds, true);

  prefs->SetBoolean(brave_shields::prefs::kTwitterEmbedControlType, false);
  WaitForAdBlockServiceThreads();
  AssertTagExists(brave_shields::kFacebookEmbeds, false);
  AssertTagExists(brave_shields::kTwitterEmbeds, false);
  AssertTagExists(brave_shields::kLinkedInEmbeds, true);

  // Toggling prefs back is reflected in the adblock client.
  prefs->SetBoolean(brave_shields::prefs::kLinkedInEmbedControlType, false);
  WaitForAdBlockServiceThreads();
  AssertTagExists(brave_shields::kFacebookEmbeds, false);
  AssertTagExists(brave_shields::kTwitterEmbeds, false);
  AssertTagExists(brave_shields::kLinkedInEmbeds, false);

  prefs->SetBoolean(brave_shields::prefs::kFBEmbedControlType, true);
  WaitForAdBlockServiceThreads();
  AssertTagExists(brave_shields::kFacebookEmbeds, true);
  AssertTagExists(brave_shields::kTwitterEmbeds, false);
  AssertTagExists(brave_shields::kLinkedInEmbeds, false);

  prefs->SetBoolean(brave_shields::prefs::kTwitterEmbedControlType, true);
  WaitForAdBlockServiceThreads();
  AssertTagExists(brave_shields::kFacebookEmbeds, true);
  AssertTagExists(brave_shields::kTwitterEmbeds, true);
  AssertTagExists(brave_shields::kLinkedInEmbeds, false);
}

// Load a page with a blocked image, and make sure it is collapsed.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CollapseBlockedImage) {
  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  // There is no way for JS to directly tell if an element has been collapsed,
  // but the clientHeight property is zero for collapsed elements and nonzero
  // otherwise.
  EXPECT_EQ(true, EvalJs(contents,
                         "let i = document.getElementsByClassName('adImage');"
                         "i[0].clientHeight === 0"));
}

// Load a page with a blocked iframe, and make sure it is collapsed.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CollapseBlockedIframe) {
  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(contents, "addFrame('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  // There is no way for JS to directly tell if an element has been collapsed,
  // but the clientHeight property is zero for collapsed elements and nonzero
  // otherwise.
  EXPECT_EQ(true, EvalJs(contents,
                         "let i = document.getElementsByClassName('adFrame');"
                         "i[0].clientHeight === 0"));
}

class CollapseBlockedElementsFlagDisabledTest : public AdBlockServiceTest {
 public:
  CollapseBlockedElementsFlagDisabledTest() {
    feature_list_.InitAndDisableFeature(kBraveAdblockCollapseBlockedElements);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Load a page with a blocked image, and make sure it is not collapsed.
IN_PROC_BROWSER_TEST_F(CollapseBlockedElementsFlagDisabledTest,
                       DontCollapseBlockedImage) {
  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  // There is no way for JS to directly tell if an element has been collapsed,
  // but the clientHeight property is zero for collapsed elements and nonzero
  // otherwise.
  EXPECT_EQ(true, EvalJs(contents,
                         "let i = document.getElementsByClassName('adImage');"
                         "i[0].clientHeight !== 0"));
}

// Load a page with a blocked iframe, and make sure it is not collapsed.
IN_PROC_BROWSER_TEST_F(CollapseBlockedElementsFlagDisabledTest,
                       DontCollapseBlockedIframe) {
  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(contents, "addFrame('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  // There is no way for JS to directly tell if an element has been collapsed,
  // but the clientHeight property is zero for collapsed elements and nonzero
  // otherwise.
  EXPECT_EQ(true, EvalJs(contents,
                         "let i = document.getElementsByClassName('adFrame');"
                         "i[0].clientHeight !== 0"));
}

class Default1pBlockingFlagDisabledTest : public AdBlockServiceTest {
 public:
  Default1pBlockingFlagDisabledTest() {
    feature_list_.InitAndDisableFeature(kBraveAdblockDefault1pBlocking);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Load a page with an image from a first party and a third party, which both
// match the same filter in the default engine. Ensure the third-party one is
// blocked while the first-party one is allowed.
IN_PROC_BROWSER_TEST_F(Default1pBlockingFlagDisabledTest, Default1pBlocking) {
  DisableAggressiveMode();
  UpdateAdBlockInstanceWithRules("^ad_banner.png");

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(1, 0, 0, 0);"
                         "addImage('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(1, 1, 0, 0);"
                         "addImage('https://thirdparty.com/ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Load a page with an image from a first party and a third party, which both
// match the same filter in the default engine. They should both be blocked on
// special URLs like this one.
IN_PROC_BROWSER_TEST_F(Default1pBlockingFlagDisabledTest, SpecialUrlException) {
  DisableAggressiveMode();
  UpdateAdBlockInstanceWithRules("^ad_banner.png");

  // Must use HTTPS because `youtube.com` is in Chromium's HSTS preload list
  GURL url = https_server_.GetURL("youtube.com", kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 2, 0, 0);"
                         "addImage('https://thirdparty.com/ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);
}

// Load a page with an image from a first party and a third party, which both
// match the same filter in the default engine. Enable aggressive mode, and
// ensure that both are blocked.
IN_PROC_BROWSER_TEST_F(Default1pBlockingFlagDisabledTest,
                       Aggressive1pBlocking) {
  UpdateAdBlockInstanceWithRules("^ad_banner.png");

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 2, 0, 0);"
                         "addImage('https://thirdparty.com/ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);
}

// Load a page with an image from a first party and a third party, which both
// match the same filter in the custom filters engine. Ensure that both are
// blocked.
IN_PROC_BROWSER_TEST_F(Default1pBlockingFlagDisabledTest, Custom1pBlocking) {
  DisableAggressiveMode();
  UpdateCustomAdBlockInstanceWithRules("^ad_banner.png");

  GURL url = embedded_test_server()->GetURL(kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 1, 0, 0);"
                         "addImage('ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  ASSERT_EQ(true, EvalJs(contents,
                         "setExpectations(0, 2, 0, 0);"
                         "addImage('https://thirdparty.com/ad_banner.png')"));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 2ULL);
}

// Load a page with a script which uses a redirect data URL.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, RedirectRulesAreRespected) {
  UpdateAdBlockResources(R"(
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
  UpdateAdBlockInstanceWithRules("js_mock_me.js$redirect=noopjs");

  const GURL url =
      embedded_test_server()->GetURL("example.com", kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  const std::string noopjs = "(function() {\\n    \\'use strict\\';\\n})();\\n";
  const GURL resource_url =
      embedded_test_server()->GetURL("example.com", "/js_mock_me.js");
  ASSERT_EQ(true,
            EvalJs(contents, base::StringPrintf(
                                 "setExpectations(0, 0, 1, 0);"
                                 "xhr_expect_content('%s', '%s');",
                                 resource_url.spec().c_str(), noopjs.c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// A redirection should only be applied if there's also a matching blocking
// rule.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, RedirectWithoutBlockIsNoop) {
  std::string resources = R"([{
        "name": "noop.js",
        "aliases": ["noopjs"],
        "kind": {
          "mime":"application/javascript"
        },
        "content": "KGZ1bmN0aW9uKCkgewogICAgJ3VzZSBzdHJpY3QnOwp9KSgpOwo="
      }])";
  UpdateAdBlockResources(resources);
  UpdateAdBlockInstanceWithRules(
      ".js?block=true\n"
      "js_mock_me.js$redirect-rule=noopjs");

  const GURL url =
      embedded_test_server()->GetURL("example.com", kAdBlockTestPage);
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  const std::string noopjs = "(function() {\\n    \\'use strict\\';\\n})();\\n";
  const GURL resource_url_1 = embedded_test_server()->GetURL(
      "example.com", "/js_mock_me.js?block=true");
  ASSERT_EQ(true, EvalJs(contents,
                         base::StringPrintf("setExpectations(0, 0, 1, 0);"
                                            "xhr_expect_content('%s', '%s');",
                                            resource_url_1.spec().c_str(),
                                            noopjs.c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);

  const std::string actual_content = "testing\\n";
  const GURL resource_url_2 =
      embedded_test_server()->GetURL("example.com", "/js_mock_me.js");
  ASSERT_EQ(true, EvalJs(contents,
                         base::StringPrintf("setExpectations(0, 0, 2, 0);"
                                            "xhr_expect_content('%s', '%s');",
                                            resource_url_2.spec().c_str(),
                                            actual_content.c_str())));
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

std::unique_ptr<net::test_server::HttpResponse> NoParamHandler(
    const net::test_server::HttpRequest& request) {
  const GURL request_url = request.GetURL();

  auto http_response = std::make_unique<net::test_server::BasicHttpResponse>();
  http_response->set_code(net::HttpStatusCode::HTTP_OK);

  if (request_url.has_query()) {
    // Should not happen, abort test
    CHECK(false);
    return nullptr;
  } else {
    std::string body =
        "<html><head><script>window.success = "
        "true;</script></head><body><p>test</p></body></html>";
    http_response->set_content(body);
    http_response->set_content_type("text/html");
    return http_response;
  }
}

// `$removeparam` should be respected for subresource requests
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, RemoveparamSubresource) {
  UpdateAdBlockInstanceWithRules("*$subdocument,removeparam=evil");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  GURL frame_url = embedded_test_server()->GetURL(
      "frame.com", "/cosmetic_frame.html?evil=true&test=true");
  content::NavigateIframeToURL(contents, "iframe", frame_url);

  ASSERT_EQ(nullptr,
            content::FrameMatchingPredicateOrNullptr(
                contents->GetPrimaryPage(),
                base::BindRepeating(content::FrameHasSourceUrl, frame_url)));

  GURL redirected_frame_url = embedded_test_server()->GetURL(
      "frame.com", "/cosmetic_frame.html?test=true");

  content::RenderFrameHost* inner_frame = content::FrameMatchingPredicate(
      contents->GetPrimaryPage(),
      base::BindRepeating(content::FrameHasSourceUrl, redirected_frame_url));

  ASSERT_EQ("?test=true", EvalJs(inner_frame, "window.location.search"));
}

// ui_test_utils::NavigateToURL isn't available on Android, but
// content::NavigateToURLBlockUntilNavigationsComplete doesn't work with these
// redirected navigations.
#if BUILDFLAG(IS_ANDROID)
#define MAYBE_RemoveparamTopLevelNavigation \
  DISABLED_RemoveparamTopLevelNavigation
#define MAYBE_DefaultRemoveparamFromCustom DISABLED_DefaultRemoveparamFromCustom
#else  // BUILDFLAG(IS_ANDROID)
#define MAYBE_RemoveparamTopLevelNavigation RemoveparamTopLevelNavigation
#define MAYBE_DefaultRemoveparamFromCustom DefaultRemoveparamFromCustom
#endif  // BUILDFLAG(IS_ANDROID)

// `$removeparam` should be respected for top-level navigations
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       MAYBE_RemoveparamTopLevelNavigation) {
  UpdateAdBlockInstanceWithRules("*$document,removeparam=evil");

  dynamic_server_.RegisterRequestHandler(base::BindRepeating(&NoParamHandler));
  ASSERT_TRUE(dynamic_server_.Start());

  GURL original_url = dynamic_server_.GetURL("/?evil=true");
  GURL landing_url = dynamic_server_.GetURL("/");
#if !BUILDFLAG(IS_ANDROID)
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), original_url));
#endif
  content::WebContents* contents = web_contents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(contents->GetLastCommittedURL(), landing_url);

  ASSERT_EQ(true, EvalJs(contents, "window.success"));

  ASSERT_TRUE(dynamic_server_.ShutdownAndWaitUntilComplete());
}

// `$removeparam` should not be activated in default blocking mode
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, DefaultNoRemoveparam) {
  DisableAggressiveMode();

  UpdateAdBlockInstanceWithRules("*$subdocument,removeparam=evil");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  GURL frame_url = embedded_test_server()->GetURL(
      "frame.com", "/cosmetic_frame.html?evil=true&test=true");
  content::NavigateIframeToURL(contents, "iframe", frame_url);

  ASSERT_NE(nullptr,
            content::FrameMatchingPredicateOrNullptr(
                contents->GetPrimaryPage(),
                base::BindRepeating(content::FrameHasSourceUrl, frame_url)));
}

// `$removeparam` should still be activated in default blocking mode if it comes
// from custom filters
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, MAYBE_DefaultRemoveparamFromCustom) {
  DisableAggressiveMode();

  UpdateCustomAdBlockInstanceWithRules("*$subdocument,removeparam=evil");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
#if !BUILDFLAG(IS_ANDROID)
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), tab_url));
#endif

  content::WebContents* contents = web_contents();

  GURL frame_url = embedded_test_server()->GetURL(
      "frame.com", "/cosmetic_frame.html?evil=true&test=true");
  content::NavigateIframeToURL(contents, "iframe", frame_url);

  ASSERT_EQ(nullptr,
            content::FrameMatchingPredicateOrNullptr(
                contents->GetPrimaryPage(),
                base::BindRepeating(content::FrameHasSourceUrl, frame_url)));

  GURL redirected_frame_url = embedded_test_server()->GetURL(
      "frame.com", "/cosmetic_frame.html?test=true");

  content::RenderFrameHost* inner_frame = content::FrameMatchingPredicate(
      contents->GetPrimaryPage(),
      base::BindRepeating(content::FrameHasSourceUrl, redirected_frame_url));

  ASSERT_EQ("?test=true", EvalJs(inner_frame, "window.location.search"));
}

// Verify that scripts violating a Content Security Policy from a `$csp` rule
// are not loaded.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CspRule) {
  UpdateAdBlockInstanceWithRules(
      "||example.com^$csp=script-src 'nonce-abcdef' 'unsafe-eval' 'self'");

  const GURL url =
      embedded_test_server()->GetURL("example.com", "/csp_rules.html");
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  auto res = EvalJs(contents, "await window.allLoaded");
  EXPECT_EQ(true, EvalJs(contents, "!!window.loadedNonceScript"));
  EXPECT_EQ(true, EvalJs(contents, "!!window.loadedEvalScript"));
  EXPECT_EQ(true, EvalJs(contents, "!!window.loadedSamePartyScript"));
  EXPECT_EQ(false, EvalJs(contents, "!!window.loadedThirdPartyScript"));
  EXPECT_EQ(false, EvalJs(contents, "!!window.loadedUnsafeInlineScript"));
  EXPECT_EQ(true, EvalJs(contents, "!!window.loadedDataImage"));

  // Violations of injected CSP directives do not increment the Shields counter
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Verify that Content Security Policies from multiple `$csp` rules are
// combined.
//
// The policy resulting from two of the same kind of directive will be the
// union of both.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CspRuleMerging) {
  UpdateAdBlockInstanceWithRules(
      "||example.com^$csp=script-src 'nonce-abcdef' 'unsafe-eval' 'self'");
  UpdateCustomAdBlockInstanceWithRules(
      "||example.com^$csp=img-src 'none'\n"
      "||sub.example.com^$csp=script-src 'nonce-abcdef' "
      "'unsafe-eval' 'unsafe-inline'");

  const GURL url =
      embedded_test_server()->GetURL("sub.example.com", "/csp_rules.html");
  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  auto res = EvalJs(contents, "await window.allLoaded");
  EXPECT_EQ(true, EvalJs(contents, "!!window.loadedNonceScript"));
  EXPECT_EQ(true, EvalJs(contents, "!!window.loadedEvalScript"));
  EXPECT_EQ(false, EvalJs(contents, "!!window.loadedSamePartyScript"));
  EXPECT_EQ(false, EvalJs(contents, "!!window.loadedThirdPartyScript"));
  EXPECT_EQ(false, EvalJs(contents, "!!window.loadedUnsafeInlineScript"));
  EXPECT_EQ(false, EvalJs(contents, "!!window.loadedDataImage"));

  // Violations of injected CSP directives do not increment the Shields counter
  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

// Lists with `hidden` set to `true` should not be shown in `GetRegionalLists`.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, HiddenListsNotPresented) {
  std::vector<brave_shields::FilterListCatalogEntry> filter_list_catalog;
  filter_list_catalog.push_back(brave_shields::FilterListCatalogEntry(
      "uuid1", "https://example.com", "Hidden list", {},
      "https://support.example.com", "first list", true, false, false, 0, {},
      "testid1", "pubkey1"));
  filter_list_catalog.push_back(brave_shields::FilterListCatalogEntry(
      "uuid2", "https://example.com", "Normal list", {},
      "https://support.example.com", "second list", false, false, false, 0, {},
      "testid2", "pubkey2"));
  g_brave_browser_process->ad_block_service()
      ->component_service_manager()
      ->SetFilterListCatalog(filter_list_catalog);

  auto regional_lists = g_brave_browser_process->ad_block_service()
                            ->component_service_manager()
                            ->GetRegionalLists();

  ASSERT_EQ(regional_lists.size(), 1UL);
}

// Verify that scripts violating a Content Security Policy from a `$csp` rule
// are not loaded.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CspRuleShieldsDown) {
  UpdateAdBlockInstanceWithRules(
      "||example.com^$csp=script-src 'nonce-abcdef' 'unsafe-eval' 'self'");

  const GURL url =
      embedded_test_server()->GetURL("example.com", "/csp_rules.html");
  ShieldsDown(url);

  NavigateToURL(url);
  content::WebContents* contents = web_contents();

  auto res = EvalJs(contents, "await window.allLoaded");
  EXPECT_EQ(true, EvalJs(contents, "!!window.loadedNonceScript"));
  EXPECT_EQ(true, EvalJs(contents, "!!window.loadedEvalScript"));
  EXPECT_EQ(true, EvalJs(contents, "!!window.loadedSamePartyScript"));
  EXPECT_EQ(true, EvalJs(contents, "!!window.loadedThirdPartyScript"));
  EXPECT_EQ(true, EvalJs(contents, "!!window.loadedUnsafeInlineScript"));
  EXPECT_EQ(true, EvalJs(contents, "!!window.loadedDataImage"));

  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

class CosmeticFilteringFlagDisabledTest : public AdBlockServiceTest {
 public:
  CosmeticFilteringFlagDisabledTest() {
    feature_list_.InitAndDisableFeature(kBraveAdblockCosmeticFiltering);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Ensure no cosmetic filtering occurs when the feature flag is disabled
IN_PROC_BROWSER_TEST_F(CosmeticFilteringFlagDisabledTest,
                       CosmeticFilteringSimple) {
  UpdateAdBlockInstanceWithRules(
      "b.com###ad-banner\n"
      "##.ad");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "checkSelector('#ad-banner', 'display', 'block')"));

  ASSERT_EQ(true, EvalJs(contents,
                         "checkSelector('.ad-banner', 'display', 'block')"));

  ASSERT_EQ(true, EvalJs(contents, "checkSelector('.ad', 'display', 'block')"));
}

#if BUILDFLAG(ENABLE_PLAYLIST)

class CosmeticFilteringPlaylistFlagEnabledTest : public AdBlockServiceTest {
 public:
  CosmeticFilteringPlaylistFlagEnabledTest() {
    feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Check cosmetic filtering is applied to any loading from Playlist's
// background web contents.
IN_PROC_BROWSER_TEST_F(CosmeticFilteringPlaylistFlagEnabledTest,
                       AllowCosmeticFiltering) {
  const GURL url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");

  // Set cosmetic filtering is not enabled.
  brave_shields::SetCosmeticFilteringControlType(
      content_settings(), brave_shields::ControlType::ALLOW, url);
  UpdateAdBlockInstanceWithRules("b.com###ad-banner");

  playlist::PlaylistBackgroundWebContentses background_web_contentses(
      profile(),
      playlist::PlaylistServiceFactory::GetForBrowserContext(profile()));
  background_web_contentses.Add(url, base::DoNothing());

  auto* web_contents = &background_web_contentses.web_contents();
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));

  // Check filter is applied properly.
  EXPECT_EQ(false, EvalJs(web_contents,
                          "checkSelector('#ad-banner', 'display', 'block')"));
}

#endif

// Ensure no cosmetic filtering occurs when the shields setting is disabled
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringDisabled) {
  brave_shields::SetCosmeticFilteringControlType(
      content_settings(), brave_shields::ControlType::ALLOW, GURL());
  UpdateAdBlockInstanceWithRules(
      "b.com###ad-banner\n"
      "##.ad");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "checkSelector('#ad-banner', 'display', 'block')"));

  ASSERT_EQ(true, EvalJs(contents,
                         "checkSelector('.ad-banner', 'display', 'block')"));

  ASSERT_EQ(true, EvalJs(contents, "checkSelector('.ad', 'display', 'block')"));
}

// Test simple cosmetic filtering
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringSimple) {
  UpdateAdBlockInstanceWithRules(
      "b.com###ad-banner\n"
      "##.ad");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  auto result_first =
      EvalJs(contents, R"(waitCSSSelector('#ad-banner', 'display', 'none'))");
  ASSERT_TRUE(result_first.error.empty());
  EXPECT_EQ(base::Value(true), result_first.value);

  auto result_second =
      EvalJs(contents, R"(waitCSSSelector('.ad-banner', 'display', 'block'))");
  ASSERT_TRUE(result_second.error.empty());
  EXPECT_EQ(base::Value(true), result_second.value);

  auto result_third =
      EvalJs(contents, R"(waitCSSSelector('.ad', 'display', 'none'))");
  ASSERT_TRUE(result_third.error.empty());
  EXPECT_EQ(base::Value(true), result_third.value);
}

// Test that cosmetic filtering is applied independently in a third-party child
// frame
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringFrames) {
  UpdateAdBlockInstanceWithRules("frame.com##.ad\n");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  GURL frame_url =
      embedded_test_server()->GetURL("frame.com", "/cosmetic_frame.html");
  content::NavigateIframeToURL(contents, "iframe", frame_url);

  content::RenderFrameHost* inner_frame = content::FrameMatchingPredicate(
      contents->GetPrimaryPage(),
      base::BindRepeating(content::FrameHasSourceUrl, frame_url));

  auto frame_result = EvalJs(inner_frame, R"(
          const testdiv = document.getElementById('testdiv');
          console.error(testdiv);
          const style = window.getComputedStyle(testdiv);
          style['display'] === 'none'
        )");
  ASSERT_TRUE(frame_result.error.empty());
  EXPECT_EQ(base::Value(true), frame_result.value);

  auto main_result =
      EvalJs(contents, R"(checkSelector('.ad', 'display', 'block'))");
  ASSERT_TRUE(main_result.error.empty());
  EXPECT_EQ(base::Value(true), main_result.value);
}

// Test cosmetic filtering ignores rules with the `:has` pseudoclass in standard
// blocking mode
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       CosmeticFilteringHasPseudoclassStandard) {
  DisableAggressiveMode();
  UpdateAdBlockInstanceWithRules("b.com##.container:has(#promotion)\n");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(contents,
                         "checkSelector('.container', 'display', 'block')"));
}

// Test cosmetic filtering applies rules with the `:has` pseudoclass in
// aggressive blocking mode
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       CosmeticFilteringHasPseudoclassAggressive) {
  UpdateAdBlockInstanceWithRules("b.com##.container:has(#promotion)\n");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  // the `#promotion` element's container is hidden
  EXPECT_EQ("none", EvalJs(contents,
                           "window.getComputedStyle(document.getElementById('"
                           "promotion').parentElement).display"));

  // the `#real-user-content` element's container is not hidden
  EXPECT_EQ("block", EvalJs(contents,
                            "window.getComputedStyle(document.getElementById('"
                            "real-user-content').parentElement).display"));

  // both inner elements have no new styles applied
  EXPECT_EQ(true, EvalJs(contents,
                         "checkSelector('#promotion', 'display', 'block')"));
  EXPECT_EQ(true,
            EvalJs(contents,
                   "checkSelector('#real-user-content', 'display', 'block')"));
}

// Test cosmetic filtering ignores content determined to be 1st party
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringProtect1p) {
  DisableAggressiveMode();
  UpdateAdBlockInstanceWithRules(
      "appspot.com##.fpsponsored\n"
      "appspot.com##.fpsponsored1\n"
      "appspot.com##.fpsponsored2\n"
      "appspot.com##.fpsponsored3\n"
      "appspot.com##.fpsponsored4\n"
      "appspot.com##.fpsponsored5\n");

  // *.appspot.com is used here to check the eTLD logic.
  // It's a private suffix from https://publicsuffix.org/list/
  GURL tab_url = embedded_test_server()->GetURL("test.lion.appspot.com",
                                                "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  EXPECT_EQ(true,
            EvalJs(contents,
                   "checkSelector('#relative-url-div', 'display', 'block')"));
  EXPECT_EQ(true,
            EvalJs(contents,
                   "checkSelector('#same-origin-div', 'display', 'block')"));
  EXPECT_EQ(
      true,
      EvalJs(contents, "checkSelector('#subdomain-div', 'display', 'block')"));
  EXPECT_EQ(true, EvalJs(contents,
                         "checkSelector('#same-etld', 'display', 'block')"));
  EXPECT_EQ(true, EvalJs(contents,
                         "checkSelector('#another-etld', 'display', 'none')"));
  EXPECT_EQ(true, EvalJs(contents,
                         "checkSelector('#another-etld-significant-text', "
                         "'display', 'block')"));
}

// Test cosmetic filtering bypasses 1st party checks in Aggressive mode
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringHide1pContent) {
  UpdateAdBlockInstanceWithRules("b.com##.fpsponsored\n");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  auto result =
      EvalJs(contents, R"(waitCSSSelector('.fpsponsored', 'display', 'none'))");
  ASSERT_TRUE(result.error.empty());
  EXPECT_EQ(base::Value(true), result.value);
}

// Test cosmetic filtering on elements added dynamically
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringDynamic) {
  UpdateAdBlockInstanceWithRules("##.blockme\n##.hide-innerhtml");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  auto result_first = EvalJs(contents,
                             R"(addElementsDynamically();
        waitCSSSelector('.blockme', 'display', 'none'))");
  ASSERT_TRUE(result_first.error.empty());
  EXPECT_EQ(base::Value(true), result_first.value);

  auto result_second = EvalJs(
      contents, R"(waitCSSSelector('.dontblockme', 'display', 'block'))");
  ASSERT_TRUE(result_second.error.empty());
  EXPECT_EQ(base::Value(true), result_second.value);

  // this class is added by setting an element's innerHTML, which doesn't
  // trigger a MutationObserver update
  auto result_third = EvalJs(
      contents, R"(waitCSSSelector('.hide-innerhtml', 'display', 'none'))");
  ASSERT_TRUE(result_third.error.empty());
  EXPECT_EQ(base::Value(true), result_third.value);
}

// Test cosmetic filtering on elements added dynamically, using a rule from the
// custom filters
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringDynamicCustom) {
  ASSERT_TRUE(g_brave_browser_process->ad_block_service()
                  ->custom_filters_provider()
                  ->UpdateCustomFilters("##.blockme"));

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  auto result_first = EvalJs(contents,
                             R"(addElementsDynamically();
        waitCSSSelector('.blockme', 'display', 'none'))");
  ASSERT_TRUE(result_first.error.empty());
  EXPECT_EQ(base::Value(true), result_first.value);

  auto result_second = EvalJs(
      contents, R"(waitCSSSelector('.dontblockme', 'display', 'block'))");
  ASSERT_TRUE(result_second.error.empty());
  EXPECT_EQ(base::Value(true), result_second.value);
}

// Test cosmetic filtering ignores generic cosmetic rules in the presence of a
// `generichide` exception rule, both for elements added dynamically and
// elements present at page load
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringGenerichide) {
  UpdateAdBlockInstanceWithRules(
      "##.blockme\n"
      "##img[src=\"https://example.com/logo.png\"]\n"
      "@@||b.com$generichide");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents,
                         "addElementsDynamically();\n"
                         "checkSelector('.blockme', 'display', 'inline')"));

  ASSERT_EQ(true,
            EvalJs(contents,
                   "checkSelector('img[src=\"https://example.com/logo.png\"]', "
                   "'display', 'inline')"));
}

// Test custom style rules
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringCustomStyle) {
  UpdateAdBlockInstanceWithRules("b.com##.ad:style(padding-bottom: 10px)");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  auto result =
      EvalJs(contents, R"(waitCSSSelector('.ad', 'padding-bottom', '10px'))");
  ASSERT_TRUE(result.error.empty());
  EXPECT_EQ(base::Value(true), result.value);
}

IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringRemoveStatic) {
  UpdateAdBlockInstanceWithRules(
      "b.com###ad-banner:remove()\n"
      "b.com##.whatever:remove()");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(contents, "check('#ad-banner', existence(false))"));
}

IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringRemoveDynamic) {
  UpdateAdBlockInstanceWithRules("b.com##.blockme:remove()");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  auto result = EvalJs(contents,
                       "addElementsDynamically();\n"
                       "wait('.dontblockme', existence(true)).then(() =>"
                       "wait('.blockme', existence(false)))");
  ASSERT_TRUE(result.error.empty());
  EXPECT_EQ(base::Value(true), result.value);
}

IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringRemoveAttribute) {
  UpdateAdBlockInstanceWithRules(
      "b.com##.ad img:remove-attr(something)\n"
      "b.com##.ad img:remove-attr(src)\n"
      "b.com##.ad img:remove-attr(nothing)\n"
      "b.com##img:remove-attr(whatever)");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(contents, "check('.ad img', attributes(['alt']))"));

  // Sanity check selector
  EXPECT_EQ(
      true,
      EvalJs(contents, "check('#relative-url-div img', attributes(['src']))"));
}

IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       CosmeticFilteringRemoveAttributeDynamic) {
  UpdateAdBlockInstanceWithRules("b.com##img.blockme:remove-attr(src)");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  auto result = EvalJs(contents,
                       "addElementsDynamically();\n"
                       "wait('img.blockme', attributes(['class']))");
  ASSERT_TRUE(result.error.empty());
  EXPECT_EQ(base::Value(true), result.value);
}

IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringRemoveClass) {
  UpdateAdBlockInstanceWithRules(
      "b.com##.ad:remove-class(ghi)\n"
      "b.com##div:remove-class(whatever)");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(contents, "check('.ghi', existence(false))"));
  EXPECT_EQ(true, EvalJs(contents, "check('.ad.jkl', classes(['ad', 'jkl']))"));
}

// `:remove` filters should still function correctly if `$generichide` is active
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringGenerichideRemove) {
  UpdateAdBlockInstanceWithRules(
      "||b.com^$generichide\n"
      "b.com###ad-banner:remove()\n"
      "b.com##.whatever:remove()\n"
      "b.com##.blockme:remove()");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(contents, "check('#ad-banner', existence(false))"));

  auto result = EvalJs(contents,
                       "addElementsDynamically();\n"
                       "wait('.dontblockme', existence(true)).then(() =>"
                       "wait('.blockme', existence(false)))");
  ASSERT_TRUE(result.error.empty());
  EXPECT_EQ(base::Value(true), result.value);
}

// Test rules overridden by hostname-specific exception rules
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringUnhide) {
  UpdateAdBlockInstanceWithRules(
      "##.ad\n"
      "b.com#@#.ad\n"
      "###ad-banner\n"
      "a.com#@##ad-banner");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  auto result_first =
      EvalJs(contents, R"(waitCSSSelector('.ad', 'display', 'block'))");
  ASSERT_TRUE(result_first.error.empty());
  EXPECT_EQ(base::Value(true), result_first.value);

  auto result_second =
      EvalJs(contents, R"(waitCSSSelector('#ad-banner', 'display', 'none'))");
  ASSERT_TRUE(result_second.error.empty());
  EXPECT_EQ(base::Value(true), result_second.value);
}

// Test scriptlet injection that modifies window attributes
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringWindowScriptlet) {
  std::string scriptlet =
      "(function() {"
      "  const send = window.getComputedStyle;"
      "  window.getComputedStyle = function(selector) {"
      "    return { 'color': 'Impossible value' };"
      "  }"
      "})();";
  UpdateAdBlockResources(
      "[{"
      "\"name\": \"hijacktest.js\","
      "\"aliases\": [\"hjt.js\"],"
      "\"kind\": {\"mime\": \"application/javascript\"},"
      "\"content\": \"" +
      base::Base64Encode(scriptlet) + "\"}]");
  UpdateAdBlockInstanceWithRules("b.com##+js(hjt)");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  auto result = EvalJs(
      contents, R"(waitCSSSelector('.ad', 'color', 'Impossible value'))");
  ASSERT_TRUE(result.error.empty());
  EXPECT_EQ(base::Value(true), result.value);
}

// Test that permissioned scriptlets can only be injected from appropriately
// permissioned lists
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, ScriptletInjectionPermissions) {
  std::string scriptlet =
      "(function() {"
      "  window.success = true;"
      "})();";
  std::string resources =
      "[{"
      "\"name\": \"set-success.js\","
      "\"aliases\": [],"
      "\"kind\": {\"mime\": \"application/javascript\"},"
      "\"permission\": 3,"  // i.e. 0b00000011
      "\"content\": \"" +
      base::Base64Encode(scriptlet) + "\"}]";
  std::string rules = "b.com##+js(set-success)";

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  content::WebContents* contents;

  UpdateAdBlockResources(resources);
  // Add the list with default (i.e. no) permissions
  UpdateAdBlockInstanceWithRules(rules);

  NavigateToURL(tab_url);
  contents = web_contents();

  {
    auto result = EvalJs(contents, R"(window.success === undefined)");
    EXPECT_EQ(base::Value(true), result.value);
  }

  // Add a list with different but still insufficient permissions
  AddNewRules(rules, 5, true);

  NavigateToURL(tab_url);
  contents = web_contents();

  {
    auto result = EvalJs(contents, R"(window.success === undefined)");
    EXPECT_EQ(base::Value(true), result.value);
  }

  // Finally add a list with sufficient permissions
  AddNewRules(rules, 7, true);

  NavigateToURL(tab_url);
  contents = web_contents();

  {
    auto result = EvalJs(contents, R"(window.success)");
    EXPECT_EQ(base::Value(true), result.value);
  }
}

class ScriptletDebugLogsFlagEnabledTest : public AdBlockServiceTest {
 public:
  ScriptletDebugLogsFlagEnabledTest() {
    feature_list_.InitAndEnableFeature(kBraveAdblockScriptletDebugLogs);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Test that scriptlet injection has access to `canDebug` inside of
// `scriptletGlobals`, and that it is set to `true`.
IN_PROC_BROWSER_TEST_F(ScriptletDebugLogsFlagEnabledTest, CanDebugSetToTrue) {
  std::string scriptlet =
      "(function() {"
      "  if (scriptletGlobals.get('canDebug') && scriptletGlobals.canDebug) {"
      "    window.success = true;"
      "  }"
      "})();";
  UpdateAdBlockResources(
      "[{"
      "\"name\": \"debuggable.js\","
      "\"aliases\": [],"
      "\"kind\": {\"mime\": \"application/javascript\"},"
      "\"content\": \"" +
      base::Base64Encode(scriptlet) + "\"}]");
  UpdateAdBlockInstanceWithRules("b.com##+js(debuggable)");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  EXPECT_EQ(true, EvalJs(contents, R"(window.success)"));
}

// Test scriptlet injection with DeAMP enabled
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CheckForDeAmpPref) {
  std::string scriptlet =
      "(function() {"
      " if (deAmpEnabled) {"
      "   window.getComputedStyle = function(selector) {"
      "     return { 'color': 'green' };"
      "   }"
      " } else {"
      "   window.getComputedStyle = function(selector) {"
      "     return { 'color': 'red' };"
      "   }"
      " }"
      "})();";
  UpdateAdBlockResources(
      "[{"
      "\"name\": \"deamp.js\","
      "\"aliases\": [\"deamp.js\"],"
      "\"kind\": {\"mime\": \"application/javascript\"},"
      "\"content\": \"" +
      base::Base64Encode(scriptlet) + "\"}]");
  UpdateAdBlockInstanceWithRules("b.*##+js(deamp)");

  GURL url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(url);
  auto result1 =
      EvalJs(web_contents(), R"(waitCSSSelector('body', 'color', 'green'))");
  ASSERT_TRUE(result1.error.empty());
  EXPECT_EQ(base::Value(true), result1.value);

  PrefService* prefs = profile()->GetPrefs();
  prefs->SetBoolean(de_amp::kDeAmpPrefEnabled, false);

  web_contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  auto result2 =
      EvalJs(web_contents(), R"(waitCSSSelector('body', 'color', 'red'))");
  ASSERT_TRUE(result2.error.empty());
  EXPECT_EQ(base::Value(true), result2.value);
}

// Test scriptlet injection that modifies window attributes
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, CosmeticFilteringIframeScriptlet) {
  std::string scriptlet =
      "(function() {"
      "  window.JSON.parse = function() { return {} }"
      "})();";
  UpdateAdBlockResources(
      "[{"
      "\"name\": \"hijacktest.js\","
      "\"aliases\": [\"hjt.js\"],"
      "\"kind\": {\"mime\": \"application/javascript\"},"
      "\"content\": \"" +
      base::Base64Encode(scriptlet) + "\"}]");
  UpdateAdBlockInstanceWithRules("b.com##+js(hjt)");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/iframe_messenger.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  ASSERT_EQ(true, EvalJs(contents, "show_ad"));
}

// Test scriptlet injection inside about:blank frames
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       CosmeticFilteringAboutBlankScriptlet) {
  std::string scriptlet =
      "(function() {"
      "  window.sval = true;"
      "})();";
  UpdateAdBlockResources(
      "[{"
      "\"name\": \"set.js\","
      "\"aliases\": [\"set.js\"],"
      "\"kind\": {\"mime\": \"application/javascript\"},"
      "\"content\": \"" +
      base::Base64Encode(scriptlet) + "\"}]");
  UpdateAdBlockInstanceWithRules("b.com##+js(set)");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  auto result = EvalJs(
      contents, R"(document.getElementById('iframe').contentWindow.sval)");
  EXPECT_EQ(true, result);
}

// Test network blocking initiated from inside about:blank frames
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, NetworkBlockAboutBlank) {
  UpdateAdBlockInstanceWithRules("/ad_banner.png^$1p");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  // Note: window.sval is a promise which will be resolved by `EvalJs`
  auto result = EvalJs(contents, R"(const i = document.getElementById('iframe');
                   const s = i.contentDocument.createElement('script');
                   s.innerText = 'window.sval = fetch("/ad_banner.png").then(() => "fetched").catch(() => "blocked")';
                   i.contentDocument.documentElement.appendChild(s);)");
  ASSERT_TRUE(result.error.empty());

  content::RenderFrameHost* inner_frame = content::FrameMatchingPredicate(
      contents->GetPrimaryPage(),
      base::BindRepeating(content::FrameHasSourceUrl, GURL("about:blank")));

  EXPECT_EQ("blocked", EvalJs(inner_frame, "window.sval"));

  EXPECT_EQ(profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
}

// Test cosmetic filtering on an element that already has an `!important`
// marker on its `display` style.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       DISABLED_CosmeticFilteringOverridesImportant) {
  UpdateAdBlockInstanceWithRules("###inline-block-important");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  auto result_first = EvalJs(
      contents,
      R"(waitCSSSelector('#inline-block-important', 'display', 'none'))");
  ASSERT_TRUE(result_first.error.empty());
  EXPECT_EQ(base::Value(true), result_first.value);
}

// Test cosmetic filtering on an element that already has an `!important`
// marker on its `display` style, from an engine without the first-party
// exception policy.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       CustomCosmeticFilteringOverridesImportant) {
  UpdateCustomAdBlockInstanceWithRules("###inline-block-important");

  GURL tab_url =
      embedded_test_server()->GetURL("b.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  auto result_first = EvalJs(
      contents,
      R"(waitCSSSelector('#inline-block-important', 'display', 'none'))");
  ASSERT_TRUE(result_first.error.empty());
  EXPECT_EQ(base::Value(true), result_first.value);
}

// Test `has-text` procedural filters in standard blocking mode
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest,
                       ProceduralFilterHasTextStandardBlocking) {
  DisableAggressiveMode();

  UpdateAdBlockInstanceWithRules(
      "a.com##.string-cases > div:has-text(hide me)\n"
      "a.com##.regex-cases > div:has-text(/should be [a-z]{6}\\./)\n"
      "a.com##.items:has-text(Sponsored)\n"
      "a.com##.items2:has-text(Sponsored) + .container:has-text(Ad)");

  GURL tab_url =
      embedded_test_server()->GetURL("a.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-has-text [data-expect]', 'display', 'block'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }
}

// Test `has-text` procedural filters
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, ProceduralFilterHasText) {
  UpdateAdBlockInstanceWithRules(
      "a.com##.string-cases > div:has-text(hide me)\n"
      "a.com##.regex-cases > div:has-text(/should be [a-z]{6}\\./)\n"
      "a.com##.items:has-text(Sponsored)\n"
      "a.com##.items2:has-text(Sponsored) + .container:has-text(Ad)");

  GURL tab_url =
      embedded_test_server()->GetURL("a.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-has-text [data-expect="hidden"]', 'display', 'none'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-has-text [data-expect="visible"]', 'display', 'block'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }
}

// Test `matches-attr` procedural filters
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, ProceduralFilterMatchesAttr) {
  UpdateAdBlockInstanceWithRules(
      "a.com##:matches-attr(\"test-attr\"=\"test-value\")\n"
      "a.com##:matches-attr(\"/test-y.{2}-attr/\"=\"/test-y[a-z]s-value/\")");

  GURL tab_url =
      embedded_test_server()->GetURL("a.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-matches-attr [data-expect="hidden"]', 'display', 'none'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-matches-attr [data-expect="visible"]', 'display', 'block'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }
}

// Test `matches-css` procedural filters
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, ProceduralFilterMatchesCss) {
  UpdateAdBlockInstanceWithRules(
      "a.com##:matches-css(opacity: 0.9)\n"
      "a.com##:matches-css-before(display: inline-block)\n"
      "a.com##:matches-css-before(content:\"advertisement\")");

  GURL tab_url =
      embedded_test_server()->GetURL("a.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-matches-css [data-expect="hidden"]', 'display', 'none'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-matches-css [data-expect="visible"]', 'display', 'block'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }
}

// Test `matches-path` procedural filters
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, ProceduralFilterMatchesPath) {
  UpdateAdBlockInstanceWithRules(
      "a.com##section .positive-string-case "
      "p.odd:matches-path(cosmetic_filtering.html)\n"
      "a.com##section .positive-regex-case "
      "p.odd:matches-path(/c[aeiou]smetic\\_[a-z]{9}/)\n"
      "a.com##section .negative-case:matches-path(/some-other-page.html)");

  GURL tab_url =
      embedded_test_server()->GetURL("a.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-matches-path [data-expect="hidden"]', 'display', 'none'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-matches-path [data-expect="visible"]', 'display', 'block'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }
}

// Test `min-text-length` procedural filters
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, ProceduralFilterMinTextLength) {
  UpdateAdBlockInstanceWithRules("a.com##p:min-text-length(50)");

  GURL tab_url =
      embedded_test_server()->GetURL("a.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-min-text-length [data-expect="hidden"]', 'display', 'none'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-min-text-length [data-expect="visible"]', 'display', 'block'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }
}

// Test `upward` procedural filters
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, ProceduralFilterUpward) {
  UpdateAdBlockInstanceWithRules(
      "a.com##.string-tests em.target:upward(div.needle)\n"
      "a.com##.int-tests "
      "em.target:upward(2):matches-attr(test-needle=\"true\")");

  GURL tab_url =
      embedded_test_server()->GetURL("a.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-upward [data-expect="hidden"]', 'display', 'none'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-upward [data-expect="visible"]', 'display', 'block'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }
}

// Test `xpath` procedural filters
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, ProceduralFilterXpath) {
  UpdateAdBlockInstanceWithRules(
      "a.com##:xpath(//div[@class='no-subject-case']/p[@class='odd'])\n"
      "a.com##div.with-subject-case:xpath(p[@class='even'])");

  GURL tab_url =
      embedded_test_server()->GetURL("a.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-xpath [data-expect="hidden"]', 'display', 'none'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }

  {
    auto result = EvalJs(
        contents,
        R"(waitCSSSelector('#procedural-filter-xpath [data-expect="visible"]', 'display', 'block'))");
    ASSERT_TRUE(result.error.empty());
    EXPECT_EQ(base::Value(true), result.value);
  }
}

class CookieListPrefObserver {
 public:
  explicit CookieListPrefObserver(PrefService* local_state) {
    pref_change_registrar_.Init(local_state);
    pref_change_registrar_.Add(
        brave_shields::prefs::kAdBlockRegionalFilters,
        base::BindRepeating(&CookieListPrefObserver::OnUpdated,
                            base::Unretained(this)));
  }
  ~CookieListPrefObserver() = default;

  CookieListPrefObserver(const CookieListPrefObserver& other) = delete;
  CookieListPrefObserver& operator=(const CookieListPrefObserver& other) =
      delete;

  void Wait() { run_loop_.Run(); }

 private:
  void OnUpdated() { run_loop_.Quit(); }

  base::RunLoop run_loop_;
  PrefChangeRegistrar pref_change_registrar_;
};

// Test that the `brave-adblock-default-1p-blocking` flag forces the Cookie
// List UUID to be enabled, until manually enabled and then disabled again.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, ListEnabled) {
  InstallRegionalAdBlockComponent(brave_shields::kCookieListUuid, false);

  {
    const auto lists = g_brave_browser_process->ad_block_service()
                           ->component_service_manager()
                           ->GetRegionalLists();
    // Although never explicitly enabled, it should be presented as enabled by
    // default at first.
    ASSERT_EQ(1UL, lists.size());
    EXPECT_EQ(true, *lists[0].GetDict().FindBool("enabled"));
  }

  // Disable the filter list.
  {
    CookieListPrefObserver pref_observer(g_browser_process->local_state());
    g_brave_browser_process->ad_block_service()
        ->component_service_manager()
        ->EnableFilterList(brave_shields::kCookieListUuid, false);
    pref_observer.Wait();
  }

  {
    const auto lists = g_brave_browser_process->ad_block_service()
                           ->component_service_manager()
                           ->GetRegionalLists();
    // It should be actually disabled now.
    ASSERT_EQ(1UL, lists.size());
    EXPECT_EQ(false, *lists[0].GetDict().FindBool("enabled"));
  }
}

// Content Picker and the context menu are disabled for Android.
#if !BUILDFLAG(IS_ANDROID)
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, ContentPicker) {
  const GURL tab_url =
      embedded_test_server()->GetURL("a.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);
  const char kPickerIsInjected[] =
      "document.getElementById('brave-element-picker') != null";
  ASSERT_FALSE(
      content::EvalJs(web_contents(), kPickerIsInjected).ExtractBool());

  const auto click_menu = [&]() {
    content::ContextMenuParams params;
    params.page_url = tab_url;
    TestRenderViewContextMenu menu(*web_contents()->GetPrimaryMainFrame(),
                                   params);
    menu.Init();
    EXPECT_TRUE(menu.IsItemEnabled(IDC_ADBLOCK_CONTEXT_BLOCK_ELEMENTS));
    menu.ExecuteCommand(IDC_ADBLOCK_CONTEXT_BLOCK_ELEMENTS, 0);
  };

  click_menu();

  ASSERT_TRUE(content::EvalJs(web_contents(), kPickerIsInjected).ExtractBool());

  EXPECT_TRUE(content::EvalJs(web_contents(),
                              "checkSelector('#ad-banner', 'display', 'block')")
                  .ExtractBool());

  // Emulate selecting some element and clicking `Create` button.
  ASSERT_TRUE(content::ExecJs(web_contents(),
                              "cf_worker.addSiteCosmeticFilter('#ad-banner')",
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL));

  // Reload the page and check the selector is blocked by the new rule.
  NavigateToURL(tab_url);
  WaitForSelectorBlocked(web_contents(), "#ad-banner");
  EXPECT_FALSE(
      content::EvalJs(web_contents(), kPickerIsInjected).ExtractBool());

  click_menu();
  // Emulate clicking `Manage filters`.
  ASSERT_TRUE(content::ExecJs(web_contents(), "cf_worker.manageCustomFilters()",
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_BRAVE_INTERNAL));

  ASSERT_EQ(2, browser()->tab_strip_model()->count());
  ASSERT_TRUE(content::WaitForLoadStop(web_contents()));
  EXPECT_EQ(web_contents()->GetLastCommittedURL(),
            "chrome://settings/shields/filters");
}
#endif  // !BUILDFLAG(IS_ANDROID)

class AdBlockServiceTestJsPerformance : public AdBlockServiceTest {
 public:
  AdBlockServiceTestJsPerformance() {
    feature_list_.InitWithFeaturesAndParameters(
        {{kCosmeticFilteringJsPerformance,
          {{"subframes_first_query_delay_ms", "3000"},
           {"switch_to_polling_threshold", "500"},
           {"fetch_throttling_ms", "500"}}}},
        {});
  }

  void AddDivsWithDynamicClasses(const content::ToRenderFrameHost& target,
                                 int start_number,
                                 int end_number) const {
    const char kTemplate[] = R"(
    for (let i = $1; i <= $2; i++) {
      const e = document.createElement('div');
      e.className = 'div-class-' + i;
      document.documentElement.appendChild(e);
    })";
    ASSERT_TRUE(content::ExecJs(
        target, content::JsReplace(kTemplate, start_number, end_number)));
  }

  void NonBlockingDelay(const base::TimeDelta& delay) {
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE, run_loop.QuitWhenIdleClosure(), delay);
    run_loop.Run();
  }

 public:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(AdBlockServiceTestJsPerformance,
                       CosmeticFilteringDynamic) {
  UpdateAdBlockInstanceWithRules(
      "##.div-class-100\n##.div-class-500\n##.div-class-1000");

  GURL tab_url =
      embedded_test_server()->GetURL("a.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  // This elements will be check by initial DOM lookup (startObserving()).
  AddDivsWithDynamicClasses(contents, 1, 1);
  EXPECT_TRUE(
      EvalJs(contents, "checkSelector('.div-class-1', 'display', 'block')")
          .ExtractBool());

  // This elements will be check by the mutation observer (without delay).
  AddDivsWithDynamicClasses(contents, 2, 200);
  WaitForSelectorBlocked(contents, ".div-class-100");

  // This elements will be check by mutation observer after throttling delay.
  AddDivsWithDynamicClasses(contents, 201, 500);
  // Wait fetch_throttling_ms/2 ms and check the selector is still visible to
  // verify that throttling works correctly.
  NonBlockingDelay(base::Milliseconds(250));
  EXPECT_TRUE(
      EvalJs(contents, "checkSelector('.div-class-500', 'display', 'block')")
          .ExtractBool());

  // Verify that it will be blocked after the delay is finished.
  WaitForSelectorBlocked(contents, ".div-class-500");

  // Add more elements to trigger switch to DOM selector polling (see
  // UseSelectorsPolling()).
  AddDivsWithDynamicClasses(contents, 501, 1000);
  WaitForSelectorBlocked(contents, ".div-class-1000");
  EXPECT_TRUE(
      EvalJs(contents, "checkSelector('.div-class-999', 'display', 'block')")
          .ExtractBool());
}

IN_PROC_BROWSER_TEST_F(AdBlockServiceTestJsPerformance,
                       CosmeticFilteringSubframeDynamic) {
  UpdateAdBlockInstanceWithRules("##.div-class-500");

  GURL tab_url =
      embedded_test_server()->GetURL("a.com", "/cosmetic_filtering.html");
  NavigateToURL(tab_url);

  content::WebContents* contents = web_contents();

  content::NavigateIframeToURL(
      contents, "iframe",
      embedded_test_server()->GetURL("frame.com", "/cosmetic_filtering.html"));

  auto* iframe = ChildFrameAt(contents, 0);
  ASSERT_TRUE(iframe);

  AddDivsWithDynamicClasses(iframe, 1, 500);
  // Disable the checkSelector() delay.
  EXPECT_TRUE(content::ExecJs(iframe, "didWait = true"));

  // Verify subframes_first_query_delay_ms delay.
  // Wait some time less than subframes_first_query_delay_ms and check the
  // selector is still visible.
  NonBlockingDelay(base::Milliseconds(100));

  EXPECT_TRUE(
      EvalJs(iframe, "checkSelector('.div-class-500', 'display', 'block')")
          .ExtractBool());

  // Should be blocked after the delay ended.
  WaitForSelectorBlocked(iframe, ".div-class-500");

  // Verify that other selectors are visible:
  EXPECT_TRUE(
      EvalJs(iframe, "checkSelector('.div-class-499', 'display', 'block')")
          .ExtractBool());
}
