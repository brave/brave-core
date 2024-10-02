/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/prefs/pref_provider.h"

#include "components/prefs/pref_service.h"

namespace brave_ads {

PrefProvider::PrefProvider(PrefService* const profile_prefs,
                           PrefService* const local_state_prefs)
    : profile_prefs_(profile_prefs), local_state_prefs_(local_state_prefs) {
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

}  // namespace brave_ads
