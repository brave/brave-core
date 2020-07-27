/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_p2a.h"

#include <stdint.h>

#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {
namespace {

const char kAdViewConfirmationCountHistogramName[] =
    "Brave.P2A.ViewConfirmationCount";
const uint16_t kAdViewConfirmationCountIntervals[] =
    { 0, 5, 10, 20, 50, 100, 250, 500 };

void EmitAdViewConfirmationHistogram(uint64_t number_of_confirmations) {
    const uint16_t* it = std::lower_bound(kAdViewConfirmationCountIntervals,
    std::end(kAdViewConfirmationCountIntervals), number_of_confirmations);
  const uint16_t answer = it - kAdViewConfirmationCountIntervals;
  EmitConfirmationsCountMetric(answer);
}

}  // namespace

void RegisterP2APrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kAdViewConfirmationCountPrefName);
}

void RecordEventInWeeklyStorage(
    PrefService* prefs,
    const std::string& pref_name) {
  if (pref_name == prefs::kAdViewConfirmationCountPrefName) {
    WeeklyStorage storage(prefs, prefs::kAdViewConfirmationCountPrefName);
    storage.AddDelta(1);
    EmitAdViewConfirmationHistogram(storage.GetWeeklySum());
  }
}

void EmitConfirmationsCountMetric(int answer) {
  UMA_HISTOGRAM_EXACT_LINEAR(kAdViewConfirmationCountHistogramName,
                             answer, 8);
}

}  // namespace brave_ads
