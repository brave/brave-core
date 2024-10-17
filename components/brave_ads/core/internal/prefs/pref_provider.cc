/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/prefs/pref_provider.h"

#include <utility>

#include "components/prefs/pref_service.h"

namespace brave_ads {

PrefProvider::PrefProvider(PrefService* const profile_prefs,
                           PrefService* const local_state_prefs,
                           base::Value::Dict virtual_prefs)
    : profile_prefs_(profile_prefs),
      local_state_prefs_(local_state_prefs),
      virtual_prefs_(std::move(virtual_prefs)) {
  CHECK(profile_prefs_);
  CHECK(local_state_prefs_);
}

PrefProvider::~PrefProvider() = default;

std::optional<base::Value> PrefProvider::GetProfilePref(
    const std::string& pref_path) const {
  if (!profile_prefs_->FindPreference(pref_path)) {
    // The preference does not exist.
    return std::nullopt;
  }

  return profile_prefs_->GetValue(pref_path).Clone();
}

bool PrefProvider::HasProfilePrefPath(const std::string& pref_path) const {
  return profile_prefs_->HasPrefPath(pref_path);
}

std::optional<base::Value> PrefProvider::GetLocalStatePref(
    const std::string& pref_path) const {
  if (!local_state_prefs_->FindPreference(pref_path)) {
    // The preference does not exist.
    return std::nullopt;
  }

  return local_state_prefs_->GetValue(pref_path).Clone();
}

bool PrefProvider::HasLocalStatePrefPath(const std::string& pref_path) const {
  return local_state_prefs_->HasPrefPath(pref_path);
}

std::optional<base::Value> PrefProvider::GetVirtualPref(
    const std::string& virtual_pref_path) const {
  if (virtual_pref_path.starts_with("[virtual]:")) {
    const std::string pref_path =
        virtual_pref_path.substr(/*pos=*/std::size("[virtual]:"));

    if (const base::Value* const value = virtual_prefs_.Find(pref_path)) {
      return value->Clone();
    }
  }

  if (virtual_pref_path.starts_with("[virtual_default]:")) {
    const std::string pref_path =
        virtual_pref_path.substr(/*pos=*/std::size("[virtual_default]:"));

    if (HasProfilePrefPath(pref_path)) {
      return GetProfilePref(pref_path);
    }

    if (HasLocalStatePrefPath(pref_path)) {
      return GetLocalStatePref(pref_path);
    }

    if (const base::Value* const value = virtual_prefs_.Find(pref_path)) {
      return value->Clone();
    }
  }

  // Unknown virtual pref path.
  return std::nullopt;
}

}  // namespace brave_ads
