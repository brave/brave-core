/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/pref_metric.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"

namespace p3a {

namespace {

bool ParseValueMap(const base::Value* value, base::Value::Dict* out) {
  if (!value->is_dict()) {
    return false;
  }
  *out = value->GetDict().Clone();
  return true;
}

std::optional<std::string> PrefValueToString(const base::Value* value) {
  if (!value) {
    return std::nullopt;
  }

  switch (value->type()) {
    case base::Value::Type::STRING:
      return value->GetString();
    case base::Value::Type::BOOLEAN:
      return value->GetBool() ? "true" : "false";
    case base::Value::Type::INTEGER:
      return base::NumberToString(value->GetInt());
    default:
      return std::nullopt;
  }
}

}  // namespace

PrefMetricDefinition::PrefMetricDefinition() = default;
PrefMetricDefinition::~PrefMetricDefinition() = default;
PrefMetricDefinition::PrefMetricDefinition(PrefMetricDefinition&& other) =
    default;

// static
void PrefMetricDefinition::RegisterJSONConverter(
    base::JSONValueConverter<PrefMetricDefinition>* converter) {
  converter->RegisterStringField("pref_name", &PrefMetricDefinition::pref_name);
  converter->RegisterCustomValueField(
      "value_map", &PrefMetricDefinition::value_map, &ParseValueMap);
  converter->RegisterBoolField("use_profile_prefs",
                               &PrefMetricDefinition::use_profile_prefs);
}

bool PrefMetricDefinition::Validate() const {
  return !pref_name.empty() && !value_map.empty();
}

// PrefMetric implementation
PrefMetric::PrefMetric(PrefService* local_state,
                       PrefMetricDefinition definition,
                       base::RepeatingCallback<void(size_t)> update_callback)
    : definition_(std::move(definition)),
      update_callback_(std::move(update_callback)) {
  // If we're not using profile prefs, set up monitoring on local state
  // immediately
  if (!definition_.use_profile_prefs) {
    current_prefs_ = local_state;
    UpdateMetric();
  }
}

PrefMetric::~PrefMetric() = default;

void PrefMetric::HandleHistogramChange(std::string_view histogram_name,
                                       size_t sample) {}

std::vector<std::string_view> PrefMetric::GetSourceHistogramNames() const {
  return {};
}

std::optional<std::string_view> PrefMetric::GetStorageKey() const {
  return std::nullopt;
}

void PrefMetric::OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) {
  if (!definition_.use_profile_prefs) {
    return;
  }

  pref_change_registrar_.Reset();

  current_prefs_ = profile_prefs;
  UpdateMetric();
}

void PrefMetric::UpdateMetric() {
  if (!current_prefs_) {
    return;
  }

  const auto* pref = current_prefs_->FindPreference(definition_.pref_name);
  if (!pref) {
    // Preference not found, no update
    return;
  }

  if (pref_change_registrar_.IsEmpty()) {
    pref_change_registrar_.Init(current_prefs_);
    pref_change_registrar_.Add(
        definition_.pref_name,
        base::BindRepeating(&PrefMetric::UpdateMetric, base::Unretained(this)));
  }

  // Convert the preference value to string for lookup
  auto string_value = PrefValueToString(pref->GetValue());
  if (!string_value) {
    return;
  }

  // Look up the value in the map
  auto metric_value = definition_.value_map.FindInt(*string_value);
  if (metric_value) {
    update_callback_.Run(*metric_value);
  }
}

}  // namespace p3a
