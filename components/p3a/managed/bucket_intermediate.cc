/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/bucket_intermediate.h"

#include <utility>

#include "base/values.h"
#include "brave/components/p3a/utils.h"

namespace p3a {

BucketIntermediateDefinition::BucketIntermediateDefinition() = default;
BucketIntermediateDefinition::~BucketIntermediateDefinition() = default;
BucketIntermediateDefinition::BucketIntermediateDefinition(
    BucketIntermediateDefinition&&) = default;

// static
void BucketIntermediateDefinition::RegisterJSONConverter(
    base::JSONValueConverter<BucketIntermediateDefinition>* converter) {
  converter->RegisterCustomValueField<base::Value>(
      "source", &BucketIntermediateDefinition::source, &ParseValue);
  converter->RegisterRepeatedInt("buckets",
                                 &BucketIntermediateDefinition::buckets);
}

BucketIntermediate::BucketIntermediate(BucketIntermediateDefinition definition,
                                       Delegate* delegate)
    : RemoteMetricIntermediate(delegate), definition_(std::move(definition)) {}

BucketIntermediate::~BucketIntermediate() = default;

bool BucketIntermediate::Init() {
  if (definition_.source.is_none() || definition_.buckets.empty()) {
    return false;
  }

  source_intermediate_ = delegate_->GetIntermediateInstance(definition_.source);
  if (!source_intermediate_) {
    return false;
  }

  // Convert and cache bucket values
  bucket_values_.clear();
  for (const auto& bucket : definition_.buckets) {
    if (!bucket) {
      return false;
    }
    bucket_values_.push_back(*bucket);
  }

  return source_intermediate_->Init();
}

base::Value BucketIntermediate::Process() {
  auto source_value = source_intermediate_->Process();
  if (!source_value.is_int()) {
    return {};
  }

  int value = source_value.GetInt();

  auto it_count =
      std::lower_bound(bucket_values_.begin(), bucket_values_.end(), value);
  int bucket_index = std::distance(bucket_values_.begin(), it_count);

  return base::Value(bucket_index);
}

base::flat_set<std::string_view> BucketIntermediate::GetStorageKeys() const {
  return source_intermediate_->GetStorageKeys();
}

void BucketIntermediate::OnLastUsedProfilePrefsChanged(
    PrefService* profile_prefs) {
  if (source_intermediate_) {
    source_intermediate_->OnLastUsedProfilePrefsChanged(profile_prefs);
  }
}

}  // namespace p3a
