/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/test/fake_time_period_store_factory.h"

#include <memory>

#include "brave/components/serp_metrics/test/fake_time_period_store.h"
#include "brave/components/time_period_storage/time_period_store.h"

namespace serp_metrics {

FakeTimePeriodStoreFactory::FakeTimePeriodStoreFactory() = default;

FakeTimePeriodStoreFactory::~FakeTimePeriodStoreFactory() = default;

std::unique_ptr<TimePeriodStore> FakeTimePeriodStoreFactory::Build(
    const char* /*metric_name*/) const {
  return std::make_unique<FakeTimePeriodStore>();
}

}  // namespace serp_metrics
