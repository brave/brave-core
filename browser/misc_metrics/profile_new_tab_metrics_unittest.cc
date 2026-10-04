/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/profile_new_tab_metrics.h"

#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class ProfileNewTabMetricsTest : public testing::Test {
 public:
  ProfileNewTabMetricsTest() = default;

  void SetUp() override {
    pref_service_.registry()->RegisterIntegerPref(
        kNewTabPageShowsOptions,
        static_cast<int>(brave::NewTabPageShowsOptions::kDashboard));
    pref_service_.registry()->RegisterStringPref(prefs::kHomePage, "");
    pref_service_.registry()->RegisterBooleanPref(prefs::kHomePageIsNewTabPage,
                                                  false);
  }

 protected:
  TestingPrefServiceSimple pref_service_;
  base::HistogramTester histogram_tester_;
};

TEST_F(ProfileNewTabMetricsTest, TestDashboardOption) {
  pref_service_.SetInteger(
      kNewTabPageShowsOptions,
      static_cast<int>(brave::NewTabPageShowsOptions::kDashboard));

  ProfileNewTabMetrics metrics(&pref_service_);

  histogram_tester_.ExpectUniqueSample(kNewTabPageDefaultHistogramName, 0, 1);
}

TEST_F(ProfileNewTabMetricsTest, TestBlankOption) {
  pref_service_.SetInteger(
      kNewTabPageShowsOptions,
      static_cast<int>(brave::NewTabPageShowsOptions::kBlankpage));

  ProfileNewTabMetrics metrics(&pref_service_);

  histogram_tester_.ExpectUniqueSample(kNewTabPageDefaultHistogramName, 1, 1);
}

TEST_F(ProfileNewTabMetricsTest, TestHomepageOptions) {
  pref_service_.SetInteger(
      kNewTabPageShowsOptions,
      static_cast<int>(brave::NewTabPageShowsOptions::kHomepage));

  pref_service_.SetBoolean(prefs::kHomePageIsNewTabPage, true);

  ProfileNewTabMetrics metrics(&pref_service_);

  histogram_tester_.ExpectUniqueSample(kNewTabPageDefaultHistogramName, 0, 1);
  histogram_tester_.ExpectTotalCount(kNewTabPageDefaultHistogramName, 1);

  pref_service_.SetBoolean(prefs::kHomePageIsNewTabPage, false);
  histogram_tester_.ExpectBucketCount(kNewTabPageDefaultHistogramName, 5, 1);

  pref_service_.SetString(prefs::kHomePage, "https://search.brave.com");
  histogram_tester_.ExpectBucketCount(kNewTabPageDefaultHistogramName, 2, 1);

  pref_service_.SetString(prefs::kHomePage, "https://www.google.com");
  histogram_tester_.ExpectBucketCount(kNewTabPageDefaultHistogramName, 3, 1);

  pref_service_.SetString(prefs::kHomePage, "https://duckduckgo.com");
  histogram_tester_.ExpectBucketCount(kNewTabPageDefaultHistogramName, 4, 1);

  pref_service_.SetString(prefs::kHomePage, "https://example.com");
  histogram_tester_.ExpectBucketCount(kNewTabPageDefaultHistogramName, 5, 2);

  histogram_tester_.ExpectTotalCount(kNewTabPageDefaultHistogramName, 6);
}

}  // namespace misc_metrics
