// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/p3a_pref_names.h"

#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_news::p3a::prefs {

void RegisterProfileNewsMetricsPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kBraveNewsWeeklySessionCount);
  registry->RegisterListPref(kBraveNewsWeeklyDisplayAdViewedCount);
  registry->RegisterListPref(kBraveNewsWeeklyAddedDirectFeedsCount);
  registry->RegisterListPref(kBraveNewsTotalCardViews);
  registry->RegisterListPref(kBraveNewsTotalCardVisits);
  registry->RegisterListPref(kBraveNewsVisitDepthSum);
  registry->RegisterListPref(kBraveNewsTotalSidebarFilterUsages);
  p3a_utils::RegisterFeatureUsagePrefs(
      registry, prefs::kBraveNewsFirstSessionTime,
      prefs::kBraveNewsLastSessionTime, prefs::kBraveNewsUsedSecondDay, nullptr,
      nullptr);
  registry->RegisterBooleanPref(kBraveNewsWasEverEnabled, false);
}

void RegisterProfileNewsMetricsPrefsForMigration(PrefRegistrySimple* registry) {
  // Reserved for future deprecated P3A-related prefs
}

void MigrateObsoleteProfileNewsMetricsPrefs(PrefService* prefs) {
  // Reserved for future deprecated P3A-related prefs
}

}  // namespace brave_news::p3a::prefs
