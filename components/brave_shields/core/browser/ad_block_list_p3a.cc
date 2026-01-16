// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_list_p3a.h"

#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

namespace {

constexpr char kEnabledDictKey[] = "enabled";
constexpr int kFilterListCountBuckets[] = {0, 1, 2, 5};

}  // namespace

AdBlockListP3A::AdBlockListP3A(PrefService* local_state)
    : local_state_(local_state) {
  if (local_state_) {
    pref_change_registrar_.Init(local_state_);
    pref_change_registrar_.Add(
        prefs::kAdBlockOnlyModeEnabled,
        base::BindRepeating(&AdBlockListP3A::ReportAdBlockOnlyEnabled,
                            base::Unretained(this)));
    ReportAdBlockOnlyEnabled();
  }
}

AdBlockListP3A::~AdBlockListP3A() = default;

void AdBlockListP3A::ReportFilterListUsage() {
  const auto& regional_filter_dict =
      local_state_->GetDict(prefs::kAdBlockRegionalFilters);

  int count = 0;
  for (const auto [uuid, dict_value] : regional_filter_dict) {
    const auto* dict = dict_value.GetIfDict();
    if (!dict || !dict->FindBool(kEnabledDictKey).value_or(false)) {
      continue;
    }
    if (default_filter_list_uuids_.contains(uuid)) {
      continue;
    }
    count++;
  }

  p3a_utils::RecordToHistogramBucket(kFilterListUsageHistogramName,
                                     kFilterListCountBuckets, count);
}

void AdBlockListP3A::OnFilterListCatalogLoaded(
    const std::vector<FilterListCatalogEntry>& entries,
    const std::string& locale) {
  for (const auto& entry : entries) {
    if (entry.default_enabled) {
      default_filter_list_uuids_.insert(entry.uuid);
    }
  }
  for (const auto& entry : FindAdBlockFilterListsByLocale(entries, locale)) {
    default_filter_list_uuids_.insert(entry.get().uuid);
  }
  ReportFilterListUsage();
}

void AdBlockListP3A::ReportAdBlockOnlyEnabled() {
  if (!base::FeatureList::IsEnabled(features::kAdblockOnlyMode)) {
    return;
  }

  if (!local_state_ ||
      !local_state_->GetBoolean(prefs::kAdBlockOnlyModeEnabled)) {
    return;
  }

  UMA_HISTOGRAM_BOOLEAN(kAdBlockOnlyModeEnabledHistogramName, true);

  adblock_only_mode_report_timer_.Start(
      FROM_HERE, base::Time::Now() + base::Hours(3),
      base::BindOnce(&AdBlockListP3A::ReportAdBlockOnlyEnabled,
                     base::Unretained(this)));
}

}  // namespace brave_shields
