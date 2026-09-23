/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/quick_search_metrics.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "components/prefs/testing_pref_service.h"
#include "components/search_engines/search_engines_test_environment.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class QuickSearchMetricsTest : public testing::Test {
 public:
  QuickSearchMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    QuickSearchMetrics::RegisterPrefs(local_state_.registry());
    auto* service = search_engines_test_environment_.template_url_service();
    TemplateURLData data;
    data.SetShortName(u"Brave Search");
    data.SetKeyword(u":br");
    data.SetURL("https://search.brave.com/?q={searchTerms}");
    auto* brave_search = service->Add(std::make_unique<TemplateURL>(data));
    service->SetUserSelectedDefaultSearchProvider(brave_search);
    quick_search_metrics_ = std::make_unique<QuickSearchMetrics>(
        &local_state_, search_engines_test_environment_.template_url_service());
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  search_engines::SearchEnginesTestEnvironment search_engines_test_environment_;
  std::unique_ptr<QuickSearchMetrics> quick_search_metrics_;
};

TEST_F(QuickSearchMetricsTest, Basic) {
  // No clicks yet — nothing reported.
  histogram_tester_.ExpectTotalCount(kQuickSearchMostUsedActionHistogramName,
                                     0);

  // Record clicks in enum order: Leo, DefaultEngine, Google, DuckDuckGo
  // 1 Leo click
  quick_search_metrics_->RecordQuickSearch(true, "");
  histogram_tester_.ExpectUniqueSample(kQuickSearchMostUsedActionHistogramName,
                                       static_cast<int>(Action::kLeo), 1);

  // 2 default engine clicks
  quick_search_metrics_->RecordQuickSearch(false, ":br");
  quick_search_metrics_->RecordQuickSearch(false, ":br");
  histogram_tester_.ExpectBucketCount(kQuickSearchMostUsedActionHistogramName,
                                      static_cast<int>(Action::kDefaultEngine),
                                      1);

  // 3 Google clicks
  for (int i = 0; i < 3; i++) {
    quick_search_metrics_->RecordQuickSearch(false, ":g");
  }
  histogram_tester_.ExpectBucketCount(kQuickSearchMostUsedActionHistogramName,
                                      static_cast<int>(Action::kGoogle), 1);

  // 4 DuckDuckGo clicks
  for (int i = 0; i < 4; i++) {
    quick_search_metrics_->RecordQuickSearch(false, ":d");
  }
  histogram_tester_.ExpectBucketCount(kQuickSearchMostUsedActionHistogramName,
                                      static_cast<int>(Action::kDuckDuckGo), 1);

  histogram_tester_.ExpectTotalCount(kQuickSearchMostUsedActionHistogramName,
                                     10);
}

TEST_F(QuickSearchMetricsTest, DailyReport) {
  quick_search_metrics_->RecordQuickSearch(false, ":yt");

  // Daily timer re-reports for 6 days (6 more samples), for a total of 7.
  task_environment_.FastForwardBy(base::Days(6));
  histogram_tester_.ExpectBucketCount(kQuickSearchMostUsedActionHistogramName,
                                      static_cast<int>(Action::kYouTube), 7);

  // Fast forward well past 7 days from the click. The timer continues to fire
  // daily but reports stop once last_click_time exceeds the 7-day window.
  task_environment_.FastForwardBy(base::Days(7));

  // Verify YouTube is the unique sample with exactly 7 reports total.
  histogram_tester_.ExpectUniqueSample(kQuickSearchMostUsedActionHistogramName,
                                       static_cast<int>(Action::kYouTube), 7);
}

}  // namespace misc_metrics
