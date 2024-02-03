/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/misc_metrics/language_metrics.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class LanguageMetricsUnitTest : public testing::Test {
 public:
  LanguageMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    profile_prefs_.registry()->RegisterStringPref(
        language::prefs::kAcceptLanguages, "");
    language_metrics_ = std::make_unique<LanguageMetrics>(&profile_prefs_);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple profile_prefs_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<LanguageMetrics> language_metrics_;
};

TEST_F(LanguageMetricsUnitTest, Basic) {
  histogram_tester_.ExpectUniqueSample(kPrimaryLanguageHistogramName,
                                       INT_MAX - 1, 1);

  // bad language code
  profile_prefs_.SetString(language::prefs::kAcceptLanguages, "zz");
  histogram_tester_.ExpectUniqueSample(kPrimaryLanguageHistogramName,
                                       INT_MAX - 1, 2);

  profile_prefs_.SetString(language::prefs::kAcceptLanguages, "en-US,en");

  histogram_tester_.ExpectBucketCount(kPrimaryLanguageHistogramName, 37, 1);

  profile_prefs_.SetString(language::prefs::kAcceptLanguages, "fr-CA,en-US,en");

  histogram_tester_.ExpectBucketCount(kPrimaryLanguageHistogramName, 47, 1);
  histogram_tester_.ExpectTotalCount(kPrimaryLanguageHistogramName, 4);
}

TEST_F(LanguageMetricsUnitTest, DeprecatedCodes) {
  histogram_tester_.ExpectUniqueSample(kPrimaryLanguageHistogramName,
                                       INT_MAX - 1, 1);

  // should convert to 'he'
  profile_prefs_.SetString(language::prefs::kAcceptLanguages, "iw");
  histogram_tester_.ExpectBucketCount(kPrimaryLanguageHistogramName, 56, 1);

  // should convert to 'ro'
  profile_prefs_.SetString(language::prefs::kAcceptLanguages, "mo");
  histogram_tester_.ExpectBucketCount(kPrimaryLanguageHistogramName, 134, 1);
  histogram_tester_.ExpectTotalCount(kPrimaryLanguageHistogramName, 3);
}

}  // namespace misc_metrics
