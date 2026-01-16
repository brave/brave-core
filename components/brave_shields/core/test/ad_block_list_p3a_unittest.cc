// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_list_p3a.h"

#include <memory>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

class AdBlockListP3ATest : public testing::Test {
 public:
  AdBlockListP3ATest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(features::kAdblockOnlyMode);

    auto* registry = local_state_.registry();
    registry->RegisterBooleanPref(prefs::kAdBlockOnlyModeEnabled, false);
    registry->RegisterDictionaryPref(prefs::kAdBlockRegionalFilters);

    ad_block_list_p3a_ = std::make_unique<AdBlockListP3A>(&local_state_);
  }

  void TearDown() override { ad_block_list_p3a_ = nullptr; }

  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  base::HistogramTester histogram_tester_;
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<AdBlockListP3A> ad_block_list_p3a_;
};

TEST_F(AdBlockListP3ATest, ReportsMetricsOnlyWhenEnabled) {
  histogram_tester_.ExpectTotalCount(kAdBlockOnlyModeEnabledHistogramName, 0);

  local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
  histogram_tester_.ExpectUniqueSample(kAdBlockOnlyModeEnabledHistogramName, 1,
                                       1);

  task_environment_.FastForwardBy(base::Hours(5));
  histogram_tester_.ExpectUniqueSample(kAdBlockOnlyModeEnabledHistogramName, 1,
                                       2);

  local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled, false);
  task_environment_.FastForwardBy(base::Hours(5));
  histogram_tester_.ExpectTotalCount(kAdBlockOnlyModeEnabledHistogramName, 2);
}

TEST_F(AdBlockListP3ATest, ReportFilterListUsage) {
  std::vector<FilterListCatalogEntry> catalog;
  FilterListCatalogEntry default_entry;
  default_entry.uuid = "default-uuid";
  default_entry.default_enabled = true;
  catalog.push_back(default_entry);

  FilterListCatalogEntry locale_entry;
  locale_entry.uuid = "locale-uuid";
  locale_entry.langs = {"en"};
  catalog.push_back(locale_entry);

  // Set up regional filters: 1 default enabled (shouldn't count),
  // 1 locale-specific enabled (shouldn't count), 5 non-default enabled.
  {
    base::Value::Dict regional_filters;
    base::Value::Dict filter_settings;
    filter_settings.Set("enabled", true);
    regional_filters.Set("default-uuid", filter_settings.Clone());
    regional_filters.Set("locale-uuid", filter_settings.Clone());

    for (size_t i = 0; i < 5; i++) {
      regional_filters.Set("regional-uuid-" + base::NumberToString(i),
                           filter_settings.Clone());
    }

    local_state_.SetDict(prefs::kAdBlockRegionalFilters,
                         std::move(regional_filters));
  }

  // Total enabled: 5 regional (excluding default and locale-specific)
  // Bucket 3 = 3-5 lists. If default/locale were counted, would be 7 =
  // bucket 4.
  ad_block_list_p3a_->OnFilterListCatalogLoaded(catalog, "en-US");
  histogram_tester_.ExpectUniqueSample(kFilterListUsageHistogramName, 3, 1);
}

}  // namespace brave_shields
