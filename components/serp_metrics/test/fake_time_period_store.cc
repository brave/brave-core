/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/test/fake_time_period_store.h"

#include <utility>

#include "base/values.h"

namespace serp_metrics {

FakeTimePeriodStore::FakeTimePeriodStore() = default;

FakeTimePeriodStore::~FakeTimePeriodStore() = default;

const base::ListValue* FakeTimePeriodStore::Get() {
  return &list_;
}

void FakeTimePeriodStore::Set(base::ListValue list) {
  list_ = std::move(list);
}

void FakeTimePeriodStore::Clear() {
  list_.clear();
}

}  // namespace serp_metrics
