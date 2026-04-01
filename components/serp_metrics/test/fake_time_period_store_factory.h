/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TEST_FAKE_TIME_PERIOD_STORE_FACTORY_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TEST_FAKE_TIME_PERIOD_STORE_FACTORY_H_

#include <memory>

#include "brave/components/time_period_storage/time_period_store.h"
#include "brave/components/time_period_storage/time_period_store_factory.h"

namespace serp_metrics {

// `TimePeriodStoreFactory` that produces `FakeTimePeriodStore` instances for
// use in unit tests.
class FakeTimePeriodStoreFactory : public TimePeriodStoreFactory {
 public:
  FakeTimePeriodStoreFactory();

  FakeTimePeriodStoreFactory(const FakeTimePeriodStoreFactory&) = delete;
  FakeTimePeriodStoreFactory& operator=(const FakeTimePeriodStoreFactory&) =
      delete;

  ~FakeTimePeriodStoreFactory() override;

  std::unique_ptr<TimePeriodStore> Build(
      const char* /*metric_name*/) const override;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TEST_FAKE_TIME_PERIOD_STORE_FACTORY_H_
