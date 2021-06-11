/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_p2a.h"

#include <cstdint>
#include <map>
#include <string>

#include "base/metrics/histogram_functions.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {
namespace {

constexpr const char* kP2AQuestionNameList[] = {
    // Ad Opportunities
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
    "Brave.P2A.AdOpportunitiesPerSegment.untargeted",
    // Ad Impressions
    "Brave.P2A.TotalAdImpressions",
    "Brave.P2A.AdImpressionsPerSegment.architecture",
    "Brave.P2A.AdImpressionsPerSegment.artsentertainment",
    "Brave.P2A.AdImpressionsPerSegment.automotive",
    "Brave.P2A.AdImpressionsPerSegment.business",
    "Brave.P2A.AdImpressionsPerSegment.careers",
    "Brave.P2A.AdImpressionsPerSegment.cellphones",
    "Brave.P2A.AdImpressionsPerSegment.crypto",
    "Brave.P2A.AdImpressionsPerSegment.education",
    "Brave.P2A.AdImpressionsPerSegment.familyparenting",
    "Brave.P2A.AdImpressionsPerSegment.fashion",
    "Brave.P2A.AdImpressionsPerSegment.folklore",
    "Brave.P2A.AdImpressionsPerSegment.fooddrink",
    "Brave.P2A.AdImpressionsPerSegment.gaming",
    "Brave.P2A.AdImpressionsPerSegment.healthfitness",
    "Brave.P2A.AdImpressionsPerSegment.history",
    "Brave.P2A.AdImpressionsPerSegment.hobbiesinterests",
    "Brave.P2A.AdImpressionsPerSegment.home",
    "Brave.P2A.AdImpressionsPerSegment.law",
    "Brave.P2A.AdImpressionsPerSegment.military",
    "Brave.P2A.AdImpressionsPerSegment.other",
    "Brave.P2A.AdImpressionsPerSegment.personalfinance",
    "Brave.P2A.AdImpressionsPerSegment.pets",
    "Brave.P2A.AdImpressionsPerSegment.realestate",
    "Brave.P2A.AdImpressionsPerSegment.science",
    "Brave.P2A.AdImpressionsPerSegment.sports",
    "Brave.P2A.AdImpressionsPerSegment.technologycomputing",
    "Brave.P2A.AdImpressionsPerSegment.travel",
    "Brave.P2A.AdImpressionsPerSegment.weather",
    "Brave.P2A.AdImpressionsPerSegment.untargeted"};

const uint16_t kIntervalBuckets[] = {0, 5, 10, 20, 50, 100, 250, 500};

}  // namespace

void RegisterP2APrefs(PrefRegistrySimple* registry) {
  for (const char* question_name : kP2AQuestionNameList) {
    std::string pref_path(prefs::kP2AStoragePrefNamePrefix);
    pref_path.append(question_name);
    registry->RegisterListPref(pref_path);
  }
}

void RecordInWeeklyStorageAndEmitP2AHistogramAnswer(PrefService* prefs,
                                                    const std::string& name) {
  std::string pref_path(prefs::kP2AStoragePrefNamePrefix);
  pref_path.append(name);
  if (!prefs->FindPreference(pref_path)) {
    return;
  }
  WeeklyStorage storage(prefs, pref_path.c_str());
  storage.AddDelta(1);
  EmitP2AHistogramAnswer(name, storage.GetWeeklySum());
}

void EmitP2AHistogramAnswer(const std::string& name, uint16_t count_value) {
  const uint16_t* iter = std::lower_bound(
      kIntervalBuckets, std::end(kIntervalBuckets), count_value);
  const uint16_t bucket = iter - kIntervalBuckets;

  for (const char* question_name : kP2AQuestionNameList) {
    if (name != question_name) {
      continue;
    }

    base::UmaHistogramExactLinear(question_name, bucket,
                                  base::size(kIntervalBuckets) + 1);
  }
}

void SuspendP2AHistograms() {
  // Record "special value" to prevent sending this week's data to P2A server.
  // Matches INT_MAX - 1 for |kSuspendedMetricValue| in |brave_p3a_service.cc|
  for (const char* question_name : kP2AQuestionNameList) {
    base::UmaHistogramExactLinear(question_name, INT_MAX,
                                  base::size(kIntervalBuckets) + 1);
  }
}

}  // namespace brave_ads
