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

testing::AssertionResult VerifyTemplateURLServiceLoad(
    TemplateURLService* service) {
  if (service->loaded()) {
    return testing::AssertionSuccess();
  }
  search_test_utils::WaitForTemplateURLServiceToLoad(service);
  if (service->loaded()) {
    return testing::AssertionSuccess();
  }
  return testing::AssertionFailure() << "TemplateURLService isn't loaded";
}

IN_PROC_BROWSER_TEST_F(SearchEngineProviderP3ATest,
                       DefaultSearchEngineP3A) {
  // Check that the metric is reported on startup.
  histogram_tester_->ExpectUniqueSample(kDefaultSearchEngineMetric,
                                        SearchEngineP3A::kGoogle, 1);

  auto* service =
      TemplateURLServiceFactory::GetForProfile(browser()->profile());

  // Make sure the service is initialized.
  EXPECT_TRUE(VerifyTemplateURLServiceLoad(service));

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
