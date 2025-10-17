/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/value_map_intermediate.h"

#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/p3a/utils.h"

namespace p3a {

ValueMapIntermediateDefinition::ValueMapIntermediateDefinition() = default;
ValueMapIntermediateDefinition::~ValueMapIntermediateDefinition() = default;
ValueMapIntermediateDefinition::ValueMapIntermediateDefinition(
    ValueMapIntermediateDefinition&&) = default;

// static
void ValueMapIntermediateDefinition::RegisterJSONConverter(
    base::JSONValueConverter<ValueMapIntermediateDefinition>* converter) {
  converter->RegisterCustomValueField<base::Value>(
      "source", &ValueMapIntermediateDefinition::source, &ParseValue);
  converter->RegisterCustomValueField<base::Value::Dict>(
      "map", &ValueMapIntermediateDefinition::map, &ParseDict);
}

ValueMapIntermediate::ValueMapIntermediate(
    ValueMapIntermediateDefinition definition,
    Delegate* delegate)
    : RemoteMetricIntermediate(delegate), definition_(std::move(definition)) {}

ValueMapIntermediate::~ValueMapIntermediate() = default;

bool ValueMapIntermediate::Init() {
  if (definition_.source.is_none() || definition_.map.empty()) {
    return false;
  }

  source_intermediate_ = delegate_->GetIntermediateInstance(definition_.source);
  if (!source_intermediate_) {
    return false;
  }

  return source_intermediate_->Init();
}

base::Value ValueMapIntermediate::Process() {
  auto source_value = source_intermediate_->Process();

  if (source_value.is_none()) {
    return {};
  }

  std::string key;

  if (source_value.is_bool()) {
    key = source_value.GetBool() ? "true" : "false";
  } else if (source_value.is_int()) {
    key = base::NumberToString(source_value.GetInt());
  } else if (source_value.is_double()) {
    key = base::NumberToString(source_value.GetDouble());
  } else if (source_value.is_string()) {
    key = source_value.GetString();
  } else {
    return {};
  }

  const base::Value* mapped_value = definition_.map.Find(key);
  if (!mapped_value) {
    return {};
  }

  return mapped_value->Clone();
}

base::flat_set<std::string_view> ValueMapIntermediate::GetStorageKeys() const {
  return source_intermediate_->GetStorageKeys();
}

void ValueMapIntermediate::OnLastUsedProfilePrefsChanged(
    PrefService* profile_prefs) {
  if (source_intermediate_) {
    source_intermediate_->OnLastUsedProfilePrefsChanged(profile_prefs);
  }
}

}  // namespace p3a
