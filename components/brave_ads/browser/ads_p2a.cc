/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_p2a.h"

#include <cstdint>

#include "base/containers/contains.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {

namespace {

constexpr const char* kAllowedEvents[] = {
    "Brave.P2A.TotalAdOpportunities",
    "Brave.P2A.AdOpportunitiesPerSegment.architecture",
    "Brave.P2A.AdOpportunitiesPerSegment.artsentertainment",
    "Brave.P2A.AdOpportunitiesPerSegment.automotive",
    "Brave.P2A.AdOpportunitiesPerSegment.business",
    "Brave.P2A.AdOpportunitiesPerSegment.careers",
    "Brave.P2A.AdOpportunitiesPerSegment.cellphones",
    "Brave.P2A.AdOpportunitiesPerSegment.crypto",
    "Brave.P2A.AdOpportunitiesPerSegment.education",
    "Brave.P2A.AdOpportunitiesPerSegment.familyparenting",
    "Brave.P2A.AdOpportunitiesPerSegment.fashion",
    "Brave.P2A.AdOpportunitiesPerSegment.folklore",
    "Brave.P2A.AdOpportunitiesPerSegment.fooddrink",
    "Brave.P2A.AdOpportunitiesPerSegment.gaming",
    "Brave.P2A.AdOpportunitiesPerSegment.healthfitness",
    "Brave.P2A.AdOpportunitiesPerSegment.history",
    "Brave.P2A.AdOpportunitiesPerSegment.hobbiesinterests",
    "Brave.P2A.AdOpportunitiesPerSegment.home",
    "Brave.P2A.AdOpportunitiesPerSegment.law",
    "Brave.P2A.AdOpportunitiesPerSegment.military",
    "Brave.P2A.AdOpportunitiesPerSegment.other",
    "Brave.P2A.AdOpportunitiesPerSegment.personalfinance",
    "Brave.P2A.AdOpportunitiesPerSegment.pets",
    "Brave.P2A.AdOpportunitiesPerSegment.realestate",
    "Brave.P2A.AdOpportunitiesPerSegment.science",
    "Brave.P2A.AdOpportunitiesPerSegment.sports",
    "Brave.P2A.AdOpportunitiesPerSegment.technologycomputing",
    "Brave.P2A.AdOpportunitiesPerSegment.travel",
    "Brave.P2A.AdOpportunitiesPerSegment.weather",
    "Brave.P2A.AdOpportunitiesPerSegment.untargeted"};

constexpr size_t kIntervalBuckets[] = {0, 5, 10, 20, 50, 100, 250, 500};

void EmitP2AHistogramName(const std::string& name, uint16_t sum) {
  CHECK(base::Contains(kAllowedEvents, name));

  const size_t* const iter =
      std::lower_bound(kIntervalBuckets, std::end(kIntervalBuckets), sum);
  const size_t bucket = iter - kIntervalBuckets;

  base::UmaHistogramExactLinear(name, static_cast<int>(bucket),
                                std::size(kIntervalBuckets) + 1);
}

}  // namespace

void RegisterP2APrefs(PrefRegistrySimple* registry) {
  for (const char* const event : kAllowedEvents) {
    const std::string pref_path =
        base::StrCat({prefs::kP2AStoragePrefNamePrefix, event});
    registry->RegisterListPref(pref_path);
  }
}

void RecordInWeeklyStorageAndEmitP2AHistogramName(PrefService* prefs,
                                                  const std::string& name) {
  CHECK(prefs);

  if (!base::Contains(kAllowedEvents, name)) {
    return;
  }

  const std::string pref_path =
      base::StrCat({prefs::kP2AStoragePrefNamePrefix, name});
  if (!prefs->FindPreference(pref_path)) {
    return;
  }

  WeeklyStorage storage(prefs, pref_path.c_str());
  storage.AddDelta(1);

  EmitP2AHistogramName(name, storage.GetWeeklySum());
}

void SuspendP2AHistograms() {
  // Record "special value" to prevent sending this week's data to P2A server.
  // Matches INT_MAX - 1 for |kSuspendedMetricValue| in |brave_p3a_service.cc|.
  for (const char* const event : kAllowedEvents) {
    base::UmaHistogramExactLinear(event, INT_MAX,
                                  std::size(kIntervalBuckets) + 1);
  }

  VLOG(1) << "P2A histograms suspended";
}

}  // namespace brave_ads
