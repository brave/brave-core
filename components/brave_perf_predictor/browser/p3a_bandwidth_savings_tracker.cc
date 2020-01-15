/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/p3a_bandwidth_savings_tracker.h"

#include <memory>
#include <numeric>
#include <utility>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_perf_predictor/browser/p3a_bandwidth_savings_permanent_state.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_perf_predictor {

P3ABandwidthSavingsTracker::P3ABandwidthSavingsTracker(PrefService* user_prefs)
    : user_prefs_(user_prefs) {}

void P3ABandwidthSavingsTracker::RecordSavings(uint64_t savings) {
  if (savings > 0) {
    // TODO(AndriusA): optimise if needed, loading permanent state on every
    // record could be costly
    std::unique_ptr<P3ABandwidthSavingsPermanentState> permanent_state =
        std::make_unique<P3ABandwidthSavingsPermanentState>(user_prefs_);
    permanent_state->AddSavings(savings);
  }
}

P3ABandwidthSavingsTracker::~P3ABandwidthSavingsTracker() = default;

// static
void P3ABandwidthSavingsTracker::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kBandwidthSavedDailyBytes);
}

}  // namespace brave_perf_predictor
