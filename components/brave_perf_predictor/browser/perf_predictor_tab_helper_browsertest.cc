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
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

class PerfPredictorTabHelperTest : public InProcessBrowserTest {
 public:
  PerfPredictorTabHelperTest() {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void SetUp() override {
    InitEmbeddedTestServer();
    InProcessBrowserTest::SetUp();
  }

  void PreRunTestOnMainThread() override {
    InProcessBrowserTest::PreRunTestOnMainThread();
    WaitForAdBlockServiceThreads();
    // Need ad_block_service to be available to get blocking savings predictions
    ASSERT_TRUE(g_brave_browser_process->ad_block_service()->IsInitialized());
  }

  void TearDown() override { InProcessBrowserTest::TearDown(); }

  void InitEmbeddedTestServer() {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    content::SetupCrossSiteRedirector(embedded_test_server());
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void WaitForAdBlockServiceThreads() {
    scoped_refptr<base::ThreadTestHelper> tr_helper(new base::ThreadTestHelper(
        g_brave_browser_process->local_data_files_service()->GetTaskRunner()));
    ASSERT_TRUE(tr_helper->Run());
    scoped_refptr<base::ThreadTestHelper> io_helper(new base::ThreadTestHelper(
        base::CreateSingleThreadTaskRunner({content::BrowserThread::IO})
            .get()));
    ASSERT_TRUE(io_helper->Run());
  }
};

IN_PROC_BROWSER_TEST_F(PerfPredictorTabHelperTest, NoBlockNoSavings) {
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(
                brave_perf_predictor::prefs::kBandwidthSavedBytes),
            0ULL);

  GURL url = embedded_test_server()->GetURL("/blocking.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(1, 0, 0, 0, 0, 0);"
                                          "addImage('logo.png')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  // Prediction triggered when web contents are closed
  contents->Close();
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(
                brave_perf_predictor::prefs::kBandwidthSavedBytes),
            0ULL);
}

IN_PROC_BROWSER_TEST_F(PerfPredictorTabHelperTest, ScriptBlockHasSavings) {
  ASSERT_TRUE(g_brave_browser_process->ad_block_custom_filters_service()
                  ->UpdateCustomFilters("*analytics.js"));
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(
                brave_perf_predictor::prefs::kBandwidthSavedBytes),
            0ULL);

  GURL url = embedded_test_server()->GetURL("/blocking.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(0, 0, 0, 0, 1, 0);"
                                          "xhr('analytics.js')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
  // Prediction triggered when web contents are closed
  contents->Close();
  EXPECT_NE(browser()->profile()->GetPrefs()->GetUint64(
                brave_perf_predictor::prefs::kBandwidthSavedBytes),
            0ULL);
}

IN_PROC_BROWSER_TEST_F(PerfPredictorTabHelperTest, NewNavigationStoresSavings) {
  ASSERT_TRUE(g_brave_browser_process->ad_block_custom_filters_service()
                  ->UpdateCustomFilters("*analytics.js"));
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(
                brave_perf_predictor::prefs::kBandwidthSavedBytes),
            0ULL);

  GURL url = embedded_test_server()->GetURL("/blocking.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(0, 0, 0, 0, 1, 0);"
                                          "xhr('analytics.js')",
                                          &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 1ULL);
  // Prediction triggered when web contents are closed
  GURL second_url =
      embedded_test_server()->GetURL("example.com", "/blocking.html");
  ui_test_utils::NavigateToURL(browser(), second_url);
  ASSERT_TRUE(ExecuteScriptAndExtractBool(contents,
                                          "setExpectations(0, 0, 0, 0, 1, 0);"
                                          "xhr('analytics.js')",
                                          &as_expected));

  auto previous_nav_savings = browser()->profile()->GetPrefs()->GetUint64(
      brave_perf_predictor::prefs::kBandwidthSavedBytes);
  EXPECT_NE(previous_nav_savings, 0ULL);

  // closing the new navigation triggers a second computation
  contents->Close();
  EXPECT_NE(browser()->profile()->GetPrefs()->GetUint64(
                brave_perf_predictor::prefs::kBandwidthSavedBytes),
            previous_nav_savings);
}
