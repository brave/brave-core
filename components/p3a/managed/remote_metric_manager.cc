/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/remote_metric_manager.h"

#include <utility>

#include "base/containers/flat_set.h"
#include "base/json/json_value_converter.h"
#include "base/values.h"
#include "brave/components/p3a/managed/pref_metric.h"
#include "brave/components/p3a/managed/time_period_events_metric.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/version_info/version_info.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace p3a {

namespace {

constexpr char kTypeKey[] = "type";
constexpr char kMinVersionKey[] = "min_version";
constexpr char kTimePeriodEventsType[] = "time_period_events";
constexpr char kPrefMetricType[] = "pref";

}  // namespace

RemoteMetricManager::RemoteMetricManager(PrefService* local_state,
                                         Delegate* delegate)
    : local_state_(local_state),
      delegate_(delegate),
      current_version_(version_info::GetBraveVersionNumberForDisplay()) {
  last_used_profile_pref_change_registrar_.Init(local_state_);
  last_used_profile_pref_change_registrar_.Add(
      ::prefs::kProfileLastUsed,
      base::BindRepeating(&RemoteMetricManager::OnLastUsedProfileChanged,
                          base::Unretained(this)));
}

RemoteMetricManager::~RemoteMetricManager() = default;

void RemoteMetricManager::HandleProfileLoad(
    PrefService* profile_prefs,
    const base::FilePath& context_path) {
  auto base_path = context_path.BaseName();
  profile_prefs_map_[base_path] = profile_prefs;

  base::FilePath last_used_path =
      local_state_->GetFilePath(::prefs::kProfileLastUsed);
  if (last_used_path.empty() || last_used_path == base_path) {
    last_used_profile_prefs_ = profile_prefs;

    for (const auto& metric : metrics_) {
      metric->OnLastUsedProfilePrefsChanged(last_used_profile_prefs_);
    }
  }
}

void RemoteMetricManager::HandleProfileUnload(
    const base::FilePath& context_path) {
  auto base_path = context_path.BaseName();
  auto it = profile_prefs_map_.find(base_path);
  if (it != profile_prefs_map_.end()) {
    // If this was the last used profile, clear that pointer
    if (last_used_profile_prefs_ == it->second) {
      last_used_profile_prefs_ = nullptr;
    }
    profile_prefs_map_.erase(it);
    for (const auto& metric : metrics_) {
      metric->OnLastUsedProfilePrefsChanged(nullptr);
    }
  }
}

void RemoteMetricManager::OnLastUsedProfileChanged() {
  base::FilePath last_used_path =
      local_state_->GetFilePath(::prefs::kProfileLastUsed);

  auto it = profile_prefs_map_.find(last_used_path);
  if (it != profile_prefs_map_.end()) {
    last_used_profile_prefs_ = it->second;
  } else {
    last_used_profile_prefs_ = nullptr;
  }

  for (const auto& metric : metrics_) {
    metric->OnLastUsedProfilePrefsChanged(last_used_profile_prefs_);
  }
}

void RemoteMetricManager::ProcessMetricDefinitions(
    const UnparsedDefinitionsMap& definitions) {
  remote_metric_sample_callbacks_.clear();
  histogram_to_metrics_.clear();
  metrics_.clear();

  // Process all definitions
  for (const auto& [metric_name, definition] : definitions) {
    const std::string* type = definition->FindString(kTypeKey);
    if (!type) {
      continue;
    }

    // Check min_version requirement if specified
    const std::string* min_version_str = definition->FindString(kMinVersionKey);
    if (min_version_str) {
      base::Version min_version(*min_version_str);
      if (!current_version_.IsValid() || !min_version.IsValid()) {
        VLOG(1) << "Skipping metric " << metric_name
                << " due to invalid version";
        continue;
      }

      if (current_version_ < min_version) {
        VLOG(1) << "Skipping metric " << metric_name
                << " due to min_version requirement: " << *min_version_str;
        continue;
      }
    }

    std::unique_ptr<RemoteMetric> metric = nullptr;
    if (*type == kTimePeriodEventsType) {
      TimePeriodEventsMetricDefinition metric_definition;
      base::JSONValueConverter<TimePeriodEventsMetricDefinition> converter;
      if (converter.Convert(*definition, &metric_definition) &&
          metric_definition.Validate()) {
        metric = std::make_unique<TimePeriodEventsMetric>(
            local_state_, std::move(metric_definition),
            base::BindRepeating(&RemoteMetricManager::OnMetricUpdated,
                                base::Unretained(this), metric_name));
      } else {
        VLOG(1) << "Invalid time period events metric definition: "
                << metric_name;
      }
    } else if (*type == kPrefMetricType) {
      PrefMetricDefinition metric_definition;
      base::JSONValueConverter<PrefMetricDefinition> converter;
      if (converter.Convert(*definition, &metric_definition) &&
          metric_definition.Validate()) {
        metric = std::make_unique<PrefMetric>(
            local_state_, std::move(metric_definition),
            base::BindRepeating(&RemoteMetricManager::OnMetricUpdated,
                                base::Unretained(this), metric_name));
      } else {
        VLOG(1) << "Invalid pref metric definition: " << metric_name;
      }
    } else {
      VLOG(1) << "Unknown metric type: " << *type;
    }

    if (!metric) {
      continue;
    }

    VLOG(1) << "Remote metric constructed: " << metric_name;

    metrics_.push_back(std::move(metric));
  }

  SetupObservers();
  CleanupStorage();

  if (last_used_profile_prefs_) {
    for (const auto& metric : metrics_) {
      metric->OnLastUsedProfilePrefsChanged(last_used_profile_prefs_);
    }
  }
}

void RemoteMetricManager::SetupObservers() {
  for (const auto& metric : metrics_) {
    for (const auto& histogram_name_view : metric->GetSourceHistogramNames()) {
      std::string histogram_name(histogram_name_view);
      histogram_to_metrics_[histogram_name].push_back(metric.get());
    }
  }
  for (const auto& [histogram_name, _] : histogram_to_metrics_) {
    remote_metric_sample_callbacks_[histogram_name] = std::make_unique<
        base::StatisticsRecorder::ScopedHistogramSampleObserver>(
        histogram_name,
        base::BindRepeating(&RemoteMetricManager::OnHistogramChanged,
                            base::Unretained(this)));
  }
}

void RemoteMetricManager::CleanupStorage() {
  base::flat_set<std::string> used_storage_keys;
  for (const auto& metric : metrics_) {
    auto storage_key = metric->GetStorageKey();
    if (storage_key) {
      used_storage_keys.insert(std::string(*storage_key));
    }
  }

  ScopedDictPrefUpdate update(local_state_, kRemoteMetricStorageDictPref);
  for (auto it = update->begin(); it != update->end();) {
    if (!used_storage_keys.contains(it->first)) {
      it = update->erase(it);
      continue;
    }
    it++;
  }
}

void RemoteMetricManager::OnHistogramChanged(
    std::string_view histogram_name,
    uint64_t name_hash,
    base::HistogramBase::Sample32 sample) {
  auto it = histogram_to_metrics_.find(std::string(histogram_name));
  if (it == histogram_to_metrics_.end()) {
    return;
  }

  for (auto metric : it->second) {
    metric->HandleHistogramChange(histogram_name, sample);
  }
}

void RemoteMetricManager::OnMetricUpdated(std::string_view metric_name,
                                          size_t bucket) {
  delegate_->UpdateMetricValue(metric_name, bucket, std::nullopt);
}

}  // namespace p3a
