/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/content/test/test_filters_provider.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

namespace {

uint64_t getProfileBandwidthSaved(Browser* browser) {
  return browser->profile()->GetPrefs()->GetUint64(
      brave_perf_predictor::prefs::kBandwidthSavedBytes);
}

uint64_t getProfileAdsBlocked(Browser* browser) {
  return browser->profile()->GetPrefs()->GetUint64(
      kAdsBlocked);
}

}  // namespace

using brave_shields::TestFiltersProvider;

class PerfPredictorTabHelperTest : public InProcessBrowserTest {
 public:
  PerfPredictorTabHelperTest() = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    InitEmbeddedTestServer();
    host_resolver()->AddRule("*", "127.0.0.1");

    auto* content_settings =
        HostContentSettingsMapFactory::GetForProfile(browser()->profile());

    // Most tests are written for aggressive mode. Individual tests should reset
    // this using `DisableAggressiveMode` if they are testing standard mode
    // behavior.
    brave_shields::SetCosmeticFilteringControlType(
        content_settings, brave_shields::ControlType::BLOCK, GURL());
  }

  void PreRunTestOnMainThread() override {
    InProcessBrowserTest::PreRunTestOnMainThread();
  }

  void TearDown() override { InProcessBrowserTest::TearDown(); }

  void InitEmbeddedTestServer() {
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    content::SetupCrossSiteRedirector(embedded_test_server());
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void UpdateAdBlockInstanceWithRules(const std::string& rules) {
    filters_provider_ = std::make_unique<TestFiltersProvider>(rules);

    brave_shields::AdBlockService* ad_block_service =
        g_brave_browser_process->ad_block_service();

    ad_block_service->UseSourceProviderForTest(filters_provider_.get());

    WaitForAdBlockServiceThreads();
  }

  void WaitForAdBlockServiceThreads() {
    scoped_refptr<base::ThreadTestHelper> tr_helper(new base::ThreadTestHelper(
        g_brave_browser_process->ad_block_service()->GetTaskRunner()));
    ASSERT_TRUE(tr_helper->Run());
  }

  std::unique_ptr<TestFiltersProvider> filters_provider_;
};

IN_PROC_BROWSER_TEST_F(PerfPredictorTabHelperTest, NoBlockNoSavings) {
  EXPECT_EQ(getProfileBandwidthSaved(browser()), 0ULL);

  GURL url = embedded_test_server()->GetURL("/blocking.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_EQ(true, EvalJs(contents,
                         "addImage('logo.png');"
                         "setExpectations(0, 0, 1, 0);"
                         "xhr('analytics.js')"));
  // Prediction triggered when web contents are closed
  contents->Close();
  EXPECT_EQ(getProfileBandwidthSaved(browser()), 0ULL);
}

IN_PROC_BROWSER_TEST_F(PerfPredictorTabHelperTest, ScriptBlockHasSavings) {
  UpdateAdBlockInstanceWithRules("^analytics.js");
  EXPECT_EQ(getProfileBandwidthSaved(browser()), 0ULL);

  GURL url = embedded_test_server()->GetURL("/blocking.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_EQ(true, EvalJs(contents,
                         "addImage('logo.png');"
                         "setExpectations(0, 0, 0, 1);"
                         "xhr('analytics.js')"));

  EXPECT_EQ(getProfileAdsBlocked(browser()), 1ULL);
  // Prediction triggered when web contents are closed
  contents->Close();
  EXPECT_NE(getProfileBandwidthSaved(browser()), 0ULL);
}

IN_PROC_BROWSER_TEST_F(PerfPredictorTabHelperTest, NewNavigationStoresSavings) {
  UpdateAdBlockInstanceWithRules("^analytics.js");
  EXPECT_EQ(getProfileBandwidthSaved(browser()), 0ULL);

  GURL url = embedded_test_server()->GetURL("/blocking.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  EXPECT_EQ(true, EvalJs(contents,
                         "addImage('logo.png');"
                         "setExpectations(0, 0, 0, 1);"
                         "xhr('analytics.js')"));
  EXPECT_EQ(getProfileAdsBlocked(browser()), 1ULL);
  // Prediction triggered when web contents are closed
  GURL second_url =
      embedded_test_server()->GetURL("example.com", "/blocking.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), second_url));
  EXPECT_EQ(true, EvalJs(contents,
                         "addImage('logo.png');"
                         "setExpectations(0, 0, 0, 1);"
                         "xhr('analytics.js')"));

  auto previous_nav_savings = getProfileBandwidthSaved(browser());
  EXPECT_NE(previous_nav_savings, 0ULL);

  // closing the new navigation triggers a second computation
  contents->Close();
  EXPECT_NE(getProfileBandwidthSaved(browser()), previous_nav_savings);
}
