/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/time_period_events_intermediate.h"

#include <utility>

#include "base/values.h"
#include "brave/components/p3a/utils.h"
#include "brave/components/time_period_storage/time_period_storage.h"

namespace p3a {

TimePeriodEventsIntermediateDefinition::
    TimePeriodEventsIntermediateDefinition() = default;
TimePeriodEventsIntermediateDefinition::
    ~TimePeriodEventsIntermediateDefinition() = default;
TimePeriodEventsIntermediateDefinition::TimePeriodEventsIntermediateDefinition(
    TimePeriodEventsIntermediateDefinition&&) = default;

// static
void TimePeriodEventsIntermediateDefinition::RegisterJSONConverter(
    base::JSONValueConverter<TimePeriodEventsIntermediateDefinition>*
        converter) {
  converter->RegisterStringField(
      "storage_key", &TimePeriodEventsIntermediateDefinition::storage_key);
  converter->RegisterIntField(
      "period_days", &TimePeriodEventsIntermediateDefinition::period_days);
  converter->RegisterBoolField(
      "replace_today", &TimePeriodEventsIntermediateDefinition::replace_today);
  converter->RegisterBoolField(
      "report_highest",
      &TimePeriodEventsIntermediateDefinition::report_highest);
  converter->RegisterBoolField(
      "add_histogram_value",
      &TimePeriodEventsIntermediateDefinition::add_histogram_value);
  converter->RegisterCustomValueField(
      "sources", &TimePeriodEventsIntermediateDefinition::sources,
      &ParseValueList);
}

TimePeriodEventsIntermediate::TimePeriodEventsIntermediate(
    TimePeriodEventsIntermediateDefinition definition,
    Delegate* delegate)
    : RemoteMetricIntermediate(delegate), definition_(std::move(definition)) {}

TimePeriodEventsIntermediate::~TimePeriodEventsIntermediate() = default;

bool TimePeriodEventsIntermediate::Init() {
  if (definition_.storage_key.empty() || definition_.period_days <= 0) {
    return false;
  }

  storage_ = delegate_->GetTimePeriodStorage(definition_.storage_key,
                                             definition_.period_days);

  for (const auto& source : definition_.sources) {
    auto intermediate = delegate_->GetIntermediateInstance(source);
    if (intermediate && intermediate->Init()) {
      source_intermediates_.push_back(std::move(intermediate));
    }
  }

  return true;
}

base::Value TimePeriodEventsIntermediate::Process() {
  for (const auto& source : source_intermediates_) {
    auto source_value = source->Process();
    if (!source_value.is_int()) {
      continue;
    }

    int value_to_insert =
        definition_.add_histogram_value ? source_value.GetInt() : 1;
    if (definition_.replace_today) {
      storage_->ReplaceTodaysValueIfGreater(value_to_insert);
    } else {
      storage_->AddDelta(value_to_insert);
    }
  }

  uint64_t value;
  if (definition_.report_highest) {
    value = storage_->GetHighestValueInPeriod();
  } else {
    value = storage_->GetPeriodSum();
  }

  return base::Value(static_cast<int>(value));
}

base::flat_set<std::string_view> TimePeriodEventsIntermediate::GetStorageKeys()
    const {
  base::flat_set<std::string_view> keys;

  keys.insert(definition_.storage_key);

  // Collect storage keys from source intermediates
  for (const auto& intermediate : source_intermediates_) {
    auto source_keys = intermediate->GetStorageKeys();
    keys.insert(source_keys.begin(), source_keys.end());
  }

  return keys;
}

void TimePeriodEventsIntermediate::OnLastUsedProfilePrefsChanged(
    PrefService* profile_prefs) {
  // Propagate to all source intermediates
  for (const auto& intermediate : source_intermediates_) {
    intermediate->OnLastUsedProfilePrefsChanged(profile_prefs);
  }
}

}  // namespace p3a
