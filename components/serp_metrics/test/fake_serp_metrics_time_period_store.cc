/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/test/fake_serp_metrics_time_period_store.h"

#include <memory>
#include <string_view>

#include "base/values.h"

namespace serp_metrics::test {

const base::ListValue* FakeSerpMetricsTimePeriodStore::Get() {
  return &list_;
}

void FakeSerpMetricsTimePeriodStore::Set(base::ListValue list) {
  list_ = std::move(list);
}

void FakeSerpMetricsTimePeriodStore::Clear() {
  list_.clear();
}

std::unique_ptr<SerpMetricsTimePeriodStore>
FakeSerpMetricsTimePeriodStoreFactory::Build(
    std::string_view /*pref_key*/) const {
  return std::make_unique<FakeSerpMetricsTimePeriodStore>();
}

}  // namespace serp_metrics::test
