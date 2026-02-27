/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/time_period_storage/profile_attributes_time_period_store.h"

#include <utility>

#include "base/values.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"

ProfileAttributesTimePeriodStore::ProfileAttributesTimePeriodStore(
    const base::FilePath& profile_path,
    ProfileAttributesStorage& profile_attributes_storage,
    std::string metric_name,
    std::string dict_key)
    : profile_path_(profile_path),
      profile_attributes_storage_(profile_attributes_storage),
      metric_name_(std::move(metric_name)),
      dict_key_(std::move(dict_key)) {}

ProfileAttributesTimePeriodStore::~ProfileAttributesTimePeriodStore() = default;

void ProfileAttributesTimePeriodStore::Set(base::ListValue data) {
  ProfileAttributesEntry* entry =
      profile_attributes_storage_->GetProfileAttributesWithPath(profile_path_);
  if (!entry) {
    return;
  }

  base::DictValue metric;
  if (const base::Value* existing_metric = entry->GetMetric(metric_name_);
      existing_metric) {
    CHECK(existing_metric->is_dict());
    metric = existing_metric->GetDict().Clone();
  }

  metric.Set(dict_key_, std::move(data));
  entry->SetMetric(metric_name_, base::Value(std::move(metric)));
}

const base::ListValue* ProfileAttributesTimePeriodStore::Get() {
  const ProfileAttributesEntry* entry =
      profile_attributes_storage_->GetProfileAttributesWithPath(profile_path_);
  if (!entry) {
    return nullptr;
  }

  const base::Value* metric = entry->GetMetric(metric_name_);
  if (!metric) {
    return nullptr;
  }
  CHECK(metric->is_dict());

  return metric->GetDict().FindList(dict_key_);
}

void ProfileAttributesTimePeriodStore::Clear() {
  ProfileAttributesEntry* entry =
      profile_attributes_storage_->GetProfileAttributesWithPath(profile_path_);
  if (!entry) {
    return;
  }

  const base::Value* metric = entry->GetMetric(metric_name_);
  if (!metric) {
    return;
  }
  CHECK(metric->is_dict());

  if (!metric->GetDict().contains(dict_key_)) {
    return;
  }

  base::DictValue updated_metric = metric->GetDict().Clone();
  updated_metric.Remove(dict_key_);

  entry->SetMetric(metric_name_, base::Value(std::move(updated_metric)));
}
