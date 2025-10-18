/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/remote_metric_manager.h"

#include <utility>

#include "base/containers/flat_set.h"
#include "base/containers/map_util.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/p3a/managed/remote_metric.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/version_info/version_info.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace p3a {

RemoteMetricManager::RemoteMetricManager(PrefService* local_state,
                                         Delegate* delegate)
    : local_state_(local_state),
      delegate_(delegate),
      current_version_(version_info::GetBraveVersionNumberForDisplay()) {}

RemoteMetricManager::~RemoteMetricManager() = default;

void RemoteMetricManager::HandleProfileLoad(PrefService* profile_prefs,
                                            const base::FilePath& context_path,
                                            bool is_last_used_profile) {
  auto base_path = context_path.BaseName();
  profile_prefs_map_[base_path] = profile_prefs;

  if (is_last_used_profile) {
    last_used_profile_prefs_ = profile_prefs;

    for (const auto& metric : metrics_) {
      metric->OnLastUsedProfilePrefsChanged(last_used_profile_prefs_);
    }

    if (metric_definitions_to_process_) {
      ProcessMetricDefinitions(std::move(*metric_definitions_to_process_));
      metric_definitions_to_process_ = std::nullopt;
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

void RemoteMetricManager::HandleLastUsedProfileChanged(
    const base::FilePath& context_path) {
  auto base_path = context_path.BaseName();
  auto it = profile_prefs_map_.find(base_path);
  if (it == profile_prefs_map_.end()) {
    last_used_profile_prefs_ = nullptr;
  } else {
    last_used_profile_prefs_ = it->second;
  }

  for (const auto& metric : metrics_) {
    metric->OnLastUsedProfilePrefsChanged(last_used_profile_prefs_);
  }

  if (last_used_profile_prefs_ && metric_definitions_to_process_) {
    ProcessMetricDefinitions(std::move(*metric_definitions_to_process_));
    metric_definitions_to_process_ = std::nullopt;
  }
}

void RemoteMetricManager::ProcessMetricDefinitions(
    UnparsedDefinitionsMap definitions) {
  if (!last_used_profile_prefs_) {
    metric_definitions_to_process_ = std::move(definitions);
    return;
  }

  metrics_.clear();

  // Process all definitions
  for (auto& [metric_name, definition] : definitions) {
    const auto* definition_dict = definition->GetIfDict();
    if (!definition_dict) {
      continue;
    }

    auto metric = std::make_unique<RemoteMetric>(
        local_state_, last_used_profile_prefs_, this, metric_name,
        std::move(definition));

    if (!metric->Init(current_version_)) {
      VLOG(1) << "Failed to initialize remote metric " << metric_name;
      continue;
    }

    VLOG(1) << "Remote metric constructed: " << metric_name;

    metrics_.push_back(std::move(metric));
  }

  CleanupStorage();
}

void RemoteMetricManager::CleanupStorage() {
  base::flat_set<std::string_view> used_storage_keys;
  for (const auto& metric : metrics_) {
    auto storage_keys = metric->GetStorageKeys();
    used_storage_keys.insert(storage_keys.begin(), storage_keys.end());
  }

  for (auto it = time_period_storages_.begin();
       it != time_period_storages_.end();) {
    if (!used_storage_keys.contains(it->first)) {
      it = time_period_storages_.erase(it);
      continue;
    }
    it++;
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

void RemoteMetricManager::UpdateMetric(std::string_view metric_name,
                                       size_t bucket) {
  delegate_->UpdateMetricValue(metric_name, bucket);
}

TimePeriodStorage* RemoteMetricManager::GetTimePeriodStorage(
    std::string_view storage_key,
    int period_days) {
  if (auto* storage = base::FindOrNull(time_period_storages_, storage_key)) {
    return storage->get();
  }

  auto it = time_period_storages_.insert({std::string(storage_key), nullptr});
  it.first->second = std::make_unique<TimePeriodStorage>(
      local_state_, kRemoteMetricStorageDictPref, it.first->first.c_str(),
      period_days);
  return it.first->second.get();
}

}  // namespace p3a
