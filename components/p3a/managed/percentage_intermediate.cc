/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/percentage_intermediate.h"

#include <utility>

#include "base/values.h"
#include "brave/components/p3a/utils.h"

namespace p3a {

PercentageIntermediateDefinition::PercentageIntermediateDefinition() = default;
PercentageIntermediateDefinition::~PercentageIntermediateDefinition() = default;
PercentageIntermediateDefinition::PercentageIntermediateDefinition(
    PercentageIntermediateDefinition&&) = default;

// static
void PercentageIntermediateDefinition::RegisterJSONConverter(
    base::JSONValueConverter<PercentageIntermediateDefinition>* converter) {
  converter->RegisterCustomValueField<base::Value>(
      "numerator", &PercentageIntermediateDefinition::numerator, &ParseValue);
  converter->RegisterCustomValueField<base::Value>(
      "denominator", &PercentageIntermediateDefinition::denominator,
      &ParseValue);
  converter->RegisterIntField("multiplier",
                              &PercentageIntermediateDefinition::multiplier);
}

PercentageIntermediate::PercentageIntermediate(
    PercentageIntermediateDefinition definition,
    Delegate* delegate)
    : RemoteMetricIntermediate(delegate), definition_(std::move(definition)) {}

PercentageIntermediate::~PercentageIntermediate() = default;

bool PercentageIntermediate::Init() {
  if (definition_.numerator.is_none() || definition_.denominator.is_none()) {
    return false;
  }

  numerator_intermediate_ =
      delegate_->GetIntermediateInstance(definition_.numerator);
  denominator_intermediate_ =
      delegate_->GetIntermediateInstance(definition_.denominator);
  if (!numerator_intermediate_ || !denominator_intermediate_) {
    return false;
  }

  return numerator_intermediate_->Init() && denominator_intermediate_->Init();
}

base::Value PercentageIntermediate::Process() {
  auto numerator_value = numerator_intermediate_->Process();
  auto denominator_value = denominator_intermediate_->Process();

  if (!numerator_value.is_int() || !denominator_value.is_int()) {
    return {};
  }

  double numerator = numerator_value.GetInt();
  double denominator = denominator_value.GetInt();

  if (denominator == 0) {
    return base::Value(0);  // Avoid division by zero, return 0%
  }

  double percentage = (numerator * 100) / denominator;

  percentage *= definition_.multiplier;

  return base::Value(static_cast<int>(percentage));
}

base::flat_set<std::string_view> PercentageIntermediate::GetStorageKeys()
    const {
  base::flat_set<std::string_view> keys;

  auto numerator_keys = numerator_intermediate_->GetStorageKeys();
  keys.insert(numerator_keys.begin(), numerator_keys.end());

  auto denominator_keys = denominator_intermediate_->GetStorageKeys();
  keys.insert(denominator_keys.begin(), denominator_keys.end());

  return keys;
}

void PercentageIntermediate::OnLastUsedProfilePrefsChanged(
    PrefService* profile_prefs) {
  if (numerator_intermediate_) {
    numerator_intermediate_->OnLastUsedProfilePrefsChanged(profile_prefs);
  }
  if (denominator_intermediate_) {
    denominator_intermediate_->OnLastUsedProfilePrefsChanged(profile_prefs);
  }
}

}  // namespace p3a
