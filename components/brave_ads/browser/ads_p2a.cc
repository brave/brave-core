/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_p2a.h"

#include <stdint.h>

#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace {

const char kAdViewConfirmationCountHistogramName[] =
    "Brave.Ads.ViewConfirmationCount";
const char kAdViewConfirmationCountPrefName[] =
    "brave.weekly_storage.ad_view_confirmation_count";
// TODO(Moritz Haller): Make sure buckets have reasonable intervals
const uint16_t kAdViewConfirmationCountIntervals[] =
    {0, 5, 10, 20, 50, 100, 500};

}  // namespace

namespace brave_ads {

AdsP2A::AdsP2A() = default;

AdsP2A::~AdsP2A() = default;

void AdsP2A::RegisterPrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterListPref(kAdViewConfirmationCountPrefName);
}

void AdsP2A::RecordEventInWeeklyStorage(
    PrefService* prefs,
    const std::string& pref_name) {
  // TODO(Moritz Haller): Maybe create enum and handle with switch statement
  if (pref_name == kAdViewConfirmationCountPrefName) {
    WeeklyStorage storage(prefs, kAdViewConfirmationCountPrefName);
    // TODO(Moritz Haller): Error handling, what if pref doesn't exist?
    storage.AddDelta(1);
    EmitAdViewConfirmationHistogram(storage.GetWeeklySum());
  }
}

void AdsP2A::EmitAdViewConfirmationHistogram(
    uint64_t number_of_confirmations) {
  const uint16_t* it = std::lower_bound(kAdViewConfirmationCountIntervals,
      std::end(kAdViewConfirmationCountIntervals), number_of_confirmations);
  const uint16_t answer = it - kAdViewConfirmationCountIntervals;
  UMA_HISTOGRAM_EXACT_LINEAR(kAdViewConfirmationCountHistogramName, answer,
      base::size(kAdViewConfirmationCountIntervals));
}

}  // namespace brave_ads
