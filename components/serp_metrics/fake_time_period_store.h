/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_FAKE_TIME_PERIOD_STORE_H_
#define BRAVE_COMPONENTS_SERP_METRICS_FAKE_TIME_PERIOD_STORE_H_

#include <memory>

#include "base/values.h"
#include "brave/components/time_period_storage/time_period_store.h"
#include "brave/components/time_period_storage/time_period_store_factory.h"

namespace serp_metrics::test {

// In-memory `TimePeriodStore` for use in tests.
class FakeTimePeriodStore : public TimePeriodStore {
 public:
  FakeTimePeriodStore() = default;

  FakeTimePeriodStore(const FakeTimePeriodStore&) = delete;
  FakeTimePeriodStore& operator=(const FakeTimePeriodStore&) = delete;

  ~FakeTimePeriodStore() override = default;

  const base::ListValue* Get() override;

  void Set(base::ListValue list) override;

  void Clear() override;

 private:
  base::ListValue list_;
};

// `TimePeriodStoreFactory` that builds `FakeTimePeriodStore` instances.
class FakeTimePeriodStoreFactory : public TimePeriodStoreFactory {
 public:
  FakeTimePeriodStoreFactory() = default;

  FakeTimePeriodStoreFactory(const FakeTimePeriodStoreFactory&) = delete;
  FakeTimePeriodStoreFactory& operator=(const FakeTimePeriodStoreFactory&) =
      delete;

  ~FakeTimePeriodStoreFactory() override = default;

  std::unique_ptr<TimePeriodStore> Build(
      const char* metric_name) const override;
};

}  // namespace serp_metrics::test

#endif  // BRAVE_COMPONENTS_SERP_METRICS_FAKE_TIME_PERIOD_STORE_H_
