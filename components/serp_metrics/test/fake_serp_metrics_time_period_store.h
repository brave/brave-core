/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TEST_FAKE_SERP_METRICS_TIME_PERIOD_STORE_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TEST_FAKE_SERP_METRICS_TIME_PERIOD_STORE_H_

#include <memory>
#include <string_view>

#include "base/values.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store_factory.h"

namespace serp_metrics::test {

// In-memory `SerpMetricsTimePeriodStore` for use in tests.
class FakeSerpMetricsTimePeriodStore final : public SerpMetricsTimePeriodStore {
 public:
  FakeSerpMetricsTimePeriodStore() = default;

  FakeSerpMetricsTimePeriodStore(const FakeSerpMetricsTimePeriodStore&) =
      delete;
  FakeSerpMetricsTimePeriodStore& operator=(
      const FakeSerpMetricsTimePeriodStore&) = delete;

  ~FakeSerpMetricsTimePeriodStore() override = default;

  const base::ListValue* Get() override;

  void Set(base::ListValue list) override;

  void Clear() override;

 private:
  base::ListValue list_;
};

// `SerpMetricsTimePeriodStoreFactory` that builds
// `FakeSerpMetricsTimePeriodStore` instances.
class FakeSerpMetricsTimePeriodStoreFactory final
    : public SerpMetricsTimePeriodStoreFactory {
 public:
  FakeSerpMetricsTimePeriodStoreFactory() = default;

  FakeSerpMetricsTimePeriodStoreFactory(
      const FakeSerpMetricsTimePeriodStoreFactory&) = delete;
  FakeSerpMetricsTimePeriodStoreFactory& operator=(
      const FakeSerpMetricsTimePeriodStoreFactory&) = delete;

  ~FakeSerpMetricsTimePeriodStoreFactory() override = default;

  std::unique_ptr<SerpMetricsTimePeriodStore> Build(
      std::string_view pref_key) const override;
};

}  // namespace serp_metrics::test

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TEST_FAKE_SERP_METRICS_TIME_PERIOD_STORE_H_
