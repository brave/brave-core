/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/pref_intermediate.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"

namespace p3a {

PrefIntermediateDefinition::PrefIntermediateDefinition() = default;
PrefIntermediateDefinition::~PrefIntermediateDefinition() = default;
PrefIntermediateDefinition::PrefIntermediateDefinition(
    PrefIntermediateDefinition&&) = default;

// static
void PrefIntermediateDefinition::RegisterJSONConverter(
    base::JSONValueConverter<PrefIntermediateDefinition>* converter) {
  converter->RegisterStringField("pref_name",
                                 &PrefIntermediateDefinition::pref_name);
  converter->RegisterBoolField("use_profile_prefs",
                               &PrefIntermediateDefinition::use_profile_prefs);
}

PrefIntermediate::PrefIntermediate(PrefIntermediateDefinition definition,
                                   PrefService* local_state,
                                   PrefService* profile_prefs,
                                   Delegate* delegate)
    : RemoteMetricIntermediate(delegate),
      definition_(std::move(definition)),
      local_state_(local_state),
      profile_prefs_(profile_prefs) {}

PrefIntermediate::~PrefIntermediate() = default;

bool PrefIntermediate::Init() {
  PrefService* pref_service = GetPrefService();
  if (!pref_service || definition_.pref_name.empty()) {
    return false;
  }

  if (!pref_service->FindPreference(definition_.pref_name)) {
    return false;
  }

  pref_change_registrar_.Init(pref_service);
  pref_change_registrar_.Add(
      definition_.pref_name,
      base::BindRepeating(&PrefIntermediate::OnPrefChanged,
                          base::Unretained(this)));

  return true;
}

base::Value PrefIntermediate::Process() {
  PrefService* pref_service = GetPrefService();
  if (!pref_service) {
    return {};
  }

  auto* pref = pref_service->FindPreference(definition_.pref_name);
  if (!pref) {
    return {};
  }

  return pref->GetValue()->Clone();
}

PrefService* PrefIntermediate::GetPrefService() const {
  if (definition_.use_profile_prefs) {
    return profile_prefs_;  // May be null if profile prefs not set yet
  }
  return local_state_;
}

void PrefIntermediate::OnPrefChanged() {
  delegate_->TriggerUpdate();
}

void PrefIntermediate::OnLastUsedProfilePrefsChanged(
    PrefService* profile_prefs) {
  profile_prefs_ = profile_prefs;

  if (!definition_.use_profile_prefs) {
    return;
  }

  pref_change_registrar_.Reset();

  if (!profile_prefs_) {
    return;
  }

  pref_change_registrar_.Init(profile_prefs_);

  if (!profile_prefs_->FindPreference(definition_.pref_name)) {
    return;
  }

  pref_change_registrar_.Add(
      definition_.pref_name,
      base::BindRepeating(&PrefIntermediate::OnPrefChanged,
                          base::Unretained(this)));

  delegate_->TriggerUpdate();
}

base::flat_set<std::string_view> PrefIntermediate::GetStorageKeys() const {
  return {};
}

}  // namespace p3a
