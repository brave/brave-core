/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TEST_FAKE_TIME_PERIOD_STORE_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TEST_FAKE_TIME_PERIOD_STORE_H_

#include "base/values.h"
#include "brave/components/time_period_storage/time_period_store.h"

namespace serp_metrics {

// In-memory `TimePeriodStore` for use in unit tests. Stores data in a
// `base::ListValue` held in memory rather than persisting to prefs.
class FakeTimePeriodStore : public TimePeriodStore {
 public:
  FakeTimePeriodStore();

  FakeTimePeriodStore(const FakeTimePeriodStore&) = delete;
  FakeTimePeriodStore& operator=(const FakeTimePeriodStore&) = delete;

  ~FakeTimePeriodStore() override;

  const base::ListValue* Get() override;
  void Set(base::ListValue list) override;
  void Clear() override;

 private:
  base::ListValue list_;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TEST_FAKE_TIME_PERIOD_STORE_H_
