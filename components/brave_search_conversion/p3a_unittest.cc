// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search_conversion/p3a.h"

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_search_conversion::p3a {

struct ConversionTypeInfo {
  ConversionType type;
  const char* histogram_name;
};

const ConversionTypeInfo type_infos[] = {
    {
        .type = ConversionType::kBannerTypeB,
        .histogram_name = kSearchPromoBannerBHistogramName,
    },
    {
        .type = ConversionType::kBannerTypeC,
        .histogram_name = kSearchPromoBannerCHistogramName,
    },
    {
        .type = ConversionType::kBannerTypeD,
        .histogram_name = kSearchPromoBannerDHistogramName,
    },
    {
        .type = ConversionType::kDDGBannerTypeC,
        .histogram_name = kSearchPromoDDGBannerCHistogramName,
    },
    {
        .type = ConversionType::kDDGBannerTypeD,
        .histogram_name = kSearchPromoDDGBannerDHistogramName,
    },
    {
        .type = ConversionType::kNTP,
        .histogram_name = kSearchPromoNTPHistogramName,
    }};

class BraveSearchConversionP3ATest : public testing::Test {
 protected:
  void SetUp() override {
    PrefRegistrySimple* registry = pref_service_.registry();
    RegisterLocalStatePrefs(registry);
  }

  PrefService* GetPrefs() { return &pref_service_; }

  base::HistogramTester histogram_tester_;

 private:
  TestingPrefServiceSimple pref_service_;
};

TEST_F(BraveSearchConversionP3ATest, TestOmniboxTriggerAndDefaultEngine) {
  for (auto& type_info : type_infos) {
    GetPrefs()->ClearPrefsWithPrefixSilently("brave.brave_search_conversion");

    histogram_tester_.ExpectTotalCount(type_info.histogram_name, 0);

    RecordPromoShown(GetPrefs(), type_info.type);

    histogram_tester_.ExpectTotalCount(type_info.histogram_name, 1);
    histogram_tester_.ExpectBucketCount(type_info.histogram_name, 0, 1);

    RecordPromoShown(GetPrefs(), type_info.type);

    // Should not record twice
    histogram_tester_.ExpectTotalCount(type_info.histogram_name, 1);

    RecordPromoTrigger(GetPrefs(), type_info.type);
    histogram_tester_.ExpectTotalCount(type_info.histogram_name, 2);
    histogram_tester_.ExpectBucketCount(type_info.histogram_name, 1, 1);

    RecordPromoTrigger(GetPrefs(), type_info.type);

    // Also should not record twice
    histogram_tester_.ExpectTotalCount(type_info.histogram_name, 2);

    RecordDefaultEngineConversion(GetPrefs());

    histogram_tester_.ExpectTotalCount(type_info.histogram_name, 3);
    histogram_tester_.ExpectBucketCount(type_info.histogram_name, 3, 1);
  }
}

TEST_F(BraveSearchConversionP3ATest, TestOmniboxShownAndDefaultEngine) {
  for (auto& type_info : type_infos) {
    GetPrefs()->ClearPrefsWithPrefixSilently("brave.brave_search_conversion");

    histogram_tester_.ExpectTotalCount(type_info.histogram_name, 0);

    RecordPromoShown(GetPrefs(), type_info.type);

    histogram_tester_.ExpectTotalCount(type_info.histogram_name, 1);
    histogram_tester_.ExpectBucketCount(type_info.histogram_name, 0, 1);

    RecordPromoShown(GetPrefs(), type_info.type);

    // Should not record twice
    histogram_tester_.ExpectTotalCount(type_info.histogram_name, 1);

    RecordDefaultEngineConversion(GetPrefs());

    histogram_tester_.ExpectTotalCount(type_info.histogram_name, 2);
    histogram_tester_.ExpectBucketCount(type_info.histogram_name, 2, 1);
  }
}

TEST_F(BraveSearchConversionP3ATest, TestQueriesBeforeChurn) {
  histogram_tester_.ExpectTotalCount(kSearchQueriesBeforeChurnHistogramName, 0);

  // user switches away from brave
  RecordDefaultEngineChurn(GetPrefs());
  histogram_tester_.ExpectUniqueSample(kSearchQueriesBeforeChurnHistogramName,
                                       0, 1);

  // user switches back to brave
  RecordDefaultEngineConversion(GetPrefs());
  for (size_t i = 0; i < 7; i++) {
    RecordLocationBarQuery(GetPrefs());
  }

  histogram_tester_.ExpectTotalCount(kSearchQueriesBeforeChurnHistogramName, 1);

  // user switches away from brave again
  RecordDefaultEngineChurn(GetPrefs());
  histogram_tester_.ExpectTotalCount(kSearchQueriesBeforeChurnHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kSearchQueriesBeforeChurnHistogramName, 4,
                                      1);

  // use switches away again, should not record since the user already churned
  // before and they have not made any queries
  RecordDefaultEngineConversion(GetPrefs());
  RecordDefaultEngineChurn(GetPrefs());
  histogram_tester_.ExpectTotalCount(kSearchQueriesBeforeChurnHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kSearchQueriesBeforeChurnHistogramName, 4,
                                      1);
}

}  // namespace brave_search_conversion::p3a
