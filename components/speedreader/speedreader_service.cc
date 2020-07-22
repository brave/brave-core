/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_service.h"

#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/speedreader/features.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace speedreader {

namespace {

// Note: append-only enumeration! Never remove any existing values, as this enum
// is used to bucket a UMA histogram, and removing values breaks that.
constexpr std::array<uint64_t, 5> kSpeedReaderToggleBuckets{
    0,   // 0
    5,   // >0-5
    10,  // 6-10
    20,  // 11-20
    30,  // 21-30
         // 30+ => bucket 5
};

constexpr char kSpeedreaderToggleUMAHistogramName[] =
    "Brave.SpeedReader.ToggleCount";

constexpr char kSpeedreaderEnabledUMAHistogramName[] =
    "Brave.SpeedReader.Enabled";

void storeTogglesHistogram(uint64_t toggles) {
  int bucket = 0;
  for (const auto& bucketUpperBound : kSpeedReaderToggleBuckets) {
    if (toggles > bucketUpperBound)
      bucket = bucket + 1;
  }

  UMA_HISTOGRAM_EXACT_LINEAR(kSpeedreaderToggleUMAHistogramName, bucket, 5);
}

void storeSpeedReaderEnabledHistogram(PrefService* prefs_,
                                      bool toggled,
                                      bool enabledNow) {
  if (toggled) {
    WeeklyStorage weekly(prefs_, kSpeedreaderPrefToggleCount);
    weekly.AddDelta(1);
    storeTogglesHistogram(weekly.GetWeeklySum());
  }

  if (toggled && enabledNow) {
    // Switched on now
    prefs_->SetBoolean(kSpeedreaderPrefEverEnabled, true);
    UMA_HISTOGRAM_EXACT_LINEAR(kSpeedreaderEnabledUMAHistogramName, 2, 3);
  } else {
    // user has had the feature enabled before pref started getting tracked
    const bool ever =
        prefs_->GetBoolean(kSpeedreaderPrefEverEnabled) || enabledNow;
    UMA_HISTOGRAM_EXACT_LINEAR(kSpeedreaderEnabledUMAHistogramName,
                               ever ? 1 : 0, 3);
  }
}

}  // namespace

SpeedreaderService::SpeedreaderService(PrefService* prefs) : prefs_(prefs) {}

SpeedreaderService::~SpeedreaderService() {}

// static
void SpeedreaderService::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kSpeedreaderPrefEnabled, false);
  registry->RegisterBooleanPref(kSpeedreaderPrefEverEnabled, false);
  registry->RegisterListPref(kSpeedreaderPrefToggleCount);
}

void SpeedreaderService::ToggleSpeedreader() {
  const bool enabled = prefs_->GetBoolean(kSpeedreaderPrefEnabled);
  prefs_->SetBoolean(kSpeedreaderPrefEnabled, !enabled);
  storeSpeedReaderEnabledHistogram(prefs_, true,
                                   !enabled);  // toggling - now enabled
}

bool SpeedreaderService::IsEnabled() {
  if (!base::FeatureList::IsEnabled(kSpeedreaderFeature)) {
    return false;
  }

  const bool enabled = prefs_->GetBoolean(kSpeedreaderPrefEnabled);
  storeSpeedReaderEnabledHistogram(prefs_, false, enabled);
  return enabled;
}

}  // namespace speedreader
