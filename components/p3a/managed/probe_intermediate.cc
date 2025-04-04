/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/probe_intermediate.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/values.h"

namespace p3a {

ProbeIntermediateDefinition::ProbeIntermediateDefinition() = default;
ProbeIntermediateDefinition::~ProbeIntermediateDefinition() = default;
ProbeIntermediateDefinition::ProbeIntermediateDefinition(
    ProbeIntermediateDefinition&&) = default;

// static
void ProbeIntermediateDefinition::RegisterJSONConverter(
    base::JSONValueConverter<ProbeIntermediateDefinition>* converter) {
  converter->RegisterStringField("histogram_name",
                                 &ProbeIntermediateDefinition::histogram_name);
  converter->RegisterRepeatedInt("filter",
                                 &ProbeIntermediateDefinition::filter);
}

ProbeIntermediate::ProbeIntermediate(ProbeIntermediateDefinition definition,
                                     Delegate* delegate)
    : RemoteMetricIntermediate(delegate), definition_(std::move(definition)) {}

ProbeIntermediate::~ProbeIntermediate() = default;

bool ProbeIntermediate::Init() {
  if (definition_.histogram_name.empty()) {
    return false;
  }

  scoped_observer_ =
      std::make_unique<base::StatisticsRecorder::ScopedHistogramSampleObserver>(
          definition_.histogram_name,
          base::BindRepeating(&ProbeIntermediate::OnHistogramSample,
                              base::Unretained(this)));

  return true;
}

base::Value ProbeIntermediate::Process() {
  if (!last_value_) {
    // No value to report
    return {};
  }

  int value = *last_value_;
  last_value_ = std::nullopt;
  return base::Value(value);
}

void ProbeIntermediate::OnHistogramSample(
    std::string_view histogram_name,
    uint64_t name_hash,
    base::HistogramBase::Sample32 sample) {
  int sample_value = static_cast<int>(sample);

  // If filter is not empty, only cache values that are in the filter
  if (!definition_.filter.empty()) {
    bool found_in_filter = false;
    for (const auto& filter_value : definition_.filter) {
      if (filter_value && *filter_value == sample_value) {
        found_in_filter = true;
        break;
      }
    }
    if (!found_in_filter) {
      return;
    }
  }

  last_value_ = sample_value;
  delegate_->TriggerUpdate();
}

base::flat_set<std::string_view> ProbeIntermediate::GetStorageKeys() const {
  return {};
}

void ProbeIntermediate::OnLastUsedProfilePrefsChanged(
    PrefService* profile_prefs) {}

}  // namespace p3a
