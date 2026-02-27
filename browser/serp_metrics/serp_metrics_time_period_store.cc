/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_time_period_store.h"

#include <utility>

#include "base/values.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"

SerpMetricsTimePeriodStore::SerpMetricsTimePeriodStore(
    const base::FilePath& profile_path,
    ProfileAttributesStorage& profile_attributes_storage,
    std::string metric_name)
    : profile_path_(profile_path),
      profile_attributes_storage_(profile_attributes_storage),
      metric_name_(std::move(metric_name)) {}

SerpMetricsTimePeriodStore::~SerpMetricsTimePeriodStore() = default;

const base::ListValue* SerpMetricsTimePeriodStore::Get() {
  const ProfileAttributesEntry* entry =
      profile_attributes_storage_->GetProfileAttributesWithPath(profile_path_);
  if (!entry) {
    return nullptr;
  }

  const base::DictValue* serp_metrics = entry->GetSerpMetrics();
  if (!serp_metrics) {
    return nullptr;
  }

  return serp_metrics->FindList(metric_name_);
}

void SerpMetricsTimePeriodStore::Set(base::ListValue list) {
  ProfileAttributesEntry* entry =
      profile_attributes_storage_->GetProfileAttributesWithPath(profile_path_);
  if (!entry) {
    return;
  }

  base::DictValue serp_metrics;
  if (const base::DictValue* existing_serp_metrics = entry->GetSerpMetrics();
      existing_serp_metrics) {
    serp_metrics = existing_serp_metrics->Clone();
  }

  serp_metrics.Set(metric_name_, std::move(list));
  entry->SetSerpMetrics(std::move(serp_metrics));
}

void SerpMetricsTimePeriodStore::Clear() {
  ProfileAttributesEntry* entry =
      profile_attributes_storage_->GetProfileAttributesWithPath(profile_path_);
  if (!entry) {
    return;
  }

  const base::DictValue* serp_metrics = entry->GetSerpMetrics();
  if (!serp_metrics || !serp_metrics->contains(metric_name_)) {
    return;
  }

  base::DictValue mutable_serp_metrics = serp_metrics->Clone();
  mutable_serp_metrics.Remove(metric_name_);
  entry->SetSerpMetrics(std::move(mutable_serp_metrics));
}
