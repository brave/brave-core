/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_pref_time_period_store_factory.h"

#include <string_view>

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

namespace {

constexpr std::string_view kPrefName = "baz";
constexpr std::string_view kFooPrefKey = "foo";
constexpr std::string_view kBarPrefKey = "bar";

}  // namespace

class SerpMetricsPrefTimePeriodStoreFactoryTest : public ::testing::Test {
 public:
  PrefService& pref_service() { return pref_service_; }

 private:
  TestingPrefServiceSimple pref_service_;
};

TEST_F(SerpMetricsPrefTimePeriodStoreFactoryTest, BuildReturnsNonNullStore) {
  // Arrange
  const SerpMetricsPrefTimePeriodStoreFactory factory(&pref_service(),
                                                      kPrefName);

  // Act & Assert
  EXPECT_TRUE(factory.Build(kFooPrefKey));
}

TEST_F(SerpMetricsPrefTimePeriodStoreFactoryTest,
       BuildWithDifferentPrefKeysReturnsDifferentInstances) {
  // Arrange
  const SerpMetricsPrefTimePeriodStoreFactory factory(&pref_service(),
                                                      kPrefName);

  // Act
  const std::unique_ptr<SerpMetricsTimePeriodStore> foo_store =
      factory.Build(kFooPrefKey);
  const std::unique_ptr<SerpMetricsTimePeriodStore> bar_store =
      factory.Build(kBarPrefKey);

  // Assert
  EXPECT_NE(foo_store, bar_store);
}

}  // namespace serp_metrics
