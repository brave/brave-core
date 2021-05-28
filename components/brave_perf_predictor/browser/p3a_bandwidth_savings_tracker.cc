/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/p3a_bandwidth_savings_tracker.h"

#include <memory>
#include <utility>

#include "base/metrics/histogram_macros.h"
#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_perf_predictor {

namespace {

// Note: append-only enumeration! Never remove any existing values, as this enum
// is used to bucket a UMA histogram, and removing values breaks that.
constexpr std::array<uint64_t, 7> kBandwidthSavingsBuckets{
    0,    // 0
    50,   // >0-50mb
    100,  // 51-100mb
    200,  // 101-200mb
    400,  // 201-400mb
    700,  // 401-700mb
    1500  // 701-1500mb
          // >1501 => bucket 7
};

constexpr char kSavingsDailyUMAHistogramName[] =
    "Brave.Savings.BandwidthSavingsMB";

}  // namespace

P3ABandwidthSavingsTracker::P3ABandwidthSavingsTracker(PrefService* user_prefs)
    : P3ABandwidthSavingsTracker(user_prefs,
                                 std::make_unique<base::DefaultClock>()) {}

P3ABandwidthSavingsTracker::P3ABandwidthSavingsTracker(
    PrefService* user_prefs,
    std::unique_ptr<base::Clock> clock)
    : user_prefs_(user_prefs), clock_(std::move(clock)) {}

void P3ABandwidthSavingsTracker::RecordSavings(uint64_t savings) {
  if (savings > 0 && user_prefs_) {
    // TODO(AndriusA): optimise if needed, loading permanent state on every
    // record could be costly
    WeeklyStorage weekly(user_prefs_, prefs::kBandwidthSavedDailyBytes);
    weekly.AddDelta(savings);
    StoreSavingsHistogram(weekly.GetWeeklySum());
  }
}

P3ABandwidthSavingsTracker::~P3ABandwidthSavingsTracker() = default;

// static
void P3ABandwidthSavingsTracker::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  if (registry)
    registry->RegisterListPref(prefs::kBandwidthSavedDailyBytes);
}

void P3ABandwidthSavingsTracker::StoreSavingsHistogram(uint64_t savings_bytes) {
  int bucket = 0;
  // divide by 1024*1024 = 2^20 to convert bytes -> MB
  uint64_t total_mb = savings_bytes >> 20;
  for (const auto& bucketUpperBound : kBandwidthSavingsBuckets) {
    if (total_mb > bucketUpperBound)
      bucket = bucket + 1;
  }

  UMA_HISTOGRAM_EXACT_LINEAR(kSavingsDailyUMAHistogramName, bucket, 7);
}

}  // namespace brave_perf_predictor
