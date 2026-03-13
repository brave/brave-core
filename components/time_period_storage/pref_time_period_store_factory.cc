/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/time_period_storage/pref_time_period_store_factory.h"

#include <memory>

#include "brave/components/time_period_storage/pref_time_period_store.h"

PrefTimePeriodStoreFactory::PrefTimePeriodStoreFactory(PrefService* prefs,
                                                       const char* pref_name)
    : prefs_(prefs), pref_name_(pref_name) {}

std::unique_ptr<TimePeriodStore> PrefTimePeriodStoreFactory::Build(
    const char* metric_name) const {
  return std::make_unique<PrefTimePeriodStore>(prefs_, pref_name_, metric_name);
}
