/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/pref_registry.h"

#include "components/prefs/pref_registry_simple.h"

namespace serp_metrics {

namespace {

inline constexpr char kSerpMetricsTimePeriodStorage[] =
    "brave.stats.serp_metrics";

}  // namespace

void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry) {
  // Added 2026-02
  registry->RegisterDictionaryPref(kSerpMetricsTimePeriodStorage);
}

}  // namespace serp_metrics
