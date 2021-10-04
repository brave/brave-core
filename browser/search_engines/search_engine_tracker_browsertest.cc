/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_engines/search_engine_tracker.h"

#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "content/public/test/browser_test.h"

class SearchEngineProviderP3ATest : public InProcessBrowserTest {
 public:
  SearchEngineProviderP3ATest() {
    histogram_tester_.reset(new base::HistogramTester);
  }

 protected:
  std::unique_ptr<base::HistogramTester> histogram_tester_;
};

IN_PROC_BROWSER_TEST_F(SearchEngineProviderP3ATest,
                       DISABLED_DefaultSearchEngineP3A) {
  // Check that the metric is reported on startup.
  histogram_tester_->ExpectUniqueSample(kDefaultSearchEngineMetric,
                                        SearchEngineP3A::kGoogle, 1);

  auto* service =
      TemplateURLServiceFactory::GetForProfile(browser()->profile());
  search_test_utils::WaitForTemplateURLServiceToLoad(service);

  // Check that changing the default engine triggers emitting of a new value.
  auto ddg_data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      browser()->profile()->GetPrefs(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO);
  TemplateURL ddg_url(*ddg_data);

  service->SetUserSelectedDefaultSearchProvider(&ddg_url);
  histogram_tester_->ExpectBucketCount(kDefaultSearchEngineMetric,
                                       SearchEngineP3A::kDuckDuckGo, 1);

  // Check that incognito or TOR profiles do not emit the metric.
  CreateIncognitoBrowser();
#if BUILDFLAG(ENABLE_TOR)
  brave::NewOffTheRecordWindowTor(browser());
#endif

  histogram_tester_->ExpectTotalCount(kDefaultSearchEngineMetric, 2);
}

IN_PROC_BROWSER_TEST_F(SearchEngineProviderP3ATest,
                       DISABLED_SwitchSearchEngineP3A) {
  // Check that the metric is reported on startup.
  // For some reason we record kNoSwitch twice, even through
  // kDefaultSearchEngineMetric is only updated once at this point.
  histogram_tester_->ExpectUniqueSample(kSwitchSearchEngineMetric,
                                        SearchEngineSwitchP3A::kNoSwitch, 2);

  // Load service for switching the default search engine.
  auto* service =
      TemplateURLServiceFactory::GetForProfile(browser()->profile());
  search_test_utils::WaitForTemplateURLServiceToLoad(service);

  // Check that changing the default engine triggers emission of a new value.
  auto ddg_data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      browser()->profile()->GetPrefs(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO);
  TemplateURL ddg_url(*ddg_data);

  // This assumes Brave Search is the default!
  histogram_tester_->ExpectBucketCount(kSwitchSearchEngineMetric,
                                       SearchEngineSwitchP3A::kBraveToDDG, 1);

  // Check additional changes.
  auto brave_data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      browser()->profile()->GetPrefs(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE);
  TemplateURL brave_url(*brave_data);

  service->SetUserSelectedDefaultSearchProvider(&brave_url);
  histogram_tester_->ExpectBucketCount(kSwitchSearchEngineMetric,
                                       SearchEngineSwitchP3A::kDDGToBrave, 1);

  // Check additional changes.
  auto bing_data = TemplateURLPrepopulateData::GetPrepopulatedEngine(
      browser()->profile()->GetPrefs(),
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BING);
  TemplateURL bing_url(*bing_data);

  service->SetUserSelectedDefaultSearchProvider(&bing_url);
  histogram_tester_->ExpectBucketCount(kSwitchSearchEngineMetric,
                                       SearchEngineSwitchP3A::kBraveToOther, 1);

  // Check that incognito or TOR profiles do not emit the metric.
  CreateIncognitoBrowser();
#if BUILDFLAG(ENABLE_TOR)
  brave::NewOffTheRecordWindowTor(browser());
#endif

  histogram_tester_->ExpectTotalCount(kSwitchSearchEngineMetric, 5);
}
