/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"

#include <cstdint>
#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/test/run_until.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_shields/ad_block_browser_test_helper.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/content/test/test_filters_provider.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/brave_shields_settings_values.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/resource_coordinator/lifecycle_unit_state.mojom.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_external.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_source.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace {

uint64_t getProfileBandwidthSaved(BrowserWindowInterface* browser) {
  return browser->GetProfile()->GetPrefs()->GetUint64(
      brave_perf_predictor::prefs::kBandwidthSavedBytes);
}

uint64_t getProfileAdsBlocked(BrowserWindowInterface* browser) {
  return browser->GetProfile()->GetPrefs()->GetUint64(
      brave_shields::prefs::kAdsBlocked);
}

}  // namespace

using brave_shields::TestFiltersProvider;

class PerfPredictorTabHelperTest : public InProcessBrowserTest {
 public:
  PerfPredictorTabHelperTest() = default;

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    ad_block_test_helper_ =
        std::make_unique<brave_shields::AdBlockBrowserTestHelper>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    InitEmbeddedTestServer();
    host_resolver()->AddRule("*", "127.0.0.1");

    auto* content_settings =
        HostContentSettingsMapFactory::GetForProfile(browser()->GetProfile());

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

  void TearDownOnMainThread() override {
    ad_block_test_helper_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

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

    filters_provider_->RegisterAsSourceProvider(ad_block_service);

    ASSERT_TRUE(brave_shields::WaitForAdBlockServiceThreads());
  }

  void PostRunTestOnMainThread() override {
    filters_provider_.reset();
    InProcessBrowserTest::PostRunTestOnMainThread();
  }

  // Counts the page load timings that reach `tab_helper` in
  // `timing_updates_`.
  void CountTimingUpdates(
      brave_perf_predictor::PerfPredictorTabHelper& tab_helper) {
    tab_helper.SetPageLoadTimingUpdatedCallbackForTesting(base::BindRepeating(
        [](int* timing_updates) { ++*timing_updates; }, &timing_updates_));
  }

  int timing_updates_ = 0;

  std::unique_ptr<brave_shields::AdBlockBrowserTestHelper>
      ad_block_test_helper_;
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

// Discarding a tab replaces its contents, which records the old page's
// savings like closing the tab does.
IN_PROC_BROWSER_TEST_F(PerfPredictorTabHelperTest, DiscardStoresSavings) {
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

  tabs::TabInterface* const tab = tabs::TabInterface::GetFromContents(contents);
  auto* const tab_helper =
      brave_perf_predictor::PerfPredictorTabHelper::From(tab);
  ASSERT_TRUE(tab_helper);

  AddBlankTabAndShow(browser());
  auto* const lifecycle_unit =
      resource_coordinator::TabLifecycleUnitSource::GetTabLifecycleUnitExternal(
          contents);
  ASSERT_TRUE(lifecycle_unit);
  ASSERT_TRUE(
      lifecycle_unit->DiscardTab(::mojom::LifecycleUnitDiscardReason::URGENT));
  EXPECT_EQ(tab_helper,
            brave_perf_predictor::PerfPredictorTabHelper::From(tab));
  EXPECT_NE(getProfileBandwidthSaved(browser()), 0ULL);
}

// The page load observer finds the tab helper through the page's tab.
IN_PROC_BROWSER_TEST_F(PerfPredictorTabHelperTest, TimingReachesTabHelper) {
  auto* const tab_helper = brave_perf_predictor::PerfPredictorTabHelper::From(
      browser()->GetActiveTabInterface());
  ASSERT_TRUE(tab_helper);
  CountTimingUpdates(*tab_helper);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/blocking.html")));
  EXPECT_TRUE(base::test::RunUntil([&] { return timing_updates_ > 0; }));
}

// Once its tab is discarded, a page can no longer report timings into the tab
// helper, which takes the timings of the page the new contents loads.
IN_PROC_BROWSER_TEST_F(PerfPredictorTabHelperTest,
                       TimingFollowsDiscardedContents) {
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), embedded_test_server()->GetURL("/blocking.html"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  TabStripModel* const tab_strip_model = browser()->tab_strip_model();
  const int index = tab_strip_model->active_index();
  tabs::TabInterface* const tab = tab_strip_model->GetTabAtIndex(index);
  auto* const tab_helper =
      brave_perf_predictor::PerfPredictorTabHelper::From(tab);
  ASSERT_TRUE(tab_helper);
  CountTimingUpdates(*tab_helper);
  // The first timings may arrive before counting starts, so reload to count.
  content::TestNavigationObserver reload_observer(tab->GetContents());
  tab->GetContents()->GetController().Reload(content::ReloadType::NORMAL,
                                             false);
  reload_observer.Wait();
  ASSERT_TRUE(base::test::RunUntil([&] { return timing_updates_ > 0; }));

  tab_strip_model->ActivateTabAt(0);
  auto* const lifecycle_unit =
      resource_coordinator::TabLifecycleUnitSource::GetTabLifecycleUnitExternal(
          tab->GetContents());
  ASSERT_TRUE(lifecycle_unit);
  const int timing_updates_before_discard = timing_updates_;
  ASSERT_TRUE(
      lifecycle_unit->DiscardTab(::mojom::LifecycleUnitDiscardReason::URGENT));
  EXPECT_EQ(timing_updates_, timing_updates_before_discard);

  content::TestNavigationObserver navigation_observer(tab->GetContents());
  tab_strip_model->ActivateTabAt(index);
  tab->GetContents()->GetController().LoadIfNecessary();
  navigation_observer.Wait();
  EXPECT_TRUE(base::test::RunUntil(
      [&] { return timing_updates_ > timing_updates_before_discard; }));
}
