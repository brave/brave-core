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

// Note: append-only array! Never remove any existing values, as this array
// is used to bucket a UMA histogram, and removing values breaks that.
constexpr std::array<uint64_t, 5> kSpeedReaderToggleBuckets{
    0,   // 0
    5,   // >0-5
    10,  // 6-10
    20,  // 11-20
    30,  // 21-30
         // 30+ => bucket 5
};

// Note: append-only enumeration! Never remove any existing values, as this enum
// is used to bucket a UMA histogram, and removing values breaks that.
enum class EnabledStatus {
  kUnused,
  kEverEnabled,
  kRecentlyUsed,
  kMaxValue = kRecentlyUsed
};

constexpr char kSpeedreaderToggleUMAHistogramName[] =
    "Brave.SpeedReader.ToggleCount";

constexpr char kSpeedreaderEnabledUMAHistogramName[] =
    "Brave.SpeedReader.Enabled";

void StoreTogglesHistogram(uint64_t toggles) {
  int bucket = 0;
  for (const auto& bucket_upper_bound : kSpeedReaderToggleBuckets) {
    if (toggles > bucket_upper_bound)
      bucket = bucket + 1;
  }

  UMA_HISTOGRAM_EXACT_LINEAR(kSpeedreaderToggleUMAHistogramName, bucket, 5);
}

void RecordHistograms(PrefService* prefs, bool toggled, bool enabled_now) {
  WeeklyStorage weekly_toggles(prefs, kSpeedreaderPrefToggleCount);
  if (toggled)
    weekly_toggles.AddDelta(1);
  const uint64_t toggle_count = weekly_toggles.GetWeeklySum();
  StoreTogglesHistogram(toggle_count);

  // Has been "recently" enabled if currently enabled,
  // or got disabled this week.
  EnabledStatus status = EnabledStatus::kUnused;
  if (enabled_now || toggle_count > 0) {
    status = EnabledStatus::kRecentlyUsed;
  } else if (prefs->GetBoolean(kSpeedreaderPrefEverEnabled)) {
    status = EnabledStatus::kEverEnabled;
  }

  UMA_HISTOGRAM_ENUMERATION(kSpeedreaderEnabledUMAHistogramName, status);
}

}  // namespace

SpeedreaderService::SpeedreaderService(PrefService* prefs) : prefs_(prefs) {}

SpeedreaderService::~SpeedreaderService() {}

// static
void SpeedreaderService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kSpeedreaderPrefEnabled, false);
  registry->RegisterBooleanPref(kSpeedreaderPrefEverEnabled, false);
  registry->RegisterListPref(kSpeedreaderPrefToggleCount);
}

void SpeedreaderService::ToggleSpeedreader() {
  const bool enabled = prefs_->GetBoolean(kSpeedreaderPrefEnabled);
  prefs_->SetBoolean(kSpeedreaderPrefEnabled, !enabled);
  if (!enabled)
    prefs_->SetBoolean(kSpeedreaderPrefEverEnabled, true);
  RecordHistograms(prefs_, true,
                   !enabled);  // toggling - now enabled
}

bool SpeedreaderService::IsEnabled() {
  if (!base::FeatureList::IsEnabled(kSpeedreaderFeature)) {
    return false;
  }

  const bool enabled = prefs_->GetBoolean(kSpeedreaderPrefEnabled);
  RecordHistograms(prefs_, false, enabled);
  return enabled;
}

}  // namespace speedreader
