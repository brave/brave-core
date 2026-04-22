/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_pref_time_period_store.h"

#include <string_view>
#include <utility>

#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace serp_metrics {

SerpMetricsPrefTimePeriodStore::SerpMetricsPrefTimePeriodStore(
    PrefService* prefs,
    std::string_view pref_name,
    std::string_view pref_key)
    : prefs_(prefs), pref_name_(pref_name), pref_key_(pref_key) {
  CHECK(!pref_name.empty());
  CHECK(!pref_key.empty());
}

SerpMetricsPrefTimePeriodStore::~SerpMetricsPrefTimePeriodStore() = default;

const base::ListValue* SerpMetricsPrefTimePeriodStore::Get() {
  const base::Value& pref_value = prefs_->GetValue(pref_name_);
  const base::DictValue* const dict = pref_value.GetIfDict();
  if (!dict) {
    return nullptr;
  }
  return dict->FindList(pref_key_);
}

void SerpMetricsPrefTimePeriodStore::Set(base::ListValue list) {
  ScopedDictPrefUpdate update(prefs_.get(), pref_name_);
  update->Set(pref_key_, std::move(list));
}

void SerpMetricsPrefTimePeriodStore::Clear() {
  ScopedDictPrefUpdate update(prefs_.get(), pref_name_);
  update->Remove(pref_key_);
}

}  // namespace serp_metrics
