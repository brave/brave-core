/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/analytics/p2a/p2a.h"

#include <cstdint>
#include <limits>
#include <string>

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_ads/browser/analytics/p2a/p2a_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {

namespace {

constexpr int kSuspendedMetricValue = std::numeric_limits<int>::max();

std::string GetPrefPath(std::string_view name) {
  return base::StrCat({prefs::kP2APrefPathPrefix, name});
}

bool ShouldRecordAndEmitP2AHistogramName(const PrefService* const prefs,
                                         std::string_view name) {
  CHECK(prefs);

  return kP2AAllowedNames.count(name) > 0 &&
         prefs->FindPreference(GetPrefPath(name)) != nullptr;
}

void EmitP2AHistogramName(std::string_view name, const uint64_t sum) {
  CHECK(kP2AAllowedNames.count(name));

  const size_t* const iter =
      std::lower_bound(std::cbegin(kP2AAnswerIndexIntervals),
                       std::cend(kP2AAnswerIndexIntervals), sum);

  const size_t answer_index =
      std::distance(std::cbegin(kP2AAnswerIndexIntervals), iter);

  const int exclusive_max =
      std::size(kP2AAnswerIndexIntervals) + /*answer_index=8*/ 1;

  base::UmaHistogramExactLinear(name, /*sample*/ static_cast<int>(answer_index),
                                exclusive_max);
}

}  // namespace

void RegisterP2APrefs(PrefRegistrySimple* registry) {
  CHECK(registry);

  for (auto name : kP2AAllowedNames) {
    registry->RegisterListPref(GetPrefPath(name));
  }
}

void RecordAndEmitP2AHistogramName(PrefService* prefs, std::string_view name) {
  CHECK(prefs);

  if (!ShouldRecordAndEmitP2AHistogramName(prefs, name)) {
    return;
  }

  const std::string pref_path = GetPrefPath(name);
  WeeklyStorage weekly_storage(prefs, pref_path.c_str());
  weekly_storage.AddDelta(1);

  EmitP2AHistogramName(name, weekly_storage.GetWeeklySum());
}

void SuspendP2AHistograms() {
  // Record `kSuspendedMetricValue` to prevent sending this week's data to the
  // P2A server. Equivalent to `kSuspendedMetricValue` in `p3a_service.cc`.

  const int exclusive_max =
      std::size(kP2AAnswerIndexIntervals) + /*answer_index=8*/ 1;

  for (auto name : kP2AAllowedNames) {
    base::UmaHistogramExactLinear(name, /*sample*/ kSuspendedMetricValue,
                                  exclusive_max);
  }

  VLOG(1) << "P2A histograms suspended";
}

}  // namespace brave_ads
