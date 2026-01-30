/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/remote_metric.h"

#include <utility>

#include "base/json/json_value_converter.h"
#include "base/logging.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/p3a/managed/bucket_intermediate.h"
#include "brave/components/p3a/managed/percentage_intermediate.h"
#include "brave/components/p3a/managed/pref_intermediate.h"
#include "components/prefs/pref_service.h"

namespace p3a {

namespace {

constexpr char kMinVersionKey[] = "min_version";
constexpr char kTypeKey[] = "type";
constexpr char kPrefIntermediateType[] = "pref";
constexpr char kBucketIntermediateType[] = "bucket";
constexpr char kPercentageIntermediateType[] = "percentage";

}  // namespace

RemoteMetric::RemoteMetric(PrefService* local_state,
                           PrefService* profile_prefs,
                           Delegate* delegate,
                           std::string_view metric_name,
                           std::unique_ptr<base::Value> definition)
    : delegate_(delegate),
      metric_name_(metric_name),
      local_state_(local_state),
      profile_prefs_(profile_prefs),
      definition_(std::move(definition)) {}

RemoteMetric::~RemoteMetric() = default;

bool RemoteMetric::Init(const base::Version& current_version) {
  if (!definition_ || !definition_->is_dict()) {
    return false;
  }

  const auto* min_version_str =
      definition_->GetDict().FindString(kMinVersionKey);
  if (min_version_str) {
    base::Version min_version(*min_version_str);
    if (!current_version.IsValid() || !min_version.IsValid()) {
      VLOG(1) << "Skipping metric " << metric_name_
              << " due to invalid version";
      return false;
    }

    if (current_version < min_version) {
      VLOG(1) << "Skipping metric " << metric_name_
              << " due to min_version requirement: " << *min_version_str;
      return false;
    }
  }

  intermediate_ = GetIntermediateInstance(*definition_);
  definition_ = nullptr;

  if (!intermediate_) {
    return false;
  }

  if (!intermediate_->Init()) {
    return false;
  }

  TriggerUpdate();
  return true;
}

base::flat_set<std::string_view> RemoteMetric::GetStorageKeys() const {
  return intermediate_->GetStorageKeys();
}

void RemoteMetric::OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) {
  profile_prefs_ = profile_prefs;
  profile_pref_update_in_progress_ = true;
  if (intermediate_) {
    intermediate_->OnLastUsedProfilePrefsChanged(profile_prefs);
  }
  profile_pref_update_in_progress_ = false;
  if (update_pending_) {
    update_pending_ = false;
    TriggerUpdate();
  }
}

// RemoteMetricIntermediate::Delegate implementation:
TimePeriodStorage* RemoteMetric::GetTimePeriodStorage(
    std::string_view storage_key,
    int period_days) {
  return delegate_->GetTimePeriodStorage(storage_key, period_days);
}

void RemoteMetric::TriggerUpdate() {
  if (profile_pref_update_in_progress_) {
    update_pending_ = true;
    return;
  }

  auto value = intermediate_->Process();
  if (value.is_int()) {
    delegate_->UpdateMetric(metric_name_, value.GetInt());
  }

  daily_timer_.Start(
      FROM_HERE, base::Time::Now() + base::Days(1),
      base::BindOnce(&RemoteMetric::TriggerUpdate, base::Unretained(this)));
}

std::unique_ptr<RemoteMetricIntermediate> RemoteMetric::GetIntermediateInstance(
    const base::Value& config) {
  const auto* dict = config.GetIfDict();
  if (!dict) {
    return nullptr;
  }

  const auto* type = dict->FindString(kTypeKey);
  if (!type) {
    return nullptr;
  }

  if (*type == kPrefIntermediateType) {
    PrefIntermediateDefinition definition;
    base::JSONValueConverter<PrefIntermediateDefinition> converter;
    if (!converter.Convert(config, &definition)) {
      return nullptr;
    }
    return std::make_unique<PrefIntermediate>(
        std::move(definition), local_state_, profile_prefs_, this);
  }

  if (*type == kBucketIntermediateType) {
    BucketIntermediateDefinition definition;
    base::JSONValueConverter<BucketIntermediateDefinition> converter;
    if (!converter.Convert(config, &definition)) {
      return nullptr;
    }
    return std::make_unique<BucketIntermediate>(std::move(definition), this);
  }

  if (*type == kPercentageIntermediateType) {
    PercentageIntermediateDefinition definition;
    base::JSONValueConverter<PercentageIntermediateDefinition> converter;
    if (!converter.Convert(config, &definition)) {
      return nullptr;
    }
    return std::make_unique<PercentageIntermediate>(std::move(definition),
                                                    this);
  }

  return nullptr;  // Unknown type
}

}  // namespace p3a
